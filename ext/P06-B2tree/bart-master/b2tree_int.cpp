#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sys/time.h>
#include <map>
#include <vector>
#include <csignal>
#include <cmath>
#include <limits>
#include <sstream>

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

//#include "profile.cpp"
#include "profile.hpp"
#include "utils.hpp"

#define SIMPLE_SLOTS

#define trap(expr) if (!!(expr)) raise(SIGTRAP);

using namespace std;

static bool dump = false;

struct __attribute__((packed)) TrieNode {
//    uint8_t ordinal; // Preorder number of this node (important to get the node located right after a span)
    struct {
        uint8_t leaf : 1;
        uint8_t spanCount : 7;
    };
    union {
        struct {
            uint8_t edges[4];
            uint8_t children[4];
        };
        uint64_t span; // padded with zeros
    };
};

using tree_key_t = uint64_t;

struct Node {
   static const unsigned nodeSize=64*1024;
   static const unsigned payloadSize=sizeof(Node*);

   bool isLeaf;
   uint16_t count;

   union {
      Node* upper; // inner
      Node* next;  // leaf
   };

    uint16_t trieSize;
    uint8_t trieData[4*1024];

   struct {
        tree_key_t key;
        union {
            Node* child;
            uint64_t payload;
        };
   } slots[1];

    uint8_t data[1];

    Node(bool isLeaf) : isLeaf(isLeaf), count(0) {}

    inline TrieNode& trieRoot() {
        return *reinterpret_cast<TrieNode*>(trieData);
    }

    inline TrieNode& getNode(uint8_t ordinal) {
        return *(reinterpret_cast<TrieNode*>(trieData) + ordinal);
    }

    inline TrieNode& getNext(const TrieNode& span) {
        return *(const_cast<TrieNode*>(&span) + 1);
    }

    inline bool validateNodeOperation(const TrieNode* parent, const TrieNode* descendant) {
        if (descendant <= parent) { return false; }
        if (reinterpret_cast<const uint8_t*>(descendant + 1) >= reinterpret_cast<const uint8_t*>(this) + nodeSize) { return false; }
        return true;
    }

    inline uint16_t* rangeArray() {
        return reinterpret_cast<uint16_t*>(trieData + trieSize);
    }

#ifndef SIMPLE_SLOTS
    inline Slot* slots() {
//        return reinterpret_cast<Slot*>(data + slotsBegin);
        return reinterpret_cast<Slot*>(data);
    }
#endif

    using node_ordinal_t = uint8_t;
    using range_array_idx_t = uint8_t;

    range_array_idx_t minimum(const TrieNode& node) {
        const TrieNode* currentNode = &node;
        while (true) {
            // navigate to the next decision node
            while (currentNode->spanCount > 0) {
                currentNode = &getNext(*currentNode);
            }
            if (currentNode->leaf) {
                return currentNode->children[0];
            }
            node_ordinal_t first = currentNode->children[0];
            auto childNode = &getNode(first);
            if (!validateNodeOperation(currentNode, childNode)) { return 1; }
            currentNode = childNode;
        }
    }

    range_array_idx_t maximum(const TrieNode& node) {
        const TrieNode* currentNode = &node;
        while (true) {
            // navigate to the next decision node
            while (currentNode->spanCount > 0) {
                currentNode = &getNext(*currentNode);
            }
            if (currentNode->leaf) {
                return currentNode->children[sizeof(currentNode->children) - 1];
            }
            node_ordinal_t last = currentNode->children[sizeof(currentNode->children) - 1];
            auto childNode = &getNode(last);
            if (!validateNodeOperation(currentNode, childNode)) { return 1; }
            currentNode = childNode;
        }
    }

