#pragma once
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4:
// "SetDock/SequenceDock/AdapterDock/ViewDock als IPruefDock-Impls (V5-Vertrag: import -> GATE -> messen)")
// -- die MESS-TREIBER der vier Container-Gattungen ueber ihre ABI-Sub-Interfaces.
//
// WAS DIESE DATEI IST: das Gegenstueck zu tier_observe_trace_abi.hpp (SearchAlgorithm-Gattung, Treiber
// drive_tier_observe_trace_abi ueber IObservableTier) fuer ISetTier/ISequenceTier/IAdapterTier/IViewTier.
// Ohne diese Treiber koennte ein Container-Dock zwar das Gate fahren, aber danach nichts messen.
//
// WAS GETEILT IST UND WAS NICHT (der Schnitt ist Absicht, nicht Bequemlichkeit):
//   * GETEILT ist die ZEIT-Seite -- Fuellstands-Checkpoints, r/w/d-Wall-Clock-Roh-Samples, der
//     Wall-Clock-korrelierte Observer-Abgriff, die Perzentile. Sie ist gattungs-unabhaengig und liegt
//     deshalb EINMAL da (GenusFillLevelSnapshot<ObserverV1> / GenusTierObserveTrace<ObserverV1>,
//     Template ueber den Wire-POD -- CT-statisch, kein Runtime-Switch, KEIN std::variant).
//   * NICHT GETEILT ist die OP-Seite. Jede Gattung hat einen anderen Antrieb, und genau das ist der
//     Gegenstand der Messung. Ein gemeinsames "treibe irgendwie"-Verb waere die stille Wiedereinfuehrung
//     eines Ganz-Tier-Konfigurators (Memory-Kanon: keine Ganz-Tier-Achsen) und wuerde die Unterschiede
//     wegdefinieren, die gemessen werden sollen. Es gibt deshalb VIER Treiber, nicht einen.
//
// DREI BENANNTE LUECKEN DER HEUTIGEN ABI-FLAECHE (deklariert statt mit Ersatz-Ops kaschiert -- eine
// erfundene Op waere eine Messluege; D2-Doktrin: lieber eine leere Spalte mit Grund als eine stille 0):
//   * SEQUENCE hat KEINE Loesch-Op. ISequenceTier traegt push_back/at/size/clear (sequence_tier.hpp:36-51).
//     -> delete_ns bleibt leer; der Grund steht in der Spalten-Doku und in der Dock-Zeile.
//   * ADAPTER hat KEINE nicht-konsumierende Lese-Op. IAdapterTier traegt put/get/size/clear
//     (adapter_tier.hpp:51-70) -- ein front()/top() gibt es an der ABI-Flaeche nicht; get IST die
//     Entnahme. -> die get-Zeiten stehen in delete_ns (dort gehoeren sie semantisch hin), read_ns bleibt
//     leer. Der Fuellstand wird nach jeder gemessenen Entnahme durch ein UNGEMESSENES put wiederhergestellt.
//   * VIEW hat gar keine Mutation ausser der Bindung. -> write_ns traegt die tier_bind-Zeiten,
//     delete_ns bleibt leer (eine non-owning Sicht besitzt nichts, was geloescht werden koennte).
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only. Die vier Sub-Interfaces werden NUR AUFGERUFEN;
// abi_adapter.hpp, die Wire-PODs, abi/*_decl.hpp und die Stempel-/Fingerprint-Flaechen bleiben unberuehrt.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md Paragraf 8.7/8.8

// Zieht ausserdem das D5-4-Trace-Schema mit (ts:: = anatomy_commands::trace_schema) und ueber dieses
// den D5-1-Perzentil-Kanon.
#include "tier_observe_trace_abi.hpp" // AbiTierTraceConfig + detail::abi_dur_ns

#include <anatomy/adapter_tier.hpp>
#include <anatomy/sequence_tier.hpp>
#include <anatomy/set_tier.hpp>
#include <anatomy/view_tier.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

