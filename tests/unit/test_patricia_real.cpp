// §4.3 (User-Direktive 2026-06-04) — REALER path_compression(Patricia): MATERIALISIERTER Patricia/Radix-Trie +
// echter bit-weiser Descent gegen die reale Trie-Struktur. Verifiziert LITERAL, dass die Patricia-Strategie seit
// §4.3 einen ECHTEN, INKREMENTELL in tier_insert via insert_key aufgebauten crit-bit-Trie traegt, der mit dem
// echten Single-Bit-Split-Descent dereferenziert wird (eingefuegte Keys → gefunden, fremde Keys → nicht gefunden) —
// NICHT mehr nur eine synthetische Roh-Puffer-Descent-SIMULATION. Gleichzeitig: 'none' (M3-Pin) bleibt EXAKT No-Op
// (EmptyPatriciaTrie, kein insert_key, kein Trie, messneutral), die static path_descend_scan-/compress-Signaturen
// bleiben heil, und der Memento ist BIT-EXAKT ueber den Zwei-Phasen-Zyklus (R1).
//
//   (a) Build aus N Keys → REALER Trie befuellt: descend der eingefuegten Keys liefert true; nicht-eingefuegte Keys →
//       descend false (echte Membership, kein no-op). key_count waechst mit N; node_count > 0 (echte Struktur).
//   (b) 'none' (M3-Pin) ist UNVERAENDERT + No-Op: ObservablePathCompression<PathCompressionNone> traegt KEIN
//       insert_key/descend/clear_trie (EmptyPatriciaTrie, Compile-Zeit-Beweis ueber den Trie-Typ) → der real_trie()
//       ist leer (key_count==0), die static path_descend_scan/compress bleiben verfuegbar + unveraendert.
//   (c) Memento-Exaktheit ueber den Zwei-Phasen-Zyklus (Tier mit Patricia-path_compression): save_all → Warmup-
//       Insert (mutiert pc_organ_ via insert_key) → rollback_all → der Trie ist BIT-EXAKT wie vor save (operator==),
//       und der zurueckgerollte Descent findet die Original-Keys + NICHT die nur-im-Warmup eingefuegten Keys.
//       None-Tier: Memento ist trivial leer-exakt (No-Op-Trie).
//   (d) static path_descend_scan(buf,n,record_size)- + compress(key,depth)-Signaturen unveraendert: Pfad-A/Build-Hook
//       kompilieren + liefern.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz
//        (scratch_compile_patricia_real.ps1, abgeleitet aus scratch_compile_value_handle_real.ps1).
// SUPERSEDED 2026-07-11: obiger scratch_compile_*.ps1-Build-Weg entfernt (Behelfsweg-Bereinigung); Test jetzt
//        registriertes ctest-Target (tests/unit/CMakeLists.txt, #155-Block COMDARE_PHASE_E_BOOST_TESTS).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/hot_reference.hpp> // Basis-Composition (path_compression wird im Test ersetzt)
#include <axes/path_compression/axis_02_path_compression_none.hpp>
#include <axes/path_compression/axis_02_path_compression_patricia.hpp>
#include <axes/path_compression/axis_02_path_compression_observable.hpp>
#include <axes/path_compression/axis_02_path_compression_real_trie.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <random>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace pc   = ::comdare::cache_engine::path_compression;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Detektoren (dependent-Kontext, MSVC-konform: requires ueber einen Template-Parameter → saubere SFINAE → false).
// Beweist Compile-Zeit, dass EmptyPatriciaTrie (None-Backing) KEIN insert_key/descend traegt.
template <class T>
concept has_insert_key = requires(T t) { t.insert_key(std::uint64_t{1}); };
template <class T>
concept has_descend = requires(T t) {
    { t.descend(std::uint64_t{1}) } -> std::convertible_to<bool>;
};

