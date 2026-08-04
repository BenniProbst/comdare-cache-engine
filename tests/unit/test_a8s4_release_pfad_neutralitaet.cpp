// A8-S4 (2026-08-04) -- RELEASE-PFAD-NEUTRALITAETS-BEWEIS der Konstitutiv-Matrix (Gattungs-Vertrag).
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.3 + 5-S4
// Matrix:  docs/architecture/20260804-a8_s4_konstitutiv_matrix_gattungs_vertrag.md
//
// ZU BEWEISENDE AUSSAGE (Gate-Pflicht der Scheibe S4): die BEOBACHTENDEN Kopplungen des Gattungs-Kerns
// liegen im Release-Build (COMDARE_MEASUREMENT_ON aus) NICHT im Hot-Path. Der Beweis wird DREIFACH
// gefuehrt -- Quelltext, ABI-Objekt, Laufzeit:
//
//   TEIL 1  PRAEPROZESSOR-PFAD-WACHE am Quelltext von anatomy/abi_adapter.hpp:
//           Der Praeprozessor-Zustand wird nachgebaut (Stack aus #if/#ifdef/#else/#endif). GEGATED
//           heisst: innerhalb eines `#if COMDARE_MEASUREMENT_ON`- ODER eines
//           `#ifdef COMDARE_CE_ENABLE_STATISTICS`-Blocks -- BEIDE sind im Release-Build aus, weil
//           die Wurzel-CMakeLists STATISTICS zwingend abschaltet, sobald MEASUREMENT_MODE aus ist.
//           (a) JEDE beobachtende Organ-Beruehrung liegt in einem solchen Block.
//           (b) Die Umkehrung, scharf: es gibt AUSSERHALB der Gates KEINE Organ-Beruehrung ausser
//               den DREI deklarierten Laufzeit-Steuer-Settern (IResourceControllableTier -- einmal je
//               Einstellung, nicht je Op). Diese Wache ist die eigentliche Drift-Bremse: sie faellt,
//               sobald irgendwer eine Mess-Kopplung aus ihrem Gate schiebt.
//           (c) Die fuenf Gattungs-Funktions-Koepfe und die konstitutiven Aufrufe liegen UNGEGATED.
//
//   TEIL 2  ABI-OBJEKT-BEWEIS: diese TU ist selbst release-kompiliert (die Mess-Defines sind unten
//           per #undef abgeschaltet, exakt wie COMDARE_RELEASE_MODE=ON). Der Adapter traegt dann
//           NUR IDriveableTier -- alle Mess-Sub-Interfaces sind per dynamic_cast nachweislich NICHT
//           vorhanden (kein Observer-, kein Memento-, kein Scan-, kein Migrations-vtable-Slot).
//
//   TEIL 3  LAUFZEIT + WALLCLOCK-KONTRAST: dieselbe Referenz-Last wie in
//           test_a8s4_konstitutiv_kette.cpp (4096 tier_insert + 4096 tier_lookup, gleiche Key-Folge)
//           laeuft OHNE jede Messung korrekt durch -- die konstitutive Kette allein traegt die
//           Gattung. Die Wallclock wird literal ausgegeben; der Kontrast zur MESSUNG-AN-Seite ist
//           damit dokumentiert (beide Zahlen stehen im Scheiben-Report).
//
// KEIN neuer Mess-Pfad, keine CSV-Beruehrung, keine ABI-Flaeche (Auflagen 1/3/10 der Scheibe).

// Release-Praeparation VOR jedem Include: exakt der Zustand von COMDARE_RELEASE_MODE=ON
// (Wurzel-CMakeLists: MEASUREMENT_MODE=OFF erzwingt COMDARE_CE_ENABLE_STATISTICS=OFF; EXPERIMENT_MODE
// verlangt MEASUREMENT und faellt damit ebenfalls). Der #undef ist der PORTABLE Weg (kein -U/-D-Kunstgriff
// in CMake, keine Compiler-Abhaengigkeit) und wirkt fuer die GESAMTE Include-Kette dieser TU.
#undef COMDARE_MEASUREMENT_ON
#undef COMDARE_CE_ENABLE_STATISTICS
#undef COMDARE_EXPERIMENT_MODE_ON

#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
#error "A8-S4: diese TU MUSS release-kompiliert sein (COMDARE_MEASUREMENT_ON aus) -- sonst beweist sie nichts."
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
#error "A8-S4: COMDARE_CE_ENABLE_STATISTICS muss im Release-Pfad aus sein (CMake erzwingt das)."
#endif