/// Eine Fuellstands-Stufe einer Container-Gattung: r/w/d-Wall-Clock-Roh-Samples + der Wall-Clock-
/// korrelierte Observer-POD der Gattung. Form BEWUSST identisch zu AbiFillLevelSnapshot
/// (tier_observe_trace_abi.hpp:46-55) -- wer beide Kurven nebeneinander auswertet, sieht dieselbe Struktur.
template <class ObserverV1>
struct GenusFillLevelSnapshot {
    std::uint64_t             fill_level = 0; ///< == tier_size() am Checkpoint
    std::vector<std::int64_t> read_ns{};      ///< Tier-Wall-Clock je Operation, GETRENNT (Paragraf 2.1)
    std::vector<std::int64_t> write_ns{};
    std::vector<std::int64_t> delete_ns{};
    std::uint64_t             read_sink       = 0;  ///< Anti-Wegoptimierungs-Senke
    ObserverV1                observer        = {}; ///< EIN konsolidierter Observer-POD je Checkpoint
    std::int64_t              observe_wall_ns = 0;  ///< Zeitstempel des Abgriffs, relativ zum Trace-Start
};

/// Mess-Trace einer Container-Gattung ueber ihr ABI-Sub-Interface.
template <class ObserverV1>
struct GenusTierObserveTrace {
    std::vector<GenusFillLevelSnapshot<ObserverV1>> checkpoints{};
};

using SetTierObserveTrace      = GenusTierObserveTrace<ana::SetObserverSnapshotV1>;
using SequenceTierObserveTrace = GenusTierObserveTrace<ana::SequenceObserverSnapshotV1>;
using AdapterTierObserveTrace  = GenusTierObserveTrace<ana::AdapterObserverSnapshotV1>;
using ViewTierObserveTrace     = GenusTierObserveTrace<ana::ViewObserverSnapshotV1>;

// ================================================================================================
// SET -- insert (write) / contains (read) / erase+reinsert (delete)
// ================================================================================================

