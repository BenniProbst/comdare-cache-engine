#ifndef COMDARE_CACHE_ENGINE_CACHELINE_POLICY_SELECTOR_HPP
#define COMDARE_CACHE_ENGINE_CACHELINE_POLICY_SELECTOR_HPP
// ─────────────────────────────────────────────────────────────────────────────
// cacheline_policy_selector — Versprechen (2): laufzeit-dynamische Cache-Line-Anpassung.
//
// Goal §8.3 / Text-Agent AP-X2 / TODO-3 / Task #172 (#174), zweites Inkrement.
//
// ZWECK: Aus einem AUSGEWERTETEN Profil-Aggregat (op-Mix-Brüche + Working-Set) leitet eine
// DATENGETRIEBENE, TRANSPARENTE Heuristik konkrete, cache-line-aware Laufzeit-Einstellungen
// (anatomy::ComdareResourceControlV1) ab und setzt sie via builder::AlgorithmResourceControl::apply_to
// am Prüf-Dock (geklammert gegen tier_query_resource_caps + Env-Limits). KEINE erfundenen Magie-
// Konstanten: jede Stützstelle ist aus einer real im Code/CSV vorhandenen Größe motiviert (HW-Cache-
// Line 64 B, axis_07-prefetch-Cap 64 Cache-Lines, axis_03a-batch-Cap 4096, axis_14-inline-Cap 256 B).
//
// EINGANG (zwei austauschbare, beide real vorhandene Quellen → struct WorkloadProfileAggregate):
//   (a) WorkloadConfig / LoadProfile: pct_insert/lookup/erase/scan/rmw + key-range (= Working-Set).
//   (b) CSV-Aggregat je search_algo aus der WIDE-Mess-CSV: op_<art>_n + working_set_n
//       (m3v2_sota_pilot_measurements.csv) → Brüche via op_X_n / Summe.
//
// HEURISTIK (Cache-Line-Lehre, je Feld begründet):
//   • prefetch_distance: scan-lastig → tiefer Prefetch-Strom (16 CL); punkt-lastig → flach (2 CL).
//   • batch_size:        scan-lastig → großer Working-Set-Batch (bis Cap); punkt-lastig → 1 HW-Linie (64).
//   • pool_budget_bytes: linear ws * record_bytes (Default = HW-Linie 64); großer WS → größere Arena.
//   • inline_threshold:  write-lastig → bis 1 Cache-Line inline (64); read-lastig → kleinere Schwelle (16).
//   • thread_count:      0 (Default), solange das Profil keine Nebenläufigkeit ausweist (ehrlich).
// ALLE Werte gehen als `desired` in AlgorithmResourceControl → clamp(desired, caps, env) GARANTIERT
// Cap-/Env-Konformität, selbst wenn die Regel einen Wert über dem Cap wünscht.
//
// LEHRBUCH-PATTERN (GoF): Strategy — das Auswahl-Kriterium (PolicyObjective: scan-/latenz-/durchsatz-
// optimierend) ist als austauschbares Template-Policy gekapselt; die Cap-Klammerung bleibt invariant.
//
// ZERO-COST / HOT-PATH: Selektor + apply_to laufen EINMALIG vor run() (abi_adapter.hpp:159 setzt nur
// Lifecycle Running; tier_apply_resource_control ist NICHT im do_batch-Mess-Loop). Im Hot-Path kein
// virtueller Resource-Control-Call → keine Laufzeitkosten. Die Heuristik selbst ist constexpr-fähig.
//
// SELF-CONTAINED: nur C++-Standardbibliothek + die beiden vorhandenen Header (resource_controllable_tier,
// algorithm_resource_control). KEIN Python.
// ─────────────────────────────────────────────────────────────────────────────