    range_array_idx_t traverse(const TrieNode& node, tree_key_t queryKey) {
        tree_key_t reversed = __builtin_bswap64(queryKey);
        size_t len = sizeof(tree_key_t);
        uint8_t* key = reinterpret_cast<uint8_t*>(&reversed);

        const TrieNode* currentNode = &node;
        while (true) {
            node_ordinal_t child = 0;

            if (currentNode->spanCount == 0) {
                uint8_t currentByte = len > 0 ? key[0] : uint8_t(0);
                auto lastIdx = sizeof(currentNode->children) - 1;
                child = currentNode->children[lastIdx]; // larger keys default to the last range
                for (size_t i = 0; i < lastIdx; ++i) {
                    // equivalent ranges will be repeated at the end
                    if (currentByte <= currentNode->edges[i]) {
                        child = currentNode->children[i];
                        break;
                    }
                }

                if (currentNode->leaf) {
                    return child;
                } else {
                    auto childNode = &getNode(child);
                    if (!validateNodeOperation(currentNode, childNode)) { return 1; }
                    currentNode = childNode;
                    continue;
                }
            } else {
                assert(!currentNode->leaf);
/*
                decltype(currentNode->span) keyPart = 0; // pad with zeros
                std::memcpy(&keyPart, key, std::min<size_t>(currentNode->spanCount, len));
                keyPart = __builtin_bswap64(keyPart);
*/
                bool smaller = false;
                bool greater = false;
                const uint8_t* span = reinterpret_cast<const uint8_t*>(&currentNode->span);
                for (size_t i = 0; i < currentNode->spanCount; ++i) {
                    smaller |= key[i] < span[i];
                    greater |= key[i] > span[i];
                }
                if (greater) {
                    return maximum(*currentNode);
                } else if (smaller) {
                    return minimum(*currentNode);
                } else {
                    key += currentNode->spanCount;
                    len -= currentNode->spanCount;
                    auto childNode = &getNext(*currentNode);
                    if (!validateNodeOperation(currentNode, childNode)) { return 1; }
                    currentNode = childNode;
                    continue;
                }
            }
        }
    }

    std::pair<size_t, size_t> drtLookup(tree_key_t key) {
        const auto& root = trieRoot();
        int idx = traverse(root, key);
        idx = std::max(idx, 1); // optimistic safety

        size_t lower = rangeArray()[idx - 1];
        size_t upper = rangeArray()[idx];

        // backward search to account for dynamic changes within the range array
        idx -= 2;
        while (lower == upper && idx >= 0) {
            lower = rangeArray()[idx];
            idx -= 1;
        }

        lower = std::min<size_t>(lower, count); // optimistic safety
        upper = std::min<size_t>(upper, count); // optimistic safety

        uint8_t* start = reinterpret_cast<uint8_t*>(&slots[lower]);
        __builtin_prefetch(start);
        __builtin_prefetch(start + 64);
        __builtin_prefetch(start + 128);
        __builtin_prefetch(start + 192);

        return {lower, upper};
    }

    struct Context {
        Context() = default;
        Context(const Context& other) = delete;
//        uint8_t trieData[4*1024];
        unsigned offset = 0;
        unsigned nextOrdinal = 0;
        unsigned unterminatedCount = 0;
        std::vector<uint16_t> rangeBounds;
    };

    inline bool drtNodeQuotaExhausted(Context& ctx, unsigned additionalyRequired = 0) {
        const unsigned requiredCnt = ctx.nextOrdinal + ctx.unterminatedCount + additionalyRequired;
        return (requiredCnt > std::numeric_limits<node_ordinal_t>::max());
    }

    inline bool tryReserveDrtNodes(Context& ctx, unsigned count) {
        if (drtNodeQuotaExhausted(ctx, count)) {
            return false;
        }
        ctx.unterminatedCount += count;
        return true;
    }

    inline void drtNodesTerminated(Context& ctx, unsigned count) {
        assert(count <= ctx.unterminatedCount);
        ctx.unterminatedCount -= count;
    }

    void fetchKeyExcerpt(size_t slotNum, uint8_t* dest, size_t len, size_t keyDepth) {
        size_t keyLength = sizeof(tree_key_t);
        size_t len2 = std::min<size_t>(len, keyLength - keyDepth);
        for (size_t i = 0; i < len2; ++i) {
            dest[i] = getKeyByte(slotNum, i + keyDepth);
        }
        // pad with zeros
        for (size_t i = len2; i < len; ++i) {
            dest[i] = 0;
        }
    }

