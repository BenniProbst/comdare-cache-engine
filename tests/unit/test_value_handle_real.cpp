// §4.3 (User-Direktive 2026-06-04) — REALER value_handle: Pool/Version/Chain-Deref gegen reale Slot-Struktur.
// Verifiziert LITERAL, dass die NICHT-inline value_handle-Strategien (ExternalPool/ImmutableSharedRef/
// VersionedPointer/ChainRef) seit §4.3 eine ECHTE persistente Slot-Struktur tragen, die aus den eingefuegten
// (key,value)-Paaren gebaut + real dereferenziert wird (Pool-Indirektion / MVCC-Tag-Strip / Chain-Chasing) —
// NICHT mehr nur eine flache Roh-Puffer-SIMULATION. Gleichzeitig: 'inline' (M3-Pin) bleibt EXAKT unveraendert
// (EmptyRealSlot, kein store_value, messneutral), die static value_access_scan-Signatur (Pfad-A seg19) bleibt heil,
// und der Memento ist BIT-EXAKT ueber den Zwei-Phasen-Zyklus (R1).
//
//   (a) Build aus N (key,value)-Paaren → REALE Slot-Struktur befuellt: deref der eingefuegten Keys liefert den
//       EXAKTEN Value zurueck (Pool/Version/Chain); nicht-eingefuegte Keys → deref liefert false (kein Slot).
//       Pool-/Chain-Fuellstand (slot_count/pool_size/chain_nodes) waechst mit N (echte Struktur, kein no-op).
//   (b) 'inline' (M3-Pin) ist UNVERAENDERT + messneutral: ObservableValueHandle<InlineValueHandle> traegt KEIN
//       store_value/deref_value (EmptyRealSlot) → der real_slot() ist leer (slot_count==0), die static
//       value_access_scan bleibt verfuegbar + bit-identisch (Pfad-A unberuehrt).
//   (c) Memento-Exaktheit ueber den Zwei-Phasen-Zyklus (Tier mit ChainRef-/ExternalPool-value_handle):
//       save_all → Warmup-Insert (mutiert vh_organ_ via store_value) → rollback_all → die reale Slot-Struktur ist
//       BIT-EXAKT wie vor save (operator==), und der zurueckgerollte Deref liefert die Original-Keys + NICHT die
//       nur-im-Warmup eingefuegten Keys (kein Leck ueber den Rollback). Inline-Tier: Memento ist trivial leer-exakt.
//   (d) static value_access_scan(buf,n,record_size)-Signatur unveraendert: der Pfad-A-Aufruf kompiliert + liefert.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz
//        (scratch_compile_value_handle_real.ps1, abgeleitet aus scratch_compile_filter_real_from_keys.ps1).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/hot_reference.hpp> // Basis-Composition (value_handle wird im Test ersetzt)
#include <axes/value_handle_axis/axis_14_value_handle_inline.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_external_pool.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_immutable_shared_ref.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_versioned_pointer.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_chain_ref.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_observable.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace vh   = ::comdare::cache_engine::value_handle_axis;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Detektoren (dependent-Kontext, MSVC-konform: ein requires-Ausdruck ueber einen NICHT existierenden Member eines
// KONKRETEN Typs ist bei MSVC ein harter C2039 — ueber einen Template-Parameter wird daraus eine saubere SFINAE-
// Substitution → false). Beweist Compile-Zeit, dass EmptyRealSlot (Inline-Backing) KEIN store_value/deref_value traegt.
template <class S>
concept has_store_value = requires(S s) { s.store_value(std::uint64_t{1}, std::uint64_t{2}); };
template <class S>
concept has_deref_value = requires(S const s) { s.deref_value(std::uint64_t{1}, (std::uint64_t*)nullptr); };

// Eingefuegte (key,value)-Paare + disjunkte Miss-Keys (hoher Offset).
static std::uint64_t ins_key(std::uint64_t i) { return 1000u + i; }
static std::uint64_t ins_val(std::uint64_t i) { return i * 7u + 3u; }
static std::uint64_t miss_key(std::uint64_t i) { return 9'000'000u + i; }

