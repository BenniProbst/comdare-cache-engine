#include <algorithm>
#include <atomic>
#include <cassert>
#include <vector>

#include <iostream>

#include <optional>
#include <stack>

#include "epoch.hpp"
#include "hazard.hpp"
#include "optimistic_lock.hpp"

//#include "Page.hpp"

#include "BufferManager.hpp"

#include "tree.hpp"
#include "search.hpp"

#include <csignal>

using namespace HazardPointers;
using namespace OptimisticLock;

class DummyPage {
    public:
    const size_t pagePayloadSpace = 0;
    std::atomic<uint64_t> version{0};
};

namespace BTree {

static constexpr size_t nodeSize = 64*1024;

struct __attribute__((packed)) VariableSizedKeyEntry {
    enum class KeyType : uint8_t { Short = 0, Long = 1 };
    union key_t {
        uint8_t data[16];
        struct __attribute__((packed)) short_key_t {
            uint8_t tag : 1;
            uint8_t length : 7;
            uint8_t key[15];
        } shortKey;
        struct __attribute__((packed)) long_key_t {
            uint32_t tag : 1;
            uint32_t length : 31;
            uint32_t offset;
            uint8_t prefix[8];
        } longKey;
    } key;
};

enum class BTreeLatchMode : uint8_t { FullAccessPath, LocallyRestricted };

struct NodeBase;

using page_ref_t = NodeBase*;

template<typename Key>
struct TreeOperation {
//    BufferManager& bm; // TODO
    Key key;
//    std::optional<double> relativePosition;
    double relativePosition = 1.0;

