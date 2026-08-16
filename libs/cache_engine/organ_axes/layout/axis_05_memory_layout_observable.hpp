#pragma once
// V42 L-74c Composition-Driver — ObservableMemoryLayout<Strategy>: ObservableAxis-Huelle um eine
// memory_layout-Strategie (axis_05). Analog axis_11_telemetry_observable.hpp + observable_composed_search.hpp:
// die Strategie selbst (CacheLineAlignedMemoryLayout etc.) traegt zwar die verhaltens-tragende Methode
// scan_field_sum(), hat aber KEIN statistics()/snapshot_t (kein ObservableAxis). Die Mess-Mechanik gehoert
// daher in diese Huelle, die scan_field_sum durchreicht UND dabei trackt.
//
// @topic memory_layout @achse 05 @saeule 2 (Per-Achsen-Observer) @task V42-L-74c
//
// **Achsen-Semantik (treu):** Die Layout-Achse misst Cache-Effekte des Zugriffs-Patterns (AoS strided vs
// SoA contiguous, axis_05_memory_layout_cache_line_aligned.hpp). Die Counter-Metriken (scan_count/
// records_scanned/field_bytes_read/cache_lines_touched) machen die Scan-Aktivitaet observable; der reine
// Latenz-Unterschied der Patterns bleibt der Wall-Clock-Messung vorbehalten (Pfad B, Hybrid-Messmodell).
// `cache_lines_touched` ist eine record_size-basierte AoS-Strided-Schaetzung (n * ceil(record_size/line)).
//
// Gating exakt nach Praezedenz: snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: scan_field_sum = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false.

