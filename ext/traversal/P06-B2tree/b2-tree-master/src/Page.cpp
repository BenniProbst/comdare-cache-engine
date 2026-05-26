#include "Page.hpp"

#include <cassert>
#include <exception>

void SwipBase::createImpl(ExclusiveLatchBase& ptr, const PageLogic* pageLogic, PageId pid) const {

    auto bm;
    auto* frame = bm.allocateBufferFrame(pid.getPageSizeClass());

    frame->pid = pid;
    frame->logic = pageLogic;
    frame->swip.store(this);

    ptr.frame = frame;
//    ptr.myVersion = getVersion(frame.latchRaw);
    frame->latchRaw.store(setExclusive(frame->latchRaw.load()));
}

//////////////////////////////////////

using raw_type = uint64_t;

//constexpr raw_type obsoleteMask = 1ull << 0;
constexpr raw_type lockedExclusiveMask = 1ull << 0;
constexpr raw_type lockedSharedMask = 1ull << 1;
constexpr raw_type stateLockedMask = 1ull << 2;
constexpr raw_type stateMask = stateLockedMask - 1;

/*
static inline bool isObsolete(raw_type latchRaw) {
    return (latchRaw & obsoleteMask);
}
*/

static inline raw_type setShared(raw_type latchRaw) {
    return (latchRaw | lockedSharedMask);
}

static inline raw_type unsetShared(raw_type latchRaw) {
    return (latchRaw & ~lockedSharedMask);
}

static inline bool isShared(raw_type latchRaw) {
    return (latchRaw & lockedSharedMask);
}

static inline raw_type setExclusive(raw_type latchRaw) {
    return (latchRaw | lockedExclusiveMask);
}

static inline raw_type unsetExclusive(raw_type latchRaw) {
    return (latchRaw & ~lockedExclusiveMask);
}

static inline bool isExclusive(raw_type latchRaw) {
    return (latchRaw & lockedExclusiveMask);
}

static inline raw_type getVersion(raw_type latchRaw) {
    return (latchRaw & ~stateMask);
}

void lockState(BufferFrame* frame) {
    raw_type current;
    do {
        while (true) {
            current = frame->latchRaw.load();
            if (current & stateLockedMask) {
                _mm_pause();
            } else {
                break;
            }
        }
    } while (!frame->latchRaw.compare_exchange_weak(current, current | stateLockedMask));
}

void unlockState(BufferFrame* frame) {
    raw_type current;
    do {
        current = frame->latchRaw.load();
    } while (!frame->latchRaw.compare_exchange_weak(current, current & ~stateLockedMask));
}

class StateGuard {
    BufferFrame* frame;
    public:
    StateGuard(BufferFrame* frame) : frame(frame) {
        lockState(frame);
    }
    ~StateGuard() {
        unlockState(frame);
    }
};

void OptimisticLatchBase::reset() {
    frame = nullptr;
    myVersion = 0ul;
}

bool OptimisticLatchBase::validate() {
//    return (!isObsolete(myVersion) && myVersion == frame->latchRaw.load());
    return (!isExclusive(myVersion) && myVersion == frame->latchRaw.load());
}

bool OptimisticLatchBase::upgradeToImpl(SharedLatchBase& latch) {
    if (!validate()) { return false; }

    StateGuard guard(frame);
    auto current = frame->latchRaw.load();
    if (!isExclusive(current)) {
        return false;
    }
    auto prev = frame->sharedLatchCount.fetch_add(1);
    if (prev == 0) {
        frame->latchRaw.store(setShared(current));
    }

    latch.frame = frame;
    frame = nullptr;
    myVersion = 0;

    return true;
}

bool OptimisticLatchBase::upgradeToImpl(ExclusiveLatchBase& latch) {
    if (!validate()) { return false; }

    StateGuard guard(frame);
    auto current = frame->latchRaw.load();
    if (isExclusive(current) || isShared(current)) {
        return false;
    }
    frame->latchRaw.store(setExclusive(current));

    return true;
}

void SharedLatchBase::downgradeToImpl(OptimisticLatchBase& latch) {
    latch.frame = frame;
    latch.myVersion = getVersion(frame->latchRaw);
    reset();
}

void SharedLatchBase::reset() {
    StateGuard guard(frame);
    auto prev = frame->sharedLatchCount.fetch_sub(1);
    if (prev == 1) {
        frame->latchRaw.store(unsetShared(frame->latchRaw.load()));
    }

    frame = nullptr;
}

void ExclusiveLatchBase::resetModified(uint64_t lsn) {
    StateGuard guard(frame);
    auto current = frame->latchRaw.load();
    assert(isExclusive(current));
    auto update = unsetExclusive(current & stateMask);
    update += (current & ~stateMask) + (stateMask + 1);
    frame->latchRaw.store(update);

    frame = nullptr;
}

void ExclusiveLatchBase::resetUnmodified() {
    StateGuard guard(frame);
    auto current = frame->latchRaw.load();
    assert(isExclusive(current));
    auto update = unsetExclusive(current & stateMask);
    // version update is omitted
    frame->latchRaw.store(update);

    frame = nullptr;
}
