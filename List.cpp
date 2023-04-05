#include <iterator>
#include <vector>
#include <cstddef>

template <size_t cap>
class alignas(std::max_align_t) StackStorage {
private:
    uint8_t pool[cap];
    size_t sz = 0;
public:
    void* alloc(size_t toAlloc, size_t allignment = 1) {
        sz = sz + (allignment - (sz % allignment)) % allignment;//upper bound of (sz/alignment)
        if (toAlloc + sz > cap)
            return nullptr;
        void* beginPtr = pool + sz;
        sz = toAlloc + sz;
        return beginPtr;
    }

    bool operator==(const StackStorage& that) {
        return that.pool == pool;
    }
};


template <typename T, size_t cap>
class StackAllocator {
public:
    StackStorage<cap>& stack;

    explicit StackAllocator(StackStorage<cap>& storage) : stack(storage) {}

    StackAllocator() = delete;

    template <typename U>
    StackAllocator(const StackAllocator<U, cap>& that) : stack(that.stack) {}

    StackAllocator& operator=(const StackAllocator& that) {
        stack = that.stack;
        return *this;
    }

    ~StackAllocator() = default;


    T* allocate(size_t toAlloc) {
        return reinterpret_cast<T*>(stack.alloc(sizeof(T) * toAlloc, alignof(T)));
    }

    void deallocate(T*, size_t) {}

    using value_type = T;

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, cap>;
    };

    bool operator==(const StackAllocator& that) {
        return stack == that.stack;
    }

    bool operator!=(const StackAllocator& that) {
        return !(stack == that.stack);
    }
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
    struct baseNode {
        baseNode* next = this;
        baseNode* prev = this;

        void link(baseNode* that) {
            next = that;
            that->prev = this;
        }

        void swap(baseNode& that) {
            if (next == this && &that == that.next) {
                return;
            }
            if (next != this && &that == that.next) {
                that.link(next);
                prev->link(&that);
                next = this;
                prev = this;
                return;
            }
            if (next == this && &that != that.next) {
                that.swap(*this);
                return;
            }
            if (next != this && &that != that.next) {
                baseNode tmp;
                tmp.swap(that);
                this->swap(that);
                this->swap(tmp);
                return;
            }
        }
    };

    struct Node : baseNode {
        T val;

        explicit Node(const T& val) : baseNode(), val(val) {}

        template <typename... Args>
        explicit Node(Args&& ... args) : baseNode(), val(std::forward<Args>(args)...) {}
    };

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocatorTraits = std::allocator_traits<NodeAllocator>;

private:
    NodeAllocator nodeAlloc;
    Allocator alloc;
    size_t sz = 0;
    baseNode fakeNode;

    template <typename... Args>
    Node* newNode(Args&& ... args) {
        Node* newNode = NodeAllocatorTraits::allocate(nodeAlloc, 1);
        try {
            std::allocator_traits<Allocator>::construct(alloc, newNode, std::forward<Args>(args)...);
        }
        catch (...) {
            NodeAllocatorTraits::deallocate(nodeAlloc, newNode, 1);
            throw;
        }
        return newNode;
    }

    void deleteNode(Node* toDel) {
        std::allocator_traits<Allocator>::destroy(alloc, toDel);
        NodeAllocatorTraits::deallocate(nodeAlloc, toDel, 1);
    }


    template <bool isConst>
    class TemplateIterator {
    public:
        baseNode* nodePtr;
        T* valPtr;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using reference = std::conditional_t<isConst, const T&, T&>;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using difference_type = std::ptrdiff_t;

        TemplateIterator(baseNode* nodePtr, T* valPtr) : nodePtr(nodePtr), valPtr(valPtr) {}

        TemplateIterator(const TemplateIterator& that) = default;

        explicit TemplateIterator(Node* nodePtr) : TemplateIterator(static_cast<baseNode*>(nodePtr), &(nodePtr->val)) {}

        operator TemplateIterator<true>() const {
            return TemplateIterator<true>(nodePtr, valPtr);
        }

        reference operator*() const {
            return *valPtr;
        }

        pointer operator->() const {
            return valPtr;
        }

        TemplateIterator& operator++() {
            nodePtr = (nodePtr->next);
            valPtr = &(static_cast<Node*>(nodePtr)->val);
            return *this;
        }

        TemplateIterator operator++(int) {
            TemplateIterator tmp(*this);
            nodePtr = (nodePtr->next);
            valPtr = &(static_cast<Node*>(nodePtr)->val);
            return tmp;
        }

        TemplateIterator& operator--() {
            nodePtr = (nodePtr->prev);
            valPtr = &(static_cast<Node*>(nodePtr)->val);
            return *this;
        }

        TemplateIterator& operator--(int) {
            nodePtr = (nodePtr->prev);
            valPtr = &(static_cast<Node*>(nodePtr)->val);
            return *this;
        }

        bool operator==(const TemplateIterator& x) const {
            return nodePtr == x.nodePtr;
        }

        bool operator!=(const TemplateIterator& x) const {
            return nodePtr != x.nodePtr;
        }
    };

