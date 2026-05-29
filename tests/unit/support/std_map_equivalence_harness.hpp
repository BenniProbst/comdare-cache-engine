#pragma once
// V41 Roadmap-4 / Saeule 3 (Doku 24 §2.3) — Test-Support: Achsen-Vergleich gegen das vereinheitlichte
// std::map-Interface (bekannter Algorithmus) als eigene KORREKTHEITS-/Interface-Dimension (latenzfrei).
//
// Zwei Bausteine:
//   (a) verify_matches_std_map<Wrapper>  — VERTIKAL: jede Variante einzeln == std::map-Ground-Truth.
//       VERBATIM extrahiert aus tests/unit/test_v41_topic_traversal.cpp:543-567 (gleiche Konstante,
//       600 Ops, i%7==0->erase, v=k*11+1, Lookup-Sweep, occupied_count==size). Die Bestandsdatei behaelt
//       ihre lokale Kopie (unveraendert) — dieser Header ist die wiederverwendbare, DRY-Variante.
//   (b) verify_variants_equivalent<Anchor, Others...> — HORIZONTAL: alle Varianten EINER Achse liefern
//       fuer DENSELBEN deterministischen Op-Stream IDENTISCHE Resultate (lookup + occupied_count). Beweist
//       sie als austauschbare Varianten (genetisches Experiment, Doku 14 §1.2). Vergleicht NUR Resultate,
//       NICHT die interne Slot-Reihenfolge (Korrektheit ⊥ Innen-Verhalten). Duck-typed → ComposedSearch
//       UND ComposedTreeSearch ohne gemeinsamen Basistyp.
//
// Latenzfrei: KEIN <chrono>, keine ns/op/Throughput — Performance-Rangfolge ist die Tier-Dimension (Roadmap-3).

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <type_traits>

namespace comdare::cache_engine::test_support {

/// (a) Vertikaler Vergleich: Wrapper == std::map-Ground-Truth (verbatim aus dem Referenz-Harness).
template <class Wrapper>
void verify_matches_std_map(std::uint32_t key_mod, std::uint32_t query_max) {
    using K = typename Wrapper::key_type;
    Wrapper w{};
    std::map<K, std::uint64_t> ref;
    for (std::uint32_t i = 0; i < 600u; ++i) {
        auto const k = static_cast<K>((i * 2654435761u) % key_mod);
        if (i % 7u == 0u) {
            bool const had = (ref.erase(k) != 0u);
            EXPECT_EQ(w.erase(k), had) << "erase-Mismatch key=" << static_cast<std::uint32_t>(k);
        } else {
            auto const v = static_cast<std::uint64_t>(k) * 11u + 1u;
            ref[k] = v;
            w.insert(k, v);
        }
    }
    for (std::uint32_t q = 0; q <= query_max; ++q) {
        auto const key = static_cast<K>(q);
        auto const it  = ref.find(key);
        auto const got = w.lookup(key);
        if (it != ref.end()) { ASSERT_TRUE(got.has_value()) << "key=" << q; EXPECT_EQ(*got, it->second); }
        else                 { EXPECT_FALSE(got.has_value()) << "key=" << q; }
    }
    EXPECT_EQ(w.occupied_count(), ref.size());
}

namespace detail {
/// Treibt EINEN deterministischen Op-Stream (identisch zu verify_matches_std_map) auf w.
template <class Wrapper>
void drive_reference_stream(Wrapper& w, std::uint32_t key_mod) {
    using K = typename Wrapper::key_type;
    for (std::uint32_t i = 0; i < 600u; ++i) {
        auto const k = static_cast<K>((i * 2654435761u) % key_mod);
        if (i % 7u == 0u) { (void)w.erase(k); }
        else              { w.insert(k, static_cast<std::uint64_t>(k) * 11u + 1u); }
    }
}

/// Vergleicht other gegen anchor nach identischem Stream: occupied_count + Lookup-Sweep.
template <class Anchor, class Other>
void compare_one_variant(Anchor const& anchor, std::uint32_t key_mod, std::uint32_t query_max) {
    Other other{};
    drive_reference_stream(other, key_mod);
    EXPECT_EQ(other.occupied_count(), anchor.occupied_count())
        << "occupied_count-Mismatch zwischen Varianten";
    using KA = typename Anchor::key_type;
    using KO = typename Other::key_type;
    for (std::uint32_t q = 0; q <= query_max; ++q) {
        auto const va = anchor.lookup(static_cast<KA>(q));
        auto const vo = other.lookup(static_cast<KO>(q));
        EXPECT_EQ(va.has_value(), vo.has_value()) << "has_value-Mismatch q=" << q;
        if (va.has_value() && vo.has_value()) EXPECT_EQ(*va, *vo) << "value-Mismatch q=" << q;
    }
}
}  // namespace detail

/// (b) Horizontaler Vergleich: Anchor + alle Others ueber DENSELBEN Stream liefern identische Resultate.
/// Transitiv mit (a): Other==Anchor && Anchor==std::map ⇒ alle Varianten == std::map (austauschbar).
template <class Anchor, class... Others>
void verify_variants_equivalent(std::uint32_t key_mod, std::uint32_t query_max) {
    static_assert(sizeof...(Others) >= 1, "mindestens eine weitere Variante zum Vergleich noetig");
    Anchor anchor{};
    detail::drive_reference_stream(anchor, key_mod);
    (detail::compare_one_variant<Anchor, Others>(anchor, key_mod, query_max), ...);
}

}  // namespace comdare::cache_engine::test_support