/// drive_set_tier_trace_abi -- faehrt ein ISetTier ueber die Fuellstands-Checkpoints.
/// Der Stagnations-Guard ist DERSELBE wie beim SA-Treiber (tier_observe_trace_abi.hpp:137-141) und aus
/// demselben realen Grund: ohne ihn laeuft `while (size < target)` ENDLOS, sobald insert den Fuellstand
/// nicht mehr erhoeht (Kapazitaets-Grenze / Key-Kollision).
[[nodiscard]] inline SetTierObserveTrace drive_set_tier_trace_abi(ana::ISetTier&            tier,
                                                                  AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64     rng{cfg.seed};
    std::uint64_t       next_key = 0;
    SetTierObserveTrace trace;
    auto const          trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        GenusFillLevelSnapshot<ana::SetObserverSnapshotV1> snap;

        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill        = tier.tier_set_size();
        while (tier.tier_set_size() < target) {
            auto const t0 = clock::now();
            (void)tier.tier_set_insert(next_key);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_key;
            std::uint64_t const cur_fill = tier.tier_set_size();
            if (cur_fill > last_fill) {
                last_fill        = cur_fill;
                write_stagnation = 0;
            } else if (++write_stagnation >= cfg.max_insert_stagnation) {
                break; // effektive Kapazitaet erreicht
            }
        }

        // READ-Phase: deterministische ~50%-Hit/Miss-Mitgliedschaftstests ueber den Key-Raum.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k   = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            auto const          t0  = clock::now();
            bool const          hit = tier.tier_set_contains(k);
            auto const          t1  = clock::now();
            snap.read_sink += hit ? 1u : 0u;
            snap.read_ns.push_back(detail::abi_dur_ns(t0, t1));
        }

        // DELETE-Phase: erase + (ungemessenes) reinsert -> der Fuellstand bleibt auf target.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_set_size() > 0; ++i) {
            std::uint64_t const dk = (next_key != 0) ? (rng() % next_key) : 0u;
            auto const          t0 = clock::now();
            bool const          ok = tier.tier_set_erase(dk);
            auto const          t1 = clock::now();
            snap.delete_ns.push_back(detail::abi_dur_ns(t0, t1));
            if (ok) (void)tier.tier_set_insert(dk);
        }

        snap.fill_level = tier.tier_set_size();
        tier.tier_observe_set(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

// ================================================================================================
// SEQUENCE -- push_back (write) / at (read) / KEINE Loesch-Op an der ABI-Flaeche
// ================================================================================================

[[nodiscard]] inline SequenceTierObserveTrace drive_sequence_tier_trace_abi(ana::ISequenceTier&       tier,
                                                                            AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64          rng{cfg.seed};
    std::uint64_t            next_value = 0;
    SequenceTierObserveTrace trace;
    auto const               trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        GenusFillLevelSnapshot<ana::SequenceObserverSnapshotV1> snap;

        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill        = tier.tier_size();
        while (tier.tier_size() < target) {
            auto const t0 = clock::now();
            tier.tier_push_back(next_value);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_value;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) {
                last_fill        = cur_fill;
                write_stagnation = 0;
            } else if (++write_stagnation >= cfg.max_insert_stagnation) {
                break;
            }
        }

        // READ-Phase: Indizes ueber [0, 2*size) -> ~50% in-bounds/out-of-bounds.
        std::uint64_t const len = tier.tier_size();
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const idx = (len != 0) ? (rng() % (len * 2u)) : 0u;
            std::uint64_t       out = 0;
            auto const          t0  = clock::now();
            bool const          hit = tier.tier_at(idx, &out);
            auto const          t1  = clock::now();
            snap.read_sink += hit ? out : 0u;
            snap.read_ns.push_back(detail::abi_dur_ns(t0, t1));
        }

        // DELETE-Phase: entfaellt -- ISequenceTier hat keine Loesch-Op (siehe Kopf). delete_ns bleibt
        // LEER; die CSV weist die 0 als Sample-ZAHL aus, nicht als gemessene 0-Dauer.

        snap.fill_level = tier.tier_size();
        tier.tier_observe_sequence(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

// ================================================================================================
// ADAPTER -- put (write) / get (delete; KEINE nicht-konsumierende Lese-Op an der ABI-Flaeche)
// ================================================================================================

[[nodiscard]] inline AdapterTierObserveTrace drive_adapter_tier_trace_abi(ana::IAdapterTier&        tier,
                                                                          AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64         rng{cfg.seed};
    std::uint64_t           next_value = 0;
    AdapterTierObserveTrace trace;
    auto const              trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        GenusFillLevelSnapshot<ana::AdapterObserverSnapshotV1> snap;

        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill        = tier.tier_size();
        while (tier.tier_size() < target) {
            auto const t0 = clock::now();
            tier.tier_put(next_value);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_value;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) {
                last_fill        = cur_fill;
                write_stagnation = 0;
            } else if (++write_stagnation >= cfg.max_insert_stagnation) {
                break;
            }
        }

        // READ-Phase: entfaellt -- der Adapter hat keine nicht-konsumierende Lese-Op (siehe Kopf).

        // DELETE-Phase: get IST die Entnahme. Jede gemessene Entnahme wird durch ein UNGEMESSENES put
        // ausgeglichen, damit der Fuellstand des Checkpoints stabil bleibt (gleiche Absicht wie
        // erase+reinsert beim SA-/Set-Treiber).
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_size() > 0; ++i) {
            std::uint64_t out = 0;
            auto const    t0  = clock::now();
            bool const    ok  = tier.tier_get(&out);
            auto const    t1  = clock::now();
            snap.delete_ns.push_back(detail::abi_dur_ns(t0, t1));
            snap.read_sink += ok ? out : 0u;
            if (ok) tier.tier_put(rng() % 100'000u);
        }

        snap.fill_level = tier.tier_size();
        tier.tier_observe_container(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

// ================================================================================================
// VIEW -- bind (write) / read (read) / KEINE Loesch-Op (non-owning)
// ================================================================================================

/// drive_view_tier_trace_abi -- der Puffer JE CHECKPOINT lebt hier im Rahmen und ueberlebt die Messung
/// dieses Checkpoints (die View referenziert ihn nur). Am Ende wird auf (nullptr, 0) zurueckgebunden,
/// damit die View KEINEN Zeiger auf einen ablaufenden Puffer behaelt -- dieselbe Lebensdauer-Disziplin,
/// die view_dock.hpp:37-39 fuer den in-process-Pfad beschreibt.
[[nodiscard]] inline ViewTierObserveTrace drive_view_tier_trace_abi(ana::IViewTier&           tier,
                                                                    AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64      rng{cfg.seed};
    ViewTierObserveTrace trace;
    auto const           trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        GenusFillLevelSnapshot<ana::ViewObserverSnapshotV1> snap;

        std::vector<std::uint64_t> buffer(static_cast<std::size_t>(target));
        for (std::uint64_t i = 0; i < target; ++i) buffer[static_cast<std::size_t>(i)] = i * 3u + 1u;

        // WRITE-Phase: die Bindung ist die EINZIGE Mutation einer non-owning Sicht.
        auto const t0 = clock::now();
        tier.tier_bind(buffer.data(), buffer.size());
        auto const t1 = clock::now();
        snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));

        // READ-Phase: Indizes ueber [0, 2*size) -> ~50% im Fenster / dahinter.
        std::uint64_t const len = tier.tier_size();
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const idx = (len != 0) ? (rng() % (len * 2u)) : 0u;
            std::uint64_t       out = 0;
            auto const          r0  = clock::now();
            bool const          hit = tier.tier_read(idx, &out);
            auto const          r1  = clock::now();
            snap.read_sink += hit ? out : 0u;
            snap.read_ns.push_back(detail::abi_dur_ns(r0, r1));
        }

        // DELETE-Phase: entfaellt -- eine non-owning Sicht besitzt nichts (siehe Kopf).

        snap.fill_level = tier.tier_size();
        tier.tier_observe_view(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));

        tier.tier_bind(nullptr, 0); // Puffer laeuft gleich ab -> Bindung VORHER loesen
    }
    return trace;
}

