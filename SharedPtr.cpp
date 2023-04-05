#include <memory>

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&& ... args);

struct BaseControlBlock {
    size_t strongCnt = 0;
    size_t weakCnt = 0;

    virtual void* getObjPtr() = 0;

    virtual void destroyObj() = 0;

    virtual void BlockDestruction() = 0;
};

template <typename T>
class SharedPtr {

    template <typename Y, typename Alloc, typename... Args>
    friend SharedPtr<Y> allocateShared(Alloc alloc, Args&& ... args);

    template <typename Y>
    friend
    class WeakPtr;

    template <typename Y>
    friend
    class SharedPtr;

private:
    template <typename Y, typename Deleter, typename Alloc>
    struct ControlBlockRegular : public BaseControlBlock {
        using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockRegular<Y, Deleter, Alloc>>;
        using BlockAllocTraits = typename std::allocator_traits<BlockAlloc>;

        Deleter del;
        Alloc alloc;
        Y* obj;

        ControlBlockRegular(Y* obj, Deleter del, Alloc alloc) : del(del), alloc(alloc) {
            this->obj = obj;
        }

        void destroyObj() override {
            del(this->obj);
            this->obj = nullptr;
        }

        void* getObjPtr() override {
            return obj;
        }

        void BlockDestruction() override {
            auto thisPtr = this;
            BlockAlloc allocTmp = std::move(alloc);

            //            BlockAllocTraits::destroy(allocTmp, thisPtr);
            thisPtr->~ControlBlockRegular();
            BlockAllocTraits::deallocate(allocTmp, thisPtr, 1);
        }
    };

    template <typename Alloc>
    struct ControlBlockMakeShared : public BaseControlBlock {
        using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockMakeShared<Alloc>>;
        using BlockAllocTraits = typename std::allocator_traits<BlockAlloc>;
        using TAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
        using TAllocTraits = typename std::allocator_traits<TAlloc>;

        alignas(T) uint8_t spaceForObj[sizeof(T)]{};
        Alloc alloc;

        template <typename ... Args>
        explicit ControlBlockMakeShared(Alloc alloc, Args&& ... args) : alloc(alloc) {
            TAlloc allocObj(alloc);
            TAllocTraits::construct(allocObj, reinterpret_cast<T*>(spaceForObj), std::forward<Args>(args)...);
        }

        void destroyObj() override {
            TAlloc objAllocator(alloc);
            TAllocTraits::destroy(objAllocator, reinterpret_cast<T*>(spaceForObj));
        }

        void* getObjPtr() override {
            return reinterpret_cast<T*>(spaceForObj);
        }

        void BlockDestruction() override {
            auto thisPtr = this;
            BlockAlloc allocTmp(std::move(alloc));

            thisPtr->~ControlBlockMakeShared();
            BlockAllocTraits::deallocate(allocTmp, thisPtr, 1);
        }
    };

private:
    BaseControlBlock* blockPtr = nullptr;
    T* obj = nullptr;

private:

    explicit SharedPtr(BaseControlBlock* block) : blockPtr(block) {
        if (!block) {
            blockPtr = nullptr;
            return;
        }
        obj = reinterpret_cast<T*>(block->getObjPtr());
        blockPtr->strongCnt++;
    }

public:
    SharedPtr() = default;

    void reset() {
        SharedPtr().swap(*this);
    }

    void swap(SharedPtr<T>& that) {
        std::swap(obj, that.obj);
        std::swap(blockPtr, that.blockPtr);
    }

    template <typename Y,
            typename Deleter = std::default_delete<Y>,
            typename Alloc = std::allocator<Y>,
            std::enable_if_t<
                    std::is_convertible_v<Y*, T*>, bool> = true>
    SharedPtr(Y* ptr, Deleter del = Deleter(), Alloc alloc = Alloc()) {
        using ControlBlock = ControlBlockRegular<Y, Deleter, Alloc>;
        using BlockAlloc = typename ControlBlock::BlockAlloc;
        using BlockAllocTraits = typename ControlBlock::BlockAllocTraits;

        BlockAlloc blockAlloc(alloc);
        auto regularBlockPtr = BlockAllocTraits::allocate(blockAlloc, 1);
        ::new(regularBlockPtr) ControlBlock(ptr, del, alloc);
        obj = ptr;
        blockPtr = regularBlockPtr;
        blockPtr->strongCnt++;
    }


    SharedPtr(const SharedPtr& that) : SharedPtr(that.blockPtr) {}

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    SharedPtr(const SharedPtr<Y>& that) : blockPtr(that.blockPtr), obj(that.obj) {
        blockPtr->strongCnt++;
    }

