#pragma once

#include <array>
#include <algorithm>
#include <atomic>
#include <array>
#include <cassert>

#include <tbb/enumerable_thread_specific.h>

namespace HazardPointers {

static constexpr size_t hazardPointerCount = 4;

/// Has to be specialised by the user
template<class T>
struct HazardPointerTraits {
    static const bool is_specialized = false;

    static bool isObsolete(void * ptr);

    static void applyDeleter(void * ptr);
};

template<class Traits>
struct HazardPointerContainer {
    static Traits traits;
    static tbb::enumerable_thread_specific<HazardPointerContainer> instance;

    std::array<std::atomic<void *>, hazardPointerCount> pointers { { nullptr } };

    void tryToReclaim(void * ptr) {
//        printf("tryToReclaim %p\n", ptr);
        for (const auto & container : instance) {
            if (this == &container) {
                continue;
            }

            const auto & remotePointers = container.pointers;
            for (size_t i = 0; i < hazardPointerCount; ++i) {
                if (remotePointers[i] == ptr) {
                    return;
                }
            }
        }

        traits.applyDeleter(ptr);
    }

    size_t protect(void * ptr) {
        for (size_t i = 0; i < hazardPointerCount; ++i) {
            if (pointers[i] == ptr) {
                return i;
            }
        }

        for (size_t i = 0; i < hazardPointerCount; ++i) {
            if (pointers[i] == nullptr) {
                pointers[i] = ptr;
                return i;
            }
        }

        throw std::runtime_error("no free slot available");
    }

    void unprotect(size_t ptrIndex) {
        void * ptr = pointers[ptrIndex];
        if (ptr == nullptr) {
            // reset() was called
            return;
        }
        if (traits.isObsolete(ptr)) {
            tryToReclaim(ptr);
        }
        pointers[ptrIndex] = nullptr;
    }

    void reset() {
        for (size_t i = 0; i < hazardPointerCount; ++i) {
            if (pointers[i] != nullptr) {
                unprotect(i);
            }
        }
    }

    bool validateOrReset(size_t ptrIndex, void * ptr) {
        bool valid = (pointers[ptrIndex] == ptr);
        if (!valid) {
            pointers[ptrIndex] = nullptr;
            reset();
        }
        return valid;
    }
};

template<class Traits>
tbb::enumerable_thread_specific<HazardPointerContainer<Traits>> HazardPointerContainer<Traits>::instance;

template<class Traits>
Traits HazardPointerContainer<Traits>::traits;

template<class Traits>
size_t protect(void * ptr) {
    return HazardPointerContainer<Traits>::instance.local().protect(ptr);
}

template<class Traits>
void unprotect(size_t ptrIndex) {
    HazardPointerContainer<Traits>::instance.local().unprotect(ptrIndex);
}

template<class Traits>
bool isProtected(void * ptr) {
    for (const auto & container : HazardPointerContainer<Traits>::instance) {
        for (size_t i = 0; i < hazardPointerCount; ++i) {
            if (container.pointers[i] == ptr) {
                return true;
            }
        }
    }
    return false;
}

template<class Traits>
bool validateOrReset(size_t ptrIndex, void * ptr) {
    return HazardPointerContainer<Traits>::instance.local().validateOrReset(ptrIndex, ptr);
}

template<class Traits>
class HazardPointerGuard {
    void * ptr = nullptr;
    size_t ptrIndex = 0;

public:
    HazardPointerGuard(const HazardPointerGuard &) = delete;
    HazardPointerGuard & operator=(const HazardPointerGuard &) = delete;

    HazardPointerGuard() { }

    HazardPointerGuard(void * ptr)
        : ptr(ptr)
    {
        ptrIndex = protect<Traits>(ptr);
    }

    ~HazardPointerGuard() {
        if (ptr) {
            unprotect<Traits>(ptrIndex);
        }
    }

    void swap(HazardPointerGuard & other) {
        std::swap(ptr, other.ptr);
        std::swap(ptrIndex, other.ptrIndex);
    }

    size_t index() const {
        return ptrIndex;
    }

    void adoptAndUnprotectPrevious(void * newPtr) {
        if (ptr) {
            unprotect<Traits>(ptrIndex);
        }
        ptr = newPtr;
        ptrIndex = protect<Traits>(newPtr);
    }

    void reset() {
        if (ptr) {
            unprotect<Traits>(ptrIndex);
            ptr = nullptr;
        }
    }

    bool validateOrResetPointers(void * ptr) {
        bool valid = validateOrReset<Traits>(ptrIndex, ptr);
        if (!valid) {
            ptr = nullptr;
        }
        return valid;
    }
};

} // end namespace HazardPointers
