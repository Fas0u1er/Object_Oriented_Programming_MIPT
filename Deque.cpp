#include <iostream>
#include <iterator>
#include <vector>
#include <cassert>

template <typename T>
class Deque {

private:
    static const size_t BLOCK_SIZE = 8;
    static const size_t DEFAULT_CAPACITY = 32;

    struct PtrDeque {
        //both ways expanding array of T*
        using V = T*;
        //V is exception-safe
    private:
        V* buffer;
        size_t idx_begin;
        size_t capacity;
        size_t size_;

        void reallocate() {
            V* new_buffer = new V[capacity * 3];
            size_t new_idx_begin = capacity + (capacity - size_) / 2;
            for (size_t i = 0; i < size_; ++i) {
                new_buffer[new_idx_begin + i] = buffer[idx_begin + i];
            }

            idx_begin = new_idx_begin;
            capacity *= 3;

            delete[] buffer;
            buffer = new_buffer;
        }

    public:
        void swap(PtrDeque& that) {
            std::swap(buffer, that.buffer);
            std::swap(idx_begin, that.idx_begin);
            std::swap(capacity, that.capacity);
            std::swap(size_, that.size_);
        }

        explicit PtrDeque(size_t cap = DEFAULT_CAPACITY / BLOCK_SIZE) :
                buffer(new V[cap]), idx_begin(cap / 2), capacity(cap),
                size_(0) {}

        PtrDeque(const PtrDeque& that) {
            idx_begin = that.idx_begin;
            capacity = that.capacity;
            size_ = that.size_;
            buffer = new V[capacity];
            for (size_t i = 0; i < size_; ++i) {
                buffer[idx_begin + i] = that.buffer[that.idx_begin + i];
            }
        }

        PtrDeque& operator=(const PtrDeque& that) {
            PtrDeque tmp(that);
            this->swap(tmp);
            return *this;
        }

        ~PtrDeque() {
            delete[] buffer;
        }

        void push_front(const V& elem) {
            if (idx_begin == 0) {
                reallocate();
            }
            buffer[--idx_begin] = elem;
            ++size_;
        }

        void push_back(const V& elem) {
            if (idx_begin + size_ == capacity)
                reallocate();
            buffer[idx_begin + size_] = elem;
            ++size_;
        }

        V back() const {
            return buffer[idx_begin + size_ - 1];
        }

        V front() const {
            return buffer[idx_begin];
        }

        void pop_back() {
            --size_;
        };

        void pop_front() {
            --size_;
            ++idx_begin;
        };


        size_t size() const {
            return static_cast<size_t>(size_);
        }

        V& operator[](size_t idx) {
            return buffer[idx_begin + idx];
        }

    };

    template <bool isConst>
    struct TemplateIterator {
        T** ptr_to_block_ptr;
        size_t block_pos;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using reference = std::conditional_t<isConst, const T&, T&>;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using difference_type = std::ptrdiff_t;

        TemplateIterator(T** ptr_to_block_ptr, size_t block_pos) :
                ptr_to_block_ptr(ptr_to_block_ptr), block_pos(block_pos) {}

        template <bool tmp>
        explicit TemplateIterator(const TemplateIterator<tmp>& that) :
                ptr_to_block_ptr(that.ptr_to_block_ptr), block_pos(that.block_pos) {}

        TemplateIterator& operator=(const TemplateIterator& that) {
            TemplateIterator tmp(that);
            this->swap(that);
            return *this;
        }

        operator TemplateIterator<true>() const {
            return TemplateIterator<true>(*this);
        }

        TemplateIterator& operator++() {
            if (block_pos == BLOCK_SIZE - 1) {
                block_pos = -1;
                ++ptr_to_block_ptr;
            }

            ++block_pos;
            return *this;
        }

        TemplateIterator& operator--() {
            if (block_pos == 0) {
                block_pos = BLOCK_SIZE;
                --ptr_to_block_ptr;
            }
            --block_pos;
            return *this;
        }

