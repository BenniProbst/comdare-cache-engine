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

static const unsigned maxStrLen=512;

static bool dump = false;

void printChars(const char* str, unsigned len) {
   for (unsigned i=0;i<len;i++)
      cout << *(str++);
}

int cmp(char* k1,unsigned k1len,char* k2,unsigned k2len) {
   if (k1len==k2len)
      return memcmp(k1,k2,k1len);

   unsigned min=k1len<k2len?k1len:k2len;
   int c=memcmp(k1,k2,min);
   if (c!=0)
     return c;
   if (k1len<k2len)
      return -1;
   return +1;
}

struct __attribute__((packed)) Slot {
    enum class KeyType : uint8_t { Short = 0, Long = 1 };
    union key_t {
        uint8_t data[16];
        struct __attribute__((packed)) short_key_t {
            uint8_t longKeyTag : 1; // not set
            uint8_t length : 7;
            uint8_t key[15];
        } shortKey;
        struct __attribute__((packed)) long_key_t {
            uint32_t longKeyTag : 1; // set
            uint32_t length : 31;
            uint32_t offset;
            uint8_t prefix[8];
        } longKey;
    } key;
};

int cmp(char* key, unsigned len, const Slot& slot) {
    throw 0;
}

struct __attribute__((packed)) TrieNode {
    struct {
        uint8_t leaf : 1;
        uint8_t spanCount : 7;
    };
    /*
    uint8_t leaf;
    uint8_t spanCount;*/
    union {/* TODO implement ternary split
        struct {
            uint8_t edges[2];
            uint16_t children[3];
        };*/
        struct {
            uint8_t edges[3];
            uint16_t children[4];
        };
        uint8_t span[1];
    };
};
//static_assert(sizeof(TrieNode) == 10);
static constexpr auto trieFanout = std::size(TrieNode().children);

struct Node {
   static const unsigned nodeSize=32*1024;
   static const unsigned payloadSize=sizeof(Node*);

   bool isLeaf;
   uint16_t count;
   uint16_t offset;
   uint16_t prefixLen;
   union {
      Node* upper; // inner
      Node* next;  // leaf
   };

    uint16_t trieSize;
//    uint16_t rangeArrayOffset;
    uint8_t trieData[4*1024] __attribute__((aligned(16)));

#ifdef SIMPLE_SLOTS
   struct {
      uint16_t len;    // key length without prefix
      uint16_t offset; // location: ((char*)this)+offset
   } slots[1]__attribute__((aligned(16)));
#endif

    uint8_t data[1];

    Node(bool isLeaf) : isLeaf(isLeaf),count(0),offset(nodeSize),prefixLen(0) {}

    using node_ptr_t = uint16_t;
    using range_array_idx_t = uint16_t;

    inline TrieNode& trieRoot() {
        return *reinterpret_cast<TrieNode*>(trieData);
    }

    inline TrieNode& getNode(node_ptr_t offset) {
        return *(reinterpret_cast<TrieNode*>(trieData + offset));
    }

    inline node_ptr_t uncast(const TrieNode* node) {
        return reinterpret_cast<const uint8_t*>(node) - trieData;
    }

    inline TrieNode& getNext(const TrieNode* spanNode) {
        uint8_t* rawAddr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(spanNode));
        rawAddr += offsetof(TrieNode, span) + spanNode->spanCount;
        return *reinterpret_cast<TrieNode*>(rawAddr);
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

    inline uint32_t getKeyLength(size_t slotNum) {
#ifdef SIMPLE_SLOTS
        return slots[slotNum].len;
#else
        auto& slot = slots()[slotNum];
        if (slot.key.longKey.longKeyTag) {
            return slot.key.longKey.length;
        } else {
            return slot.key.shortKey.length;
        }
#endif
    }