// (a)+(d) je NICHT-inline-Strategie direkt ueber die REALE Slot-Struktur (Hülle ObservableValueHandle).
template <class Strategy>
static void test_real_strategy(std::string_view name, std::uint64_t expect_chain_depth) {
    std::cout << "-- Strategie: " << name << " (REALE Slot-Struktur) --\n";
    constexpr std::uint64_t N = 200;

    vh::ObservableValueHandle<Strategy> organ{};
    // (a) leere Struktur: kein Key dereferenzierbar.
    {
        std::uint64_t v          = 0;
        bool          empty_none = true;
        for (std::uint64_t i = 0; i < N; ++i)
            if (organ.deref_value(ins_key(i), &v)) {
                empty_none = false;
                break;
            }
        tr(std::string{name} + ": (a) leere reale Struktur dereferenziert KEINEN Key", empty_none);
    }

    // (a) Build: store_value je (key,value) → REALE Struktur befuellt.
    for (std::uint64_t i = 0; i < N; ++i) organ.store_value(ins_key(i), ins_val(i));

    // (a) Deref der eingefuegten Keys → EXAKTER Value (Pool/Version/Chain-Deref gegen die echte Struktur).
    //     VersionedPointer addiert das MVCC-Version-Tag (=1 nach erstem store) → Value+1 erwartet.
    std::uint64_t hits = 0, exact = 0;
    for (std::uint64_t i = 0; i < N; ++i) {
        std::uint64_t v = ~std::uint64_t{0};
        if (organ.deref_value(ins_key(i), &v)) {
            ++hits;
            std::uint64_t const want = (name == "VersionedPointer") ? (ins_val(i) + 1u) : ins_val(i);
            if (v == want) ++exact;
        }
    }
    std::cout << "     deref-hits=" << hits << "/" << N << " exact-values=" << exact << "/" << N << "\n";
    tr(std::string{name} + ": (a) ALLE eingefuegten Keys real dereferenzierbar", hits == N);
    tr(std::string{name} + ": (a) deref liefert den EXAKTEN gespeicherten Value (Pool/Version/Chain)", exact == N);

    // (a) Fuellstand der realen Struktur waechst mit N (echtes Backing, kein no-op).
    std::size_t const real_slots = organ.real_slot().slot_count();
    std::size_t const real_back  = organ.real_slot().pool_size() + organ.real_slot().chain_nodes();
    std::cout << "     real slot_count=" << real_slots << " pool+chain=" << real_back << "\n";
    tr(std::string{name} + ": (a) reale Slot-Struktur befuellt (slot_count==N, Backing>0)",
       real_slots == N && real_back >= N);

    // (a) nicht-eingefuegte Keys → KEIN Slot (deref false). Reale Membership, kein Fake-Treffer.
    std::uint64_t miss_hits = 0;
    for (std::uint64_t i = 0; i < N; ++i) {
        std::uint64_t v = 0;
        if (organ.deref_value(miss_key(i), &v)) ++miss_hits;
    }
    tr(std::string{name} + ": (a) nicht-eingefuegte Keys dereferenzieren NICHT (echte Membership)", miss_hits == 0);

    // (a) Indirektions-Charakteristik (peak_chain_depth) korrekt: Pool=2, Chain=3 (ueber observe_value_handle).
    std::vector<unsigned char> buf(N * 16, 0xABu);
    organ.reset();
    (void)organ.observe_value_handle(buf.data(), N, std::size_t{16});
    std::uint64_t const depth = organ.statistics().peak_chain_depth;
    std::cout << "     peak_chain_depth=" << depth << " (erwartet " << expect_chain_depth << ")\n";
    tr(std::string{name} + ": (a) Indirektions-Tiefe strategie-charakteristisch", depth == expect_chain_depth);

    // (b) clear() leert die reale Struktur bit-exakt → wieder leere Baseline (== default-konstruiert).
    organ.clear_slots();
    tr(std::string{name} + ": (b) clear_slots() == leere Default-Struktur (bit-exakt)",
       organ == vh::ObservableValueHandle<Strategy>{});

    // (d) static value_access_scan(buf,n,record_size)-Signatur unveraendert (Pfad-A seg19): kompiliert + liefert.
    std::uint64_t const path_a = Strategy::value_access_scan(buf.data(), N, std::size_t{16});
    std::cout << "     Pfad-A value_access_scan checksum=" << path_a << "\n";
    tr(std::string{name} + ": (d) static value_access_scan-Signatur unveraendert (Pfad-A kompiliert)", true);
}

