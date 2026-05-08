#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include "tree.hpp"
#include "profile.hpp"

using namespace BTree;

static constexpr size_t datasetSize = 16e6;
static constexpr int seed = 7;
thread_local static std::mt19937 rg{seed};

static std::vector<long> createDenseDataset() {
    std::vector<long> values(datasetSize);
    for (size_t i = 0; i < datasetSize; ++i) {
        values[i] = i;
    }
    std::shuffle(values.begin(), values.end(), rg);
    return values;
}

static std::vector<long> createSparseDataset() {
    std::vector<long> values(datasetSize*2);
    for (size_t i = 0; i < datasetSize; ++i) {
        values[i] = i;
    }
    std::shuffle(values.begin(), values.end(), rg);
    values.resize(datasetSize); // discard half of the values
    return values;
}

static std::vector<long> createLogNormalDataset() {
    double mu = 20., sigma = 1.;
    std::lognormal_distribution<> d(mu, sigma);
    std::unordered_set<long> seen;
    std::vector<long> values(datasetSize);
    for (size_t i = 0; i < datasetSize;) {
        size_t value = d(rg);
        if (seen.count(value) > 0) { continue; }
        seen.insert(value);
        values[i] = value;
        i += 1;
//        printf("value: %lu\n", value);
    }
    return values;
}

template<class T>
auto split(std::vector<T>& vec, size_t parts) {
    using iterator = typename std::remove_reference<decltype(vec)>::type::iterator;
    std::vector<std::pair<iterator, iterator>> output;

    size_t length = vec.size() / parts;
    size_t remainder = vec.size() % parts;

    size_t begin = 0, end = 0;
    for (size_t i = 0; i < std::min(parts, vec.size()); ++i) {
        end += (remainder > 0) ? (length + !!(remainder--)) : length;
        output.push_back({vec.begin() + begin, vec.begin() + end});
        begin = end;
    }

    return output;
}

template<class T>
static void runInsert(Tree<T, size_t>& tree, typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end, const std::string& name) {
    size_t n = end - begin;

    PerfEvents e;
    // insert
    {
        e.timeAndProfile("insert", n,
            [&]() {
                size_t i = 0;
                for (auto it = begin; it != end; ++it, ++i) {
                    tree.insert(*it, i);
                }
            },
            1, {{"approach", "none"}, {"dataset", name}, {"size", std::to_string(n)}});
    }
}

template<class T, bool omitCheck = false>
static void runLookup(Tree<T, size_t>& tree, typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end, const std::string& name) {
    size_t n = end - begin;

#ifndef NDEBUG
    unsigned lookupRepetitions = 1;
#else
    unsigned lookupRepetitions = 16;
#endif
    PerfEvents e;
    // lookup
    {
        e.timeAndProfile("lookup", n,
            [&]() {
                size_t i = 0;
                for (auto it = begin; it != end; ++it, ++i) {
                    size_t result;
                    bool success = tree.lookup(*it, result);
                    assert(omitCheck || (success && result == i));
                }
            },
            lookupRepetitions, {{"approach", "vanilla"}, {"dataset", name}, {"size", std::to_string(n)}});

        e.timeAndProfile("lookup", n,
            [&]() {
                size_t i = 0;
                for (auto it = begin; it != end; ++it, ++i) {
                    size_t result;
                    bool success = tree.lookupWithPositionHint(*it, result);
                    assert(omitCheck || (success && result == i));
                }
            },
            lookupRepetitions, {{"approach", "parentHint"}, {"dataset", name}, {"size", std::to_string(n)}});

        e.timeAndProfile("lookup", n,
            [&]() {
                size_t i = 0;
                for (auto it = begin; it != end; ++it, ++i) {
                    size_t result;
                    bool success = tree.lookupWithNestedNode(*it, result);
                    assert(omitCheck || (success && result == i));
                }
            },
            lookupRepetitions, {{"approach", "nestedNode"}, {"dataset", name}, {"size", std::to_string(n)}});
    }
}

int main(int argc, char** argv) {
    auto valuesDense = createDenseDataset();
    using dense_value_type = typename decltype(valuesDense)::value_type;
    {
        Tree<dense_value_type, size_t> tree;
        runInsert(tree, valuesDense.begin(), valuesDense.end(), "uniform_full");
        runLookup(tree, valuesDense.begin(), valuesDense.end(), "uniform_full");
    }
    {
        auto splitted = split(valuesDense, 2);
        Tree<dense_value_type, size_t> tree;
        runInsert(tree, splitted[0].first, splitted[0].second, "uniform_half");
        runLookup<dense_value_type, false>(tree, splitted[0].first, splitted[0].second, "uniform_half_in");
        runLookup<dense_value_type, true>(tree, splitted[1].first, splitted[1].second, "uniform_half_out");
    }

    auto valuesSparse = createSparseDataset();
    using sparse_value_type = typename decltype(valuesSparse)::value_type;
    {
        Tree<sparse_value_type, size_t> tree;
        runInsert(tree, valuesSparse.begin(), valuesSparse.end(), "sparse_full");
        runLookup(tree, valuesSparse.begin(), valuesSparse.end(), "sparse_full");
    }
/*
    {
        auto splitted = split(valuesSparse, 2);
        Tree<sparse_value_type, size_t> tree;
        runInsert(tree, splitted[0].first, splitted[0].second, "sparse_half");
        runLookup<sparse_value_type, false>(tree, splitted[0].first, splitted[0].second, "sparse_half_in");
        runLookup<sparse_value_type, true>(tree, splitted[1].first, splitted[1].second, "sparse_half_out");
    }
*/

    auto valuesLogNorm = createLogNormalDataset();
    using lognorm_value_type = typename decltype(valuesLogNorm)::value_type;

    {
        Tree<lognorm_value_type, size_t> tree;
        runInsert(tree, valuesLogNorm.begin(), valuesLogNorm.end(), "lognormal_full");
        runLookup(tree, valuesLogNorm.begin(), valuesLogNorm.end(), "lognormal_full");
    }

    return 0;
}