    std::vector<std::unique_ptr<uint8_t[]>> keyStore;
};

template<typename Key>
struct ModifyOperation : public TreeOperation<Key> {
    BTreeLatchMode latchMode;
    std::stack<page_ref_t> path;
    std::stack<page_ref_t> remainingPath;
};

template<typename Key>
struct RemoveOperation : public ModifyOperation<Key> {};

template<typename Key, typename Payload>
struct InsertOperation : public ModifyOperation<Key> {
    Payload payload;
};

struct __attribute__((packed)) SamplingPoint {
    uint64_t hash;
    uint32_t position;
};
static const size_t nestedSize = 32;
using sampling_points_t = SamplingPoint[nestedSize];

enum class PageType : uint8_t { BTreeImaginaryRoot = 1, BTreeInnerNode = 2, BTreeLeafNode = 3 };

struct NodeBase : public DummyPage {
    PageType type;
    uint16_t count;
    std::atomic_bool obsolete{false};
//    std::atomic<uint64_t> version{0};
    uint32_t freeSpaceOffset;
};

/*
struct TreeMainPage : public NodeBase {
    static const PageType typeMarker = PageType::BTreeImaginaryRoot;

    NodeBase* root;

    TreeMainPage() {
        type = typeMarker;
    }
};
*/

template<typename Key>
struct MergeResult {
    Key obsoleteSuperjacentKey;
    NodeBase* mergedNode;
};


static std::atomic_uint64_t searchSteps = {0};

template<class NodeType, class Key>
double nextRelativePositionHint(const TreeOperation<Key>& op, size_t pos, const NodeType& node) {
// FIXME implement non-integral key handling
    if (pos >= node.count) {
        return 1.0;
    }
    double newHint = 1.0;
    if (pos > 0) {
        double current = node.keys[pos];
        double prev = node.keys[pos - 1];
        newHint = (op.key - prev)/(current - prev);
    }
//    op.relativePosition = newHint;
//printf("newHint: %f\n", newHint);
    return newHint;
}

template<class TreeType, class NodeType>
std::tuple<size_t, size_t> determineBounds(const TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
    size_t lower = 0, upper = node.count;
    size_t scale = node.maxEntries/nestedSize;

    for (size_t i = 0; i < nestedSize; ++i) {
        if (node.nestedNode[i] >= op.key) {
            lower = i*scale;
            upper = (i + 1)*scale;
            break;
        }
    }

    upper = std::min<size_t>(upper, node.count);
    return {lower, upper};
/*
    assert(node.count > 1);
    double lowerKey, upperKey;
    lowerKey = node.keys[lower];
    upperKey = node.keys[upper - 1];
    double hint = (op.key - lowerKey)/(upperKey - lowerKey);

    hint = std::min(1., hint);
    hint = std::max(0., hint);

    //printf("hint: %f\n", hint);
    return {lower, upper, hint};
*/
}

//template<class NodeType, class TreeType>
template<class TreeType, class NodeType>
size_t determineUpperBound(const TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
    // TODO
    if constexpr (TreeType::hasVariableSizedKeys) {
        throw std::runtime_error("not implemented");
    }
    if constexpr (sizeof(typename TreeType::KeyType) != 4) {
        throw std::runtime_error("not implemented");
    }

#ifdef TREE_ENABLE_AVX512
    uint32_t result[16] = {16};
    __m512i indices = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    __m512i sampling_points = _mm512_load_epi32(node.nestedNode);
    __m512i key = _mm512_set1_epi32(op.key);
    __mmask16 ge_mask = _mm512_cmpge_epi32_mask(sampling_points, key);
    _mm512_mask_compressstoreu_epi32(result, ge_mask, indices);
    return result[0];
#else
    size_t upper = nestedSize;
    for (size_t i = 0; i < nestedSize; ++i) {
        if (node.nestedNode[i] >= op.key) {
            return i;
        }
    }
    return upper;
/*
    size_t upper = nestedSize;
    for (size_t i = 0; i < nestedSize; ++i) {
        bool test = node.nestedNode[i] >= op.key;
        upper = test*std::min(upper, i) + !test*upper;
    }
    return upper;
*/
/*
    size_t upper = nestedSize;
    for (int i = nestedSize - 1; i >= 0; --i) {
        bool test = node.nestedNode[i] >= op.key;
        upper = test*i + !test*upper;
    }
    return upper;
*/
#endif
}

#if 0
//template<class NodeType, class TreeType>
template<class TreeType, class NodeType>
std::tuple<size_t, size_t, double> interpolatePosition(const TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
    auto upper = determineUpperBound<TreeType, NodeType>(op, node);
    if (upper == nestedSize) {
        return {0, node.count, 1.0};
    }

    size_t lower = 0;
    if (upper > 0) {
        lower = upper - 1;
    }

    static_assert(NodeType::maxEntries%nestedSize == 0, "incompatible sampling frequency");
    size_t scale = node.maxEntries/nestedSize;

    double current = node.nestedNode[upper];
    double prev = node.nestedNode[lower];
    if (lower < 1) {
        prev = node.keys[0];
    }
    double hint = (op.key - prev)/(current - prev);

    if (lower > 0) {
        lower = lower*scale - 1;
    }
    upper += 1;
    upper = upper*scale - 1;
    upper = std::min<size_t>(upper, node.count - 1);

    return {lower, upper, hint};
}
#endif

#if 0
template<class TreeType, class NodeType>
std::tuple<size_t, size_t, double> interpolatePosition(const TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
    auto upper = determineUpperBound<TreeType, NodeType>(op, node);
    if (upper == nestedSize) {
        return {0, node.count, 1.0};
    }

    static_assert(NodeType::maxEntries%nestedSize == 0, "incompatible sampling frequency");
    size_t scale = node.maxEntries/nestedSize;

    double current = node.nestedNode[upper];
    double prev = 0.;

    bool first = upper == 0;
    bool not_first = !first;
    prev += (not_first)*node.nestedNode[(not_first)*(upper - 1)];
    prev += first*node.keys[0];
    double hint = (op.key - prev)/(current - prev);

hint = std::max(hint, 0.);

    size_t lower = 0;
    lower += not_first*((upper - 1)*scale - 1);
/*
    upper += 1;
    upper = upper*scale - 1;
    upper = std::min<size_t>(upper, node.count - 1);
*/
    upper += 1;
    upper = upper*scale;
    upper = std::min<size_t>(upper, node.count);

    return {lower, upper, hint};
}
#endif


enum class SearchApproach : uint8_t { VanillaBinarySearch = 1, ParentInterpolationHintBinarySearch = 2, NestedNodeBinarySearch = 3 };

template<SearchApproach approach, class TreeType, class NodeType>
struct LocatePositionAndPrepareNextIteration {};

template<class TreeType, class NodeType>
struct LocatePositionAndPrepareNextIteration<SearchApproach::VanillaBinarySearch, TreeType, NodeType> {
    static size_t doImpl(TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
//        return binarySearch(op.key, node);
        return binarySearch<typename TreeType::KeyType>(op.key, node.keys, 0, node.count);
    }
};

template<class TreeType, class NodeType>
struct LocatePositionAndPrepareNextIteration<SearchApproach::ParentInterpolationHintBinarySearch, TreeType, NodeType> {
    static size_t doImpl(TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
//        auto pos = guidedBinarySearch(op, node);
        auto pos = guidedExponentialSearch<typename TreeType::KeyType>(op.key, node.keys, node.count, op.relativePosition);
        if constexpr (std::is_same<typename TreeType::InnerType, NodeType>::value) {
            op.relativePosition = nextRelativePositionHint(op, pos, node);
        }
        return pos;
    }
};

template<class TreeType, class NodeType>
struct LocatePositionAndPrepareNextIteration<SearchApproach::NestedNodeBinarySearch, TreeType, NodeType> {
    static size_t doImpl(TreeOperation<typename TreeType::KeyType>& op, const NodeType& node) {
        if (node.count < 2) { return 0; }

        auto [lower, upper] = determineBounds<TreeType, NodeType>(op, node);
        auto n = upper - lower;
        auto pos = lower + binarySearch(op.key, &node.keys[lower], n);
        if (pos == upper) {
            return node.count;
        } else {
            return pos;
        }
    }
};

template<class Tree>
struct ExtendedHeader : public NodeBase {
    using Key = typename Tree::KeyType;
    Key nestedNode[nestedSize] __attribute__ ((aligned (16)));
};

template<class Tree>
struct LeafNode : public ExtendedHeader<Tree> {
    using MyType = LeafNode<Tree>;
    using Key = typename Tree::KeyType;
    using Payload = typename Tree::PayloadType;

    static const PageType typeMarker = PageType::BTreeLeafNode;

    static constexpr size_t calcMaxEntryCount() {
        size_t remaining = nodeSize - sizeof(ExtendedHeader<Tree>);
        return remaining / (sizeof(Key) + sizeof(Payload));
    }

    static constexpr size_t maxEntries = calcMaxEntryCount();// 1<<10;
    static constexpr size_t minEntries = maxEntries/2;
    Key keys[maxEntries];
    Payload payloads[maxEntries];

    static constexpr size_t pagePayloadSpacePlaceholder = maxEntries*(sizeof(Key) + sizeof(Payload));

    LeafNode() {
        this->count = 0;
        this->type = typeMarker;
        updateLimits();
    }

    void updateLimits() {
#ifdef VARIABLE_SIZED_KEYS
        auto availableSpace = pagePayloadSpace - sizeof(decltype(*this));
        maxEntries = availableSpace/(sizeof(Key) + sizeof(Payload));
        minEntries = maxEntries/2;
#endif
    }

    auto resize(size_t sizeHint) {
#ifdef VARIABLE_SIZED_KEYS
        // TODO
        updateLimits();
        return nullptr;
#else
        return this;
#endif
    }

