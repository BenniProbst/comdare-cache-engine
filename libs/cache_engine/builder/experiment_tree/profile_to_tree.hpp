#pragma once
// KF-9-Adapter (2026-06-02) — comdare_thesis_profile → AxisLevels für den Experiment-B+-Baum (Permutations-/Präfixbaum, kein textbook-B+-Baum — s. experiment_tree.hpp:2-10).
//
// Brückt das geparste Profil (comdare::builder::xml::ThesisProfile, KF-1) an den Baum-Kern (experiment_tree.hpp):
//   • Paper/Tier-Dimension (oben): Fanout = base_tiers (jeder = ein gepinntes Paper-Tupel).
//   • freigegebene permute_axes (mode.active_axes) → STATISCHE Ebenen (→ Binaries); cacheline → statische
//     Sub-Ebenen (line_size/alignment/sw_hint, compile-time); node_width → statische Sub-Ebene
//     (width_in_lines, compile-time; C2/FF2 2026-07-12, Muster cacheline); alloc_hw → statische Sub-Ebenen
//     (numa_node/page, compile-time; F-B 2026-07-12, Muster cacheline).
//   • runtime_dynamic (thread_count/hw_prefetcher) → DYNAMISCHE Ebenen (Laufzeit-for-Schleife, Algorithm_Resource_Control).
// Werte: explizit aus dem Profil; fehlen sie, expandiert die AxisRegistry (permutation_axes.xml) die volle Liste.
// Doc architecture/26. C++23, header-only.

#include "axis_path_serialization.hpp" // kCompositionAxisNames (17 Organ-Slots, Single-Source) — #1-Fix-Guard
#include "experiment_tree.hpp"
#include "xml_config_parser/xml_config_parser.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Achsen-Wertebereiche aus permutation_axes.xml (axis-ref → volle Werteliste).
using AxisRegistry = std::map<std::string, std::vector<std::string>>;

/// #1-Fix-Guard (9dim-G1 == Ketten-A1): ist `ref` eine der 17 Organ-Kompositions-Achsen (die EINZIGEN
/// binary_id-tragenden Achsen)? Single-Source ist kCompositionAxisNames (axis_path_serialization.hpp) — KEIN
/// dupliziertes Achsen-Verzeichnis. Die statischen Organ-Sub-Achsen (cacheline/node_width/alloc_hw) werden im
/// Aufrufer VOR diesem Punkt separat behandelt (eigene if-Zweige) und erreichen den Guard nicht.
[[nodiscard]] inline bool is_organ_composition_axis(std::string const& ref) {
    for (std::string_view a : kCompositionAxisNames)
        if (a == ref) return true;
    return false;
}