        TemplateIterator operator+(long long shift) const {
            TemplateIterator tmp(*this);

            if (static_cast<int>(block_pos) + shift >= 0) {
                tmp.ptr_to_block_ptr += (block_pos + shift) / BLOCK_SIZE;
                tmp.block_pos = (block_pos + shift) % BLOCK_SIZE;
            } else {
                //                assert(false);
                //C++ integer arithmetics is the best!
                tmp.ptr_to_block_ptr -= ((-(block_pos + shift) + BLOCK_SIZE - 1) / BLOCK_SIZE);
                tmp.block_pos = (BLOCK_SIZE - ((-(block_pos + shift)) % BLOCK_SIZE)) % BLOCK_SIZE;
            }

            return tmp;
        }

        TemplateIterator operator-(long long shift) const {
            return *this + (-shift);
        }

        std::ptrdiff_t operator-(const TemplateIterator& that) const {
            return static_cast<int>(BLOCK_SIZE) * (ptr_to_block_ptr - that.ptr_to_block_ptr)
                   + static_cast<int>(block_pos) - static_cast<int>(that.block_pos);
        }

        bool operator<(const TemplateIterator& that) const {
            return (*this) - that < 0;
        }

        bool operator>(const TemplateIterator& that) const {
            return that < *this;
        }

        bool operator<=(const TemplateIterator& that) const {
            return !((*this) > that);
        }

        bool operator>=(const TemplateIterator& that) const {
            return that <= *this;
        }

        bool operator==(const TemplateIterator& that) const {
            return (ptr_to_block_ptr == that.ptr_to_block_ptr) &&
                   (block_pos == that.block_pos);
        }

        bool operator!=(const TemplateIterator& that) const {
            return !(*this == that);
        }


        reference operator*() const {
            return *((*ptr_to_block_ptr) + block_pos);
        }

        pointer operator->() const {
            return (*ptr_to_block_ptr) + block_pos;
        }

        void swap(TemplateIterator& that) {
            std::swap(ptr_to_block_ptr, that.ptr_to_block_ptr);
            std::swap(block_pos, that.block_pos);
        }
    };

    void add_block_back() {
        external.push_back(reinterpret_cast<T*>(new uint8_t[BLOCK_SIZE * sizeof(T)]));
    }

    void add_block_front() {
        external.push_front(reinterpret_cast<T*>(new uint8_t[BLOCK_SIZE * sizeof(T)]));
    }

    void delete_block_back() {
        delete[] reinterpret_cast<uint8_t*>(external.back());
        external.pop_back();
    }

    void delete_block_front() {
        delete[] reinterpret_cast<uint8_t*>(external.front());
        external.pop_front();
    }//adding and deleting blocks

    PtrDeque external;
    TemplateIterator<false> begin_;
    size_t size_;

public:
    //    mutable std::vector<std::string> loger;
    using iterator = TemplateIterator<false>;
    using const_iterator = TemplateIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    iterator begin() {
        //loger.push_back("begin");
        return begin_;
    }

    const_iterator begin() const {
        //loger.push_back("begin const");

        return const_iterator(begin_);
    }

    iterator end() {
        //loger.push_back("end");

        return begin_ + size_;
    }

    const_iterator end() const {
        //loger.push_back("end const");

        return const_iterator(begin_ + size_);
    }

    const_iterator cbegin() const {
        //loger.push_back("cbegin");

        return const_iterator(begin_);
    }

    const_iterator cend() const {
        //loger.push_back("cend");

        return const_iterator(begin_ + size_);
    }