#include <builder/codegen/all_axes_umbrella.hpp>
#include <anatomy/abi_adapter.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/scannable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef COMDARE_A8S4_ABI_ADAPTER_SRC
#error "A8-S4: COMDARE_A8S4_ABI_ADAPTER_SRC (Pfad auf anatomy/abi_adapter.hpp) muss gesetzt sein."
#endif

namespace comdare::cache_engine::compositions {
// Inhaltsgleich zur Prueffall-Komposition der MESSUNG-AN-Seite (test_a8s4_konstitutiv_kette.cpp),
// eigener Name, damit beide TUs unabhaengig bleiben. Store-traversierbar => T4/T5/T6 tragen die
// FLACHE konstitutive Store-Kette.
struct A8S4ReleaseComposition {
    using search_algo      = traversal::axis_03a_search_algo::LinearScanSearchAlgo;
    using cache_traversal  = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping          = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type        = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;
    using memory_layout    = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;
    using allocator     = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch      = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency   = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization = serialization::axis_10_serialization::ObservableSerialization<
        serialization::axis_10_serialization::RawBinarySerialization>;
    using telemetry = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;
    using value_handle                         = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                                  = hardware::axis_09_isa::Amd64Isa;
    using index_organization                   = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch                          = io::axis_io::InMemoryOnly;
    using migration_policy                     = migration::axis_migration::NoMigration;
    using filter                               = filter::axis_filter::BloomFilter;
    using queuing_q1                           = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2                           = queuing::axis_q2_queuing::LazyFlush;
    using persistence_target                   = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
    static constexpr std::string_view paper_id = "A8-S4 (Release-Pfad-Neutralitaet)";
    static constexpr std::string_view paper_title =
        "A8-S4 Konstitutiv-Matrix -- Release-Pfad-Neutralitaets-Beweis (Messung AUS)";
    static constexpr std::string_view name = "A8S4ReleaseComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::A8S4ReleaseComposition",
                                        "tests/unit/test_a8s4_release_pfad_neutralitaet.cpp");
};
} // namespace comdare::cache_engine::compositions

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