    std::tuple<TrieNode&, node_ordinal_t> allocateTrieNode(Context& ctx) {
        assert(ctx.nextOrdinal <= std::numeric_limits<node_ordinal_t>::max());
        node_ordinal_t o = ctx.nextOrdinal++;
        TrieNode& node = getNode(o);
        new (&node) TrieNode();
        node.leaf = 0;
        node.spanCount = 0;
        return {node, o};
    }

    template<class T, class CmpFunc>
    size_t lowerBound(size_t lower, size_t upper, const T& value, CmpFunc cmpFunc) {
        size_t count, step;
        count = upper - lower;

        while (count > 0) {
            auto pos = lower;
            step = count / 2;
            pos += step;
            if (cmpFunc(pos, value)) {
                lower = pos + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }
        return lower;
    }

    template<class T, class CmpFunc>
    size_t upperBound(size_t lower, size_t upper, const T& value, CmpFunc cmpFunc) {
        size_t count, step;
        count = upper - lower;

        while (count > 0) {
            auto pos = lower;
            step = count / 2;
            pos += step;
            if (!cmpFunc(value, pos)) {
                lower = pos + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }
        return lower;
    }

    void divide(size_t firstIdx, size_t lastIdx, size_t keyDepth, size_t targetSplitDepth, std::vector<size_t>& result) {
        if (targetSplitDepth == 0) return;
        if (lastIdx <= firstIdx) return;

        if (getKeyByte(firstIdx, keyDepth) == getKeyByte(lastIdx, keyDepth)) {
            // a span will be constructed in this case
            return;
        }

        const size_t rangeSize = (1 + lastIdx - firstIdx);
//        if (rangeSize < 2) return;
        if (rangeSize < 24) return;
        size_t preliminarySplitPoint = firstIdx + rangeSize/2;

        auto cmpNthByte = [&](size_t a, size_t b) {
            return getKeyByte(a, keyDepth) < getKeyByte(b, keyDepth);
        };
        // TODO try both directions and take the one resulting in a more balanced split
        size_t splitPoint = upperBound(firstIdx, lastIdx + 1, preliminarySplitPoint, cmpNthByte) - 1;
        if (splitPoint >= lastIdx) { // no information gain
            // search in the opposite direction
            splitPoint = lowerBound(firstIdx, lastIdx + 1, preliminarySplitPoint, cmpNthByte);
            assert(splitPoint > firstIdx);
            splitPoint -= 1; // take the upper bound of the previous byte at position 'keyDepth'
        }
        assert(splitPoint >= firstIdx);
        assert(splitPoint < lastIdx);

        size_t minDist = std::min(splitPoint - firstIdx, lastIdx - splitPoint); // min{left_distance, right_distance}
        if (minDist < 4) { return; }

        divide(firstIdx, splitPoint - 1, keyDepth, targetSplitDepth - 1, result);
        result.push_back(splitPoint);
        divide(splitPoint + 1, lastIdx, keyDepth, targetSplitDepth - 1, result);
    }

    inline uint8_t getKeyByte(size_t slotNum, size_t byteNum) {
        assert(byteNum < sizeof(tree_key_t));
        auto& slot = slots[slotNum];
        tree_key_t reversed = __builtin_bswap64(slot.key);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&reversed);
        return ptr[byteNum];
    }

    node_ordinal_t constructBranchyNode(Context& ctx, size_t firstIdx, size_t lastIdx, size_t trieDepth, size_t keyDepth, bool forceLeaf = false) {
        assert(firstIdx <= lastIdx);
        std::vector<size_t> splitPoints;
        size_t count = 1 + lastIdx - firstIdx;

        auto [node, ordinal] = allocateTrieNode(ctx);
        node.spanCount = 0;

        size_t targetSplitDepth = std::min<size_t>(2, std::abs(std::log2(count))); // min(2, abs(log2(count)))
        divide(firstIdx, lastIdx, keyDepth, targetSplitDepth, splitPoints);
        splitPoints.push_back(lastIdx);

        bool isLeaf = forceLeaf || (splitPoints.size() == 1);
        isLeaf |= drtNodeQuotaExhausted(ctx);
        if (!isLeaf) {
            int prev = firstIdx - 1;
            size_t maxSubRangeSize = 0;
            for (size_t splitPoint : splitPoints) {
                size_t subRangeSize = 1 + splitPoint - prev;
                maxSubRangeSize = std::max(maxSubRangeSize, subRangeSize);
                prev = splitPoint;
            }
            isLeaf = (maxSubRangeSize <= 24);
        }
        if (!isLeaf) {
            bool nodeQuotaExhausted = !tryReserveDrtNodes(ctx, splitPoints.size());
            isLeaf |= nodeQuotaExhausted;
        }
        node.leaf = isLeaf;

        size_t i = 0;
        int prev = firstIdx - 1; // reset
        for (size_t splitPoint : splitPoints) {
            uint8_t currentByte = getKeyByte(splitPoint, keyDepth);
            node.edges[i] = currentByte;
            if (isLeaf) {
//                cout << "add range upper bound: " << splitPoint + 1 << " page count: " << this->count << endl;
                ctx.rangeBounds.push_back(splitPoint + 1); // upper bound (excluded)
                size_t rangeIdx = ctx.rangeBounds.size() - 1;
                assert(rangeIdx < 255);
                node.children[i] = static_cast<uint8_t>(rangeIdx);
            } else {
                node_ordinal_t childOrdinal = constructNode(ctx, prev + 1, splitPoint, trieDepth + 1, keyDepth);
                node.children[i] = childOrdinal;

                drtNodesTerminated(ctx, 1);
            }
            prev = splitPoint;
            i += 1;
        }
        size_t last = splitPoints.size() - 1;
        for (size_t j = i; j < 4; ++j) {
            node.edges[j] = node.edges[last];
            node.children[j] = node.children[last];
        }

        return ordinal;
    }

#pragma GCC push_options
#pragma GCC optimize ("O0")
    size_t __attribute__ ((noinline)) commonPrefixBetween(size_t slotNum1, size_t slotNum2) {
        assert(slotNum1 != slotNum2);
        tree_key_t reversedKey1 = __builtin_bswap64(slots[slotNum1].key);
        tree_key_t reversedKey2 = __builtin_bswap64(slots[slotNum2].key);
        uint8_t* view1 = reinterpret_cast<uint8_t*>(&reversedKey1);
        uint8_t* view2 = reinterpret_cast<uint8_t*>(&reversedKey2);
        size_t len = sizeof(tree_key_t);
        for (size_t i = 0; i < len; ++i) {
            if (view1[i] != view2[i]) {
                return i;
            }
        }
        return len;
    }
#pragma GCC pop_options

    node_ordinal_t constructNode(Context& ctx, size_t firstIdx, size_t lastIdx, size_t trieDepth, size_t keyDepth) {
        if (drtNodeQuotaExhausted(ctx, 2)) { // span+leaf
//            return constructTerminationLeaf();
            return constructBranchyNode(ctx, firstIdx, lastIdx, trieDepth, keyDepth, true);
        }

        size_t spanLength = commonPrefixBetween(firstIdx, lastIdx) - keyDepth; // TODO can be limited to the max span lenght
//cout << "full span len: " << spanLength << endl;
        if (spanLength == 0) {
            return constructBranchyNode(ctx, firstIdx, lastIdx, trieDepth, keyDepth);
        }

        node_ordinal_t finalOrdinal = std::numeric_limits<node_ordinal_t>::max();
//        while (spanLength > 0) {
//        if (spanLength > 0) {
            auto len = std::min(sizeof(TrieNode::span), spanLength);
            decltype(TrieNode::span) keyPart = 0;
            fetchKeyExcerpt(firstIdx, reinterpret_cast<uint8_t*>(&keyPart), len, keyDepth);
//cout << "span: "; printChars(reinterpret_cast<char*>(&keyPart), len); cout << endl;
            auto [node, ordinal] = allocateTrieNode(ctx);
            bool nodeQuotaExhausted = !tryReserveDrtNodes(ctx, 1);
            assert(!nodeQuotaExhausted);
            node.leaf = false;
            node.spanCount = len;
            node.span = keyPart;// __builtin_bswap64(keyPart);

            keyDepth += len;
//            spanLength -= len;

            finalOrdinal = std::min(finalOrdinal, ordinal);
//        }

        constructNode(ctx, firstIdx, lastIdx, trieDepth + 1, keyDepth);
drtNodesTerminated(ctx, 1);

        return finalOrdinal;
    }

    Node* constructTrie() {
        Context ctx;
        ctx.rangeBounds.push_back(0);
        constructNode(ctx, 0, count - 1, 0, 0);

        size_t trieSize = ctx.nextOrdinal*sizeof(TrieNode);
        this->trieSize = trieSize;

//        uint16_t* destRangeArray = reinterpret_cast<uint16_t*>(ctx.trieData + trieSize);
        uint16_t* destRangeArray = reinterpret_cast<uint16_t*>(trieData + trieSize);
        for (size_t i = 0; i < ctx.rangeBounds.size(); ++i) {
            destRangeArray[i] = ctx.rangeBounds[i];
        }

        size_t completeSize = trieSize + ctx.rangeBounds.size()*sizeof(uint16_t);
        assert(completeSize <= sizeof(trieData));

#ifndef NDEBUG
        cout << "completeSize: " << completeSize << endl;
        cout << "next ordinal: " << ctx.nextOrdinal << " range bounds size: " << ctx.rangeBounds.size() << endl;
#endif

//        std::memcpy(trieData, ctx.trieData, completeSize);
        return this;
    }

    void dumpDRNode(std::ostream& out, node_ordinal_t ordinal) {
        auto& node = getNode(ordinal);
        if (node.spanCount > 0) {
            uint64_t span = __builtin_bswap64(node.span);
            std::string_view spanView(reinterpret_cast<const char*>(&span), node.spanCount);
            out << "\tstruct" << static_cast<int>(ordinal) << " [label=\"<f0>span: '" << spanView << "'\"];\n";
            dumpDRNode(out, ordinal + 1);
            // edge
            out << "\tstruct" << static_cast<int>(ordinal) << ":f0 -> struct" << static_cast<int>(ordinal + 1) << ";\n";
        } else {
            out << "\tstruct" << static_cast<int>(ordinal) << " [label=\"";
            uint8_t prev = 0;
            for (size_t i = 0; i < 4; ++i) {
                if (prev == node.edges[i]) {
                    continue;
                } else {
                    prev = node.edges[i];
                }

                if (i > 0) {
                    out << " | " << node.edges[i - 1] << " | ";
                }
                out << "<f" << i << ">.";
            }
            out << "\"];\n";

            prev = 0;
            for (size_t i = 0; i < 4; ++i) {
                if (prev == node.edges[i]) {
                    continue;
                } else {
                    prev = node.edges[i];
                }

                if (node.leaf) {
                    int idx = node.children[i];
                    out << "\trange" << idx << " [shape=plaintext, label=<r<SUB>" << idx << "</SUB>>];\n";
                    // edge
                    out << "\tstruct" << static_cast<int>(ordinal) << ":f" << i << " -> range" << idx << ";\n";

                } else {
                    dumpDRNode(out, node.children[i]);
                    // edge
                    out << "\tstruct" << static_cast<int>(ordinal) << ":f" << i << " -> struct" << static_cast<int>(node.children[i]) << ";\n";
                }
            }
        }
    }

    void dumpDR(std::ostream& out) {
        out << "digraph structs {\n";
        out << "\tnode [shape=record];\n";
        dumpDRNode(out, 0);
        out << "}" << std::endl;
    }

    inline Node** getValue(unsigned slot) {
        return &slots[slot].child;
    }

    inline tree_key_t getKey(unsigned slot) {
        return slots[slot].key;
    }

    bool hasSpace() {
       size_t free = (reinterpret_cast<uint8_t*>(this) + nodeSize) - reinterpret_cast<uint8_t*>(&slots[count]);
       return  free > sizeof(slots[0]);
    }

    template<class T>
    inline size_t binarySearch(T x, size_t begin, size_t end) {
        do {
            size_t mid = ((end - begin) / 2) + begin;
            if (x < slots[mid].key) {
                end = mid;
            } else if (x > slots[mid].key) {
                begin = mid + 1;
            } else {
                return mid;
            }
        } while (begin < end);
        return begin;
    }

    template<class T>
    inline size_t binarySearchBF(T x, size_t begin, size_t end) {
        auto n = end - begin;
        auto lower = begin;
        while (auto half = n/2) {
            auto middle = lower + half;
            auto v = slots[middle].key;
    //        __builtin_prefetch(&slots[middle]);
            lower = (v <= x) ? middle : lower;
            n -= half;
        }
    //    return ((*lower) == x) ? lower : -1;
        auto v = slots[lower].key;
        return (v == x) ? lower : end;
    }

    bool lookup(tree_key_t key, uint64_t& ret) {
        if (!isLeaf) {
            auto pos = binarySearch(key, 0, count);
            if (pos >= count) {
                return upper->lookup(key, ret);
            }
            return slots[pos].child->lookup(key, ret);
        } else {
            auto pos = binarySearch(key, 0, count);
            if (pos < count) {
                ret = slots[pos].payload;
                return true;
            }
            return false;
        }
    }

    bool lookupWithNestedTree(tree_key_t key, uint64_t& ret) {
        if (!isLeaf) {
            auto [begin, end] = drtLookup(key);
            end = std::min<size_t>(end + 1, count);
            auto pos = binarySearch(key, begin, end);
            if (pos >= count) {
                return upper->lookupWithNestedTree(key, ret);
            }
            return slots[pos].child->lookupWithNestedTree(key, ret);
        } else {
            auto [begin, end] = drtLookup(key);
            auto pos = binarySearch(key, begin, end);
            if (pos < count) {
                ret = slots[pos].payload;
                return true;
            }
            return false;
        }
    }

    tree_key_t minKey() {
        if (isLeaf) {
            return slots[0].key;
        } else {
            return (*getValue(0))->minKey();
        }
    }

    tree_key_t maxKey() {
        if (isLeaf) {
            return slots[count - 1].key;
        } else {
            return (*getValue(count - 1))->maxKey();
        }
    }

    bool insert(tree_key_t key, Node* value) {
        if (!hasSpace()) {
            return false;
        }

        auto& slot = slots[count];
        slot.key = key;
        slot.child = value;
        count += 1;
        return true;
    }

    static Node* makeLeaf() { return new (static_cast<Node*>(malloc(nodeSize))) Node(true); }
    static Node* makeInner() { return new (static_cast<Node*>(malloc(nodeSize))) Node(false); }
};

Node* finalizeNode(Node* node, bool dump = false) {
    node = node->constructTrie();
    // TODO validate
//    cout << "done" << endl;
//if (dump) {
//    node->dumpDR(std::cerr);
//    exit(0);
//}
//    exit(0);
    return node;
}

Node* constructInnerNodes(vector<Node*> lowerLevel) {
   if (lowerLevel.size() == 1) {
      return lowerLevel.front();
   }

   vector<Node*> currentLevel;
   Node* node=Node::makeInner();
   for (unsigned i=0; i<lowerLevel.size()-1; i++) {
      Node* curr=lowerLevel[i];
      Node* next=lowerLevel[i+1];

//      tree_key_t sep = next->minKey();
      tree_key_t sep = curr->maxKey();

      if (!node->insert(sep, curr)) {
         node->upper=curr;
//         currentLevel.push_back(node);
         currentLevel.push_back(finalizeNode(node, true));
         node=Node::makeInner();
      }
   }
   node->upper=lowerLevel[lowerLevel.size()-1];
//   currentLevel.push_back(node);
   currentLevel.push_back(finalizeNode(node));
   cout << "countPerNode:" << lowerLevel.size()/currentLevel.size() << endl;

   return constructInnerNodes(currentLevel);
}

std::tuple<Node*, std::vector<Node*>> constructTree(const vector<tree_key_t>& keys) {
    size_t n = keys.size();

    vector<Node*> leaves;
    vector<Node*> keyLeaf;
    Node* node=Node::makeLeaf();
    for (size_t i = 0; i  <n; i++) {
        auto k = keys[i];
        Node* value = (Node*)i;

        if (!node->insert(k, value)) {
            node = finalizeNode(node);

    //{ validation:
            for (size_t j = 0; j < node->count; ++j) {
                size_t keyArrayIdx = i - node->count + j;
                tree_key_t keyInput = keys[keyArrayIdx];
                tree_key_t ret;
                assert(node->lookup(keyInput, ret));
                assert(ret == keyArrayIdx);
            }
    //}

            leaves.push_back(node);
    //         leaves.push_back(finalizeNode(node));
            node = Node::makeLeaf();
            bool inserted = node->insert(k, value);
            assert(inserted);
            leaves.back()->next = node;
        }

        keyLeaf.push_back(node);
    }
    //   leaves.push_back(node);
    leaves.push_back(finalizeNode(node, true));

    cout << "countPerNode:" << n/leaves.size() << endl;

    Node* root = constructInnerNodes(leaves);
    return {root, keyLeaf};
}

int main(int argc,char** argv) {
// TODO

    vector<tree_key_t> keys;

    if (true) {
        size_t count = atof(argv[1]);
        keys.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            keys.push_back(i);
        }
    }

