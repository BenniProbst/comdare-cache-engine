#pragma once
// progress_delta.hpp -- Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal, 2026-07-19). Der Up-Channel-POD +
// die Delta-Logik der Dock-Naht, herausgeloest in die BUILDER-Schicht (Machart-Vorbild: die No-Op-Naht-Typen
// von artifact_transport/artifact_cache.hpp). GRUND (Layering): der achsen-blinde Iterator
// (cache_engine_builder_iterator.hpp) darf NICHT aufwaerts in profile_facade/planner inkludieren -> die
// Fortschritts-Naht-Typen liegen hier, IM builder-Namespace, und der Iterator inkludiert nur diesen Sibling-Leaf.
// Der planner (experiment_dock_payload.hpp) inkludiert diesen Header und aliast die Typen in seinen Namespace
// (planner -> builder = korrekte Include-Richtung).
//
// SEMANTIK: Erste Meldung eines Fensters = Voll-Konfiguration (alle Achsen gelistet), danach mixed-radix-minimale
// Deltas in StaticBinaryView-Ordnung; done=true genau einmal am Fensterende (= §38.b-Fertig-Signal). KEIN
// Mess-Daten-Rueckfluss. LEAF-Header: haengt NUR an stdlib (kein xml_reader, kein topics, kein experiment_tree).
// Header-only, C++23, ASCII-only.

#include <cstddef>
#include <functional>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// EINE geaenderte Achse einer Fortschritts-Meldung: Ebenen-Index d (StaticBinaryView-Ordnung) + neue mixed-radix-
/// Ziffer (Varianten-Index der Ebene d). REINE Indizes (kein Mess-Daten-Rueckfluss).
struct ProgressAxisChange {
    std::size_t axis_index    = 0; // Ebenen-Index d (0 = hoechstwertig, wie StaticBinaryView::operator[])
    std::size_t variant_index = 0; // mixed-radix-Ziffer tuple[d]

    [[nodiscard]] bool operator==(ProgressAxisChange const&) const = default;
};

/// EINE Fortschritts-Meldung. Erste Meldung eines Fensters: changed listet ALLE Achsen (Voll-Konfiguration).
/// Danach: nur die Achsen, deren Varianten-Ziffer sich zur vorigen Meldung aenderte (mixed-radix-minimal). done=true
/// = das §38.b-Fertig-Signal am Fensterende (genau EINMAL, changed leer). cursor = fenster-relativer Perm-Index.
struct ProgressDelta {
    std::size_t                     cursor = 0;
    std::vector<ProgressAxisChange> changed;
    bool                            done = false;

    [[nodiscard]] bool operator==(ProgressDelta const&) const = default;
};

/// Injektions-Naht (No-Op-Default = leere std::function -> byte-neutral). Muster EXAKT wie CachePushFn/
/// MeasurementSinkFn (artifact_transport): der Iterator ruft sie SYNCHRON an der Per-Binary-Naht + einmal am
/// Fensterende. Leer (Default) => No-Op => golden/CI byte-identisch (Anti-Phantom).
using ProgressSinkFn = std::function<void(ProgressDelta const&)>;

/// progress_delta_between -- EIN Delta zwischen der zuletzt gemeldeten Konfiguration `prev` und der aktuellen `cur`
/// (beides mixed-radix-Ziffernfolgen je Ebene, StaticBinaryView-Ordnung). prev leer => Voll-Konfiguration (alle
/// Achsen); sonst nur die geaenderten Ebenen (mixed-radix-minimal). cursor = fenster-relativer Perm-Index. Diese
/// EINE Primitive nutzen sowohl der freistehende compute_progress_deltas (Test) als auch der Iterator (Feuerung).
[[nodiscard]] inline ProgressDelta progress_delta_between(std::vector<std::size_t> const& prev,
                                                          std::vector<std::size_t> const& cur, std::size_t cursor) {
    ProgressDelta d;
    d.cursor = cursor;
    d.done   = false;
    for (std::size_t ax = 0; ax < cur.size(); ++ax)
        if (prev.empty() || ax >= prev.size() || prev[ax] != cur[ax]) d.changed.push_back({ax, cur[ax]});
    return d;
}

/// compute_progress_deltas -- die volle Delta-Folge fuer eine GEORDNETE Konfig-Liste (je Konfig eine mixed-radix-
/// Ziffernfolge in StaticBinaryView-Ordnung) + genau EIN done=true-Delta am Ende (§38.b-Fertig-Signal). Freistehend,
/// iterator-/view-UNABHAENGIG (reine Kombinatorik) -> direkt testbar (kein DLL-Bau).
[[nodiscard]] inline std::vector<ProgressDelta>
compute_progress_deltas(std::vector<std::vector<std::size_t>> const& configs) {
    std::vector<ProgressDelta> out;
    out.reserve(configs.size() + 1);
    std::vector<std::size_t> prev; // leer => die erste Meldung wird Voll-Konfiguration
    for (std::size_t k = 0; k < configs.size(); ++k) {
        out.push_back(progress_delta_between(prev, configs[k], k));
        prev = configs[k];
    }
    ProgressDelta term; // §38.b: done genau EINMAL am Fensterende (cursor == Fenster-Groesse, changed leer)
    term.cursor = configs.size();
    term.done   = true;
    out.push_back(term);
    return out;
}

/// reconstruct_configs -- INVERSE zu compute_progress_deltas: rekonstruiert die Konfig-Folge aus einem Delta-Strom
/// (das done-Delta wird uebersprungen). axis_count = Zahl der Ebenen (fuer die laufende Voll-Konfiguration). Der
/// Rueck-Kanal-Konsument (und das Roundtrip-Gate) baut so die Konfigurationsfolge Zug um Zug wieder auf.
[[nodiscard]] inline std::vector<std::vector<std::size_t>> reconstruct_configs(std::vector<ProgressDelta> const& deltas,
                                                                               std::size_t axis_count) {
    std::vector<std::vector<std::size_t>> out;
    std::vector<std::size_t>              cur(axis_count, 0);
    for (ProgressDelta const& d : deltas) {
        if (d.done) continue; // Terminal-Signal traegt keine Konfiguration
        for (ProgressAxisChange const& c : d.changed)
            if (c.axis_index < cur.size()) cur[c.axis_index] = c.variant_index;
        out.push_back(cur);
    }
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