/// Baut die geordneten AxisLevels eines Modus aus dem Thesis-Profil.
[[nodiscard]] inline std::vector<AxisLevel> build_axis_levels(comdare::builder::xml::ThesisProfile const& tp,
                                                              std::string const&                          mode_name,
                                                              AxisRegistry const&                         registry) {
    std::vector<AxisLevel> levels;

    // 1. Paper/Tier-Dimension (oben).
    {
        AxisLevel tier;
        tier.axis      = "tier";
        tier.is_static = true;
        for (auto const& t : tp.base_tiers) tier.values.push_back(t.id);
        if (!tier.values.empty()) levels.push_back(std::move(tier));
    }

    // Modus auflösen (active_axes = freigegebene Achsen).
    comdare::builder::xml::ThesisMode const* mode = nullptr;
    for (auto const& m : tp.modes)
        if (m.name == mode_name) mode = &m;
    auto is_active = [&](std::string const& ref) -> bool {
        if (mode == nullptr) return true; // kein Modus → alle frei
        for (auto const& a : mode->active_axes)
            if (a == ref) return true;
        return false;
    };

    // BIDIREKTIONALITAET (block_id): jede statische Ebene traegt die Rueck-Referenz auf ihren AxisBlock.
    // Die Konvention ist `block_id == axis` -- so setzt sie der compile-time-Pfad (axis_reflect.hpp:42:
    // `/*block_id=*/std::string{axis}`), und genau so pruefen sie test_br1_subset, test_br1_full22_count
    // ("jeder Knoten block_id()==axis()") und test_harness_compile ("!d.block_id().empty()").
    // DER BEFUND: der PROFIL-Pfad hier liess das Feld an sieben Stellen leer (-Wmissing-field-initializers,
    // GCC und clang). Als Speicher-Warnung war das Rauschen -- std::string wird wertinitialisiert, es gibt
    // keinen unbestimmten Wert. Als FACHLOGIK war es ein Riss: profil-erzeugte Baeume verletzten eine
    // Zusicherung, die drei Tests fuehren -- die Tests bauen ihre Baeume aber ueber den compile-time-Pfad
    // und sahen den Bruch nie. GOLDEN-NEUTRAL, und das ist nachgesehen statt vermutet: block_id geht NICHT
    // in den Schluessel ein (StaticAxisNode::serialize() == `axis_ + "=" + value_`), also weder in die
    // binary_id noch in den golden-CRC.
    auto statische_ebene = [](std::string axis, std::vector<std::string> werte) {
        std::string block = axis; // Rueck-Referenz auf den AxisBlock
        return AxisLevel{std::move(axis), std::move(werte), /*is_static=*/true, /*variable=*/"", std::move(block)};
    };

    // 2. Freigegebene permute_axes → statische Ebenen.
    for (auto const& ax : tp.permute_axes) {
        if (!is_active(ax.ref)) continue;
        // A9b/P6, O-8 Schritt 11b -- DER VERBRAUCHER der expliziten Abwahl (declared_count).
        // Bis hierher war <axis active="false"> nur LESBAR (Parser) und TRANSPORTIERBAR
        // (validate_profile.hpp ResolverReport::declared_inactive), aber wirkungslos. Der Owner-Satz
        // "Die Achse wird per XML deaktiviert und das muss unterstuetzt sein" war damit nicht erfuellt:
        // eine Anwender-XML konnte eine Achse nur durch WEGLASSEN abwaehlen, und das kehrt den Kanon
        // um (Registry = ANGEBOT, Anwender-XML = ANZEIGE -- Abschnitt 27). Die Abwahl wirkt hier an
        // genau der Stelle, an der die Modus-Freigabe schon wirkt: die Achse erzeugt keine Ebene und
        // damit kein binary_id-Segment.
        // WARUM DAS BYTE-NEUTRAL BLEIBT: ein abwesendes Attribut ist true (Bestands-Profile
        // unveraendert), und KEIN Profil in beiden Baeumen waehlt eine Achse ab -- gemessen am
        // 27.07.2026 ueber beide Baeume: das Muster '<axis[^>]*active=' hat 0 Treffer, kein Profil-XML
        // traegt das Attribut ueberhaupt. Ein wirksames active="false" veraenderte die
        // Katalog-Kardinalitaet und damit den golden-CRC; das bleibt untersagt, und die
        // kNewGolden131072Crc64-Wache (source_catalog.hpp) ist genau der Schutz dagegen.
        if (!ax.active) continue;
        if (ax.ref == "cacheline") { // compile-time → statische Sub-Ebenen (Binary-Identität)
            if (!ax.line_sizes.empty()) levels.push_back(statische_ebene("cacheline.line_size", ax.line_sizes));
            if (!ax.alignments.empty()) levels.push_back(statische_ebene("cacheline.alignment", ax.alignments));
            if (!ax.sw_prefetch_hints.empty())
                levels.push_back(statische_ebene("cacheline.sw_hint", ax.sw_prefetch_hints));
            continue;
        }
        if (ax.ref == "node_width") { // C2/FF2: Knoten-Breite in Cache-Lines, compile-time → statische Sub-Ebene.
            // Exakt das cacheline-Muster: NUR wenn ein Profil die Achse deklariert UND aktiviert, entsteht eine
            // Ebene (Binary-Identität); ohne Profil-Aktivierung ändert sich KEINE binary_id.
            if (!ax.width_in_lines.empty())
                levels.push_back(statische_ebene("node_width.width_in_lines", ax.width_in_lines));
            continue;
        }
        if (ax.ref == "alloc_hw") { // F-B: NUMA/Page→allocator, compile-time → statische Sub-Ebenen.
            // Exakt das cacheline-/node_width-Muster: NUR wenn ein Profil die Achse deklariert UND aktiviert,
            // entstehen Ebenen (Binary-Identität); ohne Profil-Aktivierung ändert sich KEINE binary_id.
            if (!ax.alloc_numa_nodes.empty())
                levels.push_back(statische_ebene("alloc_hw.numa_node", ax.alloc_numa_nodes));
            if (!ax.alloc_pages.empty()) levels.push_back(statische_ebene("alloc_hw.page", ax.alloc_pages));
            continue;
        }
        // #1-Fix (9dim-G1 == Ketten-A1): STRUKTURELLER Organ-only-binary_id-Guard. Nur die 17 Organ-
        // Kompositions-Achsen (kCompositionAxisNames) duerfen ein statisches (= binary_id-serialisiertes) Level
        // erzeugen. Eine System-Achse (isa/simd_extension/page_type/telemetry) im permute_axes-Block darf die
        // binary_id NICHT verunreinigen — sie laeuft ueber die CEB-System-Achsen-Schicht (build_system_axis_levels,
        // +ext=/+target=-Sidecar), NICHT den 17-Slot-Organ-Pfad. Damit ist "Organ-only-binary_id" STRUKTUR statt
        // Autor-Konvention (m3v2/golden waren schon organ-rein → golden-neutral). Der lautstarke Reject-/Routing-
        // Pfad in validate_profile ist erledigt in S3 (resolve_axis_refs_against_trio), der auf diesem Guard aufsetzt.
        if (!is_organ_composition_axis(ax.ref)) continue;
        std::vector<std::string> vals = ax.values; // explizit?
        if (vals.empty()) {                        // sonst volle Liste aus dem Registry
            auto it = registry.find(ax.ref);
            if (it != registry.end()) vals = it->second;
        }
        if (!vals.empty()) levels.push_back(statische_ebene(ax.ref, std::move(vals)));
    }

    // 3. runtime_dynamic → dynamische Ebenen (Laufzeit-for-Schleife). DIE EINZIGE Quelle dieser Dimensionen
    //    (Inc2 dyn-Dim-Konsolidierung, 2026-06-18): vorher hingen profile_runner::profile_dynamic_dims dieselben
    //    Dimensionen ein ZWEITES Mal an (Doppelquelle). Jetzt emittiert build_axis_levels sie selbst — vollstaendig
    //    + mit den Mess-Pfad-Konventionen (concurrency.thread_count / prefetch.hw_prefetcher + Wiederholungs-Achse
    //    repetition.repetition_index, KF-10). Die binary_id wird davon NICHT beruehrt (is_static=false → der Baum
    //    filtert sie via static_filter() raus; Round-Trip bewiesen). variable/block_id = ComdareResourceControlV1-
    //    Feldname bzw. architektonische Ausnahme (repetition_index ist KEIN POD-Feld → set_field ignoriert es).
    if (!tp.thread_counts.empty())
        levels.push_back(AxisLevel{"concurrency", tp.thread_counts, false, "thread_count", "concurrency"});
    if (!tp.hw_prefetcher.empty())
        levels.push_back(AxisLevel{"prefetch", tp.hw_prefetcher, false, "hw_prefetcher", "prefetch"});
    if (!tp.prefetch_distances.empty())
        levels.push_back(AxisLevel{"prefetch", tp.prefetch_distances, false, "prefetch_distance", "prefetch"});
    if (!tp.pool_budgets_bytes.empty())
        levels.push_back(AxisLevel{"allocator", tp.pool_budgets_bytes, false, "pool_budget_bytes", "allocator"});
    if (!tp.batch_sizes.empty())
        levels.push_back(AxisLevel{"cache_traversal", tp.batch_sizes, false, "batch_size", "cache_traversal"});
    if (!tp.inline_thresholds_bytes.empty())
        levels.push_back(
            AxisLevel{"value_handle", tp.inline_thresholds_bytes, false, "inline_threshold_bytes", "value_handle"});
    // Wiederholungs-Achse (D, KF-10): immer vorhanden (Default 3, separat, nie interpoliert). repetitions<=0 → 1.
    {
        int const                reps = (tp.repetitions <= 0) ? 1 : tp.repetitions;
        std::vector<std::string> rep_vals;
        rep_vals.reserve(static_cast<std::size_t>(reps));
        for (int r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
        levels.push_back(AxisLevel{"repetition", std::move(rep_vals), false, "repetition_index", "repetition"});
    }

    return levels;
}

} // namespace comdare::cache_engine::builder::experiment