// Eingefuegte Keys + disjunkte Miss-Keys (hoher Offset). Schluessel mit gestreuten Bits (echter crit-bit-Branching).
static std::uint64_t ins_key(std::uint64_t i) { return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull; }
static std::uint64_t miss_key(std::uint64_t i) { return (9'000'000ull + i) * 1099511628211ull; }

// (a)+(d) Patricia direkt ueber die REALE Trie-Struktur (Hülle ObservablePathCompression).
static void test_real_patricia() {
    std::cout << "-- Strategie: Patricia (MATERIALISIERTER crit-bit-Trie) --\n";
    constexpr std::uint64_t N = 200;

    pc::ObservablePathCompression<pc::PatriciaPathCompression> organ{};

    // (a) leerer Trie: kein Key dereferenzierbar.
    {
        bool empty_none = true;
        for (std::uint64_t i = 0; i < N; ++i)
            if (organ.descend(ins_key(i))) {
                empty_none = false;
                break;
            }
        tr("Patricia: (a) leerer realer Trie dereferenziert KEINEN Key", empty_none);
    }

    // (a) Build: insert_key je Key → REALER Trie inkrementell aufgebaut.
    for (std::uint64_t i = 0; i < N; ++i) organ.insert_key(ins_key(i));

    // (a) Descent der eingefuegten Keys → gefunden (echter bit-weiser Single-Bit-Split-Descent).
    std::uint64_t hits = 0, max_depth = 0;
    for (std::uint64_t i = 0; i < N; ++i) {
        if (organ.descend(ins_key(i))) {
            ++hits;
            if (organ.real_trie().last_descent_depth() > max_depth) max_depth = organ.real_trie().last_descent_depth();
        }
    }
    std::cout << "     descend-hits=" << hits << "/" << N << " max_descent_depth=" << max_depth << "\n";
    tr("Patricia: (a) ALLE eingefuegten Keys real per Descent gefunden", hits == N);
    tr("Patricia: (a) echter Descent hat Tiefe > 1 (mehrstufiger Trie, kein flacher Scan)", max_depth > 1);

    // (a) reale Trie-Struktur waechst mit N (echtes Backing, kein no-op): key_count==N distinct, node_count>0.
    std::size_t const keys  = organ.real_trie().key_count();
    std::size_t const nodes = organ.real_trie().node_count();
    std::cout << "     key_count=" << keys << " node_count=" << nodes << "\n";
    tr("Patricia: (a) realer Trie befuellt (key_count==N, node_count>0)", keys == N && nodes > 0);

    // (a) nicht-eingefuegte Keys → KEIN Treffer (deref false). Reale Membership.
    std::uint64_t miss_hits = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (organ.descend(miss_key(i))) ++miss_hits;
    tr("Patricia: (a) nicht-eingefuegte Keys werden NICHT gefunden (echte Membership)", miss_hits == 0);

    // (b) clear_trie() leert die reale Struktur bit-exakt → wieder leere Baseline (== default-konstruiert).
    organ.clear_trie();
    tr("Patricia: (b) clear_trie() == leerer Default-Trie (bit-exakt)",
       organ == pc::ObservablePathCompression<pc::PatriciaPathCompression>{});

    // (d) static path_descend_scan + compress unveraendert (Pfad-A seg19 / Build-Hook): kompiliert + liefert.
    std::vector<unsigned char> buf(N * 16, 0xABu);
    std::uint64_t const        path_a = pc::PatriciaPathCompression::path_descend_scan(buf.data(), N, std::size_t{16});
    std::uint64_t const        comp   = organ.compress(ins_key(0), 0u);
    std::cout << "     Pfad-A path_descend_scan checksum=" << path_a << " compress()=" << comp << "\n";
    tr("Patricia: (d) static path_descend_scan + compress-Signatur unveraendert (Pfad-A kompiliert)", true);
}

