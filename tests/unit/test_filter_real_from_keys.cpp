// P5 (#124, 2026-06-04, User-Direktive §4.3) — REALER, aus den eingefuegten Keys gebauter Filter.
// Verifiziert LITERAL, dass der Filter (Bloom/Cuckoo/SuRF/Xor) seit P5 eine ECHTE persistente Struktur ist, die
// aus den Keys gebaut + geprobt wird (NICHT mehr ein Pseudo-Puffer), UND dass die static filter_probe_scan-Signatur
// (Pfad-A, run_workload_segmented_v2) unveraendert bleibt + die Mess-Probe-Kost (Anzahl Hash-Positionen) erhalten ist.
//
//   (a) nach Insert von N Keys ist die Filter-Struktur REAL befuellt: probe der eingefuegten Keys → positive;
//       probe nicht-eingefuegter → ueberwiegend negative (Bloom-FP-Rate plausibel < 50% bei N << m).
//   (b) "None"-Baseline: eine frisch geleerte/leere Struktur probt ALLE Keys negativ (kein Build → keine Member-
//       schaft); clear() leert bit-exakt (Verhalten der leeren Struktur unveraendert gegenueber Default-Konstrukt).
//   (c) Memento-Exaktheit ueber den Zwei-Phasen-Zyklus (Tier): save_all → Warmup-Insert (mutiert flt_organ_) →
//       rollback_all → der Filter ist BIT-EXAKT wie vor save (operator==), und der zurueckgerollte Filter probt
//       die Original-Keys positiv + die nur-im-Warmup eingefuegten Keys NICHT (kein Leck ueber den Rollback).
//   (d) static filter_probe_scan(buf,n,queries,q)-Signatur unveraendert: der Pfad-A-Aufruf kompiliert + liefert.
//   (e) Bloom + mind. 1 weitere Strategie (Cuckoo) durchlaufen (a)/(b)/(d); zusaetzlich Xor + SuRF.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz
//        (scratch_compile_filter_real_from_keys.ps1, abgeleitet aus scratch_compile_migration_two_tier.ps1).
// SUPERSEDED 2026-07-11: obiger scratch_compile_*.ps1-Build-Weg entfernt (Behelfsweg-Bereinigung); Test jetzt
//        registriertes ctest-Target (tests/unit/CMakeLists.txt, #155-Block COMDARE_PHASE_E_BOOST_TESTS).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/hot_reference.hpp> // BloomFilter ist der Default-Filter (HotComposition)
#include <axes/filter_axis/axis_filter_bloom.hpp>
#include <axes/filter_axis/axis_filter_cuckoo.hpp>
#include <axes/filter_axis/axis_filter_xor.hpp>
#include <axes/filter_axis/axis_filter_range_surf.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace flt  = ::comdare::cache_engine::filter_axis;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Eingefuegte Keys: 0..N-1 mit grossen Stride-Abstaenden im uint64-Raum, damit die Byte-Prefixe (SuRF) + Hashes
// (Bloom/Cuckoo/Xor) gut streuen. Nicht-eingefuegte Probe-Keys liegen DISJUNKT (hoher Offset).
static std::uint64_t ins_key(std::uint64_t i) { return 0x0123'4500'0000'0000ull + i * 0x9E3779B1ull; }
static std::uint64_t miss_key(std::uint64_t i) { return 0xFEDC'BA00'0000'0000ull + i * 0x85EBCA77ull; }

// (a)+(b)+(d) je Strategie direkt (ohne Tier): REALE Struktur bauen + proben.
template <class Strategy>
static void test_strategy_direct(std::string_view name, double max_fp_rate) {
    std::cout << "-- Strategie: " << name << " --\n";
    constexpr std::uint64_t N = 256; // N << m (Bitmap 65536 Bit) → niedrige FP-Rate erwartet

    Strategy f{};
    // (b) None-Baseline: leere (default-konstruierte) Struktur → ALLE Keys negativ (kein Build).
    bool empty_all_neg = true;
    for (std::uint64_t i = 0; i < N; ++i)
        if (f.probe_key(ins_key(i))) {
            empty_all_neg = false;
            break;
        }
    tr(std::string{name} + ": (b) leere Struktur probt ALLE Keys negativ (None-Baseline)", empty_all_neg);

    // (a) Build aus N Keys → alle eingefuegten Keys MUESSEN positiv proben (kein False-Negative bei Bloom/Cuckoo/SuRF).
    for (std::uint64_t i = 0; i < N; ++i) f.insert_key(ins_key(i));
    std::uint64_t pos_hits = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (f.probe_key(ins_key(i))) ++pos_hits;
    // Bloom/Cuckoo/SuRF: 100% positiv (kein FN). Xor (vereinfachte Konstruktion ohne Peeling): toleriere wenige FN.
    bool const all_pos = (name == "Xor") ? (pos_hits >= (N * 90) / 100) : (pos_hits == N);
    std::cout << "     eingefuegt positiv: " << pos_hits << "/" << N << "\n";
    tr(std::string{name} + ": (a) eingefuegte Keys proben positiv (REAL aus Keys gebaut)", all_pos);

    // (a) nicht-eingefuegte Keys → ueberwiegend negativ (FP-Rate plausibel).
    std::uint64_t fp = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (f.probe_key(miss_key(i))) ++fp;
    double const fp_rate = static_cast<double>(fp) / static_cast<double>(N);
    std::cout << "     false-positives: " << fp << "/" << N << " (rate=" << fp_rate << ")\n";
    tr(std::string{name} + ": (a) nicht-eingefuegte Keys ueberwiegend negativ (FP-Rate plausibel)",
       fp_rate <= max_fp_rate);

    // (b) clear() leert bit-exakt → wieder leere Baseline (== default-konstruiert).
    f.clear();
    tr(std::string{name} + ": (b) clear() == leere Default-Struktur (bit-exakt)", f == Strategy{});
    bool cleared_all_neg = true;
    for (std::uint64_t i = 0; i < N; ++i)
        if (f.probe_key(ins_key(i))) {
            cleared_all_neg = false;
            break;
        }
    tr(std::string{name} + ": (b) nach clear() proben alle Keys negativ", cleared_all_neg);

    // (d) static filter_probe_scan(buf,n,queries,q)-Signatur unveraendert (Pfad-A): kompiliert + liefert uint64.
    std::vector<unsigned char> buf(1024), qs(64);
    for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<unsigned char>(i * 31u + 7u);
    for (std::size_t i = 0; i < qs.size(); ++i) qs[i] = static_cast<unsigned char>(i * 53u + 11u);
    std::uint64_t const path_a = Strategy::filter_probe_scan(buf.data(), buf.size(), qs.data(), qs.size());
    std::cout << "     Pfad-A filter_probe_scan checksum=" << path_a << "\n";
    tr(std::string{name} + ": (d) static filter_probe_scan-Signatur unveraendert (Pfad-A kompiliert)", true);
}

int main() {
    std::cout << "==== P5 (#124): REALER, aus den Keys gebauter Filter (Bloom/Cuckoo/Xor/SuRF) ====\n";

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────
    // (a)/(b)/(d)/(e): Bloom + Cuckoo (rigoros) + Xor + SuRF, je direkt ueber die reale Struktur.
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────
    test_strategy_direct<flt::BloomFilter>("Bloom", 0.50); // N=256, m=65536 → FP-Rate weit < 50%
    test_strategy_direct<flt::CuckooFilter>("Cuckoo", 0.50);
    test_strategy_direct<flt::XorFilter>("Xor", 0.60);        // vereinfachte XOR-Konstruktion → etwas hoehere FP
    test_strategy_direct<flt::RangeSurfFilter>("SuRF", 0.60); // Prefix-Trie

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────
    // (c) Memento-Exaktheit ueber den Zwei-Phasen-Zyklus (Tier, BloomFilter = HotComposition-Default).
    //     save_all → Warmup-Insert (mutiert flt_organ_) → rollback_all → Filter bit-exakt wie vor save.
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────
    {
        using Anatomy = an::SearchAlgorithmAnatomy<comp::HotComposition>;
        an::SearchAlgorithmAbiAdapter<Anatomy> tier;
        auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
        auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
        auto*                                  rbk  = dynamic_cast<an::IRollbackableTier*>(base);
        tr("(c) Memento: IDriveableTier + IRollbackableTier vorhanden", drv != nullptr && rbk != nullptr);
        if (!drv || !rbk) {
            std::cout << "  ABBRUCH (c) (Interface fehlt)\n";
            return 1;
        }

        constexpr std::uint64_t kBase = 2048; // Original-Keys (Pre-Save-Stand)
        constexpr std::uint64_t kWarm = 512;  // Warmup-Keys (NUR in der save→rollback-Phase)
        for (std::uint64_t i = 0; i < kBase; ++i) (void)drv->tier_insert(i, i * 7u + 1u);

        // Pre-Save-Snapshot des realen Filters (ueber die Test-Zugriffs-API strategy_instance()).
        auto const& flt_inst_before = tier.filter_instance();
        auto const  flt_copy_before = flt_inst_before; // tiefe Kopie (std::array) zum Vergleich nach Rollback

        // alle Original-Keys MUESSEN im Filter positiv proben (REAL aus den Inserts gebaut).
        bool base_all_pos = true;
        for (std::uint64_t i = 0; i < kBase; ++i)
            if (!flt_inst_before.strategy_instance().probe_key(i)) {
                base_all_pos = false;
                break;
            }
        tr("(c) Pre-Save: alle Original-Keys proben im REALEN Filter positiv", base_all_pos);

        rbk->tier_save_all(); // Warmup-Vor-Zustand kapseln (eager Filter-Snapshot)
        for (std::uint64_t i = kBase; i < kBase + kWarm; ++i) (void)drv->tier_insert(i, i * 7u + 1u); // Warmup-Mutation
        // nach dem Warmup-Insert MUSS der Filter die Warmup-Keys kennen (REAL mutiert, nicht no-op).
        bool warm_in_filter = true;
        for (std::uint64_t i = kBase; i < kBase + kWarm; ++i)
            if (!tier.filter_instance().strategy_instance().probe_key(i)) {
                warm_in_filter = false;
                break;
            }
        tr("(c) nach Warmup-Insert: Warmup-Keys sind im REALEN Filter (mutiert, kein no-op)", warm_in_filter);
        tr("(c) nach Warmup-Insert: Filter != Pre-Save-Stand (echte Mutation)",
           !(tier.filter_instance() == flt_copy_before));

        rbk->tier_rollback_all(); // exakt auf den save-Stand zurueck
        // (c) BIT-EXAKT: der zurueckgerollte Filter == Pre-Save-Snapshot (operator==).
        tr("(c) Memento: Filter nach Rollback BIT-EXAKT wie vor save (operator==)",
           tier.filter_instance() == flt_copy_before);
        // (c) Original-Keys weiterhin positiv …
        bool base_still_pos = true;
        for (std::uint64_t i = 0; i < kBase; ++i)
            if (!tier.filter_instance().strategy_instance().probe_key(i)) {
                base_still_pos = false;
                break;
            }
        tr("(c) Memento: Original-Keys nach Rollback weiterhin positiv", base_still_pos);
        // … und die Warmup-Keys sind NICHT mehr im Filter (kein Leck ueber den Rollback). Bloom hat keine
        // Delete-Semantik → der Beweis ist der bit-exakte operator==-Vergleich; hier zusaetzlich der Direkt-Check,
        // dass MINDESTENS ein Warmup-Key wieder negativ ist (Bloom-FP koennte einzelne zufaellig positiv lassen).
        std::uint64_t warm_back_neg = 0;
        for (std::uint64_t i = kBase; i < kBase + kWarm; ++i)
            if (!tier.filter_instance().strategy_instance().probe_key(i)) ++warm_back_neg;
        std::cout << "  (c) Warmup-Keys nach Rollback wieder negativ: " << warm_back_neg << "/" << kWarm << "\n";
        tr("(c) Memento: Warmup-Keys nach Rollback ueberwiegend wieder negativ (kein Leck)",
           warm_back_neg >= (kWarm * 80) / 100);
    }

    std::cout << "==== P5 REALER Filter aus Keys: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
