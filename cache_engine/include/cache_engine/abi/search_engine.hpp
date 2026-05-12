#pragma once
// search_engine - Stufe 3 der Drei-Schichten-Hierarchie (REV 7 §4.2(a)+(f))
//
// Erbt von execution_engine + spezialisiert auf Suchalgorithmus-Typen.
// STELLT Such-spezifische, komplexere experimentelle Standard-OS-Such- und
// Speicherzugriffsmuster + Routinen BEREIT. Dach des Konstrukts mit
// Suchheuristiken und Konzepten, die die CacheEngine-Limits "weit ueberschreiten".

#include "configuration_permutation.hpp"
#include "execution_engine.hpp"
#include "search_algorithm_type_collection.hpp"

#include <cstddef>
#include <optional>

namespace comdare {

// SearchEngine erbt von ExecutionEngine + spezialisiert auf Suchalgorithmus-Typen
template <typename Collection, typename ConfigPermutation>
class search_engine
    : public execution_engine<typename ConfigPermutation::strategy_t>
{
public:
    using collection_t = Collection;
    using config_t     = ConfigPermutation;
    using key_t        = typename Collection::key_t;
    using value_t      = typename Collection::value_t;
    using binary_key_t = typename Collection::binary_key_t;
    using base_t       = execution_engine<typename ConfigPermutation::strategy_t>;

    using base_t::base_t;   // inherit constructors

    // SearchEngine-Patterns (REV 7 §4.2(f) - Provider-Rolle):
    //   Trie-Walk, B+-Range-Scan, Prefix-Scan, Hot-Path-Lookup, Density-Threshold-Transition

    [[nodiscard]] virtual std::optional<value_t> lookup(key_t const& key) = 0;
    virtual void                                  insert(key_t const& key, value_t const& value) = 0;
    [[nodiscard]] virtual bool                    erase(key_t const& key) = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    [[nodiscard]] virtual bool        empty() const noexcept = 0;

    // Such-Heuristik-Layer: Hot-Path-Recognition, Adaptive-Prefetch-Distance, ...
    virtual void notify_density_threshold(std::size_t bucket_density_pct) {}
    virtual void notify_hot_path_detected(binary_key_t const& path)        {}
    virtual void notify_workload_change(double estimated_locality)         {}

    virtual ~search_engine() = default;
};

}  // namespace comdare