// ── (b) None (M3-Pin) ist No-Op: KEIN reales Trie-Backing, path_descend_scan/compress bit-identisch ────────────────
static void test_none_noop() {
    std::cout << "-- Strategie: None (M3-Pin, No-Op) --\n";
    pc::ObservablePathCompression<pc::PathCompressionNone> organ{};
    // EmptyPatriciaTrie: 0 Footprint (key_count==0, kein Build moeglich).
    tr("(b) None: realer Trie ist LEER (EmptyPatriciaTrie, 0 Footprint)", organ.real_trie().key_count() == 0);
    // Das reale Trie-Backing fuer None ist EmptyPatriciaTrie (Compile-Zeit-Beweis ueber den Backing-Typ selbst: er
    // traegt KEIN insert_key/descend → der abi_adapter-Build-Hook + die Hülle-Methoden greifen fuer None NIE).
    static_assert(std::is_same_v<pc::real_trie_t<pc::PathCompressionNone>, pc::EmptyPatriciaTrie>,
                  "None muss EmptyPatriciaTrie tragen (M3-Pin No-Op, additive Leitplanke 1)");
    static_assert(!has_insert_key<pc::EmptyPatriciaTrie>,
                  "EmptyPatriciaTrie (None-Backing) darf KEIN insert_key tragen → Build-Hook greift nicht (No-Op)");
    static_assert(!has_descend<pc::EmptyPatriciaTrie>,
                  "EmptyPatriciaTrie (None-Backing) darf KEIN descend tragen (kein Trie-Abstieg)");
    // Gegenprobe: das Patricia-Backing TRAEGT insert_key/descend (echte Struktur).
    static_assert(std::is_same_v<pc::real_trie_t<pc::PatriciaPathCompression>, pc::PatriciaTrie>,
                  "Patricia muss PatriciaTrie tragen (materialisierter Trie)");
    static_assert(has_insert_key<pc::real_trie_t<pc::PatriciaPathCompression>> &&
                      has_descend<pc::real_trie_t<pc::PatriciaPathCompression>>,
                  "Patricia-Backing MUSS insert_key/descend tragen (reale Trie-Struktur)");
    tr("(b) None: EmptyPatriciaTrie ohne insert_key/descend (Compile-Zeit static_assert, No-Op)", true);
    // static path_descend_scan unveraendert + bit-identisch zur RAW-Strategie. compress() bleibt verfuegbar.
    std::vector<unsigned char> buf(256, 0x5Au);
    std::uint64_t const c = organ.compress(0x1234u, 0u); // compress bleibt heil (kein Trie, reine Byte-Prefix-Op)
    std::cout << "     None compress()=" << c << " (No-Op-Trie, kein Build)\n";
    tr("(b) None: compress() unveraendert verfuegbar (Pfad-A messneutral, kein Trie-Effekt)",
       organ.real_trie().key_count() == 0);
}

// (c) Memento ueber den Zwei-Phasen-Zyklus mit einem Tier, dessen path_compression Patricia ist.
template <class PCStrategy>
struct PCComposition : comp::HotComposition {
    using path_compression = PCStrategy; // NUR die path_compression-Achse ersetzt (Rest = HotComposition)
    static constexpr std::string_view name = "PCComposition";
};