    reverse_iterator rbegin() {
        //loger.push_back("rbegin");

        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const {
        //loger.push_back("rbegin const");

        return const_reverse_iterator(end());
    }

    reverse_iterator rend() {
        //loger.push_back("rend");

        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const {
        //loger.push_back("rend const");

        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        //loger.push_back("crbegin");

        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const {
        //loger.push_back("crend");

        return const_reverse_iterator(begin());
    }//iterators end()/begin()

    void clear() {
        //loger.push_back("clear");

        while (size_ > 0) {
            pop_back();
        }
    }

    Deque() : external(DEFAULT_CAPACITY / BLOCK_SIZE), begin_(nullptr, 0), size_(0) {
        //loger.push_back("default c-tor");
    }

    Deque(const Deque& that) : Deque() {
        //loger.push_back("copy c-tor");
        try {
            for (auto i : that) {
                push_back(i);
            }
        } catch (...) {
            clear();
            throw;
        }
    }


    explicit Deque(size_t sz, const T& elem = T()) :
            external(std::max(3 * sz, size_t(DEFAULT_CAPACITY)) / BLOCK_SIZE), begin_(nullptr, 0), size_(0) {
        //loger.push_back("size_t c-tor");
        try {
            for (size_t i = 0; i < sz; ++i) {
                push_back(elem);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    ~Deque() {
        //loger.push_back("d-tor");
        //        std::cerr << loger.size() << "\n";
        clear();
    }

    void swap(Deque& that) {
        //loger.push_back("swap");

        external.swap(that.external);
        begin_.swap(that.begin_);
        std::swap(size_, that.size_);
    }

    Deque& operator=(const Deque& that) {
        //loger.push_back("operator =");

        Deque tmp(that);
        (this)->swap(tmp);
        return *this;
    }

    size_t size() const {
        //loger.push_back("size()");

        return size_;
    }

    const T& operator[](size_t idx) const {
        //loger.push_back("const []");

        return *(begin_ + idx);
    }

    T& operator[](size_t idx) {
        //loger.push_back("[]");

        return *(begin_ + idx);
    }

    T& at(size_t idx) {
        //loger.push_back("at()");

        if (idx >= size_ or idx < 0) {
            throw std::out_of_range("too far...");
        }
        return (*this)[idx];
    }

    const T& at(size_t idx) const {
        //loger.push_back("const at()");

        if (idx >= size_ or idx < 0) {
            throw std::out_of_range("too far...");
        }
        return (*this)[idx];
    }

    void push_back(const T& elem) {
        //loger.push_back("push_back");

        if (size_ == 0) {
            push_front(elem);
            return;
        }

        if (end().block_pos == 0) {
            add_block_back();
            begin_.ptr_to_block_ptr = &(external[0]);
        }

        try {
            new(&(*end())) T(elem);
        } catch (...) {
            if (end().block_pos == 0)
                delete_block_back();
            begin_.ptr_to_block_ptr = &(external[0]);
            throw;
        }

        ++size_;
        begin_.ptr_to_block_ptr = &(external[0]);
    }

    void pop_back() {
        //loger.push_back("pop_back");


        if (size_ == 1) {
            pop_front();
            return;
        }

        (--end())->~T();
        if ((--end()).block_pos == 0) {
            delete_block_back();
        }
        --size_;
    }

    void push_front(const T& elem) {
        //loger.push_back("push_front");

        if (size_ == 0) {
            add_block_front();
            begin_.ptr_to_block_ptr = &(external[0]);
            begin_.block_pos = BLOCK_SIZE;
        }

        if (begin_.block_pos == 0) {
            add_block_front();
            begin_.block_pos = BLOCK_SIZE - 1;
            begin_.ptr_to_block_ptr = &(external[0]);
        } else {
            --(begin_.block_pos);
        }

        try {
            new(&(*begin())) T(elem);
        } catch (...) {
            if (begin_.block_pos == BLOCK_SIZE - 1) {
                delete_block_front();
                //TODO:begin_.block_pos = -1;
            }
            ++(begin_.block_pos);
            begin_.ptr_to_block_ptr = &(external[0]);
            throw;
        }
        ++size_;
    }

    void pop_front() {
        //loger.push_back("pop_front");

        (begin())->~T();
        --size_;

        if (size_ == 0) {
            delete_block_front();
            begin_.ptr_to_block_ptr = nullptr;
            return;
        }

        if (begin_.block_pos == BLOCK_SIZE - 1) {
            delete_block_front();
        }
        ++begin_;
    }

    void insert(const iterator insert_it, const T& elem) {
        //loger.push_back("insert");

        if (size() == 0) {
            push_back(elem);
            return;
        }

        Deque<T> backup(*this);
        iterator backup_it = backup.begin() + (insert_it - (this->begin()));


        T tmp = *(--(backup.end()));
        for (iterator it = (--(backup.end())); it != backup_it; --it) {
            *(it) = *(it - 1);
        }
        *(backup_it) = elem;
        backup.push_back(tmp);

        *this = backup;
    }

    void erase(const iterator& erase_it) {
        //loger.push_back("erase");

        if (size_ == 1) {
            pop_back();
            return;
        }

        Deque<T> backup(*this);
        iterator backup_it = backup.begin() + (erase_it - (this->begin()));

        for (iterator it(backup_it); it != backup.end() - 1; ++it) {
            *(it) = *(it + 1);
        }
        backup.pop_back();

        *this = backup;
    }
};