    SharedPtr(SharedPtr&& that) noexcept: blockPtr(that.blockPtr), obj(that.obj) {
        that.blockPtr = nullptr;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    SharedPtr(SharedPtr<Y>&& that) noexcept : blockPtr(that.blockPtr), obj(that.obj) {
        that.blockPtr = nullptr;
    }

    SharedPtr& operator=(const SharedPtr& that) {
        SharedPtr(that).swap(*this);
        return *this;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    SharedPtr& operator=(const SharedPtr<Y>& that) {
        SharedPtr(that).swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& that) noexcept {
        SharedPtr(std::move(that)).swap(*this);
        return *this;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    SharedPtr& operator=(SharedPtr<Y>&& that) noexcept {
        SharedPtr(std::move(that)).swap(*this);
        return *this;
    }

    size_t use_count() const {
        return blockPtr ? blockPtr->strongCnt : 0;
    }

    template <typename Y>
    void reset(Y* ptr) {
        SharedPtr(ptr).swap(*this);
    }

    ~SharedPtr() {
        if (!blockPtr)
            return;


        if (blockPtr->strongCnt == 1) {
            blockPtr->destroyObj();
            if (blockPtr->weakCnt == 0) {
                blockPtr->BlockDestruction();
                return;
            }
        }

        blockPtr->strongCnt--;
    }

    T* get() const {
        return obj;
    }

    T* operator->() const {
        return get();
    }

    T& operator*() const {
        return *get();
    }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&& ... args) {
    using ControlBlock = typename SharedPtr<T>::template ControlBlockMakeShared<Alloc>;
    using BlockAlloc = typename ControlBlock::BlockAlloc;
    using BlockAllocTraits = typename ControlBlock::BlockAllocTraits;

    BlockAlloc blockAlloc(alloc);

    ControlBlock* blockPtr = BlockAllocTraits::allocate(blockAlloc, 1);

    ::new(blockPtr) ControlBlock(alloc, std::forward<Args>(args)...);

    return SharedPtr<T>(static_cast<BaseControlBlock*>(blockPtr));
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&& ... args) {
    return allocateShared<T, std::allocator<T>, Args...>(std::allocator<T>(), std::forward<Args>(args)...);
}


template <typename T>
class WeakPtr {

    template <typename Y>
    friend
    class WeakPtr;

    template <typename Y>
    friend
    class SharedPtr;

private:
    BaseControlBlock* blockPtr = nullptr;

    explicit WeakPtr(BaseControlBlock* block) : blockPtr(block) {
        blockPtr->weakCnt++;
    }

public:
    WeakPtr() = default;

    void swap(WeakPtr& that) {
        std::swap(blockPtr, that.blockPtr);
    }

    template <class Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr(const SharedPtr<Y>& shPtr) : WeakPtr(shPtr.blockPtr) {}

    WeakPtr(const WeakPtr& that) : WeakPtr(that.blockPtr) {}

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr(const WeakPtr<Y>& that) : blockPtr(that.blockPtr) {
        blockPtr->weakCnt++;
    }

    WeakPtr(WeakPtr&& that) noexcept: blockPtr(that.blockPtr) {
        that.blockPtr = nullptr;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr(WeakPtr<Y>&& that) noexcept : blockPtr(that.blockPtr) {
        blockPtr->weakCnt++;
        that.blockPtr = nullptr;
    }

    WeakPtr& operator=(const WeakPtr& that) {
        WeakPtr(that).swap(*this);
        return *this;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr& operator=(const WeakPtr<Y>& that) {
        WeakPtr(that).swap(*this);
        return *this;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr& operator=(const SharedPtr<Y>& that) {
        WeakPtr(that).swap(*this);
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& that) noexcept {
        WeakPtr(std::move(that)).swap(*this);
        return *this;
    }

    template <typename Y, std::enable_if_t<
            std::is_convertible_v<Y*, T*>, bool> = true>
    WeakPtr& operator=(WeakPtr<Y>&& that) noexcept {
        WeakPtr(std::move(that)).swap(*this);
        return *this;
    }

    bool expired() const {
        return !blockPtr || (blockPtr->strongCnt == 0);
    }

    SharedPtr<T> lock() const {
        return expired() ? SharedPtr<T>() : SharedPtr<T>(blockPtr);
    }

    ~WeakPtr() {
        if (!blockPtr)
            return;

        blockPtr->weakCnt--;

        if ((blockPtr->strongCnt == 0) && (blockPtr->weakCnt == 0)) {
            blockPtr->BlockDestruction();
        }
    }

    size_t use_count() const {
        return blockPtr ? blockPtr->strongCnt : 0;
    }
};