#include "../algorithm_resource_control.hpp"
#include "../../anatomy/resource_controllable_tier.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::cacheline_policy {

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadProfileAggregate — das ausgewertete Profil-Aggregat (Eingang der Heuristik).
// Beide realen Quellen (WorkloadConfig/LoadProfile ODER CSV-Aggregat je search_algo) füllen DIESES
// neutrale Aggregat. `op_*_n` sind Roh-Zählwerte (CSV) ODER Anteils-Proxys (Config × num_ops); die
// Heuristik bildet daraus ausschließlich Brüche, daher ist die absolute Skala irrelevant.
// ─────────────────────────────────────────────────────────────────────────────
struct WorkloadProfileAggregate {
    std::uint64_t op_insert_n   = 0; // axis_03a/value_handle (Schreib-Pfad)
    std::uint64_t op_lookup_n   = 0; // Punkt-Lese-Pfad
    std::uint64_t op_erase_n    = 0; // Schreib-Pfad (Punkt)
    std::uint64_t op_scan_n     = 0; // Range/sequenziell (axis_03a traversal, prefetch-affin)
    std::uint64_t op_rmw_n      = 0; // Read-Modify-Write (sequenziell, schreib-affin)
    std::uint64_t working_set_n = 0; // Anzahl Records bzw. key-range-Breite (axis_06 Arena / axis_03a Batch)

    /// Summe aller op-Zählwerte (Normierungs-Basis). 0 → leeres Profil (Heuristik setzt dann nichts).
    [[nodiscard]] constexpr std::uint64_t op_total() const noexcept {
        return op_insert_n + op_lookup_n + op_erase_n + op_scan_n + op_rmw_n;
    }

    // ── Brüche (transparent; bei leerer Summe 0.0) ──
    [[nodiscard]] constexpr double scan_share() const noexcept {
        auto t = op_total();
        return t == 0 ? 0.0 : static_cast<double>(op_scan_n + op_rmw_n) / static_cast<double>(t);
    }
    [[nodiscard]] constexpr double point_share() const noexcept {
        auto t = op_total();
        return t == 0 ? 0.0 : static_cast<double>(op_lookup_n + op_insert_n + op_erase_n) / static_cast<double>(t);
    }
    [[nodiscard]] constexpr double write_share() const noexcept {
        auto t = op_total();
        return t == 0 ? 0.0 : static_cast<double>(op_insert_n + op_erase_n + op_rmw_n) / static_cast<double>(t);
    }

