#pragma once

#include <algorithm>
#include <atomic>
#include <array>
#include <limits>
#include <vector>
#include <cassert>
#include <cstdint>

#include <tbb/enumerable_thread_specific.h>

namespace Epoch {

using epoch_timestamp_t = uint64_t;

static constexpr size_t epochCount = 3;

// TODO atomic changed flag

template<class Deleter>
struct ThreadEpoch {
    static tbb::enumerable_thread_specific<ThreadEpoch> epochs;
    static std::atomic<epoch_timestamp_t> globalEpochTS;

    std::atomic<epoch_timestamp_t> threadEpochTS { 1 };
    std::atomic<size_t> activeGuards { 0 };
    std::array<std::vector<void *>, epochCount> deletionLists;
    bool changed;

    ThreadEpoch() { }

    ~ThreadEpoch() {
//        printf("~ThreadEpoch()\n");
        for (size_t i = 0; i < epochCount; ++i) {
            cleanup(i);
        }
    }

    void addToDeletionList(void * ptr) {
        assert(activeGuards > 0);
        deletionLists[globalEpochTS].push_back(ptr);
        changed = true;
    }

    void cleanup(epoch_timestamp_t epochTS) {
        static Deleter deleter;

        auto & deletionList = deletionLists[epochTS];
        for (void * ptr : deletionList) {
//            printf("delete %p\n", ptr);
            deleter(ptr);
        }
        deletionList.resize(0);
    }

    void enter() {
        if (activeGuards == 0) {
            threadEpochTS = globalEpochTS.load();
            changed = false;
        }
        activeGuards += 1;
    }

    void exit() {
        if (activeGuards == 1) {
            activeGuards = 0;

            if (!changed) {
                return;
            }

            epoch_timestamp_t expectedGlobalTS = globalEpochTS;
            for (const auto & epoch : epochs) {
                // check if all threads are in the current (global) epoch
                if (epoch.activeGuards > 0 && epoch.threadEpochTS != expectedGlobalTS) {
                    return;
                }
            }

            epoch_timestamp_t newGlobalTS = (expectedGlobalTS + 1) % epochCount;
            if (globalEpochTS.compare_exchange_strong(expectedGlobalTS, newGlobalTS)) {
                cleanup((newGlobalTS + 1) % epochCount);
            }
        } else {
            activeGuards -= 1;
        }
    }
};

template<class Deleter>
tbb::enumerable_thread_specific<ThreadEpoch<Deleter>> ThreadEpoch<Deleter>::epochs;

template<class Deleter>
std::atomic<epoch_timestamp_t> ThreadEpoch<Deleter>::globalEpochTS = 1;

template<class Deleter>
class EpochGuard {
public:
    EpochGuard() {
        ThreadEpoch<Deleter>::epochs.local().enter();
    }

    ~EpochGuard() {
        ThreadEpoch<Deleter>::epochs.local().exit();
    }
};

template<class Deleter>
void addToDeletionList(void * ptr)
{
    ThreadEpoch<Deleter>::epochs.local().addToDeletionList(ptr);
}

} // end namespace Epoch