    range_array_idx_t minimum(const TrieNode& node) {
        const TrieNode* currentNode = &node;
        while (true) {
            // navigate to the next decision node
            while (currentNode->spanCount > 0) {
                currentNode = &getNext(currentNode);
            }
            if (currentNode->leaf) {
                return currentNode->children[0];
            }
            auto first = currentNode->children[0];
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
                currentNode = &getNext(currentNode);
            }
            if (currentNode->leaf) {
                return currentNode->children[trieFanout - 1];
            }
            auto last = currentNode->children[trieFanout - 1];
            auto childNode = &getNode(last);
            if (!validateNodeOperation(currentNode, childNode)) { return 1; }
            currentNode = childNode;
        }
    }

    range_array_idx_t traverse(const TrieNode& node, uint8_t* key, unsigned len) {
        const TrieNode* currentNode = &node;
        while (true) {
            auto child = 0;

            if (currentNode->spanCount == 0) {
                uint8_t currentByte = len > 0 ? key[0] : uint8_t(0);
                constexpr auto lastIdx = trieFanout - 1;
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

                bool exhausted = (len < currentNode->spanCount);
                auto cmpLen = std::min<size_t>(currentNode->spanCount, len);
                int c = std::memcmp(key, currentNode->span, cmpLen);

                bool smaller = c < 0;
                bool greater = c > 0;
                if (greater) {
                    return maximum(*currentNode);
                } else if (smaller || exhausted) {
                    return minimum(*currentNode);
                } else {
                    key += currentNode->spanCount;
                    len -= currentNode->spanCount;
                    auto childNode = &getNext(currentNode);
                    if (!validateNodeOperation(currentNode, childNode)) { return 1; }
                    currentNode = childNode;
                    continue;
                }
            }
        }
    }

    std::pair<size_t, size_t> drtLookup(uint8_t* key, unsigned len) {
        key += prefixLen;
        len -= prefixLen;

        const auto& root = trieRoot();
        int idx = traverse(root, key, len);
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
        return {lower, upper};
    }

    struct Context {
        Context() = default;
        Context(const Context& other) = delete;
        unsigned nodeCount = 0;
        unsigned unterminatedCount = 0;
        unsigned spaceRequirement = 0; // nodes + rangeBound entries
        node_ptr_t nextPtr = 0;
        std::vector<uint16_t> rangeBounds;
    };

    inline bool drtNodeQuotaExhausted(Context& ctx, unsigned additionalyRequired = 0) {
        const unsigned requiredCnt = ctx.nodeCount + ctx.unterminatedCount + additionalyRequired;
//        return (requiredCnt > std::numeric_limits<node_ordinal_t>::max());
        return requiredCnt > 1024; // TODO use proper limit
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

    // \0 in case the index is out of range
    uint8_t getKeyByteOrNUL(size_t slotNum, size_t byteNum) {
#ifdef SIMPLE_SLOTS
        auto& slot = slots[slotNum];
        if (byteNum >= slot.len) {
            return 0;
        } else {
            return getKey(slotNum)[byteNum];
        }
#else
        auto& slot = slots()[slotNum];
        if (byteNum >= slot.key.shortKey.length) {
            return 0;
        }

        if (slot.key.longKey.longKeyTag) {
            if (byteNum < 8) {
                return slot.key.longKey.prefix[byteNum];
            } else {
                uint8_t* ptr = reinterpret_cast<uint8_t*>(this) + slot.key.longKey.offset;
                return ptr[byteNum - 8];
            }
        } else {
            return slot.key.shortKey.key[byteNum];
        }
#endif
    }

    void fetchKeyExcerpt(size_t slotNum, uint8_t* dest, size_t len, size_t keyDepth) {
        size_t len2 = std::min<size_t>(len, getKeyLength(slotNum) - keyDepth);
        for (size_t i = 0; i < len2; ++i) {
            dest[i] = getKeyByteOrNUL(slotNum, i + keyDepth);
        }
        // pad with zeros
        for (size_t i = len2; i < len; ++i) {
            dest[i] = 0;
        }
    }

    std::tuple<TrieNode&, node_ptr_t> allocateBranchyNode(Context& ctx) {
        assert(ctx.spaceRequirement + sizeof(TrieNode)); // FIXME range bound entries
        node_ptr_t ptr = ctx.nextPtr;
        TrieNode& node = getNode(ptr);
        new (&node) TrieNode();
        node.leaf = 0;
        node.spanCount = 0;
        ctx.nextPtr += sizeof(node);
        return {node, ptr};
    }

    std::tuple<TrieNode&, node_ptr_t> allocateSpanNode(Context& ctx, unsigned spanLength) {
        assert(spanLength < 127);
        // TODO
        node_ptr_t ptr = ctx.nextPtr;
        TrieNode& node = getNode(ptr);
        new (&node) TrieNode();
        node.leaf = 0;
        node.spanCount = 0;
        ctx.nextPtr += offsetof(TrieNode, span) + spanLength;
        return {node, ptr};
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

        if (getKeyByteOrNUL(firstIdx, keyDepth) == getKeyByteOrNUL(lastIdx, keyDepth)) {
            // a span will be constructed in this case
            return;
        }

        const size_t rangeSize = (1 + lastIdx - firstIdx);
//        if (rangeSize < 2) return;
        if (rangeSize < 16) return;
        size_t preliminarySplitPoint = firstIdx + rangeSize/2;

        auto cmpNthByte = [&](size_t a, size_t b) {
            return getKeyByteOrNUL(a, keyDepth) < getKeyByteOrNUL(b, keyDepth);
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

    node_ptr_t constructBranchyNode(Context& ctx, size_t firstIdx, size_t lastIdx, size_t trieDepth, size_t keyDepth, bool forceLeaf = false) {
        assert(firstIdx <= lastIdx);
        std::vector<size_t> splitPoints;
        size_t count = 1 + lastIdx - firstIdx;

        auto [node, ptr] = allocateBranchyNode(ctx);
        node.spanCount = 0;

        size_t targetSplitDepth = std::min<size_t>(2, std::abs(std::log2(count))); // min(2, abs(log2(count)))
        divide(firstIdx, lastIdx, keyDepth, targetSplitDepth, splitPoints);
        splitPoints.push_back(lastIdx);

        bool isLeaf = forceLeaf || (splitPoints.size() == 1);
        isLeaf |= drtNodeQuotaExhausted(ctx);
        if (!isLeaf) {
            int prev = firstIdx - 1;
            size_t maxSubRangeSize = 0;
            size_t minSubRangeSize = std::numeric_limits<size_t>::max();
            for (size_t splitPoint : splitPoints) {
                size_t subRangeSize = 1 + splitPoint - prev;
                maxSubRangeSize = std::max(maxSubRangeSize, subRangeSize);
                minSubRangeSize = std::min(minSubRangeSize, subRangeSize);
                prev = splitPoint;
            }
            isLeaf = (maxSubRangeSize <= 16);
// TODO evaluate            isLeaf |= (minSubRangeSize <= 8);
        }
        if (!isLeaf) {
            bool nodeQuotaExhausted = !tryReserveDrtNodes(ctx, splitPoints.size());
            isLeaf |= nodeQuotaExhausted;
        }
        node.leaf = isLeaf;

        size_t i = 0;
        int prev = firstIdx - 1; // reset
        int cnt = splitPoints.size();
        unsigned lastEdge = 0;
        for (size_t splitPoint : splitPoints) {
            if (i < cnt - 1) {
                uint8_t currentByte = getKeyByteOrNUL(splitPoint, keyDepth);
                node.edges[i] = currentByte;
                lastEdge = i;
            }
            if (isLeaf) {
//                cout << "add range upper bound: " << splitPoint + 1 << " page count: " << this->count << endl;
                ctx.rangeBounds.push_back(splitPoint + 1); // upper bound (excluded)
                size_t rangeIdx = ctx.rangeBounds.size() - 1;
                assert(rangeIdx < 255);
                node.children[i] = static_cast<uint8_t>(rangeIdx);
            } else {
                node_ptr_t childPtr = constructNode(ctx, prev + 1, splitPoint, trieDepth + 1, keyDepth);
                node.children[i] = childPtr;

                drtNodesTerminated(ctx, 1);
            }
            prev = splitPoint;
            i += 1;
        }

        // fill remaining entries
        for (size_t j = lastEdge + 1; j < std::size(node.edges); ++j) {
            node.edges[j] = node.edges[lastEdge];
        }
        size_t lastChild = cnt - 1;
        for (size_t j = i; j < trieFanout; ++j) {
            node.children[j] = node.children[lastChild];
        }

        return ptr;
    }

#pragma GCC push_options
#pragma GCC optimize ("O0")
    size_t __attribute__ ((noinline)) commonPrefixBetween(size_t slotNum1, size_t slotNum2) {
        assert(slotNum1 != slotNum2);
        auto& slot1 = slots[slotNum1];
        auto& slot2 = slots[slotNum2];
//        size_t minLen = std::min(slot1.len, slot2.len);
        size_t minLen = slot1.len < slot2.len ? slot1.len : slot2.len;

//        printf("%d %d\n", slotNum1, slotNum2);
        volatile char* k1 = getKey(slotNum1);
        volatile char* k2 = getKey(slotNum2);
//        printf("%s %s\n", k1, k2);

        for (size_t i = 0; i < minLen; ++i) {
            if (k1[i] != k2[i]) {
                return i;
            }
        }
        return minLen;
    }
#pragma GCC pop_options

/*
    node_ordinal_t constructTerminationLeaf(Context& ctx, size_t firstIdx, size_t lastIdx, size_t trieDepth, size_t keyDepth) {
    }
*/
    node_ptr_t constructNode(Context& ctx, size_t firstIdx, size_t lastIdx, size_t trieDepth, size_t keyDepth) {
        assert(firstIdx <= lastIdx);
        size_t rangeSize = lastIdx - firstIdx;

        bool terminate = drtNodeQuotaExhausted(ctx, 2); // span+leaf
        terminate |= rangeSize < 2;
        if (terminate) {
//            return constructTerminationLeaf();
            return constructBranchyNode(ctx, firstIdx, lastIdx, trieDepth, keyDepth, true);
        }

        size_t spanLength = commonPrefixBetween(firstIdx, lastIdx) - keyDepth; // TODO can be limited to the max span lenght
//cout << "full span len: " << spanLength << endl;
        if (spanLength == 0) {
            return constructBranchyNode(ctx, firstIdx, lastIdx, trieDepth, keyDepth);
        }

        auto [node, nodePtr] = allocateSpanNode(ctx, spanLength);
        fetchKeyExcerpt(firstIdx, node.span, spanLength, keyDepth);
//cout << "span: "; printChars(reinterpret_cast<char*>(&node.span), spanLength); cout << endl;

// FIXME            bool nodeQuotaExhausted = !tryReserveDrtNodes(ctx, 1);
//            assert(!nodeQuotaExhausted);
        node.leaf = false;
        node.spanCount = spanLength;

        keyDepth += spanLength;

        constructNode(ctx, firstIdx, lastIdx, trieDepth + 1, keyDepth);
// FIXME        drtNodesTerminated(ctx, 1);

        return nodePtr;
    }

    Node* constructTrie() {
        Context ctx;
        ctx.rangeBounds.push_back(0);
        constructNode(ctx, 0, count - 1, 0, 0);

        this->trieSize = ctx.nextPtr;

        uint16_t* destRangeArray = reinterpret_cast<uint16_t*>(trieData + trieSize);
        for (size_t i = 0; i < ctx.rangeBounds.size(); ++i) {
            destRangeArray[i] = ctx.rangeBounds[i];
        }

        size_t completeSize = trieSize + ctx.rangeBounds.size()*sizeof(uint16_t);
        assert(completeSize <= sizeof(trieData));

#ifndef NDEBUG
        cout << "completeSize: " << completeSize << endl;
#endif

//        std::memcpy(trieData, ctx.trieData, completeSize);
        return this;
    }

/*
digraph structs {
    node [shape=record];
    struct1 [label="<f0> left|<f1> mid&#92; dle|<f2> right"];
    struct2 [label="<f0> one|<f1> two"];
    struct3 [label="hello&#92;nworld |{ b |{c|<here> d|e}| f}| g | h"];
    struct1:f1 -> struct2:f0;
    struct1:f2 -> struct3:here;
}
*/
    void dumpDRNode(std::ostream& out, node_ptr_t nodePtr) {
        auto& node = getNode(nodePtr);
        if (node.spanCount > 0) {
            std::string_view spanView(reinterpret_cast<const char*>(node.span), node.spanCount);
            out << "\tstruct" << static_cast<int>(nodePtr) << " [label=\"<f0>span: '" << spanView << "'\"];\n";

            auto& nextNode = getNext(&node);
            node_ptr_t nextPtr = uncast(&nextNode);
            dumpDRNode(out, nextPtr);
            // edge
            out << "\tstruct" << static_cast<int>(nodePtr) << ":f0 -> struct" << static_cast<int>(nextPtr) << ";\n";
        } else {
            out << "\tstruct" << static_cast<int>(nodePtr) << " [label=\"";

            // determine the real fanout
            uint8_t prev = 0;
            unsigned fanout = trieFanout;
            for (size_t i = 0; i < std::size(node.edges); ++i) {
                if (prev == node.edges[i]) {
                    fanout = i + 1;
                    break;
                }
                prev = node.edges[i];
            }

            for (size_t i = 0; i < fanout; ++i) {
                if (i > 0) {
                    out << " | " << node.edges[i - 1] << " | ";
                }
                out << "<f" << i << ">.";
            }
            out << "\"];\n";

            prev = 0;
            for (size_t i = 0; i < fanout; ++i) {
                if (node.leaf) {
                    int idx = node.children[i];
                    out << "\trange" << idx << " [shape=plaintext, label=<r<SUB>" << idx << "</SUB>>];\n";
                    // edge
                    out << "\tstruct" << static_cast<int>(nodePtr) << ":f" << i << " -> range" << idx << ";\n";

                } else {
                    dumpDRNode(out, node.children[i]);
                    // edge
                    out << "\tstruct" << static_cast<int>(nodePtr) << ":f" << i << " -> struct" << static_cast<int>(node.children[i]) << ";\n";
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

   inline char* getPrefix() { return reinterpret_cast<char*>(this)+nodeSize-prefixLen; }
   inline Node** getValue(unsigned slot) { return reinterpret_cast<Node**>(getKey(slot)+slots[slot].len); }
   inline char* getKey(unsigned slot) { return reinterpret_cast<char*>(this)+slots[slot].offset; }

   unsigned combineKey(unsigned index,char* key) {
      memcpy(key,getPrefix(),prefixLen);
      memcpy(key+prefixLen,getKey(index),slots[index].len);
      return slots[index].len+prefixLen;
   }

   void combineKey(unsigned index, std::stringstream& key) {
       key << getPrefix();
       key << getKey(index);
   }

   unsigned commonPrefix(char* b,unsigned len) {
      char* a=getPrefix();
      unsigned minLen=len<prefixLen?len:prefixLen;
      for (unsigned i=0;i<minLen;i++)
         if (a[i]!=b[i])
            return i;
      return minLen;
   }

   bool hasSpace(unsigned size) { return ((reinterpret_cast<char*>(this)+offset-size-payloadSize)>reinterpret_cast<char*>(&slots[count+1])); }

   inline int checkPrefix(char* key,unsigned len) {
      char* prefix=getPrefix();
      for (unsigned i=0;i<prefixLen;i++) {
         if (i==len)
            // Key too short
            return -2;
         if (key[i]<prefix[i])
            // Key less
            return -1;
         if (key[i]>prefix[i])
            // Key larger
            return 1;
      }
      return 0;
   }

   inline Node* binarySearch(char* key,unsigned len, unsigned begin, unsigned end) {
      while (true) {
         if (end-begin<4) {
            for (unsigned pos=begin;pos<end;pos++)
               if (cmp(key,len,getKey(pos),slots[pos].len)<=0)
                  return *getValue(pos);
            return upper;
         } else {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(key,len,getKey(mid),slots[mid].len);
            if (c>0)
               begin=mid+1; else
               end=mid+1;
         }
      }
   }
#if 0
   bool lookup(char* key,unsigned len,uint64_t& ret) {
      int prefixStatus=checkPrefix(key,len);
      if (!isLeaf) {
         switch (prefixStatus) {
            case -2: return false;//nullptr;
            case -1: return (*getValue(0))->lookup(key,len,ret);
            case +1: return upper->lookup(key,len,ret);
         }

         return binarySearch(key+prefixLen,len-prefixLen, 0, count)->lookup(key,len,ret);
      } else {
         if (prefixStatus!=0)
            return false;

         char* k=key+prefixLen;
         unsigned l=len-prefixLen;
         unsigned begin=0,end=count;
         while (begin<end) {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(k,l,getKey(mid),slots[mid].len);
            if (c==0) {
               ret=reinterpret_cast<uint64_t>(*getValue(mid));
               return true;
            }
            if (c<0)
               end=mid; else
               begin=mid+1;
         }
         return false;
      }
   }
#endif
   bool lookup(char* key,unsigned len,uint64_t& ret) {
      char* shortenedKey = key+prefixLen;
      unsigned shortenedLen = len-prefixLen;

      int prefixStatus=checkPrefix(key,len);
      if (!isLeaf) {
         switch (prefixStatus) {
            case -2: return false;//nullptr;
            case -1: return (*getValue(0))->lookup(key,len,ret);
            case +1: return upper->lookup(key,len,ret);
         }

//            size_t begin = 0, end = count;
         auto [begin, end] = drtLookup(reinterpret_cast<uint8_t*>(key), len);
         end = std::min<size_t>(end + 1, count);
                 return binarySearch(shortenedKey, shortenedLen, begin, end)->lookup(key,len,ret);
//                 return binarySearch(shortenedKey, shortenedLen, 0, count)->lookup(key,len,ret);
      } else {
         if (prefixStatus!=0)
            return false;

         auto [begin, end] = drtLookup(reinterpret_cast<uint8_t*>(key), len);
         while (begin<end) {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(shortenedKey, shortenedLen, getKey(mid),slots[mid].len);
            if (c==0) {
               ret=reinterpret_cast<uint64_t>(*getValue(mid));
               return true;
            }
            if (c<0)
               end=mid; else
               begin=mid+1;
         }

         return false;
      }
   }

   Node* lookupLeaf(char* key, unsigned len) {
        char* shortenedKey = key+prefixLen;
        unsigned shortenedLen = len-prefixLen;

        int prefixStatus=checkPrefix(key,len);
        if (!isLeaf) {
            switch (prefixStatus) {
                case -2: return nullptr;
                case -1: return (*getValue(0))->lookupLeaf(key, len);
                case +1: return upper->lookupLeaf(key, len);
            }

            auto [begin, end] = drtLookup(reinterpret_cast<uint8_t*>(key), len);
            end = std::min<size_t>(end + 1, count);
            return binarySearch(shortenedKey, shortenedLen, begin, end)->lookupLeaf(key, len);
        } else {
            return this;
        }
   }

    size_t lookupLeafPosition(char* key, unsigned len) {
        char* shortenedKey = key+prefixLen;
        unsigned shortenedLen = len-prefixLen;

        int prefixStatus=checkPrefix(key,len);

        if (prefixStatus < 0) {
            return 0;
        } else if (prefixStatus > 0) {
            return count;
        }

        auto [begin, end] = drtLookup(reinterpret_cast<uint8_t*>(key), len);
        while (begin<end) {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(shortenedKey, shortenedLen, getKey(mid),slots[mid].len);
            if (c==0) {
                return mid;
            }
            if (c<0) {
                end=mid;
            } else {
                begin=mid+1;
            }
        }

        return end;
    }

   void add(char* key,unsigned size,Node* value) {
      // add a single entry
      assert(hasSpace(size));
      offset-=size+payloadSize;
      slots[count].len=size;
      slots[count].offset=offset;
      memcpy(getKey(count),key,size);
      *getValue(count)=value;
      count++;
   }

   unsigned minKey(char* k) {
      if (isLeaf)
         return combineKey(0,k);

      return (*getValue(0))->minKey(k);
   }

   unsigned maxKey(char* k) {
      if (isLeaf)
         return combineKey(count-1,k);

      return upper->maxKey(k);
   }

   void minKey(std::stringstream& k) {
       if (isLeaf) {
           combineKey(0, k);
       } else {
            (*getValue(0))->minKey(k);
       }
   }

   void maxKey(std::stringstream& k) {
       if (isLeaf) {
           combineKey(count - 1, k);
       } else {
           upper->maxKey(k);
       }
   }

   bool insert(char* key,unsigned size,Node* value) {
      if (count==0) {
         // Key becomes prefix
         prefixLen=size;
         offset-=size;
         memcpy(getPrefix(),key,size);
         // Store empty (remaining) key and value
         add(key,0,value);
         return true;
      }

      unsigned newPrefixLength=commonPrefix(key,size);
      if (newPrefixLength==prefixLen) {
         if (!hasSpace(size))
            return false;
         // Same prefix
         add(key+prefixLen,size-prefixLen,value);
         return true;
      }

      assert(prefixLen>newPrefixLength);
      // Is there enough space with a shorter prefix?
      unsigned additionalSpace=size+payloadSize+count*(prefixLen-newPrefixLength);
      if ((reinterpret_cast<char*>(this)+offset-additionalSpace)<reinterpret_cast<char*>(&slots[count+1]))
         return false;

      // Enough space, grow all entries
      char scratchSpace[nodeSize];
      auto newNode=new (reinterpret_cast<Node*>(scratchSpace)) Node(isLeaf);
      newNode->prefixLen=newPrefixLength;
      newNode->count=count;

      // Set new prefix
      newNode->offset-=newPrefixLength;
      memcpy(newNode->getPrefix(),getPrefix(),newPrefixLength);

      // Copy slots and keys
      unsigned prefixDiff=prefixLen-newPrefixLength;
      for (unsigned i=0;i<count;i++) {
         newNode->offset-=slots[i].len+prefixDiff+payloadSize;
         newNode->slots[i].len=slots[i].len+prefixDiff;
         newNode->slots[i].offset=newNode->offset;
         memcpy(scratchSpace+newNode->offset,getPrefix()+newPrefixLength,prefixDiff);
         memcpy(scratchSpace+newNode->offset+prefixDiff,getKey(i),slots[i].len+payloadSize);
         assert(*getValue(i) == *newNode->getValue(i));
      }

      if (newNode->insert(key, size, value)) {
         memcpy(this,newNode,nodeSize);
         return true;
      }
      return false;
   }

   static Node* makeLeaf() { return new (static_cast<Node*>(malloc(nodeSize))) Node(true); }
   static Node* makeInner() { return new (static_cast<Node*>(malloc(nodeSize))) Node(false); }

   struct RangeIterator {
      Node* currentLeaf;
      size_t currentIndex;
      uint64_t* upperLimitPtr;
      uint64_t value;
      uint64_t* valuePtr;
      bool done = false;
      bool next();
   };

   RangeIterator __attribute__ ((noinline)) createRangeIterator(char* lower, unsigned l1, char* upper, unsigned l2) {
        Node* leftPage = lookupLeaf(lower, l1);
        assert(leftPage);
        Node* rightPage = lookupLeaf(upper, l2);
        assert(rightPage);

        RangeIterator it;
        it.currentLeaf = leftPage;
        it.currentIndex = leftPage->lookupLeafPosition(lower, l1); // may exceed the page boundary

        size_t last = rightPage->lookupLeafPosition(upper, l2);
        last = std::min<size_t>(rightPage->count - 1, last);
        it.upperLimitPtr = reinterpret_cast<uint64_t*>(rightPage->getValue(last));

        return it;
   }
};
/*
void advancePage(Node::RangeIterator& it) {
    it.currentLeaf = it.currentLeaf->next;
}
*/
bool Node::RangeIterator::next() {
    if (done) { return false; }

    if (currentIndex >= currentLeaf->count) {
//        advancePage(*this);
        currentLeaf = currentLeaf->next;
        currentIndex = 0;
        done = (currentLeaf == nullptr);
        return next();
    }

    valuePtr = reinterpret_cast<uint64_t*>(currentLeaf->getValue(currentIndex));
    value = *valuePtr;
    done = (valuePtr == upperLimitPtr);
    currentIndex += 1;

    return true;
}

struct cmp_str { bool operator()(char const *a, char const *b) { return std::strcmp(a, b)<0; } };

unsigned truncateSeparator(char* lower,unsigned lowerLen,char* upper,unsigned upperLen,char* k) {
   unsigned minLen=lowerLen<upperLen?lowerLen:upperLen;
   unsigned commonPrefix;
   for (commonPrefix=0; commonPrefix<minLen; commonPrefix++)
      if (lower[commonPrefix]!=upper[commonPrefix])
         break;
   if (commonPrefix==lowerLen) {
      memcpy(k,lower,lowerLen);
      return lowerLen;
   }
   if (lower[commonPrefix]<upper[commonPrefix]&&(lower[commonPrefix]+1<upper[commonPrefix])) {
      memcpy(k,lower,commonPrefix+1);
      k[commonPrefix]++;
      return commonPrefix+1;
   }
   memcpy(k,lower,lowerLen);
   return lowerLen;
}

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
      char lower[maxStrLen]; unsigned lowerLen=curr->maxKey(lower);
      char upper[maxStrLen]; unsigned upperLen=next->minKey(upper);
      char k[maxStrLen]; unsigned len=truncateSeparator(lower,lowerLen,upper,upperLen,k);
      assert(cmp(lower,lowerLen,k,len)<=0);

      if (!node->insert(k,len,curr)) {
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

std::tuple<Node*, std::vector<Node*>> constructTree(const vector<char*>& keys) {
   uint64_t n=keys.size();

   vector<Node*> leaves;
   vector<Node*> keyLeaf;
   Node* node=Node::makeLeaf();
   for (uint64_t i=0;i<n;i++) {
      char* k=keys[i];
      unsigned len=strlen(keys[i]);
      Node* value=(Node*)i;
//cout << "insert: "; printChars(k, len); cout << endl;
      if (!node->insert(k,len,value)) {


//trap(i == 19811);
          node = finalizeNode(node);

//{ validation:
         for (size_t j = 0; j < node->count; ++j) {
            size_t keyArrayIdx = i - node->count + j;
            char* keyInput = keys[keyArrayIdx];
            unsigned len = strlen(keyInput);
            char keyStored[maxStrLen];
            node->combineKey(j, keyStored);
            bool match = cmp(keyInput, len, keyStored, len) == 0;
            if (!match) {
               cout << "expected: "; printChars(keyInput, len);
               cout << " got: "; printChars(keyStored, len); cout << endl;
               assert(match);
            }

            uint64_t ret;
//            trap(!node->lookup(keyInput, len, ret));
//node->dumpDR(std::cerr);
//exit(0);
if (!node->lookup(keyInput, len, ret)) {
    node->dumpDR(std::cerr);
//    exit(0);
trap(true);
}

            assert(node->lookup(keyInput, len, ret));
            assert(ret == keyArrayIdx);
         }
/*
         char maxKey[maxStrLen]; unsigned maxKeyLen=node->maxKey(maxKey);
         char minKey[maxStrLen]; unsigned minKeyLen=node->minKey(minKey);
         cout << "minKey: "; printChars(minKey, minKeyLen); cout << endl;
         cout << "maxKey: "; printChars(maxKey, maxKeyLen); cout << endl;
*/
//}

//exit(0);

         leaves.push_back(node);
//         leaves.push_back(finalizeNode(node));
         node=Node::makeLeaf();
         bool inserted = node->insert(k,len,value);
         assert(inserted);
         leaves.back()->next = node;
      }

      keyLeaf.push_back(node);
   }
//   leaves.push_back(node);
   leaves.push_back(finalizeNode(node, true));

   cout << "countPerNode:" << n/leaves.size() << endl;
//exit(0);
   Node* root = constructInnerNodes(leaves);
   return {root, keyLeaf};
}

int main(int argc,char** argv) {
   vector<char*> keys;
   char* buffvar=nullptr;
   size_t len=0;
   FILE* file=fopen(argv[1],"r");
   while (getline(&buffvar, &len, file)!=-1) {
      unsigned len = strlen(buffvar);
      buffvar[len - 1] = 'x';
      keys.push_back(buffvar);
      buffvar=nullptr;
   }

   sort(keys.begin(),keys.end(),[] (char *a, char *b) { return cmp(a,strlen(a),b,strlen(b))<0; });

   uint64_t n=keys.size();

   auto [root, keyLeaf] = constructTree(keys);
   Node* rootPtr = root;
   cout << "root count: " << root->count << endl;
//root->dumpDR(std::cerr);

#ifndef NDEBUG
    {
        uint64_t ret;
        for (unsigned i=0; i<n; i++) {
            if (!root->lookup(keys[i],strlen(keys[i]),ret)) {
                trap(true);
                dump = true;
            }
            assert(root->lookup(keys[i],strlen(keys[i]),ret));
            assert(ret==i);
        }
    }
#endif

   struct RND {
      char* key;
      Node* leaf;
      unsigned pos;
   };

   RND* rnd=new RND[n];
   for (uint64_t i=0;i<n;i++) {
      rnd[i].key=keys[i];
      rnd[i].leaf=keyLeaf[i];
      rnd[i].pos=i;
   }
   std::random_shuffle(rnd,rnd+n);

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
                for (uint64_t i=0;i<n;i++) {
                    uint64_t ret;
                    rnd[i].leaf->lookup(rnd[i].key,strlen(rnd[i].key),ret);
                    assert(ret==rnd[i].pos);
                }
            }, repetitions, {{"approach", "drt"}, {"dataset", dataset}});
    }

    {
        e.timeAndProfile("lookup", n, [&]() {
                for (uint64_t i=0;i<n;i++) {
                    uint64_t ret;
                    bool found = rootPtr->lookup(rnd[i].key,strlen(rnd[i].key),ret);
                    assert(found);
                    assert(ret==rnd[i].pos);
                }
            }, repetitions, {{"approach", "drt"}, {"dataset", dataset}});
    }

    {
        // range benchmark:
        auto rangeFn = [&]() {
            const unsigned intervalCount = 8;
            const unsigned intervalSize = keys.size()/intervalCount;
            size_t keyIdx = 0;
            for (size_t i = 0; i < intervalCount; ++i) {
                auto lowerIdx = i*intervalSize;
                auto upperIdx = (i == intervalCount - 1) ? keys.size() - 1 : (i + 1)*intervalSize - 1;
                char* lower = keys[lowerIdx];
                char* upper = keys[upperIdx];
                unsigned len1 = strlen(lower);
                unsigned len2 = strlen(upper);
                auto it = rootPtr->createRangeIterator(lower, len1, upper, len2);

                while (it.next()) {
                    char* key = keys[keyIdx];
    //                std::cout << "range lookup " << keyIdx << " for: " << key << endl;
                    assert(it.value == keyIdx);
                    keyIdx += 1;
                }
            }
        };
        e.timeAndProfile("rangeLookup", n, rangeFn, repetitions, {{"approach", "drt"}, {"dataset", dataset}});
    }

    return 0;
}