    void insert(InsertOperation<Key, Payload>& op) {
        if (this->count > 0) {
            auto pos = binarySearch(op.key, keys, this->count);
            if (pos < this->count && keys[pos] == op.key) {
                // overwrite payload
                payloads[pos] = op.payload;
                return;
            }
            memmove(keys + pos + 1, keys + pos, sizeof(Key) * (this->count - pos));
            memmove(payloads + pos + 1, payloads + pos, sizeof(Payload) * (this->count - pos));
            keys[pos] = op.key;
            payloads[pos] = op.payload;
        } else {
            keys[0] = op.key;
            payloads[0] = op.payload;
        }
        this->count++;


// TODO pick sampling points
//throw 0;
        assert(this->count > 0);
#if 0
        size_t scale = maxEntries/nestedSize;
        for (size_t i = 0; i < nestedSize; ++i) {
            size_t idx = (i + 1)*scale - 1;
            if (idx < count) {
                nestedNode[i] = keys[idx];
            } else {
//                assert(i > 0);
//                nestedNode[i] = nestedNode[i - 1];
                nestedNode[i] = keys[count - 1];
            }
        }
#endif
        size_t scale = maxEntries/nestedSize;
        for (size_t i = 0; i < nestedSize; ++i) {
            size_t idx = (i + 1)*scale - 1;
            idx = std::min<size_t>(idx, this->count - 1);
            this->nestedNode[i] = keys[idx];
        }
    }

    bool remove(RemoveOperation<Key>& op) {
        if (this->count > 0) {
            auto pos = binarySearch(op.key, keys, this->count);
            if (pos < this->count && keys[pos] == op.key) {
                memmove(keys + pos, keys + pos + 1, sizeof(Key) * (this->count - pos - 1));
                memmove(payloads + pos, payloads + pos + 1, sizeof(Payload) * (this->count - pos - 1));
                this->count -= 1;
            } else {
                return false;
            }
        }
        if (this->count <= minEntries) {
            assert(op.latchMode == BTreeLatchMode::FullAccessPath);

            // TODO implement rebalancing?
            // try to merge a sibling into this page
            auto mergeResult = tryMerge(op);
            op.remainingPath.pop();
            if (mergeResult) {
//                auto parent = getParent(op);
                auto parent = static_cast<typename Tree::InnerType*>(op.remainingPath.top());
                assert(parent->type == PageType::BTreeInnerNode);
//                parent->remove(mergeResult->obsoleteSuperjacentKey, op);
                parent->finalizeMerge(op, mergeResult.value());
            }
        }
        return true;
    }

    std::tuple<std::unique_ptr<LeafNode>, Key> split() {
        auto newLeaf = std::make_unique<LeafNode>();
        newLeaf->count = this->count - (this->count / 2);
        this->count = this->count - newLeaf->count;
        memcpy(newLeaf->keys, keys + this->count, sizeof(Key) * newLeaf->count);
        memcpy(newLeaf->payloads, payloads + this->count, sizeof(Payload) * newLeaf->count);
        auto sep = keys[this->count - 1];
        return std::make_tuple(std::move(newLeaf), sep);
    }

    size_t nodeEntriesSpaceConsumption() const {
        // TODO
#ifdef VARIABLE_SIZED_KEYS
        throw 0;
#else
        return this->count*(sizeof(Key) + sizeof(Payload));
#endif
    }

    bool fits(size_t size) const {
        // TODO replace placeholder
        size_t availableSpace = pagePayloadSpacePlaceholder - nodeEntriesSpaceConsumption();
        return (size <= availableSpace);
    }

    std::optional<MergeResult<Key>> tryMerge(ModifyOperation<Key>& op) {
        if (this->count > minEntries) {
            return std::nullopt;
        }

        auto parent = static_cast<typename Tree::InnerType*>(op.remainingPath.top());
        assert(parent->type == PageType::BTreeInnerNode);

        MyType* otherNode = nullptr;
        auto [leftNode, leftSeparatorKey, rightNode] = parent->getSiblings(this, op);
        assert(leftNode == nullptr || leftNode->type == PageType::BTreeLeafNode);
        assert(rightNode == nullptr || rightNode->type == PageType::BTreeLeafNode);
        auto separatorKey = op.key;
        if (rightNode == nullptr) {
            otherNode = static_cast<MyType*>(leftNode);
            separatorKey = leftSeparatorKey;
        }
        if (otherNode == nullptr) {
            return std::nullopt;
        }
// FIXME aquire read lock or exclusive lock?
        auto self = this;
        auto spaceConsumption = nodeEntriesSpaceConsumption() + otherNode->nodeEntriesSpaceConsumption();
        if (!fits(spaceConsumption)) {
            // TODO resize page
            self = resize(spaceConsumption);
        }

        // copy contents
        std::memcpy(keys + this->count, otherNode->keys, sizeof(Key)*otherNode->count);
        std::memcpy(payloads + this->count, otherNode->payloads, sizeof(Payload)*otherNode->count);
        this->count += otherNode->count;

        return MergeResult<Key>{separatorKey, self};
    }
};

template<class Tree>
struct InnerNode : public ExtendedHeader<Tree> {
    using MyType = InnerNode<Tree>;
    using Key = typename Tree::KeyType;

    static const PageType typeMarker = PageType::BTreeInnerNode;

    static constexpr size_t calcMaxEntryCount() {
        size_t remaining = nodeSize - sizeof(ExtendedHeader<Tree>);
        return remaining / (sizeof(Key) + sizeof(NodeBase*));
    }

    static constexpr size_t maxEntries = calcMaxEntryCount();// 1<<10;
    static constexpr size_t minEntries = maxEntries/2;

    static constexpr size_t pagePayloadSpacePlaceholder = maxEntries*(sizeof(Key) + sizeof(NodeBase*));

    Key keys[maxEntries];
    std::atomic<NodeBase *> atomicChildren[maxEntries];
    NodeBase ** children;

    InnerNode() {
        children = reinterpret_cast<NodeBase **>(atomicChildren);
        this->count = 0;
        this->type = typeMarker;
    }

