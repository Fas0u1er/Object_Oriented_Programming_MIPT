#include <iterator>
#include <vector>
#include <cstddef>
#include <cmath>

template <typename T, typename Allocator = std::allocator<T>>
class List {
    template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
    friend
    class UnorderedMap;

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

    void insertNode(baseNode* pos, baseNode* toIns) {
        pos->prev->link(toIns);
        toIns->link(pos);
        ++sz;
    }


    template <bool isConst>
    class TemplateIterator {
        template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
        friend
        class UnorderedMap;

    private:
        baseNode* nodePtr;
        T* valPtr;

        explicit operator TemplateIterator<false>() const {
            return TemplateIterator<false>(nodePtr, valPtr);
        }

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


    List(List&& that) noexcept {
        fakeNode.swap(that.fakeNode);
        nodeAlloc = std::move(that.nodeAlloc);
        alloc = std::move(that.alloc);
        sz = that.sz;
        that.sz = 0;
    }

    List& operator=(List&& that) noexcept {
        NodeAllocator newAlloc = std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
                                 ? std::move(that.alloc) : alloc;
        List<T, Allocator> tmp(newAlloc);

        tmp.fakeNode.swap(that.fakeNode);

        this->swap(tmp);
        return *this;
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
        insertNode(it.nodePtr, toIns);
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


template <typename Key, typename Value, typename Hash = std::hash<Key>,
        typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
    static const size_t DEFAULT_CAPACITY = 5;
    constexpr static const float DEFAULT_LOAD_FACTOR = 0.95;
    constexpr static const float DEFAULT_EXPAND_FACTOR = 2;

    template <bool isConst>
    class TemplateIterator;

public:
    using NodeType = std::pair<const Key, Value>;
    using iterator = TemplateIterator<false>;
    using const_iterator = TemplateIterator<true>;

private:
    struct Element {
        NodeType keyVal;
        size_t hash;

        template <typename... Args>
        Element(Args&& ...args) : keyVal(std::forward<Args>(args)...) {}

        Element(const Element& other) : keyVal(other.keyValue), hash(other.hash) {}

        Element(Element&& other) noexcept: keyVal(std::move(other.keyValue)), hash(other.hash) {}
    };

    using ElementAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Element>;
    using MapList = List<Element, ElementAlloc>;
    using Node = typename MapList::Node;
    using NodePtrAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node*>;

private:
    Hash hasher;
    Equal keyEqual;
    Alloc allocator;
    MapList elements;
    std::vector<Node*, NodePtrAlloc> pointers;
    float maxLoadFactor;

private:
    void check_load() {
        if (load_factor() >= max_load_factor()) {
            rehash(DEFAULT_CAPACITY + static_cast<size_t>(std::ceil(DEFAULT_EXPAND_FACTOR * pointers.size())));
        }
    }

    void rehash(size_t newPtrSize) {
        MapList copy(std::move(elements));
        elements = MapList();
        pointers.assign(newPtrSize, nullptr);

        auto next(copy.begin());

        for (auto it = copy.begin(); it != copy.end(); it = next) {
            next = std::next(it);

            size_t index = get_pos(it->hash);

            if (!pointers[index]) {
                elements.insertNode(&elements.fakeNode,it.nodePtr);
            } else {
                elements.insertNode(pointers[index],it.nodePtr);
            }

            pointers[index] = static_cast<typename MapList::Node*>(it.nodePtr);
        }
        copy.sz = 0;
    }

    void swap(UnorderedMap& that) {
        std::swap(hasher, that.hasher);
        std::swap(keyEqual, that.keyEqual);
        std::swap(allocator, that.allocator);
        elements.swap(that.elements);
        std::swap(pointers, that.pointers);
        std::swap(maxLoadFactor, that.maxLoadFactor);
    }

    size_t get_pos(size_t hash) const {
        return hash % pointers.size();
    }

    const_iterator const_find(const Key& key) const {
        size_t hash = hasher(key);

        if(pointers[get_pos(hash)] == nullptr) {
            return cend();
        }

        const_iterator it(pointers[get_pos(hash)]);

        while (cend() != it) {
            if (get_pos(it.get_hash()) != get_pos(hash)) {
                break;
            }
            if (it.get_hash() != hash || !keyEqual(key, it->first)) {
                ++it;
                continue;
            }

            return it;
        }
        return cend();
    }

public:
    void clear() {
        for (auto it = begin(); it != end(); ++it) {
            erase(it);
        }
    }

    UnorderedMap() :
            hasher(), keyEqual(), allocator(), elements(allocator),
            pointers(DEFAULT_CAPACITY, nullptr, allocator), maxLoadFactor(DEFAULT_LOAD_FACTOR) {}

    UnorderedMap(const UnorderedMap& that) :
            hasher(that.hasher), keyEqual(that.keyEqual),
            allocator(
                    std::allocator_traits<Alloc>::select_on_container_copy_construction(
                            that.allocator)),
            elements(), pointers(DEFAULT_CAPACITY, nullptr, allocator),
            maxLoadFactor(that.maxLoadFactor) {
        try {
            for (auto e : that) {
                emplace(e);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    UnorderedMap(UnorderedMap&& that) noexcept:
            hasher(std::move(that.hasher)), keyEqual(std::move(that.keyEqual)), allocator(std::move(that.allocator)),
            elements(std::move(that.elements)), pointers(std::move(that.pointers)),
            maxLoadFactor(std::move(that.maxLoadFactor)) {}

    ~UnorderedMap() = default;

    UnorderedMap& operator=(const UnorderedMap& that) {
        UnorderedMap tmp(that);
        tmp.allocator = std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value ?
                        tmp.allocator : allocator;
        this->swap(tmp);
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& that) noexcept {
        UnorderedMap tmp(std::move(that));
        this->swap(tmp);
        return *this;
    }

    Value& at(const Key& key) {
        auto it = find(key);
        if (it != end())
            return it->second;
        throw std::out_of_range("No such key, may be use []?");
    }

    const Value& at(const Key& key) const {
        auto it = find(key);
        if (it != cend())
            return it->second;
        throw std::out_of_range("No such key, may be use []?");
    }

    Value& operator[](const Key& key) {
        auto it = find(key);
        if (it != end())
            return it->second;
        return insert({key, Value()}).first->second;
    }

    size_t size() const {
        return elements.size();
    }

    std::pair<iterator, bool> insert(const NodeType& node) {
        return emplace(node);
    }

    template <typename... Args>
    std::pair<iterator, bool> insert(Args&& ... args) {
        return emplace(std::forward<Args>(args)...);
    }

    template <typename InputIterator>
    void insert(InputIterator b, InputIterator e) {
        for (auto it = b; it != e; ++it)
            insert(*it);
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&& ... args) {
        Node* newNode = MapList::NodeAllocatorTraits::allocate(elements.nodeAlloc, 1);
        NodeType* keyValPos = &(newNode->val.keyVal);

        try {
            std::allocator_traits<Alloc>::construct(allocator, keyValPos, std::forward<Args>(args)...);
            newNode->val.hash = hasher(newNode->val.keyVal.first);
        } catch (...) {
            MapList::NodeAllocatorTraits::deallocate(elements.nodeAlloc, newNode, 1);
            throw;
        }

        try {
            auto to_ins = find(keyValPos->first);

            if (to_ins != end()) {
                std::allocator_traits<Alloc>::destroy(allocator, keyValPos);
                MapList::NodeAllocatorTraits::deallocate(elements.nodeAlloc, newNode, 1);
                return {to_ins, false};
            }

            size_t newHash = newNode->val.hash;

            typename MapList::baseNode* nextNodePtr = pointers[get_pos(newHash)];
            if (nextNodePtr == nullptr) {
                nextNodePtr = &elements.fakeNode;
            }

            elements.insertNode(nextNodePtr, newNode);

            pointers[get_pos(newHash)] = newNode;

            check_load();
            return {iterator(newNode), true};
        } catch (...) {
            std::allocator_traits<Alloc>::destroy(allocator, keyValPos);
            MapList::NodeAllocatorTraits::deallocate(elements.nodeAlloc, newNode, 1);
            throw;
        }
    }

    void erase(const_iterator toErase) {
        auto nextIt = std::next(toErase);
        auto nodeToErase = toErase.listIt.nodePtr;

        nodeToErase->prev->link(nodeToErase->next);
        --elements.sz;

        size_t hash = toErase.get_hash();
        size_t idx = get_pos(hash);

        if (pointers[idx] != nodeToErase)
            return;

        if (get_pos(nextIt.get_hash()) != idx) {
            pointers[idx] = nullptr;
            return;
        }

        pointers[idx] = static_cast<typename MapList::Node*>(nextIt.listIt.nodePtr);
    }

    void erase(const_iterator b, const_iterator e) {
        for (auto it = b; it != e; ++it)
            erase(it);
    }

    iterator find(const Key& key) {
        return static_cast<iterator>(const_find(key));
    }

    const_iterator find(const Key& key) const {
        return const_find(key);
    }

    void reserve(size_t newCap) {
        if(std::ceil(newCap / maxLoadFactor) < pointers.size()) {
            return;
        }
        rehash(std::ceil(newCap / maxLoadFactor));
    }

    void max_load_factor(float) {
        maxLoadFactor = max_load_factor();
        check_load();
    }

    float max_load_factor() const {
        return maxLoadFactor;
    }

    float load_factor() const {
        return size() / static_cast<float>(pointers.size());
    }

    iterator begin() {
        return iterator(elements.begin());
    }

    const_iterator begin() const {
        return const_iterator(elements.begin());

    }

    const_iterator cbegin() const {
        return const_iterator(elements.begin());
    }

    iterator end() {
        return iterator(elements.end());
    }

    const_iterator end() const {
        return const_iterator(elements.end());
    }

    const_iterator cend() const {
        return const_iterator(elements.end());
    }

private:
    template <bool isConst>
    class TemplateIterator {
        friend UnorderedMap;
        using iterType = std::conditional_t<isConst,
                typename MapList::const_iterator,
                typename MapList::iterator>;
    private:
        iterType listIt;

        size_t get_hash() const {
            return listIt.valPtr->hash;
        }

        explicit operator TemplateIterator<false>() const {
            return TemplateIterator<false>(static_cast<typename MapList::iterator>(listIt));
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using reference = typename std::conditional_t<isConst, const NodeType&, NodeType&>;
        using pointer = typename std::conditional_t<isConst, const NodeType*, NodeType*>;
        using value_type = typename std::conditional_t<isConst, const NodeType, NodeType>;
        using difference_type = std::ptrdiff_t;

        explicit TemplateIterator(Node* nodePtr) : listIt(nodePtr) {}

        explicit TemplateIterator(const iterType& iter) : listIt(iter) {}


        operator TemplateIterator<true>() const {
            return TemplateIterator<true>(listIt);
        }


        reference operator*() const {
            return listIt->keyVal;
        }

        pointer operator->() const {
            return &(listIt->keyVal);
        }

        TemplateIterator& operator++() {
            ++listIt;
            return *this;
        }

        TemplateIterator operator++(int) {
            return TemplateIterator(listIt++);
        }

        bool operator==(const TemplateIterator& that) const {
            return listIt == that.listIt;
        }

        bool operator!=(const TemplateIterator& that) const {
            return listIt != that.listIt;
        }
    };
};