#include "axis_05_memory_layout_strategy_base.hpp" // RepresentationKind (2026-07-06: Job 214061 — TU-Reihenfolge-Glueck beendet)
#include "concepts/axis_05_memory_layout_concept.hpp"
#include <organ_axes/cacheline/cacheline_line_bytes.hpp> // B14-NB3: Linienzaehlung NUR ueber die cacheline-Unterachse
#include <anatomy/organ_location.hpp>              // INC-A #6: per-Organ-Codegen-Lokation (header_include)
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::layout {

/// ABI-taugliches Memory-Layout-Snapshot (standard_layout + trivially_copyable).
struct MemoryLayoutSnapshot {
    std::uint64_t scan_count          = 0; ///< Anzahl scan_field_sum-Aufrufe
    std::uint64_t records_scanned     = 0; ///< kumulierte Datensatz-Zahl ueber alle Scans
    std::uint64_t field_bytes_read    = 0; ///< P-MD1: REAL belegte Nutzbytes je Layout (n * useful, LAYOUT-ABHAENGIG)
    std::uint64_t cache_lines_touched = 0; ///< P-MD1: REAL beruehrte Lines je Layout (ceil(n*span/line),
                                           ///< LAYOUT-ABHAENGIG). B14-NB3: `line` ist die Groesse der
                                           ///< cacheline-Unterachse (Default 64), kein Literal mehr.
    std::uint64_t last_checksum = 0;       ///< letztes scan_field_sum-Ergebnis (Korrektheits-Anker)
    /// B14-NB4 (2026-08-06) -- die EINHEIT von cache_lines_touched, mitgefuehrt statt vorausgesetzt.
    /// BEFUND, der dieses Feld erzwingt (Codex + Opus unabhaengig, Lead-Entscheid Weg (ii)): seit B14-NB3
    /// zaehlt cache_lines_touched in Linien DER ACHSE, nicht mehr in 64-B-Linien. Der einzige aktive
    /// Verbraucher (ObserverSnapshotSystemAxis, measurement/system_axis.hpp) bildet daraus die CLU als
    /// field_bytes/(cache_lines*line) -- und hatte dort das Literal 64 stehen. Unter KF-6 haette das
    /// 8/16/33/66 % statt durchgaengig ~16 % ergeben, obwohl die WAHREN beruehrten Bytes konstant sind.
    /// Der Verbraucher sitzt HINTER der Modul-ABI (er liest einen POD aus einer geladenen .so) und kann
    /// die Line-Groesse dort NICHT rekonstruieren -- sie ist eine Compile-Zeit-Eigenschaft der DLL.
    /// Deshalb reist sie ab jetzt NEBEN dem Zaehler: wer den Zaehler liest, bekommt seine Einheit mit.
    /// 0 = KEINE eindeutige Einheit (nichts gemessen ODER zwei Produzenten mit verschiedenen Linien) ->
    /// der Verbraucher meldet ehrlich source-unavailable statt eine Zahl zu erfinden (fail-closed).
    std::uint64_t line_bytes = 0;

    [[nodiscard]] bool operator==(MemoryLayoutSnapshot const&) const noexcept = default;
};

/// ObservableAxis-Huelle: memory_layout-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) -> direkt als Anatomie-Member haltbar.
template <class Strategy>
    requires concepts::MemoryLayoutStrategy<Strategy>
class ObservableMemoryLayout {
public:
    using strategy_type = Strategy;
    // topic_tag durchgereicht → die Huelle erfuellt MemoryLayoutComponent/MemoryLayoutStrategy und ist damit
    // als L in ComposedStore<N,L,A> einsetzbar (node_type-Achse, anatomy_execution_context.hpp:46 / abi_adapter.hpp:393).
    using topic_tag = typename Strategy::topic_tag;

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): Strategie-Inspektion durchgereicht (composition_registry/axis_path_serialization
    // rufen C::memory_layout::name()).
    [[nodiscard]] static constexpr std::size_t cache_line_size() noexcept { return Strategy::cache_line_size(); }
    // B14-NB2 (2026-08-06) -- FEHLENDE WEITERLEITUNG, gefunden beim Heilen der KF-6-Auflage (D):
    // MemoryLayoutStrategyBase bietet cacheline_subaxis_line_bytes() (die PERMUTIERBARE Line-Groesse der
    // cacheline-Unterachse), diese Huelle reichte sie aber NICHT durch. Der Registry-Eintrag der Achse ist
    // ObservableMemoryLayout<Strategy> -- line_bytes_of<> sah also nie die Strategie, sondern fiel auf den
    // Achsen-Default kDefaultLineBytes (64) zurueck. Heute ist das folgenlos (die Strategien tragen den
    // Default), mit der KF-6-NTTP-Belegung waere es ein STILLER Fehler gewesen: die abgeleitete Puffer-
    // Dimensionierung in abi_adapter.hpp haette weiter 64 gerechnet, waehrend der Scan bei 256 laeuft --
    // also genau der OOB, den die Ableitung verhindern soll, nur unsichtbar. Muster + Begruendung:
    // reference_observable_wrapper_must_forward_concept_members (Observable-Wrapper muessen Concept-Member
    // forwarden). Byte-neutral: Strategy liefert heute denselben Wert wie der Fallback.
    // NICHT VERWECHSELN mit cache_line_size() darueber -- das ist der INTRINSISCHE Design-Deskriptor der
    // Layout-Strategie (aos_strict=1, cache_line_aligned=64), nicht die permutierbare Hardware-Line.
    [[nodiscard]] static constexpr std::size_t cacheline_subaxis_line_bytes() noexcept
        requires requires { Strategy::cacheline_subaxis_line_bytes(); }
    {
        return Strategy::cacheline_subaxis_line_bytes();
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    static constexpr std::string_view               algo_version =
        Strategy::algo_version; // #50 Caching: algo_version-Weiterleitung (Organ-Provenienz, reflect_versions)
    // INC-A #6: per-Organ-Codegen-Lokation. Wrapper-Typ = ObservableMemoryLayout-Huelle (Enabled-Eintrag =
    // ObservableMemoryLayout<Strategy>); Header = diese Huellen-Datei.
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::layout::ObservableMemoryLayout",
                                  "organ_axes/layout/axis_05_memory_layout_observable.hpp");
    // P-MD1-ERDUNG (#167): die REALE Repraesentation der gewrappten Strategie transparent durchreichen, damit die
    // Huelle als L im LayoutAwareChunkedStore (z.B. via ArtComposition/observable_composed_search) denselben
    // physischen Store-Footprint erzeugt wie die nackte Strategie.
    [[nodiscard]] static constexpr ::comdare::cache_engine::layout::RepresentationKind representation_kind() noexcept {
        return Strategy::representation_kind();
    }
    [[nodiscard]] static constexpr std::size_t block_width() noexcept
        requires requires { Strategy::block_width(); }
    {
        return Strategy::block_width();
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept
        requires requires { Strategy::family_name(); }
    {
        return Strategy::family_name();
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept
        requires requires { Strategy::flag_suffix(); }
    {
        return Strategy::flag_suffix();
    }
    // AxisBase-Eigenschaft (Default "original") transparent durchgereicht: test_v41_compositions ruft
    // C::memory_layout::get_compiler() auf der gewrappten Achse. SFINAE-sicher (Methode existiert nur,
    // wenn die Strategie sie traegt).
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); }
    {
        return Strategy::get_compiler();
    }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode wird unveraendert durchgereicht,
    /// damit die Huelle als memory_layout-Slot die bestehenden static-Aufrufer NICHT bricht
    /// (abi_adapter.hpp:215 `MemLayout::scan_field_sum`, axis_04_node_type_composed_store.hpp:90 `L::scan_field_sum`).
    /// Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return Strategy::scan_field_sum(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): delegiert an die Strategie + trackt. Der Observer-
    /// Treiber ruft dies → die Layout-Scan-Aktivitaet wird observable (statistics()). Getrennt von der static-
    /// Variante, weil die bestehenden Aufrufer static bleiben muessen.
    [[nodiscard]] std::uint64_t observe_scan(unsigned char const* buf, std::size_t n,
                                             std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::scan_field_sum(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.scan_count;
        stats_.records_scanned += static_cast<std::uint64_t>(n);
        // GENERISCHER Footprint (Raw-Buffer-Pfad ohne realen Store, z.B. abi_adapter/Anatomie-Test): voller
        // Record-Stride. Die LAYOUT-DISTINKTE, REALE CLU kommt aus observe_real_footprint() (P-MD1-ERDUNG #167),
        // das vom LayoutAwareChunkedStore mit dem ECHTEN, representation-spezifischen Key-Scan-Footprint gefuettert
        // wird — NICHT mehr aus einem entkoppelten record_useful_bytes/record_line_span-Deskriptor.
        // B14-NB3 (2026-08-06) -- GESCHWISTER-BEFUND des Lead-Befundes, beim Heilen der Scan-Seite gefunden:
        // hier stand `constexpr std::size_t kLineBytes = 64u;`. Das ist dieselbe Klasse wie das Literal in
        // CacheLineAlignedMemoryLayout::scan_field_sum, nur einen Zaehler weiter: cache_lines_touched ist eine
        // MESSGROESSE (sie geht ueber die Observer-Statistik in die CSV), und mit einer permutierten
        // line_size haette sie weiter in 64-B-Einheiten gezaehlt -- die Achse waere in der CLU unsichtbar
        // geblieben. Die Line kommt deshalb aus der Unterachse der GEWRAPPTEN Strategie.
        // BYTE-NEUTRAL: alle fuenf Strategien stehen am Default-NTTP CacheLineConfig{} == B64.
        // (Der REALE, representation-genaue Footprint kommt weiterhin aus observe_real_footprint(); der
        // Store speist ihn seit P-CACHELINE-LITERAL bereits achsen-treu.)
        // B14-NB4 (Codex-Befund "die Scan-Seite ist fail-closed, die OBSERVER-Seite nicht"): dieselbe
        // Disziplin wie detail::layout_scan_stride_bytes im abi_adapter. Ohne den Zwang faellt eine
        // Strategie/Huelle, die cacheline_subaxis_line_bytes() nicht beantwortet, in line_bytes_of<>
        // STILL auf den Achsen-Default 64 zurueck -- und zaehlt dann eine MESSGROESSE in einer Einheit,
        // die mit ihrem eigenen Scan-Stride nichts zu tun hat. Genau dieses stille Auseinanderlaufen ist
        // die Klasse, die B14 heilt; sie darf auf der Mess-Seite nicht durch die Hintertuer bleiben.
        static_assert(::comdare::cache_engine::cacheline::CacheLineLineBytesAware<Strategy>,
                      "ObservableMemoryLayout<Strategy>: die gewrappte Strategie beantwortet "
                      "cacheline_subaxis_line_bytes() nicht. line_bytes_of<> faellt damit STILL auf den "
                      "Achsen-Default 64 zurueck und cache_lines_touched zaehlte in einer Einheit, die "
                      "der Scan gar nicht benutzt. Strategie von MemoryLayoutStrategyBase ableiten oder "
                      "den Zugriff forwarden (reference_observable_wrapper_must_forward_concept_members).");
        constexpr std::size_t kKeyBytes  = sizeof(std::uint64_t);
        constexpr std::size_t kLineBytes = ::comdare::cache_engine::cacheline::line_bytes_of<Strategy>();
        static_assert(kLineBytes > 0u, "cacheline-Unterachse liefert Line-Groesse 0 -- Linienzaehlung unmoeglich");
        std::size_t const   rs            = (record_size == 0) ? (2u * kKeyBytes) : record_size;
        std::uint64_t const touched_bytes = static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(rs);
        stats_.field_bytes_read += static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(kKeyBytes);
        stats_.cache_lines_touched += (touched_bytes + (kLineBytes - 1u)) / kLineBytes;
        note_line_unit_(static_cast<std::uint64_t>(kLineBytes)); // B14-NB4: Einheit reist mit dem Zaehler
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

    /// observe_real_footprint (P-MD1-ERDUNG #167): der REALE, vom LayoutAwareChunkedStore representation-genau
    /// vermessene Key-Scan-Footprint EINES Chunks. `field_bytes` = real beruehrte NUTZ-Key-Bytes, `cache_lines` =
    /// real beruehrte DISTINKTE Linien aus dem echten Byte-Layout der Repraesentation, `line_bytes` = die
    /// Groesse GENAU DIESER Linien (die cacheline-Unterachse des Stores). CLU = field_bytes/(cache_lines*
    /// line_bytes) folgt damit aus dem ECHTEN Store-Footprint, nicht aus einem Modell. Der Store ruft dies
    /// pro Chunk; die Werte werden akkumuliert (`records` = Chunk-Record-Zahl als Anker).
    ///
    /// B14-NB4 -- WARUM line_bytes ein PARAMETER ist und nicht line_bytes_of<Strategy>(): der Zaehler
    /// `cache_lines` entsteht im STORE (LayoutAwareChunkedStore<N,L,A>::key_scan_footprint_), nicht hier.
    /// Wer den Zaehler bildet, muss auch seine Einheit nennen -- sonst ist die Gleichheit "Store-L ==
    /// Organ-Strategy" eine ANNAHME, und die Einheit im Snapshot waere ein Selbstbeleg der Huelle statt
    /// eine Aussage ueber die Zahlen, die wirklich hereinkamen (der tautologische Selbstbeleg, den der
    /// Codex-Review an der NB3-Fassung geruegt hat: "ein Wrapper, der 32 meldet, waehrend seine innere
    /// Strategie mit 256 scannt, besteht den Audit").
    /// EHRLICHE REICHWEITE (nicht mehr behaupten, als der Code haelt): note_line_unit_ vergleicht die
    /// Store-Quelle und die Organ-Quelle NUR DANN gegeneinander, wenn BEIDE Pfade in DENSELBEN Snapshot
    /// schreiben. Auf dem produktiven ABI-Pfad laeuft je Organ genau EIN Pfad -- dort ist der Effekt nicht
    /// "Divergenz-Erkennung", sondern "die Einheit stammt von dem, der gezaehlt hat". Das ist die
    /// eigentliche Zusage; die Divergenz-Erkennung ist der Zusatznutzen im Misch-Fall (Wache unten).
    [[nodiscard]] std::uint64_t observe_real_footprint(std::uint64_t checksum, std::size_t records,
                                                       std::uint64_t field_bytes, std::uint64_t cache_lines,
                                                       std::uint64_t line_bytes) noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.scan_count;
        stats_.records_scanned += static_cast<std::uint64_t>(records);
        stats_.field_bytes_read += field_bytes;
        stats_.cache_lines_touched += cache_lines;
        note_line_unit_(line_bytes);
        stats_.last_checksum = checksum;
#else
        (void)records;
        (void)field_bytes;
        (void)cache_lines;
        (void)line_bytes;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = MemoryLayoutSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_            = {};
        einheit_gemischt_ = false; // die Vergiftung gehoert zum Zaehler -- sie endet mit ihm, nicht frueher
    }

private:
    /// B14-NB4 -- die EINE Stelle, an der die Einheit des Linien-Zaehlers gebucht wird.
    /// REGEL (fail-closed, bewusst ohne Ausnahme): solange alle Beitraege eines Snapshots in DERSELBEN
    /// Linien-Groesse zaehlen, traegt der Snapshot diese Groesse. Melden zwei Beitraege verschiedene
    /// Groessen (Store-Achse != Organ-Achse, oder ein 0-Wert), ist die Einheit des akkumulierten
    /// Zaehlers nicht mehr definiert -- dann wird sie auf 0 vergiftet, und der Verbraucher meldet
    /// ehrlich source-unavailable statt eine Prozentzahl aus zwei Einheiten zu mischen.
    ///
    /// DIE VERGIFTUNG IST KLEBRIG, und das ist der Kern dieser Funktion (Befund am uebernommenen WIP,
    /// B14-NB4): `cache_lines_touched` AKKUMULIERT ueber alle Beitraege. Ist einmal in zwei Einheiten
    /// addiert worden, ist die SUMME dauerhaft einheitenlos -- auch dann noch, wenn danach wieder nur
    /// Beitraege der ersten Einheit kommen. Ohne das Flag haette die Folge (64, 128, 64) am Ende wieder
    /// "64" gemeldet und die Mischung in der Summe unsichtbar gemacht; das waere exakt die Klasse
    /// "Zaehler behauptet mehr als gedeckt", nur eine Ebene tiefer als der Befund, der B14 blockiert hat.
    /// Zurueckgesetzt wird die Vergiftung deshalb NUR von reset() -- dort, wo auch der Zaehler faellt.
    void note_line_unit_(std::uint64_t line_bytes) noexcept {
        if (line_bytes == 0u || (stats_.line_bytes != 0u && stats_.line_bytes != line_bytes)) einheit_gemischt_ = true;
        stats_.line_bytes = einheit_gemischt_ ? 0u : line_bytes;
    }

    snapshot_t stats_{};
    /// true, sobald mindestens ZWEI verschiedene Linien-Groessen (oder eine 0) in denselben Zaehler
    /// geflossen sind. Kein Snapshot-Feld: er beschreibt die HISTORIE der Akkumulation, nicht ihren
    /// Endstand -- und MemoryLayoutSnapshot bleibt so der reine uint64-POD, den die ABI-Naht braucht.
    bool einheit_gemischt_ = false;
#endif
};

} // namespace comdare::cache_engine::layout