static int  g_fail = 0;
static void chk(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ---------------------------------------------------------------------------
// TEIL 1 -- Praeprozessor-Pfad-Wache
// ---------------------------------------------------------------------------

/// Eine Quelltext-Zeile mit ihrem Gate-Zustand.
struct GatedLine {
    std::size_t nr    = 0;     ///< 1-basierte Zeilennummer (fuer den literalen Beleg)
    bool        gated = false; ///< innerhalb #if COMDARE_MEASUREMENT_ON oder #ifdef COMDARE_CE_ENABLE_STATISTICS
    std::string code;          ///< Zeile OHNE //-Kommentar-Schwanz
};

[[nodiscard]] static std::string trim_links(std::string const& s) {
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

/// Schneidet den //-Kommentar-Schwanz ab -- sonst zaehlten Kommentar-Erwaehnungen der Organ-Namen
/// als Aufrufstellen (die Datei ist prosa-reich; das ist der haeufigste Fehl-Treffer).
[[nodiscard]] static std::string ohne_kommentar(std::string const& s) {
    auto const p = s.find("//");
    return (p == std::string::npos) ? s : s.substr(0, p);
}

/// Baut die Gate-Karte der Datei nach. Bewusst KEIN voller Praeprozessor: die Datei nutzt genau die
/// zwei Gate-Formen, die hier erkannt werden; jede andere Direktive schiebt nur den Stack.
[[nodiscard]] static std::vector<GatedLine> gate_karte(std::string const& pfad, bool& ok) {
    std::vector<GatedLine> out;
    std::ifstream          in(pfad);
    ok = in.good();
    if (!ok) return out;
    std::vector<int> stack; // 1 = dieser Block ist ein Release-Gate, 0 = neutral
    std::string      zeile;
    std::size_t      nr = 0;
    while (std::getline(in, zeile)) {
        ++nr;
        std::string const t           = trim_links(zeile);
        bool const        offen_gated = [&] {
            for (int f : stack)
                if (f == 1) return true;
            return false;
        }();
        if (t.rfind("#if", 0) == 0 || t.rfind("# if", 0) == 0) {
            std::string bed = trim_links(ohne_kommentar(t));
            // Bedingungs-Text hinter der Direktive isolieren.
            std::size_t sp = bed.find(' ');
            std::string rest;
            if (sp != std::string::npos) rest = trim_links(bed.substr(sp + 1));
            while (!rest.empty() && (rest.back() == ' ' || rest.back() == '\t' || rest.back() == '\r')) rest.pop_back();
            bool const ist_gate = (bed.rfind("#if ", 0) == 0 && rest == "COMDARE_MEASUREMENT_ON") ||
                                  (bed.rfind("#ifdef ", 0) == 0 && rest == "COMDARE_CE_ENABLE_STATISTICS") ||
                                  (bed.rfind("#ifdef ", 0) == 0 && rest == "COMDARE_MEASUREMENT_ON");
            stack.push_back(ist_gate ? 1 : 0);
            out.push_back(GatedLine{nr, offen_gated, ""});
            continue;
        }
        if (t.rfind("#else", 0) == 0 || t.rfind("#elif", 0) == 0) {
            if (!stack.empty()) stack.back() = 0; // der else-Zweig ist der Release-Zweig
            out.push_back(GatedLine{nr, offen_gated, ""});
            continue;
        }
        if (t.rfind("#endif", 0) == 0) {
            if (!stack.empty()) stack.pop_back();
            out.push_back(GatedLine{nr, offen_gated, ""});
            continue;
        }
        if (t.rfind("//", 0) == 0 || t.rfind("*", 0) == 0 || t.rfind("/*", 0) == 0) {
            out.push_back(GatedLine{nr, offen_gated, ""}); // reine Kommentar-Zeile: kein Code
            continue;
        }
        out.push_back(GatedLine{nr, offen_gated, ohne_kommentar(zeile)});
    }
    return out;
}

/// Erste Zeilennummer, in der `tok` als CODE vorkommt; `gated_soll` filtert den Gate-Zustand.
[[nodiscard]] static std::size_t erste_zeile(std::vector<GatedLine> const& k, std::string_view tok, bool gated_soll) {
    for (auto const& g : k)
        if (g.gated == gated_soll && !g.code.empty() && g.code.find(tok) != std::string::npos) return g.nr;
    return 0;
}

/// Zahl der CODE-Vorkommen von `tok` mit dem geforderten Gate-Zustand.
[[nodiscard]] static std::size_t zahl_vorkommen(std::vector<GatedLine> const& k, std::string_view tok,
                                                bool gated_soll) {
    std::size_t n = 0;
    for (auto const& g : k)
        if (g.gated == gated_soll && !g.code.empty() && g.code.find(tok) != std::string::npos) ++n;
    return n;
}

// Die BEOBACHTENDEN Kopplungen des Gattungs-Kerns + die Observer-Fill-Routen, je Achse mindestens
// eine Aufrufstelle. Alle MUESSEN gegated sein (Release-Pfad tot).
struct Beobachtend {
    char const* achse;
    char const* token;
};
static constexpr Beobachtend kBeobachtend[] = {
    {"T1 cache_traversal", "ct_organ_.register_entry("},
    {"T1 cache_traversal", "ct_organ_.resolve("},
    {"T2 mapping", "map_organ_.register_slot("},
    {"T2 mapping", "map_organ_.resolve_offset("},
    {"T3 path_compression", "pc_organ_.compress("},
    {"T3 path_compression", "pc_organ_.insert_key("},
    {"T4 node_type (Mess-Organ)", "store_observe_node_type("},
    {"T5 memory_layout (Mess-Organ)", "store_observe_layout("},
    {"T6 allocator (Stats-Route)", "store_allocator_statistics()"},
    {"T7 prefetch", "pf_organ_.observe_prefetch_descent("},
    {"T8 concurrency", "cc_organ_.observe_critical_section()"},
    {"T9 serialization", "store_observe_serialization("},
    {"T10 value_handle", "vh_organ_.store_value("},
    {"T10 value_handle", "store_observe_value_handle("},
    {"T11 index_organization", "store_observe_index_org("},
    {"T12 io_dispatch", "store_observe_io_dispatch("},
    {"T13 migration_policy", "store_observe_migration("},
    {"T14 filter", "flt_organ_.insert_key("},
    {"T14 filter", "flt_organ_.clear_filter("},
    {"T15 queuing_q1", "queuing_q1_organ_.put("},
    {"T15 queuing_q1", "queuing_q1_organ_.get()"},
    {"T16 queuing_q2", "queuing_q2_organ_.should_flush("},
    {"T16 queuing_q2", "queuing_q2_organ_.on_flush_complete()"},
    {"T17 persistence_target", "pt_organ_.observe_writeback("},
};

// Die KONSTITUTIVEN Aufrufe des Gattungs-Kerns -- sie MUESSEN ungegated sein (Release-Hot-Path).
static constexpr char const* kKonstitutiv[] = {
    "container_algorithm_.insert(", "container_algorithm_.lookup(",          "container_algorithm_.erase(",
    "container_algorithm_.clear()", "container_algorithm_.occupied_count()",
};

// Die fuenf Gattungs-Funktions-Koepfe -- ebenfalls ungegated (der Antrieb ist IMMER einkompiliert).
static constexpr char const* kGattungsKoepfe[] = {
    "bool tier_insert(", "bool tier_lookup(", "bool tier_erase(", "void tier_clear()", "tier_size() const",
};

// Die EINZIGEN erlaubten ungegateten Organ-Beruehrungen: die drei Laufzeit-Steuer-Setter des
// IResourceControllableTier (einmal je dynamischer Einstellung, NICHT je Op). Sie sind deklarierte
// Steuer-Naht, kein Mess-Pfad -- die Matrix fuehrt sie als eigene Zeile.
static constexpr char const* kErlaubteSteuerSetter[] = {
    "cc_organ_.set_runtime_thread_count",
    "pf_organ_.set_runtime_distance",
    "vh_organ_.set_runtime_inline_threshold",
};

/// Sammelt alle ungegateten `*_organ_.`-Beruehrungen, die NICHT zu den drei Steuer-Settern gehoeren.
[[nodiscard]] static std::vector<std::string> fremde_ungegatete_organ_beruehrungen(std::vector<GatedLine> const& k) {
    std::vector<std::string> treffer;
    for (auto const& g : k) {
        if (g.gated || g.code.empty()) continue;
        std::size_t p = 0;
        while ((p = g.code.find("_organ_.", p)) != std::string::npos) {
            // Organ-Name nach links, Methode nach rechts einsammeln.
            std::size_t l = p;
            while (l > 0) {
                char const c = g.code[l - 1];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    --l;
                else
                    break;
            }
            std::size_t r = p + 8; // hinter "_organ_."
            while (r < g.code.size()) {
                char const c = g.code[r];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    ++r;
                else
                    break;
            }
            std::string const fund    = g.code.substr(l, r - l);
            bool              erlaubt = false;
            for (auto const* e : kErlaubteSteuerSetter)
                if (fund == e) erlaubt = true;
            if (!erlaubt) treffer.push_back(std::to_string(g.nr) + ": " + fund);
            p = r;
        }
    }
    return treffer;
}

// ---------------------------------------------------------------------------
// TEIL 3 -- Referenz-Last, identisch zur MESSUNG-AN-Seite
// ---------------------------------------------------------------------------
static constexpr std::uint64_t kN = 4096;

int main() {
    std::cout << "==== A8-S4: Release-Pfad-Neutralitaets-Beweis (MESSUNG AUS) ====\n";

    // ---- TEIL 1 --------------------------------------------------------------------------
    bool       lesbar = false;
    auto const karte  = gate_karte(COMDARE_A8S4_ABI_ADAPTER_SRC, lesbar);
    chk("T1 abi_adapter.hpp lesbar (Praeprozessor-Pfad-Wache)", lesbar && !karte.empty());
    if (lesbar && !karte.empty()) {
        std::size_t gated_zeilen = 0;
        for (auto const& g : karte)
            if (g.gated) ++gated_zeilen;
        std::cout << "    Quelle: " << karte.size() << " Zeilen, davon " << gated_zeilen
                  << " im Release-Gate (MEASUREMENT_ON / CE_ENABLE_STATISTICS)\n";

        // SELBSTBEWEIS der Wache (sonst waere jedes gruene Ergebnis unten wertlos): die Gate-Karte MUSS
        // beide Zustaende unterscheiden koennen. Referenz-UNGEGATED = ein Steuer-Setter der immer
        // einkompilierten IResourceControllableTier-Naht; Referenz-GEGATED = eine Pfad-B-Observer-Op.
        chk("T1(0) Selbstbeweis: bekannt-UNGEGATED wird als ungegated erkannt",
            erste_zeile(karte, "cc_organ_.set_runtime_thread_count", false) != 0 &&
                erste_zeile(karte, "cc_organ_.set_runtime_thread_count", true) == 0);
        chk("T1(0) Selbstbeweis: bekannt-GEGATED wird als gegated erkannt",
            erste_zeile(karte, "cc_organ_.observe_critical_section()", true) != 0 &&
                erste_zeile(karte, "cc_organ_.observe_critical_section()", false) == 0);

        std::size_t beob_ok = 0, beob_leck = 0;
        for (auto const& b : kBeobachtend) {
            std::size_t const gz  = erste_zeile(karte, b.token, true);
            std::size_t const ugz = erste_zeile(karte, b.token, false);
            if (gz != 0 && ugz == 0) {
                ++beob_ok;
            } else {
                ++beob_leck;
                std::cout << "    [LECK] " << b.achse << " '" << b.token << "' ungegated in Zeile " << ugz << "\n";
            }
        }
        chk("T1(a) alle " + std::to_string(sizeof(kBeobachtend) / sizeof(kBeobachtend[0])) +
                " beobachtenden Kopplungen liegen AUSSCHLIESSLICH im Release-Gate (" + std::to_string(beob_ok) +
                " ok / " + std::to_string(beob_leck) + " Leck)",
            beob_leck == 0);

        auto const fremd = fremde_ungegatete_organ_beruehrungen(karte);
        for (auto const& f : fremd) std::cout << "    [FREMD] ungegatete Organ-Beruehrung " << f << "\n";
        chk("T1(b) ausserhalb der Gates GENAU die 3 deklarierten Steuer-Setter, sonst keine Organ-Beruehrung",
            fremd.empty());

        bool konst_ok = true;
        for (auto const* t : kKonstitutiv) {
            std::size_t const n = zahl_vorkommen(karte, t, false);
            if (n == 0) {
                konst_ok = false;
                std::cout << "    [FEHLT] konstitutiver Aufruf '" << t << "' NICHT ungegated gefunden\n";
            }
        }
        chk("T1(c) alle 5 konstitutiven container_algorithm_-Aufrufe liegen ungegated (Release-Hot-Path)", konst_ok);

        bool koepfe_ok = true;
        for (auto const* t : kGattungsKoepfe) {
            if (erste_zeile(karte, t, false) == 0) {
                koepfe_ok = false;
                std::cout << "    [FEHLT] Gattungs-Funktions-Kopf '" << t << "' nicht ungegated\n";
            }
        }
        chk("T1(c) alle 5 Gattungs-Funktions-Koepfe sind ungegated (Antrieb IMMER einkompiliert)", koepfe_ok);
    }

    // ---- TEIL 2: ABI-Objekt-Beweis -------------------------------------------------------
    using Anatomy = an::SearchAlgorithmAnatomy<comp::A8S4ReleaseComposition>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;
    Adapter tier;
    auto*   base = static_cast<an::IAnatomyBase*>(&tier);

    auto* drv = dynamic_cast<an::IDriveableTier*>(base);
    chk("T2 IDriveableTier VORHANDEN (die Gattung faehrt ohne jede Messung)", drv != nullptr);
    chk("T2 IObservableTier ABWESEND (kein Observer-vtable-Slot im Release)",
        dynamic_cast<an::IObservableTier*>(base) == nullptr);
    chk("T2 IMeasurableWorkload ABWESEND (kein Pfad-A im Release)",
        dynamic_cast<an::IMeasurableWorkload*>(base) == nullptr);
    chk("T2 IRollbackableTier ABWESEND (kein memento_all im Release)",
        dynamic_cast<an::IRollbackableTier*>(base) == nullptr);
    chk("T2 IScannableTier ABWESEND (kein Range-Scan im Release)", dynamic_cast<an::IScannableTier*>(base) == nullptr);
    chk("T2 IMigratableTier ABWESEND (kein 2-Ebenen-Move im Release)",
        dynamic_cast<an::IMigratableTier*>(base) == nullptr);

    if (drv == nullptr) {
        std::cout << "==== A8-S4: ABBRUCH ====\n";
        return 1;
    }

    // ---- TEIL 3: Referenz-Last + Wallclock -----------------------------------------------
    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);
    std::uint64_t treffer = 0;
    for (std::uint64_t i = 0; i < kN; ++i) {
        std::uint64_t v = 0;
        if (drv->tier_lookup(i, &v) && v == i * 7u + 1u) ++treffer;
    }
    auto const t1 = std::chrono::steady_clock::now();
    auto const drive_ns =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

    chk("T3 tier_size == kN (konstitutive Kette traegt die Gattung allein)", drv->tier_size() == kN);
    chk("T3 alle kN Lookups treffen mit korrektem Wert", treffer == kN);
    chk("T3 tier_erase wirkt", drv->tier_erase(7u) && drv->tier_size() == kN - 1u);
    drv->tier_clear();
    chk("T3 tier_clear leert den konstitutiven Speicher", drv->tier_size() == 0u);

    std::cout << "    A8-S4 REFERENZ-LAST       : " << kN << " tier_insert + " << kN << " tier_lookup\n";
    std::cout << "    A8-S4 WALLCLOCK RELEASE   : " << drive_ns << " ns\n";
    std::cout << "    (Kontrast: die MESSUNG-AN-Zahl derselben Last gibt test_a8s4_konstitutiv_kette aus)\n";

    std::cout << "==== A8-S4: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
