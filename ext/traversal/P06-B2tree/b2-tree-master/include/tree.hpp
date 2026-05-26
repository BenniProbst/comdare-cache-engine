#pragma once

#include <memory>
#include <experimental/propagate_const>

namespace BTree {

struct ByteArrayKey {
    uint8_t* start;
    size_t length;
};

template<class Key, class Payload>
class Tree {
    public:
    Tree();
//    Tree(BufferManager& bm);
    ~Tree();
    void insert(const Key& key, const Payload& payload);
    bool lookup(const Key& key, Payload& result) const;
    bool lookupWithPositionHint(const Key& key, Payload& result) const;
    bool lookupWithNestedNode(const Key& key, Payload& result) const;
    bool remove(const Key& key);
    bool validateInvariant() const;
    private:
    template<class InnerKey, class InnerPayload>
    struct Impl;
    std::experimental::propagate_const<std::unique_ptr<Impl<Key,  Payload>>> pImpl;
};

}
