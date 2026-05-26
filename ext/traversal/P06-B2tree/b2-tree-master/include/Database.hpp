#pragma once

#include "Page.hpp"

class Database {
    public:
    void allocatePage(PageId& pid, unsigned sizeClass);
};