// ── (b) Inline (M3-Pin) ist messneutral: KEIN reales Slot-Backing, value_access_scan bit-identisch ────────────────
static void test_inline_unchanged() {
    std::cout << "-- Strategie: Inline (M3-Pin, messneutral) --\n";
    vh::ObservableValueHandle<vh::InlineValueHandle> organ{};
    // EmptyRealSlot: 0 Footprint (slot_count==0, kein Build moeglich).
    tr("(b) Inline: reale Slot-Struktur ist LEER (EmptyRealSlot, 0 Footprint)", organ.real_slot().slot_count() == 0);
    // Das reale Slot-Backing fuer Inline ist EmptyRealSlot (Compile-Zeit-Beweis ueber den Backing-Typ selbst: er
    // traegt KEIN store_value/deref_value → der abi_adapter-Build-Hook + die Hülle-Methoden greifen fuer Inline NIE).
    static_assert(std::is_same_v<vh::real_slot_t<vh::InlineValueHandle>, vh::EmptyRealSlot>,
                  "Inline muss EmptyRealSlot tragen (M3-Pin messneutral, additive Leitplanke 1)");
    static_assert(
        !has_store_value<vh::EmptyRealSlot>,
        "EmptyRealSlot (Inline-Backing) darf KEIN store_value tragen → Build-Hook greift nicht (messneutral)");
    static_assert(
        !has_deref_value<vh::EmptyRealSlot>,
        "EmptyRealSlot (Inline-Backing) darf KEIN deref_value tragen (direkter Slot-Read, keine Indirektion)");
    // Gegenprobe: die realen Backings TRAGEN store_value/deref_value (echte Struktur).
    static_assert(has_store_value<vh::real_slot_t<vh::ChainRefValueHandle>> &&
                      has_deref_value<vh::real_slot_t<vh::ChainRefValueHandle>>,
                  "ChainRef-Backing MUSS store_value/deref_value tragen (reale Slot-Struktur)");
    tr("(b) Inline: EmptyRealSlot ohne store_value/deref_value (Compile-Zeit static_assert, messneutral)", true);
    // static value_access_scan unveraendert + bit-identisch zur RAW-Strategie.
    std::vector<unsigned char> buf(256, 0x5Au);
    std::uint64_t const        via_hull =
        vh::ObservableValueHandle<vh::InlineValueHandle>::value_access_scan(buf.data(), 16, 16);
    std::uint64_t const via_raw = vh::InlineValueHandle::value_access_scan(buf.data(), 16, 16);
    tr("(b) Inline: static value_access_scan bit-identisch (Hülle == RAW-Strategie, Pfad-A messneutral)",
       via_hull == via_raw);
}

// (c) Memento ueber den Zwei-Phasen-Zyklus mit einem Tier, dessen value_handle NICHT-inline ist.
template <class VHStrategy>
struct VHComposition : comp::HotComposition {
    using value_handle                     = VHStrategy; // NUR die value_handle-Achse ersetzt (Rest = HotComposition)
    static constexpr std::string_view name = "VHComposition";
};

template <class VHStrategy>
static void test_tier_memento(std::string_view name) {
    std::cout << "-- Tier-Memento (value_handle=" << name << ") --\n";
    using Anatomy = an::SearchAlgorithmAnatomy<VHComposition<VHStrategy>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  rbk  = dynamic_cast<an::IRollbackableTier*>(base);
    tr(std::string{name} + ": (c) IDriveableTier + IRollbackableTier vorhanden", drv != nullptr && rbk != nullptr);
    if (!drv || !rbk) {
        std::cout << "  ABBRUCH (c) (Interface fehlt)\n";
        ++g_fail;
        return;
    }

    constexpr std::uint64_t kBase = 1024; // Original-Keys (Pre-Save-Stand)
    constexpr std::uint64_t kWarm = 256;  // Warmup-Keys (NUR in der save→rollback-Phase)
    for (std::uint64_t i = 0; i < kBase; ++i) (void)drv->tier_insert(i, i * 11u + 5u);

    // Pre-Save-Snapshot des realen value_handle-Organs (ueber value_handle_instance()).
    auto const& vh_before      = tier.value_handle_instance();
    auto const  vh_copy_before = vh_before; // tiefe Kopie (std::vector) zum Vergleich nach Rollback

    // alle Original-Keys MUESSEN real dereferenzierbar sein (REAL aus den Inserts gebaut).
    bool base_all_deref = true;
    for (std::uint64_t i = 0; i < kBase; ++i) {
        std::uint64_t v = 0;
        if (!vh_before.deref_value(i, &v)) {
            base_all_deref = false;
            break;
        }
    }
    tr(std::string{name} + ": (c) Pre-Save: alle Original-Keys real dereferenzierbar (echte Struktur)", base_all_deref);
    std::cout << "     Pre-Save real slot_count=" << vh_before.real_slot().slot_count() << "\n";

    rbk->tier_save_all(); // Warmup-Vor-Zustand kapseln (eager vh-Snapshot)
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i) (void)drv->tier_insert(i, i * 11u + 5u); // Warmup-Mutation
    // nach dem Warmup-Insert MUSS die reale Struktur die Warmup-Keys kennen (REAL mutiert, kein no-op).
    bool warm_in_vh = true;
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i) {
        std::uint64_t v = 0;
        if (!tier.value_handle_instance().deref_value(i, &v)) {
            warm_in_vh = false;
            break;
        }
    }
    tr(std::string{name} + ": (c) nach Warmup-Insert: Warmup-Keys in der REALEN Struktur (mutiert, kein no-op)",
       warm_in_vh);
    tr(std::string{name} + ": (c) nach Warmup-Insert: Struktur != Pre-Save-Stand (echte Mutation)",
       !(tier.value_handle_instance() == vh_copy_before));

    rbk->tier_rollback_all(); // exakt auf den save-Stand zurueck
    // (c) BIT-EXAKT: die zurueckgerollte Struktur == Pre-Save-Snapshot (operator==).
    tr(std::string{name} + ": (c) Memento: Struktur nach Rollback BIT-EXAKT wie vor save (operator==)",
       tier.value_handle_instance() == vh_copy_before);
    // (c) Original-Keys weiterhin dereferenzierbar …
    bool base_still_deref = true;
    for (std::uint64_t i = 0; i < kBase; ++i) {
        std::uint64_t v = 0;
        if (!tier.value_handle_instance().deref_value(i, &v)) {
            base_still_deref = false;
            break;
        }
    }
    tr(std::string{name} + ": (c) Memento: Original-Keys nach Rollback weiterhin dereferenzierbar", base_still_deref);
    // … und die Warmup-Keys sind NICHT mehr in der Struktur (kein Leck ueber den Rollback).
    std::uint64_t warm_back_gone = 0;
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i) {
        std::uint64_t v = 0;
        if (!tier.value_handle_instance().deref_value(i, &v)) ++warm_back_gone;
    }
    std::cout << "     Warmup-Keys nach Rollback wieder weg: " << warm_back_gone << "/" << kWarm << "\n";
    tr(std::string{name} + ": (c) Memento: Warmup-Keys nach Rollback NICHT mehr dereferenzierbar (kein Leck)",
       warm_back_gone == kWarm);
}