    size_t nodeEntriesSpaceConsumption() const {
/*
#ifdef VARIABLE_SIZED_KEYS
        throw 0;
#else
        return count*(sizeof(Key) + sizeof(NodeBase*));
#endif
*/
        if constexpr (Tree::hasVariableSizedKeys) {
            throw 0; // TODO
        } else {
            return this->count*(sizeof(Key) + sizeof(NodeBase*));
        }
    }

    bool fits(size_t size) const {
        // TODO replace placeholder
        size_t availableSpace = pagePayloadSpacePlaceholder - nodeEntriesSpaceConsumption();
        return (size <= availableSpace);
    }

    // return {rightNode, separator}
    std::tuple<std::unique_ptr<InnerNode>, Key> split() {
        auto newInner = std::make_unique<MyType>();
        newInner->count = this->count - (this->count / 2);
        this->count = this->count - newInner->count - 1;
        auto sep = keys[this->count];
        memcpy(newInner->keys, keys + this->count + 1, sizeof(Key) * (newInner->count + 1));
        memcpy(newInner->children, children + this->count + 1, sizeof(NodeBase*) * (newInner->count + 1));
        return std::make_tuple(std::move(newInner), sep);
    }

    void insert(Key key, NodeBase* child) {
        // Splits can't be performed in an eager manner in case variable sized keys are used.
        // Hence, they are handled at this point in contrast to the eager implementation strategy which can found in Tree::insert.
        if constexpr (Tree::hasVariableSizedKeys) {
            size_t entrySize = calculateKeySize(key) + sizeof(child);
            if (!fits(entrySize)) {
                auto [newRightNode, separator] = split();
                if (key >= separator) {
                    newRightNode->insert(key, child);
                    return;
                }
            }
        }

        auto pos = binarySearch(key, keys, this->count);
        memmove(keys + pos + 1, keys + pos, sizeof(Key) * (this->count - pos + 1));
        memmove(children + pos + 1, children + pos, sizeof(NodeBase*) * (this->count - pos + 1));
        keys[pos] = key;
        children[pos] = child;
        std::swap(children[pos], children[pos + 1]);
        this->count++;


// TODO pick sampling points
//throw 0;

        assert(this->count > 0);
#if 0
        size_t scale = maxEntries/nestedSize;
        for (size_t i = 0; i < nestedSize; ++i) {
            size_t idx = (i + 1)*scale - 1;
            if (idx < count) {
                nestedNode[i] = keys[idx];
            } else {
//                assert(i > 0);
//                nestedNode[i] = nestedNode[i - 1];
                nestedNode[i] = keys[count - 1];
            }
        }
#endif
        size_t scale = maxEntries/nestedSize;
        for (size_t i = 0; i < nestedSize; ++i) {
            size_t idx = (i + 1)*scale - 1;
            idx = std::min<size_t>(idx, this->count - 1);
            this->nestedNode[i] = keys[idx];
        }
    }

    std::tuple<NodeBase*, Key, NodeBase*> getSiblings(const NodeBase* child, const TreeOperation<Key>& op) {
        NodeBase* left = nullptr;
        NodeBase* right = nullptr;
        Key leftSeparator;
        auto pos = binarySearch(op.key, keys, this->count);
        if (pos > 0) {
            left = children[pos - 1];
            leftSeparator = keys[pos - 1];
        }
        if (pos + 1 < this->count) {
            right = children[pos + 1];
        }
        return {left, leftSeparator, right};
    }

    void finalizeMerge(ModifyOperation<Key>& op, const MergeResult<Key>& mergeResult) {
        assert(this->count >= 1);

        auto pos = binarySearch(mergeResult.obsoleteSuperjacentKey, keys, this->count);
        if (this->count > 0) {
            assert(pos <= this->count);
            memmove(keys + pos, keys + pos + 1, sizeof(Key) * (this->count - pos + 1));
            memmove(children + pos, children + pos + 1, sizeof(NodeBase*) * (this->count - pos + 1));
            children[pos] = mergeResult.mergedNode;
            this->count -= 1;
        }
        if (this->count <= minEntries) {
            // TODO implement rebalancing?
            // try to merge a sibling into this page
            auto newMergeResult = tryMerge(op);
            op.remainingPath.pop();
            if (newMergeResult) {
                auto parent = static_cast<typename Tree::InnerType*>(op.remainingPath.top());
                assert(parent->type == PageType::BTreeInnerNode);
                parent->finalizeMerge(op, newMergeResult.value());
            }
        }
    }

    std::optional<MergeResult<Key>> tryMerge(ModifyOperation<Key>& op) {
        // TODO
        throw 0;
    }
};

template<class Tree>
struct NodeDeleter {
    void operator()(void * ptr) const {
        NodeBase * node = static_cast<NodeBase *>(ptr);
        if (node->type == PageType::BTreeInnerNode) {
            auto * inner = static_cast<typename Tree::InnerType*>(node);
            delete inner;
        } else if (node->type == PageType::BTreeInnerNode) {
            auto * leaf = static_cast<typename Tree::LeafType*>(node);
            delete leaf;
        } else {
            assert(false);
        }
    }
};

template<class Tree>
struct BTreeHazardPointerTraits {
    static const bool is_specialized = true;

    static bool isObsolete(void * ptr) {
        NodeBase * node = static_cast<NodeBase *>(ptr);
        return node->obsolete.load();
    }

    static void applyDeleter(void * ptr) {
        static NodeDeleter<Tree> deleter;
        deleter(ptr);
    }
};

template<class Tree>
using NodeEpochGuardType = Epoch::EpochGuard<NodeDeleter<Tree>>;

template<class Tree>
using NodeHazardGuard = HazardPointerGuard<BTreeHazardPointerTraits<Tree>>;

class DummyHazardPointerGuard {
public:
    DummyHazardPointerGuard(const DummyHazardPointerGuard &) = delete;
    DummyHazardPointerGuard & operator=(const DummyHazardPointerGuard &) = delete;

    DummyHazardPointerGuard() { }

    DummyHazardPointerGuard(void * ptr) { }