// ================================================================================================
// Serialisierung -- geteilte Zeit-Spalten, gattungs-eigene Observer-Spalten
// ================================================================================================

// D5-4 (2026-08-09): hier standen `kSharedCsvHead`, `write_shared_csv_cells` und
// `write_shared_json_fields` -- die zweite Haelfte einer ABSCHRIFT. Der JSON-Feldblock war Zeichen fuer
// Zeichen derselbe wie in serialize_abi_tier_trace_json, und als delete_p99_ns fehlte, fehlte es
// deshalb in BEIDEN. Alle drei sind ERSATZLOS geloescht (kein Alias, keine Weiterleitung): eine
// uebersehene Aufrufstelle soll compile-hart brechen, nicht still eine zweite Feldliste bedienen.
// Der EINE Ort ist jetzt tier_trace_schema.hpp (trace_schema::kLatenzFelder + die Schreiber darueber);
// die Namen dort sind BEWUSST andere, damit kein Aufruf per Namensgleichheit still weiterlebt.
// Der Namensraum `genus_trace_detail` faellt damit ganz weg -- er trug nichts anderes.
//
// SELBSTCHECK (D5-4, 2026-08-09)
//   ZUSICHERT: die acht Serialisierer unten (4 Container-Gattungen x CSV/JSON) rufen fuer den
//              geteilten Teil ts:: -- dieselben Funktionen, die auch die SA-Gattung rendern.
//   ZUSICHERT NICHT: die gattungs-eigenen Observer-Spalten -- die stehen je Serialisierer und sollen
//              das auch (der Schnitt ist im Kopf dieser Datei begruendet: geteilte ZEIT, eigene OP).

[[nodiscard]] inline std::string serialize_set_tier_trace_csv(SetTierObserveTrace const& trace) {
    std::ostringstream os;
    os << ts::kGeteilterCsvKopf
       << ",set_insert,set_contains,set_hit,set_miss,set_erase,set_current_size,set_peak_size,observable_axes\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        ts::schreibe_geteilte_csv_zellen(os, i, cp);
        os << ',' << o.insert_count << ',' << o.contains_count << ',' << o.contains_hit_count << ','
           << o.contains_miss_count << ',' << o.erase_count << ',' << o.current_size << ',' << o.peak_size << ','
           << o.observable_axis_count << '\n';
    }
    return os.str();
}

[[nodiscard]] inline std::string serialize_set_tier_trace_json(SetTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        ts::schreibe_geteilten_json_vorspann(os, i, cp);
        os << ",\"set_insert\":" << o.insert_count << ",\"set_contains\":" << o.contains_count
           << ",\"set_hit\":" << o.contains_hit_count << ",\"set_miss\":" << o.contains_miss_count
           << ",\"set_erase\":" << o.erase_count << ",\"set_peak_size\":" << o.peak_size
           << ",\"observable_axes\":" << o.observable_axis_count << ",\"organ_count\":" << o.organ_count << '}';
    }
    os << ']';
    return os.str();
}

