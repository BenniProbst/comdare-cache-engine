#pragma once

#include "Page.hpp"

class BufferManager {
    private:
    BufferFrame& getFrame(const Page* page);
    BufferFrame* allocateBufferFrame(unsigned sizeClass);
    BufferFrame* deallocateBufferFrame(BufferFrame* frame, unsigned sizeClass, bool doNotRemove);
    public:
    BufferFrame* loadPage(PageId pid, SwipBase* swip, const PageLogic* pageLogic);
    BufferFrame* createPage(PageId pid, SwipBase* swip, const PageLogic* pageLogic);
};