public:
    using iterator = TemplateIterator<false>;
    using const_iterator = TemplateIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    void clear() {
        while (sz > 0) {
            pop_back();
        }
    }

    explicit List(size_t size, const T& val, const Allocator& allocator = Allocator()) :
            nodeAlloc(allocator), alloc(allocator) {
        try {
            for (size_t i = 0; i < size; ++i) {
                push_back(val);
            }
        }
        catch (...) {
            clear();
            throw;
        }
    }

    explicit List(size_t size, const Allocator& allocator = Allocator()) :
            nodeAlloc(allocator), alloc(allocator) {
        try {
            for (size_t i = 0; i < size; ++i) {
                emplace_back();
            }
        }
        catch (...) {
            clear();
            throw;
        }
    }

    explicit List(const Allocator& allocator = Allocator()) :
            nodeAlloc(allocator), alloc(allocator) {}

    Allocator get_allocator() const {
        return alloc;
    }

    List(const List& that) :
            nodeAlloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(that.nodeAlloc)),
            alloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(that.alloc)) {
        try {
            for (const T& val : that) {
                push_back(val);
            }
        }
        catch (...) {
            clear();
            throw;
        }
    }


    ~List() {
        clear();
    }

    void swap(List& that) {
        std::swap(nodeAlloc, that.nodeAlloc);
        std::swap(alloc, that.alloc);
        std::swap(sz, that.sz);
        fakeNode.swap(that.fakeNode);
    }


    List(List&& that) {
        this->swap(that);
    }

    List& operator=(List&& that) noexcept {
        clear();
        this->swap(that);
    }

    List& operator=(const List& that) {
        NodeAllocator newAlloc = std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
                                         ? that.alloc : alloc;
        List<T, Allocator> tmp(newAlloc);

        for (const T& val : that) {
            tmp.push_back(val);
        }

        this->swap(tmp);
        return *this;
    }

    [[nodiscard]] size_t size() const {
        return sz;
    }

    template <typename... Args>
    void emplace_back(Args&& ... args) {
        Node* nodePtr = newNode(std::forward<Args>(args)...);
        fakeNode.prev->link(nodePtr);
        nodePtr->link(&fakeNode);
        ++sz;
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    template <typename... Args>
    void push_back(Args&& ... args) {
        emplace_back(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void emplace_front(Args&& ... args) {
        Node* nodePtr = newNode(std::forward<Args>(args)...);
        nodePtr->link(fakeNode.next);
        fakeNode.link(nodePtr);
        ++sz;
    }

    void push_front(const T& value) {
        emplace_front(value);
    }

    template <typename... Args>
    void push_front(Args&& ... args) {
        emplace_front(std::forward<Args>(args)...);
    }

    void pop_back() {
        Node* toDel = static_cast<Node*>(fakeNode.prev);
        toDel->prev->link(&fakeNode);
        deleteNode(static_cast<Node*>(toDel));
        --sz;
    }

    void pop_front() {
        Node* toDel = static_cast<Node*>(fakeNode.next);
        fakeNode.link(toDel->next);
        deleteNode(static_cast<Node*>(toDel));
        --sz;
    }

    void insert(const_iterator it, const T& val) {
        Node* toIns = newNode(val);
        it.nodePtr->prev->link(toIns);
        toIns->link(it.nodePtr);
        ++sz;
    }

    void erase(const_iterator it) {
        Node* toDel = static_cast<Node*>(it.nodePtr);
        toDel->prev->link(toDel->next);
        deleteNode(toDel);
        --sz;
    }


    iterator begin() {
        return iterator(static_cast<Node*>(fakeNode.next));
    }

    const_iterator begin() const {
        return const_iterator(const_cast<Node*>(static_cast<const Node*>(fakeNode.next)));
    }

    iterator end() {
        return iterator(&fakeNode, nullptr);
    }

    const_iterator end() const {
        return const_iterator(const_cast<baseNode*>(&fakeNode), nullptr);
    }

    const_iterator cbegin() const {
        return const_iterator(begin());
    }

    const_iterator cend() const {
        return const_iterator(end());
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(begin());
    }
};

