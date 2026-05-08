#include "BufferManager.hpp"
/*
class BufferFrame {
    public:
    PageId pid;
    std::atomic<SwipBase*> swip;
    const PageLogic* logic;
    Page* data;
    std::atomic<uint64_t> latchRaw = 0;
    std::atomic<uint64_t> sharedLatchCount = 0;
};
*/
BufferFrame* BufferManager::allocateBufferFrame(unsigned sizeClass) {
    size_t sizeInBytes = 0;

    auto frame = new BufferFrame();

    frame->pid = frame;
    
}