    /// Aus den Anteils-Brüchen (z.B. WorkloadConfig.pct_*) + num_operations ein Aggregat bauen.
    /// num_operations skaliert die Brüche auf Zählwerte; die Heuristik nutzt nur die Verhältnisse.
    [[nodiscard]] static constexpr WorkloadProfileAggregate from_shares(double pct_insert, double pct_lookup,
                                                                        double pct_erase, double pct_scan,
                                                                        double pct_rmw, std::uint64_t num_operations,
                                                                        std::uint64_t working_set) noexcept {
        auto q = [num_operations](double p) -> std::uint64_t {
            if (p <= 0.0) return 0;
            double v = p * static_cast<double>(num_operations);
            return v < 0.0 ? 0u : static_cast<std::uint64_t>(v + 0.5); // round-to-nearest
        };
        WorkloadProfileAggregate a{};
        a.op_insert_n   = q(pct_insert);
        a.op_lookup_n   = q(pct_lookup);
        a.op_erase_n    = q(pct_erase);
        a.op_scan_n     = q(pct_scan);
        a.op_rmw_n      = q(pct_rmw);
        a.working_set_n = working_set;
        return a;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// HW-/Cap-Stützstellen — KEINE freien Magie-Konstanten, sondern dokumentierte Quellen.
//   kHwCacheLineBytes = 64  → build_variant_definition.hpp:26 (hw_cache_line, cache_line_size()).
//   Die Caps (64/64/1 GiB/4096/256) liegen im Tier (abi_adapter.hpp:177-186); die Heuristik darf
//   ungeklammert wünschen — clamp() im Prüf-Dock erzwingt die Cap-/Env-Grenze.
// ─────────────────────────────────────────────────────────────────────────────
inline constexpr std::uint64_t kHwCacheLineBytes = 64; // HW-Linie (Quelle hw_cache_line)

// Schwellen für die scan/punkt/gemischt-Klassifikation (rein über scan_share; transparent).
inline constexpr double kScanHeavyThreshold = 0.50; // ≥ 50 % sequenziell ⇒ scan-lastig
inline constexpr double kScanMixedThreshold = 0.10; // 10–50 % ⇒ gemischt; < 10 % ⇒ punkt-lastig

// Prefetch-Distanz-Stützstellen (Cache-Lines, Zweier-Potenzen unter Cap 64; monoton mit scan_share).
inline constexpr std::uint64_t kPrefetchScan  = 16; // tiefer Prefetch-Strom für Range-Scan
inline constexpr std::uint64_t kPrefetchMixed = 8;
inline constexpr std::uint64_t kPrefetchPoint = 2; // flach — Pointer-Chase profitiert kaum

// inline_threshold-Stützstellen (Bytes; Teiler/Vielfache der HW-Linie, unter Cap 256).
inline constexpr std::uint64_t kInlineWrite         = 64; // bis 1 Cache-Line inline (Schreib-Pfad)
inline constexpr std::uint64_t kInlineRead          = 16; // kleinere Schwelle bei Lese-Dominanz
inline constexpr double        kWriteHeavyThreshold = 0.50;

// pool_budget: angenommene Record-Größe = 1 HW-Linie (transparent, linear in ws).
inline constexpr std::uint64_t kApproxRecordBytes = kHwCacheLineBytes;

// ─────────────────────────────────────────────────────────────────────────────
// PolicyObjective (Strategy-Punkt) — austauschbares Auswahl-Kriterium.
//   ScanOptimizing   : die oben beschriebene cache-line-aware Regel (Default — Thesis-Kernfall).
//   LatencyOptimizing: punkt-/latenz-betont — drückt Prefetch & Batch auch bei gemischt nach unten.
// Beide liefern NUR `desired` (ungeklammert); die Cap-Klammerung bleibt invariant in apply_to/clamp.
// ─────────────────────────────────────────────────────────────────────────────
struct ScanOptimizing {
    [[nodiscard]] static constexpr anatomy::ComdareResourceControlV1
    derive(WorkloadProfileAggregate const& a) noexcept {
        anatomy::ComdareResourceControlV1 d{};
        double const                      scan = a.scan_share();
        double const                      wr   = a.write_share();
        std::uint64_t const               ws   = a.working_set_n;

        // (1) prefetch_distance (Cache-Lines) — monoton mit scan_share.
        d.prefetch_distance = (scan >= kScanHeavyThreshold)   ? kPrefetchScan
                              : (scan >= kScanMixedThreshold) ? kPrefetchMixed
                                                              : kPrefetchPoint;

        // (2) batch_size — Working-Set-Achse (axis_03a). scan: großer Batch (ws); gemischt: ws/4;
        //     punkt: 1 HW-Linie (kHwCacheLineBytes als Elementzahl). clamp im Prüf-Dock kappt an Cap 4096.
        if (ws == 0) {
            d.batch_size = 0; // kein Working-Set bekannt → nicht setzen (Default beibehalten)
        } else if (scan >= kScanHeavyThreshold) {
            d.batch_size = ws;
        } else if (scan >= kScanMixedThreshold) {
            d.batch_size = std::max<std::uint64_t>(ws / 4, 1);
        } else {
            d.batch_size = kHwCacheLineBytes; // kleiner Batch — eine Cache-Line je Punkt-Op
        }

        // (3) pool_budget_bytes — linear: ws * Record-Größe (Default 1 HW-Linie). clamp kappt an 1 GiB.
        if (ws != 0) { d.pool_budget_bytes = ws * kApproxRecordBytes; }

        // (4) inline_threshold_bytes — write-lastig → 1 Cache-Line inline; sonst kleinere Schwelle.
        d.inline_threshold_bytes = (wr >= kWriteHeavyThreshold) ? kInlineWrite : kInlineRead;

        // (5) thread_count — ehrlich 0 (Default), solange kein Nebenläufigkeits-Signal vorliegt.
        d.thread_count = 0;

        d.controllable_axis_count = 0; // Meta: Host/Tier füllt; vom Selektor nicht gesetzt.
        return d;
    }
};

struct LatencyOptimizing {
    [[nodiscard]] static constexpr anatomy::ComdareResourceControlV1
    derive(WorkloadProfileAggregate const& a) noexcept {
        // Start von der scan-optimierenden Regel, dann latenz-betont nach unten ziehen:
        // gemischte Last wird wie punkt-lastig behandelt (flacher Prefetch, kleiner Batch).
        anatomy::ComdareResourceControlV1 d    = ScanOptimizing::derive(a);
        double const                      scan = a.scan_share();
        if (scan < kScanHeavyThreshold) { // alles unter „echt scan-lastig" → Punkt-Profil
            d.prefetch_distance = kPrefetchPoint;
            if (a.working_set_n != 0) d.batch_size = kHwCacheLineBytes;
        }
        return d;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CacheLinePolicySelector<Objective> — GoF Strategy.
// Mappt ein WorkloadProfileAggregate → ComdareResourceControlV1 (`desired`, ungeklammert), baut daraus
// einen AlgorithmResourceControl (mit optionalen Env-Limits) und setzt ihn via apply_to am Prüf-Dock
// (clamp gegen tier-caps + env). Objective ist das austauschbare Auswahl-Kriterium (Default scan-opt.).
// ─────────────────────────────────────────────────────────────────────────────
template <class Objective = ScanOptimizing>
class CacheLinePolicySelector {
public:
    CacheLinePolicySelector() = default;
    explicit constexpr CacheLinePolicySelector(anatomy::ComdareResourceControlV1 env_limits) noexcept
        : env_limits_(env_limits) {}

    /// Reine Heuristik: Profil → gewünschte (ungeklammerte) Steuerung. constexpr/zero-cost.
    [[nodiscard]] constexpr anatomy::ComdareResourceControlV1 select(WorkloadProfileAggregate const& a) const noexcept {
        return Objective::derive(a);
    }

    /// Baut den Controller (desired = select(a), env_limits = gesetzte System-Grenzen).
    [[nodiscard]] constexpr AlgorithmResourceControl make_control(WorkloadProfileAggregate const& a) const noexcept {
        AlgorithmResourceControl c{};
        c.desired    = select(a);
        c.env_limits = env_limits_;
        return c;
    }

    /// Wendet die abgeleitete Steuerung am Prüf-Dock an (clamp gegen tier-caps + env). tier==nullptr → 0.
    /// Liefert die Zahl real angenommener Achsen (≤ controllable_axis_count des Tiers).
    [[nodiscard]] std::uint64_t apply(WorkloadProfileAggregate const&     a,
                                      anatomy::IResourceControllableTier* tier) const noexcept {
        return make_control(a).apply_to(tier);
    }

    [[nodiscard]] constexpr anatomy::ComdareResourceControlV1 env_limits() const noexcept { return env_limits_; }
    void set_env_limits(anatomy::ComdareResourceControlV1 e) noexcept { env_limits_ = e; }

private:
    anatomy::ComdareResourceControlV1 env_limits_{}; // 0 = keine zusätzliche Env-Grenze
};

// ─────────────────────────────────────────────────────────────────────────────
// CSV-Aggregat-Quelle (b): WIDE-Mess-CSV → WorkloadProfileAggregate je search_algo.
// Header-getrieben, ';'-getrennt (identisches Parser-Muster wie best_binary_selector). Liest die
// real vorhandenen Spalten op_<art>_n + working_set_n und gruppiert über das search_algo=<name>-Token
// der binary_id-Spalte (mittelt working_set_n, summiert op-Zählwerte je Gruppe).
//
// EHRLICHE EINSCHRÄNKUNG (verifiziert): m3v2_sota_pilot_measurements.csv enthält NUR lookup-Last
// (op_lookup_n>0, Rest 0) → die CSV liefert real ausschließlich den PUNKT-Pol. Der scan-lastige Pol
// kommt aus WorkloadConfig/LoadProfile (ycsb_e). Das ist KEIN Fake, sondern Konsequenz daraus, dass
// der Pilot-Lauf nur Punkt-Lookups gemessen hat.
// ─────────────────────────────────────────────────────────────────────────────
namespace detail {

[[nodiscard]] inline std::string extract_search_algo(std::string_view binary_id) {
    // binary_id-Form: "search_algo=<name>/<weitere achsen>". Token bis zum ersten '/'.
    constexpr std::string_view key = "search_algo=";
    auto                       pos = binary_id.find(key);
    if (pos == std::string_view::npos) return std::string(binary_id);
    auto start = pos + key.size();
    auto slash = binary_id.find('/', start);
    return std::string(
        binary_id.substr(start, slash == std::string_view::npos ? std::string_view::npos : slash - start));
}

} // namespace detail

/// Aggregiert eine WIDE-Mess-CSV je search_algo. Gibt Anzahl gelesener Datenzeilen zurück, -1 bei Fehler.
/// out wird (search_algo → WorkloadProfileAggregate) gefüllt: op-Zählwerte summiert, working_set_n gemittelt.
[[nodiscard]] inline int aggregate_csv_by_search_algo(std::filesystem::path const&                     csv,
                                                      std::map<std::string, WorkloadProfileAggregate>& out) {
    std::ifstream in(csv);
    if (!in) return -1;
    std::string header_line;
    if (!std::getline(in, header_line)) return -1;

    // Header-Spalten indexieren (Reihenfolge-agnostisch).
    std::map<std::string, std::size_t> col;
    {
        std::stringstream hs(header_line);
        std::string       field;
        std::size_t       idx = 0;
        while (std::getline(hs, field, ';')) col[field] = idx++;
    }
    auto need = [&](char const* name) -> long {
        auto it = col.find(name);
        return it == col.end() ? -1L : static_cast<long>(it->second);
    };
    long ci_bin = need("binary_id");
    long ci_ins = need("op_insert_n");
    long ci_lk  = need("op_lookup_n");
    long ci_er  = need("op_erase_n");
    long ci_sc  = need("op_scan_n");
    long ci_rmw = need("op_rmw_n");
    long ci_ws  = need("working_set_n");
    if (ci_bin < 0 || ci_ins < 0 || ci_lk < 0 || ci_er < 0 || ci_sc < 0 || ci_rmw < 0 || ci_ws < 0) return -1;

    std::map<std::string, std::uint64_t> ws_sum, ws_cnt;
    int                                  rows = 0;
    std::string                          line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::vector<std::string> cells;
        cells.reserve(col.size());
        std::stringstream ls(line);
        std::string       cell;
        while (std::getline(ls, cell, ';')) cells.push_back(cell);
        auto at = [&](long i) -> std::string const& {
            static std::string const empty;
            return (i >= 0 && static_cast<std::size_t>(i) < cells.size()) ? cells[static_cast<std::size_t>(i)] : empty;
        };
        auto to_u64 = [](std::string const& s) -> std::uint64_t {
            if (s.empty()) return 0;
            try {
                return static_cast<std::uint64_t>(std::stoull(s));
            } catch (...) { return 0; }
        };

        std::string               algo = detail::extract_search_algo(at(ci_bin));
        WorkloadProfileAggregate& a    = out[algo];
        a.op_insert_n += to_u64(at(ci_ins));
        a.op_lookup_n += to_u64(at(ci_lk));
        a.op_erase_n += to_u64(at(ci_er));
        a.op_scan_n += to_u64(at(ci_sc));
        a.op_rmw_n += to_u64(at(ci_rmw));
        ws_sum[algo] += to_u64(at(ci_ws));
        ws_cnt[algo] += 1;
        ++rows;
    }
    // working_set_n je Gruppe mitteln.
    for (auto& [algo, agg] : out) {
        auto c            = ws_cnt[algo];
        agg.working_set_n = (c == 0) ? 0 : (ws_sum[algo] / c);
    }
    return rows;
}

} // namespace comdare::cache_engine::builder::cacheline_policy

#endif // COMDARE_CACHE_ENGINE_CACHELINE_POLICY_SELECTOR_HPP
