#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>

#include "NonCopyable.hpp"

class SwipBase;

class Page {
    public:
    static constexpr unsigned minPageSizeShift = 16;
    static constexpr uint64_t minPageSize = 1ull << minPageSizeShift;
};

//using PageId = Page*;
//using PageId = BufferFrame*;

class PageId {
    unsigned sizeClass;
    public:
    PageId(uint64_t pageNo, unsigned pageSizeClass) : sizeClass(pageSizeClass) {}
    unsigned getPageSizeClass() const {
        return sizeClass;
    }
    uint64_t getPageSizeInBytes() const {
        throw 0; // TODO
    }
    static unsigned computePageSizeClass(uint64_t size) {
        throw 0; // TODO
    }
};

class PageLogic {
    public:
    virtual SwipBase* unswizzle(Page* page, bool checkOnly) const = 0;
};

template<class T>
class PageAdapter : public PageLogic {
    public:
    SwipBase* unswizzle(Page* page, bool checkOnly) const override {
        return T::unswizzle(*static_cast<T*>(page), checkOnly);
    }
    static const PageAdapter instance;
};

template<class T>
const PageAdapter<T> PageAdapter<T>::instance;

class BufferFrame {
    public:
    PageId pid;
    std::atomic<SwipBase*> swip;
    const PageLogic* logic;
    Page* data;
    std::atomic<uint64_t> latchRaw = 0;
    std::atomic<uint64_t> sharedLatchCount = 0;
};

class LatchBase {
    protected:
    BufferFrame* frame;
    LatchBase() = default;
    explicit LatchBase(BufferFrame* frame) : frame(frame) {}
    ~LatchBase() = default;

    friend class SwipBase;
};

class SharedLatchBase;

template<class T>
class SharedLatch;

class ExclusiveLatchBase;

template<class T>
class ExclusiveLatch;

class OptimisticLatchBase : public LatchBase, private NonCopyable<OptimisticLatchBase> {
    public:
    using version_type = uint64_t;

    OptimisticLatchBase() = default;

    void reset();

    bool validate();

    protected:
    version_type myVersion = 0;

    OptimisticLatchBase(OptimisticLatchBase&& other) : LatchBase(other.frame), myVersion(other.myVersion) {
        other.frame = nullptr;
        other.myVersion = 0ul;
    }


    bool upgradeToImpl(SharedLatchBase& latch);
    bool upgradeToImpl(ExclusiveLatchBase& latch);

    friend class SharedLatchBase;
//    friend class SwipBase;
};

template<class T>
class OptimisticLatch : public OptimisticLatchBase {
    public:
    OptimisticLatch() = default;
    OptimisticLatch(OptimisticLatch&& other);

    bool upgradeTo(SharedLatch<T>& latch) {
        return upgradeToImpl(latch);
    }

    bool upgradeTo(ExclusiveLatch<T>& latch) {
        return upgradeToImpl(latch);
    }
};

class NonOptimisticLatchBase : public LatchBase {
};

class SharedLatchBase : public NonOptimisticLatchBase, private NonCopyable<SharedLatchBase> {
    public:
    void reset();
    protected:
    void downgradeToImpl(OptimisticLatchBase& latch);

    friend class OptimisticLatchBase;
};

template<class T>
class SharedLatch : public SharedLatchBase {
    public:
    SharedLatch() = default;

    void downgradeTo(OptimisticLatch<T> latch) {
        downgradeToImpl(latch);
    }
};

class ExclusiveLatchBase : public NonOptimisticLatchBase, private NonCopyable<ExclusiveLatchBase> {
    public:
    ExclusiveLatchBase() = default;

    void resetModified(uint64_t lsn);
    void resetUnmodified();
};

template<class T>
class ExclusiveLatch : public ExclusiveLatchBase {
    public:
    ExclusiveLatch() = default;
};

class SwipBase {
    public:
    SwipBase() = default;
    protected:
    void setPointer(BufferFrame* frame) {
        alwaysPointer = frame;
    }
    void swizzle(const PageLogic* pageLogic, BufferFrame*& ptr, uint64_t& version, unsigned flags) const {
        // NOP
    }
    void moveFrom(SwipBase&& other);
    void createImpl(ExclusiveLatchBase& ptr, const PageLogic* pageLogic, PageId pid) const;
    protected:
    BufferFrame* alwaysPointer;
};

template<class T>
class Swip : public SwipBase {
    void setPointer(BufferFrame* frame) {
        SwipBase::setPointer(frame);
    }
    [[nodiscard]] bool tryLoadOptimistic(OptimisticLatch<T>& ptr, const OptimisticLatchBase& outerLatch) const;
    void loadOptimistic(OptimisticLatch<T>& ptr, const NonOptimisticLatchBase& /* outerLatch */) const;
    void create(ExclusiveLatch<T>& ptr, PageId pid, const ExclusiveLatchBase& /* outerLatch */) const {
        createImpl(ptr, &PageAdapter<T>::instance, pid);
    }
};