    ~DummyHazardPointerGuard() { }

    void swap(DummyHazardPointerGuard & other) { }

    void adoptAndUnprotectPrevious(void * newPtr) { }

    void reset() { }

    bool validateOrResetPointers(void * ptr) {
        return true;
    }
};

class DummyEpochGuard {};

template<class Key, class Payload>
template<class InnerKey, class InnerPayload>
struct Tree<Key, Payload>::Impl {
    using KeyType = Key;
    using PayloadType = Payload;
    using MyType = Impl<Key, Payload>;

    using InnerType = InnerNode<MyType>;
    using LeafType = LeafNode<MyType>;

    using GuardType = NodeEpochGuardType<MyType>;
    using MyEpochGuardType = NodeEpochGuardType<MyType>;
    using MyHazardGuardType = NodeHazardGuard<MyType>;
    using MyDeleterType = NodeDeleter<MyType>;

    using ChosenHazardGuardType =
        typename std::conditional<
            std::is_same<GuardType, MyHazardGuardType>::value, MyHazardGuardType, DummyHazardPointerGuard>::type;

    using ChosenEpochGuardType =
        typename std::conditional<
            std::is_same<GuardType, MyEpochGuardType>::value, MyEpochGuardType, DummyEpochGuard>::type;

    template<class T>
    using useIntegralKeysCheck = std::is_integral<T>;
    template<class T>
    using hasVariableSizedKeysCheck = std::is_same<T, ByteArrayKey>;
    static constexpr bool useIntegralKeys = useIntegralKeysCheck<Key>::value;
    static constexpr bool hasVariableSizedKeys = hasVariableSizedKeysCheck<Key>::value;
    static_assert(useIntegralKeys || hasVariableSizedKeys, "'Key' has to be either an integral data type or 'ByteArrayKey'");

//    BufferManager& bm;

    struct RootNodeGuard {
        std::atomic<uint64_t> version {0};
    } rootNodeGuard;
    std::atomic<NodeBase*> root;

    Impl() {
        root = new LeafType();
    }

    ~Impl() {
        freeNode(root);
    }

    void freeNode(NodeBase* node) {
        if (node == nullptr) {
            return;
        }

        if (node->type == PageType::BTreeInnerNode) {
            auto inner = static_cast<InnerType*>(node);
            for (size_t i = 0; i <= node->count; ++i) {
                NodeBase* child = inner->atomicChildren[i];
                freeNode(child);
            }
            delete inner;
        } else {
            auto leaf = static_cast<LeafType*>(node);
            delete leaf;
        }
    }

    void makeRoot(Key k, NodeBase* leftChild, NodeBase* rightChild) {
        auto inner = new InnerType();
        inner->count = 1;
        inner->keys[0] = k;
        inner->atomicChildren[0] = leftChild;
        inner->atomicChildren[1] = rightChild;
        root = inner;
    }

    template<class U = GuardType, typename std::enable_if< std::is_same<U, MyEpochGuardType>::value, int >::type = 0 >
    inline void enqueueForDeletion(NodeBase * node) {
        Epoch::addToDeletionList<MyDeleterType>(node);
    }

    template<class U = GuardType, typename std::enable_if< std::is_same<U, MyHazardGuardType>::value, int >::type = 0 >
    inline void enqueueForDeletion(NodeBase * node) {
        node->obsolete = true;
    }

    template<class U = GuardType, typename std::enable_if< std::is_same<U, void>::value, int >::type = 0 >
    inline void enqueueForDeletion(NodeBase * node) {
        // NOP
    }

    enum class InsertResult { Success, RestartAsInvasiveOperation };

    void insert(Key key, Payload payload) {
        ChosenEpochGuardType guard;

        InsertOperation<Key, Payload> op;
        op.key = key;
        op.payload = payload;
        op.latchMode = BTreeLatchMode::LocallyRestricted;
        auto result = simpleInsert(op);
        if (result == InsertResult::RestartAsInvasiveOperation) {
            op.latchMode = BTreeLatchMode::FullAccessPath;
            result = invasiveInsert(op);
        }
    }

    template<class U = KeyType, typename std::enable_if<hasVariableSizedKeysCheck<U>::value, int>::type = 0>
    InsertResult simpleInsert(InsertOperation<Key, Payload>& op) {
        // TODO
        throw 0;
// TODO relax locking contraints (like simpleRemove())
        assert(op.latchMode == BTreeLatchMode::LocallyRestricted);
        InsertResult result = InsertResult::Success;
        restart:
        {
            uint64_t nodeVersion = 0, parentVersion = 0, rootPtrVersion = 0;
            ChosenHazardGuardType nodeHazardGuard, parentHazardGuard;
            bool restartRequired = false;

            std::tie(rootPtrVersion, restartRequired) = readLockOrRestart(&rootNodeGuard);
            if (restartRequired) { goto restart; }

            NodeBase* node = root.load();
            // NOTE: the following two lines will be optimized out if Guard != HazardPointerGuard
            // (checked with gcc 7.2)
            nodeHazardGuard.adoptAndUnprotectPrevious(node);
            if (!nodeHazardGuard.validateOrResetPointers(root.load())) { goto restart; }
            std::tie(nodeVersion, restartRequired) = readLockOrRestart(node);
            if (restartRequired) { goto restart; }

            InnerNode<Key>* parent = nullptr;

            while (node->type == PageType::BTreeInnerNode) {
                auto inner = static_cast<InnerType*>(node);
//                auto pos = inner->binarySearch(op.key);
                auto pos = binarySearch(op.key, inner->keys, inner->count);
                NodeBase* child = inner->atomicChildren[pos];

                ChosenHazardGuardType childHazardGuard(child);
                if (!childHazardGuard.validateOrResetPointers(inner->atomicChildren[pos].load())) { goto restart; }

                restartRequired = checkOrRestart(inner, nodeVersion);
                if (restartRequired) { goto restart; }
                assert(child != nullptr);
/*
                if (child == nullptr) {
                    auto newLeaf = std::make_unique<LeafNode<Key, Payload>>();
                    child = newLeaf.get();

                    restartRequired = upgradeToWriteLockOrRestart(inner, nodeVersion);
                    if (restartRequired) { goto restart; }

                    inner->atomicChildren[pos] = newLeaf.release();

                    nodeVersion = downgradeToReadLock(inner);
                }
*/

                uint64_t childVersion;
                std::tie(childVersion, restartRequired) = readLockOrRestart(child);
                if (restartRequired) { goto restart; }

                if (parent) {
                    restartRequired = readUnlockOrRestart(parent, parentVersion);
                    if (restartRequired) { goto restart; }
                } else {
                    restartRequired = readUnlockOrRestart(&rootNodeGuard, rootPtrVersion);
                    if (restartRequired) { goto restart; }
                }

                parent = inner;
                parentVersion = nodeVersion;
                node = child;
                nodeVersion = childVersion;
                // adjust guards
                parentHazardGuard.swap(nodeHazardGuard);
                nodeHazardGuard.swap(childHazardGuard);
            }

            // upgrade locks
            if (parent) {
                restartRequired = upgradeToWriteLockOrRestart(parent, parentVersion);
                if (restartRequired) { goto restart; }
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, parent);
                if (restartRequired) { goto restart; }
            } else {
                restartRequired = upgradeToWriteLockOrRestart(&rootNodeGuard, rootPtrVersion);
                if (restartRequired) { goto restart; }
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, &rootNodeGuard);
                if (restartRequired) { goto restart; }
            }