    uint64_t n = keys.size();
    auto [root, keyLeaf] = constructTree(keys);
    auto* keyLeafPtr = &keyLeaf;
    Node* rootPtr = root;
    cout << "root count: " << root->count << endl;
//root->dumpDR(std::cerr);

    size_t repetitions = 1;
    char* repetitionsEnv = getenv("REPETITIONS");
    if (repetitionsEnv) {
        repetitions = std::atoi(repetitionsEnv);
    }

    string dataset("'");
    dataset += getFileName(argv[1]);
    dataset += "'";


    PerfEvents e;

    {
        e.timeAndProfile("leafLookup", n, [&]() {
                for (size_t i = 0; i < n; i++) {
                    tree_key_t ret;
                    (*keyLeafPtr)[i]->lookup(keys[i], ret);
//                    (*keyLeafPtr)[i]->lookupWithNestedTree(keys[i], ret);
                    if (ret != i) throw;
                }
            }, repetitions, {{"approach", "vanilla"}, {"dataset", dataset}});
    }
/*
    {
        e.timeAndProfile("lookup", n, [&]() {
                for (size_t i = 0; i < n; i++) {
                    tree_key_t ret;
                    bool found = rootPtr->lookup(keys[i], ret);
//                    bool found = rootPtr->lookupWithNestedTree(keys[i], ret);
                    assert(found);
                    if (ret != i) throw;
                }
            }, repetitions, {{"approach", "vanilla"}, {"dataset", dataset}});
    }
*/
    {
        e.timeAndProfile("leafLookup", n, [&]() {
                for (size_t i = 0; i < n; i++) {
                    tree_key_t ret;
//                    (*keyLeafPtr)[i]->lookup(keys[i], ret);
                    (*keyLeafPtr)[i]->lookupWithNestedTree(keys[i], ret);
                    if (ret != i) throw;
                }
            }, repetitions, {{"approach", "b2"}, {"dataset", dataset}});
    }
/*
    {
        e.timeAndProfile("lookup", n, [&]() {
                for (size_t i = 0; i < n; i++) {
                    tree_key_t ret;
//                    bool found = rootPtr->lookup(keys[i], ret);
                    bool found = rootPtr->lookupWithNestedTree(keys[i], ret);
                    assert(found);
                    if (ret != i) throw;
                }
            }, repetitions, {{"approach", "b2"}, {"dataset", dataset}});
    }
*/

    return 0;
}