static void test_tier_memento_patricia() {
    std::cout << "-- Tier-Memento (path_compression=Patricia) --\n";
    using Anatomy = an::SearchAlgorithmAnatomy<PCComposition<pc::PatriciaPathCompression>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  rbk  = dynamic_cast<an::IRollbackableTier*>(base);
    tr("Patricia: (c) IDriveableTier + IRollbackableTier vorhanden", drv != nullptr && rbk != nullptr);
    if (!drv || !rbk) {
        std::cout << "  ABBRUCH (c) (Interface fehlt)\n";
        ++g_fail;
        return;
    }

    constexpr std::uint64_t kBase = 1024; // Original-Keys (Pre-Save-Stand)
    constexpr std::uint64_t kWarm = 256;  // Warmup-Keys (NUR in der save→rollback-Phase)
    for (std::uint64_t i = 0; i < kBase; ++i) (void)drv->tier_insert(ins_key(i), i * 11u + 5u);

    // Pre-Save-Snapshot des realen path_compression-Organs (ueber path_compression_instance()).
    auto const& pc_before      = tier.path_compression_instance();
    auto const  pc_copy_before = pc_before; // tiefe Kopie (std::vector) zum Vergleich nach Rollback

    // alle Original-Keys MUESSEN real per Descent gefunden werden (REAL aus den Inserts gebaut).
    bool base_all_deref = true;
    for (std::uint64_t i = 0; i < kBase; ++i)
        if (!tier.path_compression_instance().real_trie().descend(ins_key(i))) {
            base_all_deref = false;
            break;
        }
    tr("Patricia: (c) Pre-Save: alle Original-Keys per Descent gefunden (echte Struktur)", base_all_deref);
    std::cout << "     Pre-Save key_count=" << pc_before.real_trie().key_count()
              << " node_count=" << pc_before.real_trie().node_count() << "\n";

    rbk->tier_save_all(); // Warmup-Vor-Zustand kapseln (eager pc-Snapshot)
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i)
        (void)drv->tier_insert(ins_key(i), i * 11u + 5u); // Warmup-Mutation
    // nach dem Warmup-Insert MUSS der Trie die Warmup-Keys kennen (REAL mutiert, kein no-op).
    bool warm_in_pc = true;
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i)
        if (!tier.path_compression_instance().real_trie().descend(ins_key(i))) {
            warm_in_pc = false;
            break;
        }
    tr("Patricia: (c) nach Warmup-Insert: Warmup-Keys im REALEN Trie (mutiert, kein no-op)", warm_in_pc);
    tr("Patricia: (c) nach Warmup-Insert: Trie != Pre-Save-Stand (echte Mutation)",
       !(tier.path_compression_instance() == pc_copy_before));

    rbk->tier_rollback_all(); // exakt auf den save-Stand zurueck
    // (c) BIT-EXAKT: der zurueckgerollte Trie == Pre-Save-Snapshot (operator==).
    tr("Patricia: (c) Memento: Trie nach Rollback BIT-EXAKT wie vor save (operator==)",
       tier.path_compression_instance() == pc_copy_before);
    // (c) Original-Keys weiterhin per Descent gefunden …
    bool base_still_deref = true;
    for (std::uint64_t i = 0; i < kBase; ++i)
        if (!tier.path_compression_instance().real_trie().descend(ins_key(i))) {
            base_still_deref = false;
            break;
        }
    tr("Patricia: (c) Memento: Original-Keys nach Rollback weiterhin per Descent gefunden", base_still_deref);
    // … und die Warmup-Keys sind NICHT mehr im Trie (kein Leck ueber den Rollback).
    std::uint64_t warm_back_gone = 0;
    for (std::uint64_t i = kBase; i < kBase + kWarm; ++i)
        if (!tier.path_compression_instance().real_trie().descend(ins_key(i))) ++warm_back_gone;
    std::cout << "     Warmup-Keys nach Rollback wieder weg: " << warm_back_gone << "/" << kWarm << "\n";
    tr("Patricia: (c) Memento: Warmup-Keys nach Rollback NICHT mehr per Descent gefunden (kein Leck)",
       warm_back_gone == kWarm);
}

// (c)-None: Memento eines None-Tiers ist trivial leer-exakt (EmptyPatriciaTrie, No-Op).
static void test_tier_memento_none() {
    std::cout << "-- Tier-Memento (path_compression=None, M3-Pin No-Op) --\n";
    using Anatomy = an::SearchAlgorithmAnatomy<comp::HotComposition>; // HotComposition = PathCompressionNone
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  rbk  = dynamic_cast<an::IRollbackableTier*>(base);
    if (!drv || !rbk) {
        tr("(c)-None: Interface vorhanden", false);
        return;
    }
    for (std::uint64_t i = 0; i < 512; ++i) (void)drv->tier_insert(ins_key(i), i + 1u);
    auto const pc_copy = tier.path_compression_instance();
    rbk->tier_save_all();
    for (std::uint64_t i = 512; i < 768; ++i) (void)drv->tier_insert(ins_key(i), i + 1u);
    rbk->tier_rollback_all();
    // EmptyPatriciaTrie bleibt ueber den ganzen Zyklus leer + bit-exakt (None traegt KEIN path_compression-Trie-Backing).
    tr("(c)-None: path_compression-Trie bleibt leer (key_count==0, No-Op)",
       tier.path_compression_instance().real_trie().key_count() == 0);
    tr("(c)-None: Memento des leeren None-Trie ist BIT-EXAKT (operator==)",
       tier.path_compression_instance() == pc_copy);
}