            auto leaf = static_cast<LeafType*>(node);
            if (parent) {
                writeUnlock(parent);
            } else {
                writeUnlock(&rootNodeGuard);
            }

            if (leaf->count == leaf->maxEntries) {
                result = InsertResult::RestartAsInvasiveOperation;
            } else {
                leaf->insert(op);
                writeUnlock(node);
            }
       }

        return result;
    }

    template<class U = KeyType, typename std::enable_if<!hasVariableSizedKeysCheck<U>::value, int>::type = 0>
    InsertResult simpleInsert(InsertOperation<Key, Payload>& op) {
        assert(op.latchMode == BTreeLatchMode::LocallyRestricted);
        InsertResult result = InsertResult::Success;
        restart:
        {
            uint64_t nodeVersion = 0, parentVersion = 0, rootPtrVersion = 0;
            ChosenHazardGuardType nodeHazardGuard, parentHazardGuard;
            bool restartRequired = false;

            std::tie(rootPtrVersion, restartRequired) = readLockOrRestart(&rootNodeGuard);
            if (restartRequired) { goto restart; }

            NodeBase* node = root.load();
            // NOTE: the following two lines will be optimized out if Guard != HazardPointerGuard
            // (checked with gcc 7.2)
            nodeHazardGuard.adoptAndUnprotectPrevious(node);
            if (!nodeHazardGuard.validateOrResetPointers(root.load())) { goto restart; }

            std::tie(nodeVersion, restartRequired) = readLockOrRestart(node);
            if (restartRequired) { goto restart; }

            InnerType* parent = nullptr;

            while (node->type == PageType::BTreeInnerNode) {
                auto inner = static_cast<InnerType*>(node);

                if (inner->count == inner->maxEntries - 1) {
                    // upgrade locks
                    if (parent) {
                        restartRequired = upgradeToWriteLockOrRestart(parent, parentVersion);
                        if (restartRequired) { goto restart; }
                        restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, parent);
                        if (restartRequired) { goto restart; }
                    } else {
                        restartRequired = upgradeToWriteLockOrRestart(&rootNodeGuard, rootPtrVersion);
                        if (restartRequired) { goto restart; }
                        restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, &rootNodeGuard);
                        if (restartRequired) { goto restart; }
                    }

                    // Split inner eagerly
//                    Key sep;
                    auto* left = inner;
                    auto [newRightInner, separator] = left->split();

                    if (op.key >= separator) {
                        uint64_t newInnerVersion;
                        std::tie(newInnerVersion, restartRequired) = writeLockOrRestart(newRightInner.get());
                        assert(!restartRequired);
                        writeUnlock(node);

                        inner = newRightInner.get();
                        node = newRightInner.get();
                        nodeVersion = newInnerVersion;

                        // change hazard pointer
                        nodeHazardGuard.adoptAndUnprotectPrevious(newRightInner.get());
                        // new node -> no validation required
                    }

                    if (parent) {
                        parent->insert(separator, newRightInner.release());

                        nodeVersion = downgradeToReadLock(node);
                        parentVersion = downgradeToReadLock(parent);
                    } else {
                        makeRoot(separator, left, newRightInner.release());

                        nodeVersion = downgradeToReadLock(node);
                        rootPtrVersion = downgradeToReadLock(&rootNodeGuard);
                    }
                }

//                auto pos = inner->binarySearch(op.key);
                auto pos = binarySearch(op.key, inner->keys, inner->count);
                NodeBase * child = inner->atomicChildren[pos];

                ChosenHazardGuardType childHazardGuard(child);
                if (!childHazardGuard.validateOrResetPointers(inner->atomicChildren[pos].load())) { goto restart; }

                restartRequired = checkOrRestart(inner, nodeVersion);
                if (restartRequired) { goto restart; }
                assert(child != nullptr);
/*
                if (child == nullptr) {
                    auto newLeaf = std::make_unique<LeafNode<Key, Payload>>();
                    child = newLeaf.get();

                    restartRequired = upgradeToWriteLockOrRestart(inner, nodeVersion);
                    if (restartRequired) { goto restart; }

                    inner->atomicChildren[pos] = newLeaf.release();

                    nodeVersion = downgradeToReadLock(inner);
                }
*/

                uint64_t childVersion;
                std::tie(childVersion, restartRequired) = readLockOrRestart(child);
                if (restartRequired) { goto restart; }

                if (parent) {
                    restartRequired = readUnlockOrRestart(parent, parentVersion);
                    if (restartRequired) { goto restart; }
                } else {
                    restartRequired = readUnlockOrRestart(&rootNodeGuard, rootPtrVersion);
                    if (restartRequired) { goto restart; }
                }

                parent = inner;
                parentVersion = nodeVersion;
                node = child;
                nodeVersion = childVersion;
                // adjust guards
                parentHazardGuard.swap(nodeHazardGuard);
                nodeHazardGuard.swap(childHazardGuard);
            }

            // upgrade locks
            if (parent) {
                restartRequired = upgradeToWriteLockOrRestart(parent, parentVersion);
                if (restartRequired) { goto restart; }
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, parent);
                if (restartRequired) { goto restart; }
            } else {
                restartRequired = upgradeToWriteLockOrRestart(&rootNodeGuard, rootPtrVersion);
                if (restartRequired) { goto restart; }
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, &rootNodeGuard);
                if (restartRequired) { goto restart; }
            }

            auto leaf = static_cast<LeafType*>(node);
            if (leaf->count == leaf->maxEntries) {
                // Leaf is full, split it
//                Key sep;
                auto * left = leaf;
                auto [newRightLeaf, separator] = left->split();

                if (op.key >= separator) {
                    uint64_t newLeafVersion;
                    std::tie(newLeafVersion, restartRequired) = writeLockOrRestart(newRightLeaf.get());
                    assert(!restartRequired);

                    writeUnlock(leaf);
                    leaf = newRightLeaf.get();
                    node = newRightLeaf.get();
                    nodeVersion = newLeafVersion;

                    // change hazard pointer
                    nodeHazardGuard.adoptAndUnprotectPrevious(newRightLeaf.get());
                    // new node -> no validation required
                }

                if (parent) {
                    parent->insert(separator, newRightLeaf.release());
                } else {
                    makeRoot(separator, left, newRightLeaf.release());
                }
            }

            if (parent) {
                writeUnlock(parent);
            } else {
                writeUnlock(&rootNodeGuard);
            }

            leaf->insert(op);
            writeUnlock(node);
       }

        return result;
    }

    InsertResult invasiveInsert(InsertOperation<Key, Payload>& op) {
        assert(op.latchMode == BTreeLatchMode::FullAccessPath);
        // TODO
        throw 0;
    }

    enum class RemoveResult { Success, KeyNotFound, RestartAsInvasiveOperation };

    bool remove(Key key) {
        // TODO
        // constexpr if

        RemoveOperation<Key> op;
        op.key = key;
        op.latchMode = BTreeLatchMode::LocallyRestricted;
        auto result = simpleRemove(op);
        if (result == RemoveResult::RestartAsInvasiveOperation) {
            op.latchMode = BTreeLatchMode::FullAccessPath;
            result = invasiveRemove(op);
        }
        return (result == RemoveResult::Success);
    }

    // should be key-type agnostic
    RemoveResult simpleRemove(RemoveOperation<Key>& op) {
        assert(op.latchMode == BTreeLatchMode::LocallyRestricted);
        RemoveResult result = RemoveResult::Success;
        restart:
        {
            uint64_t nodeVersion = 0, parentVersion = 0, rootPtrVersion = 0;
            ChosenHazardGuardType nodeHazardGuard, parentHazardGuard;
            bool restartRequired = false;

            std::tie(rootPtrVersion, restartRequired) = readLockOrRestart(&rootNodeGuard);
            if (restartRequired) { goto restart; }

            NodeBase* node = root.load();

            nodeHazardGuard.adoptAndUnprotectPrevious(node);
            if (!nodeHazardGuard.validateOrResetPointers(root.load())) { goto restart; }

            std::tie(nodeVersion, restartRequired) = readLockOrRestart(node);
            if (restartRequired) { goto restart; }
            restartRequired = readUnlockOrRestart(&rootNodeGuard, rootPtrVersion);
            if (restartRequired) { goto restart; }

            InnerType* parent = nullptr;

            while (node->type == PageType::BTreeInnerNode) {
                auto inner = static_cast<InnerType*>(node);
//                auto pos = inner->binarySearch(op.key);
                auto pos = binarySearch(op.key, inner->keys, inner->count);
                NodeBase* child = inner->atomicChildren[pos];

                ChosenHazardGuardType childHazardGuard(child);
                if (!childHazardGuard.validateOrResetPointers(inner->atomicChildren[pos].load())) { goto restart; }

                restartRequired = checkOrRestart(inner, nodeVersion);
                if (restartRequired) { goto restart; }
                assert(child != nullptr);
/* should not happen
                if (child == nullptr) {
                    if (parent) {
                        restartRequired = readUnlockOrRestart(parent, parentVersion);
                        if (restartRequired) { goto restart; }
                    }
                    restartRequired = readUnlockOrRestart(node, nodeVersion);
                    if (restartRequired) { goto restart; }
                    // nothing to do
                    return;
                }
*/
                uint64_t childVersion;
                std::tie(childVersion, restartRequired) = readLockOrRestart(child);
                if (restartRequired) { goto restart; }

                if (parent) {
                    restartRequired = readUnlockOrRestart(parent, parentVersion);
                    if (restartRequired) { goto restart; }
                }

                parent = inner;
                parentVersion = nodeVersion;
                node = child;
                nodeVersion = childVersion;
                // adjust guards
                parentHazardGuard.swap(nodeHazardGuard);
                nodeHazardGuard.swap(childHazardGuard);
            }

            // upgrade locks
            if (parent) {
//                restartRequired = upgradeToWriteLockOrRestart(parent, parentVersion);
//                if (restartRequired) { goto restart; }
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion, parent);
                if (restartRequired) { goto restart; }
            } else {
                restartRequired = upgradeToWriteLockOrRestart(node, nodeVersion);
                if (restartRequired) { goto restart; }
            }

            auto leaf = static_cast<LeafType*>(node);
            if (leaf->count > LeafType::minEntries) {
                bool removed = leaf->remove(op);
                writeUnlock(leaf);
                if (!removed) { result = RemoveResult::KeyNotFound; }
            } else {
                result = RemoveResult::RestartAsInvasiveOperation;
            }
/*
            if (parent) {
                writeUnlock(parent);
            }
*/
        }

        return result;
    }

    RemoveResult invasiveRemove(RemoveOperation<Key>& op) {
        assert(op.latchMode == BTreeLatchMode::FullAccessPath);


//        std::stack<optimisticlock> locks;
        // TODO

        // perform action

        // release locks

        throw 0;
    }

    template<SearchApproach approach>
    bool lookup(Key key, Payload& result) const {
        ChosenEpochGuardType guard;
        TreeOperation<Key> op;
        op.key = key;
        return lookupImpl<approach>(op, result);
    }

    template<SearchApproach approach>
    bool lookupImpl(TreeOperation<Key>& op, Payload& result) const {
        restart:
        {
            uint64_t nodeVersion = 0, parentVersion = 0, rootPtrVersion = 0;
            ChosenHazardGuardType nodeHazardGuard, parentHazardGuard;
            bool restartRequired = false;

            std::tie(rootPtrVersion, restartRequired) = readLockOrRestart(&rootNodeGuard);
            if (restartRequired) { goto restart; }

            NodeBase* node = root.load();
            // NOTE: the following two lines will be optimized out if Guard != HazardPointerGuard
            // (checked with gcc 7.2)
            nodeHazardGuard.adoptAndUnprotectPrevious(node);
            if (!nodeHazardGuard.validateOrResetPointers(root.load())) { goto restart; }

            std::tie(nodeVersion, restartRequired) = readLockOrRestart(node);
            if (restartRequired) { goto restart; }
            restartRequired = readUnlockOrRestart(&rootNodeGuard, rootPtrVersion);
            if (restartRequired) { goto restart; }

            InnerType* parent = nullptr;

            while (node->type == PageType::BTreeInnerNode) {
                auto inner = static_cast<InnerType*>(node);
                using LocatorType = LocatePositionAndPrepareNextIteration<approach, MyType, typename std::remove_reference<decltype(*inner)>::type>;
                auto pos = LocatorType::doImpl(op, *inner);

                NodeBase* child = inner->atomicChildren[pos];

                ChosenHazardGuardType childHazardGuard(child);
                if (!childHazardGuard.validateOrResetPointers(inner->atomicChildren[pos].load())) { goto restart; }

                restartRequired = checkOrRestart(inner, nodeVersion);
                if (restartRequired) { goto restart; }
                assert(child != nullptr);

                uint64_t childVersion;
                std::tie(childVersion, restartRequired) = readLockOrRestart(child);
                if (restartRequired) { goto restart; }

                if (parent) {
                    restartRequired = readUnlockOrRestart(parent, parentVersion);
                    if (restartRequired) { goto restart; }
                }

                parent = inner;
                parentVersion = nodeVersion;
                node = child;
                nodeVersion = childVersion;
                // adjust guards
                parentHazardGuard.swap(nodeHazardGuard);
                nodeHazardGuard.swap(childHazardGuard);
            }

            if (parent) {
                restartRequired = readUnlockOrRestart(parent, parentVersion);
                if (restartRequired) { goto restart; }
            }

            auto leaf = static_cast<LeafType*>(node);
            using LocatorType = LocatePositionAndPrepareNextIteration<approach, MyType, typename std::remove_reference<decltype(*leaf)>::type>;
            auto pos = LocatorType::doImpl(op, *leaf);

            bool inTree = false;
            if ((pos < leaf->count) && (leaf->keys[pos] == op.key)) {
                result = leaf->payloads[pos];
                inTree = true;
            }

            restartRequired = readUnlockOrRestart(node, nodeVersion);
            if (restartRequired) { goto restart; }

            return inTree;
        }
    }

    bool validateInvariant() const {
        /*
        auto innerValidator = [](const InnerType& node) {

        }

        uint64_t rootPtrVersion;
        bool restartRequired = false;
        std::tie(rootPtrVersion, restartRequired) = writeLockOrRestart(&rootNodeGuard);

        return true; // TODO
        */
       return true;
    }
};