[[nodiscard]] inline std::string serialize_sequence_tier_trace_csv(SequenceTierObserveTrace const& trace) {
    std::ostringstream os;
    // delete_samples ist hier strukturell 0: ISequenceTier hat keine Loesch-Op (Kopf-Kommentar).
    os << ts::kGeteilterCsvKopf
       << ",seq_push,seq_at,seq_at_oob,seq_current_size,seq_peak_size,seq_growth_events,observable_axes\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        ts::schreibe_geteilte_csv_zellen(os, i, cp);
        os << ',' << o.push_count << ',' << o.at_count << ',' << o.at_oob_count << ',' << o.current_size << ','
           << o.peak_size << ',' << o.growth_events << ',' << o.observable_axis_count << '\n';
    }
    return os.str();
}

[[nodiscard]] inline std::string serialize_sequence_tier_trace_json(SequenceTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        ts::schreibe_geteilten_json_vorspann(os, i, cp);
        os << ",\"seq_push\":" << o.push_count << ",\"seq_at\":" << o.at_count << ",\"seq_at_oob\":" << o.at_oob_count
           << ",\"seq_peak_size\":" << o.peak_size << ",\"seq_growth_events\":" << o.growth_events
           << ",\"observable_axes\":" << o.observable_axis_count << ",\"organ_count\":" << o.organ_count << '}';
    }
    os << ']';
    return os.str();
}

[[nodiscard]] inline std::string serialize_adapter_tier_trace_csv(AdapterTierObserveTrace const& trace) {
    std::ostringstream os;
    // read_samples ist hier strukturell 0: der Adapter hat keine nicht-konsumierende Lese-Op; die
    // Entnahme-Zeiten stehen in delete_samples (Kopf-Kommentar).
    os << ts::kGeteilterCsvKopf
       << ",adp_push,adp_pop,adp_front_reads,adp_back_reads,adp_current_occupancy,adp_peak_occupancy,organ_count\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        ts::schreibe_geteilte_csv_zellen(os, i, cp);
        os << ',' << o.push_count << ',' << o.pop_count << ',' << o.front_reads << ',' << o.back_reads << ','
           << o.current_occupancy << ',' << o.peak_occupancy << ',' << o.organ_count << '\n';
    }
    return os.str();
}

[[nodiscard]] inline std::string serialize_adapter_tier_trace_json(AdapterTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        ts::schreibe_geteilten_json_vorspann(os, i, cp);
        os << ",\"adp_push\":" << o.push_count << ",\"adp_pop\":" << o.pop_count
           << ",\"adp_front_reads\":" << o.front_reads << ",\"adp_back_reads\":" << o.back_reads
           << ",\"adp_peak_occupancy\":" << o.peak_occupancy << ",\"organ_count\":" << o.organ_count << '}';
    }
    os << ']';
    return os.str();
}

[[nodiscard]] inline std::string serialize_view_tier_trace_csv(ViewTierObserveTrace const& trace) {
    std::ostringstream os;
    // delete_samples ist hier strukturell 0: eine non-owning Sicht besitzt nichts (Kopf-Kommentar).
    os << ts::kGeteilterCsvKopf
       << ",view_read,view_read_oob,view_bound_size,view_bind_count,observable_axes,organ_count\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        ts::schreibe_geteilte_csv_zellen(os, i, cp);
        os << ',' << o.read_count << ',' << o.read_oob_count << ',' << o.bound_size << ',' << o.bind_count << ','
           << o.observable_axis_count << ',' << o.organ_count << '\n';
    }
    return os.str();
}

[[nodiscard]] inline std::string serialize_view_tier_trace_json(ViewTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        ts::schreibe_geteilten_json_vorspann(os, i, cp);
        os << ",\"view_read\":" << o.read_count << ",\"view_read_oob\":" << o.read_oob_count
           << ",\"view_bound_size\":" << o.bound_size << ",\"view_bind_count\":" << o.bind_count
           << ",\"observable_axes\":" << o.observable_axis_count << ",\"organ_count\":" << o.organ_count << '}';
    }
    os << ']';
    return os.str();
}

} // namespace comdare::cache_engine::builder::anatomy_commands