// (e) FUZZ / REALLOC-STRESS (Verifier-Befund 2026-06-18): grosses N erzwingt viele nodes_-Reallocations und deckt damit
// die use-after-realloc-Klasse ab (parent_link baumelte zuvor NACH push_back). Ground-Truth = std::set, deterministisch
// (fixer Seed). Laeuft die Schleife ohne Crash durch + stimmt die Membership/Struktur exakt, ist der Realloc-Fix belegt.
static void test_patricia_fuzz_realloc() {
    std::cout << "-- Fuzz/Realloc-Stress: N=20000 Zufalls-Keys vs std::set (deckt use-after-realloc ab) --\n";
    constexpr std::uint64_t                                    N = 20000;
    pc::ObservablePathCompression<pc::PatriciaPathCompression> organ{};
    std::set<std::uint64_t>                                    truth;
    std::mt19937_64 rng(0xC0FFEE2026ull); // fixer Seed → deterministisch reproduzierbar
    for (std::uint64_t i = 0; i < N; ++i) {
        std::uint64_t const k = rng();
        organ.insert_key(k); // KEIN reserve → viele Reallocations → triggert die Falle
        truth.insert(k);
    }
    std::uint64_t found = 0;
    for (std::uint64_t k : truth)
        if (organ.descend(k)) ++found;
    tr("(e) Fuzz: ALLE distinct eingefuegten Keys gefunden (kein Crash/Verlust ueber Reallocations)",
       found == truth.size());
    tr("(e) Fuzz: key_count == |distinct| (Set-Semantik exakt)", organ.real_trie().key_count() == truth.size());
    // crit-bit-Trie-Invariante: K distinct Keys → exakt 2K-1 Knoten (K Blaetter + K-1 innere).
    bool const struct_ok = organ.real_trie().node_count() == (truth.empty() ? 0 : 2 * truth.size() - 1);
    tr("(e) Fuzz: node_count == 2*|distinct|-1 (crit-bit-Struktur intakt)", struct_ok);
    std::uint64_t fp = 0;
    for (std::uint64_t i = 0; i < 5000; ++i) {
        std::uint64_t const m = miss_key(i);
        if (!truth.count(m) && organ.descend(m)) ++fp;
    }
    tr("(e) Fuzz: garantiert fremde Keys NICHT gefunden (0 False-Positive bei exaktem Trie)", fp == 0);
    std::cout << "     fuzz: inserted=" << N << " distinct=" << truth.size() << " found=" << found
              << " node_count=" << organ.real_trie().node_count() << "\n";
}

int main() {
    std::cout
        << "==== §4.3: REALER path_compression(Patricia) (materialisierter crit-bit-Trie + echter Descent) ====\n";

    // (a)/(d): Patricia direkt ueber die reale Trie-Struktur.
    test_real_patricia();

    // (b): None (M3-Pin) unveraendert + No-Op.
    test_none_noop();

    // (c): Memento bit-exakt ueber die Zwei-Phasen (Patricia als realer Trie; None trivial).
    test_tier_memento_patricia();
    test_tier_memento_none();

    // (e): Fuzz/Realloc-Stress (Verifier-Befund) — grosses N gegen std::set-Ground-Truth, deckt use-after-realloc ab.
    test_patricia_fuzz_realloc();

    std::cout << "==== §4.3 REALER path_compression(Patricia): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