// (c)-Inline: Memento eines Inline-Tiers ist trivial leer-exakt (EmptyRealSlot, messneutral).
static void test_tier_memento_inline() {
    std::cout << "-- Tier-Memento (value_handle=Inline, M3-Pin messneutral) --\n";
    using Anatomy = an::SearchAlgorithmAnatomy<comp::HotComposition>; // HotComposition = InlineValueHandle
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  rbk  = dynamic_cast<an::IRollbackableTier*>(base);
    if (!drv || !rbk) {
        tr("(c)-Inline: Interface vorhanden", false);
        return;
    }
    for (std::uint64_t i = 0; i < 512; ++i) (void)drv->tier_insert(i, i + 1u);
    auto const vh_copy = tier.value_handle_instance();
    rbk->tier_save_all();
    for (std::uint64_t i = 512; i < 768; ++i) (void)drv->tier_insert(i, i + 1u);
    rbk->tier_rollback_all();
    // EmptyRealSlot bleibt ueber den ganzen Zyklus leer + bit-exakt (Inline traegt KEIN value_handle-Backing).
    tr("(c)-Inline: value_handle-Struktur bleibt leer (slot_count==0, messneutral)",
       tier.value_handle_instance().real_slot().slot_count() == 0);
    tr("(c)-Inline: Memento der leeren Inline-Struktur ist BIT-EXAKT (operator==)",
       tier.value_handle_instance() == vh_copy);
}

int main() {
    std::cout << "==== §4.3: REALER value_handle (Pool/Version/Chain-Deref gegen reale Slot-Struktur) ====\n";

    // (a)/(d): die 4 NICHT-inline-Strategien je direkt ueber die reale Slot-Struktur (Pool/Version/Chain).
    test_real_strategy<vh::ExternalPoolValueHandle>("ExternalPool", 2); // 1 abh. Deref → depth 2
    test_real_strategy<vh::ImmutableSharedRefValueHandle>("ImmutableSharedRef", 2);
    test_real_strategy<vh::VersionedPointerValueHandle>("VersionedPointer", 2); // MVCC-Tag-Strip + 1 Deref
    test_real_strategy<vh::ChainRefValueHandle>("ChainRef", 3);                 // 2 abh. Derefs → depth 3

    // (b): Inline (M3-Pin) unveraendert + messneutral.
    test_inline_unchanged();

    // (c): Memento bit-exakt ueber die Zwei-Phasen (ChainRef + ExternalPool als reale Strukturen; Inline trivial).
    test_tier_memento<vh::ChainRefValueHandle>("ChainRef");
    test_tier_memento<vh::ExternalPoolValueHandle>("ExternalPool");
    test_tier_memento_inline();

    std::cout << "==== §4.3 REALER value_handle: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