template<class Key, class Payload>
Tree<Key, Payload>::Tree() : pImpl(std::make_unique<Impl<Key, Payload>>()) {}

template<class Key, class Payload>
Tree<Key, Payload>::~Tree() {}

template<class Key, class Payload>
void Tree<Key, Payload>::insert(const Key& key, const Payload& payload) {
    return pImpl->insert(key, payload);
}

template<class Key, class Payload>
bool Tree<Key, Payload>::lookup(const Key& key, Payload& result) const {
    return pImpl->template lookup<SearchApproach::VanillaBinarySearch>(key, result);
}

template<class Key, class Payload>
bool Tree<Key, Payload>::lookupWithPositionHint(const Key& key, Payload& result) const {
    return pImpl->template lookup<SearchApproach::ParentInterpolationHintBinarySearch>(key, result);
}

template<class Key, class Payload>
bool Tree<Key, Payload>::lookupWithNestedNode(const Key& key, Payload& result) const {
    return pImpl->template lookup<SearchApproach::NestedNodeBinarySearch>(key, result);
}

template<class Key, class Payload>
bool Tree<Key, Payload>::remove(const Key& key) {
    return pImpl->remove(key);
}

template<class Key, class Payload>
bool Tree<Key, Payload>::validateInvariant() const {
    return pImpl->validateInvariant();
}

// available template specializations
//template class Tree<int, int>;
template class Tree<uint32_t, uint32_t>;
template class Tree<uint32_t, size_t>;
template class Tree<long, size_t>;

} // end namespace BTree
