// ─────────────────────────────────────────────────────────────────────────────
// best_binary_selector — Versprechen (1): automatische Auswahl + Versand der besten Binary.
//
// Goal §8.3 / Text-Agent AP-X2 / TODO-3 / Task #172, erstes Inkrement.
//
// ZWECK (Thesis-Ausblick → Implementierung): Die Mess-Pipeline (lazy_csv_header,
// cache_engine_builder_iterator.hpp) liefert die feingliedrige Rangbildung bereits als
// WIDE-CSV (binary_id;…;ns_per_op;…;op_<art>_p50_ns;…;two_phase_valid;…). DIESES Werkzeug
// schließt die offene Lücke: es liest eine solche Mess-CSV, RANKT je Interface-Funktion/Metrik
// die beste Permutation (binary_id) über NUR two_phase_valid-Zeilen (Median, nearest-rank),
// findet die zugehörige REALE perm.dll im tiere/-Baum (über die orch_make_stem-Identität =
// sanitize + FNV-1a-Suffix, identisch zum BuildOrchestrator) und liefert sie als eigenständiges,
// benanntes, ABI-stabiles Artefakt aus (Kopie der perm.dll + .version-Sidecar + ein Manifest mit
// binary_id, Metrik, Median, ABI-Major/Minor/Magic).
//
// LEHRBUCH-PATTERN (GoF / erweitert):
//   • Strategy   — RankingCriterion: das Vergleichs-/Selektions-Kriterium (ns_per_op | insert |
//                  lookup | erase | scan | rmw) ist austauschbar gekapselt. Geringere ns ⇒ besser.
//   • Builder    — ShippedArtifactBuilder: schrittweiser, validierter Zusammenbau des Versand-
//                  Artefakts (DLL-Kopie → Version-Sidecar → Manifest) mit klaren Vor-/Nachbedingungen.
//   • Repository — TiereDllRepository: kapselt die binary_id→perm.dll-Auflösung im tiere/-Baum
//                  (orch_make_stem-Round-Trip), entkoppelt vom Ranking.
//
// SELF-CONTAINED: nur C++17-Standardbibliothek (header-getriebener CSV-Parser, FNV-1a, <filesystem>).
// KEIN Python (wie die übrige Pipeline). KEINE externen Abhängigkeiten.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef COMDARE_CACHE_ENGINE_BEST_BINARY_SELECTOR_HPP
#define COMDARE_CACHE_ENGINE_BEST_BINARY_SELECTOR_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::best_binary {

// ── ABI-Identität fürs Manifest (Quelle: anatomy_module_abi_v1_decl.hpp) ──────────────────────────
// Hart gespiegelt (KEINE Build-Abhängigkeit aufs Engine-Target); muss mit dem Decl-Header übereinstimmen.
// K-5 (2026-07-19): Spiegel-Drift 5/".A5." -> 6/".A6." gesynct (Bau-INC-2d isa-Herausloesung, ABI-6) --
// der Spiegel schrieb sonst FALSCHE Manifest-Provenienz. Paritaets-Gate: static_assert gegen den
// Decl-Header in tests/unit/test_best_binary_selector_parse_rank.cpp (Tool selbst bleibt self-contained).
inline constexpr std::uint32_t kAbiMajor = 6; // Bau-INC-2d 5→6 (vorher INC-2b 4→5, #216-H2: 4)
inline constexpr std::uint32_t kAbiMinor = 0;
inline constexpr std::uint64_t kAbiMagic = 0x434F4D444141362EULL; // COMDARE_ANATOMY_ABI_MAGIC "COMDA.A6."

// ── orch_make_stem-Round-Trip (identisch BuildOrchestrator: sanitize + FNV-1a + kStemMax=120) ─────
// Diese drei Funktionen sind eine 1:1-Spiegelung von build_orchestrator.hpp:114/122/138 — sie MÜSSEN
// byte-gleich bleiben, sonst bricht die binary_id→Verzeichnis-Auflösung. (Bewusste Duplikation statt
// Engine-Link: das Werkzeug ist self-contained und hat keinerlei Engine-Compile-Abhängigkeit.)
[[nodiscard]] inline std::string orch_sanitize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    return out;
}

[[nodiscard]] inline std::string orch_fnv1a_hex(std::string_view s) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= 0x100000001b3ULL;
    }
    static constexpr char hexd[] = "0123456789abcdef";
    std::string           out(16, '0');
    for (int i = 15; i >= 0; --i) {
        out[i] = hexd[h & 0xF];
        h >>= 4;
    }
    return out;
}

inline constexpr std::size_t kStemMax = 120;

// ── (REV-DATA-05, WP-5 2026-07-16) Artefaktnamen-Allowlist ────────────────────────────────────────
/// Nur ein EINFACHER Dateistamm ist als --name/artifact_name zulässig: [A-Za-z0-9_-], 1..kStemMax Zeichen.
/// Verboten damit by-construction: Verzeichnistrenner ('/', '\\'), dot/dotdot, absolute Pfade, Laufwerks-/
/// UNC-Formen und Erweiterungs-Tricks (kein '.'). Zusätzlich werden reservierte Windows-Basisnamen
/// (CON/PRN/AUX/NUL/COM1-9/LPT1-9, case-insensitiv) abgelehnt. Vorher konnte `--name ../../x` mit
/// overwrite_existing beliebige Dateien AUSSERHALB des out_dir überschreiben (Pfad-Traversal).
[[nodiscard]] inline bool valid_artifact_stem(std::string_view s) {
    if (s.empty() || s.size() > kStemMax) return false;
    for (char c : s) {
        bool const ok = std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-';
        if (!ok) return false;
    }
    // Reservierte Windows-Basisnamen (case-insensitiv; ohne '.'-Erweiterung, die ist oben schon verboten).
    std::string upper;
    upper.reserve(s.size());
    for (char c : s) upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    static constexpr std::string_view kReserved[] = {"CON", "PRN", "AUX", "NUL"};
    for (auto r : kReserved)
        if (upper == r) return false;
    if (upper.size() == 4 && (upper.compare(0, 3, "COM") == 0 || upper.compare(0, 3, "LPT") == 0) && upper[3] >= '1' &&
        upper[3] <= '9')
        return false;
    return true;
}

// ── Mess-Zeile (header-getrieben; NUR die auswertungs-relevanten Felder) ──────────────────────────
// (REV-DATA-07, WP-5 2026-07-16): zusätzlich der KANONISCHE ZELL-SCHLÜSSEL der Messung — Workload,
// Working-Set, Plattform, Build, Serie und Setting (OHNE Wiederholungs-Segment; Wiederholungen sind die
// Samples INNERHALB einer Zelle). Alle Zell-Spalten sind optional/header-getrieben (fehlende Spalte ⇒ ""
// konstant über alle Zeilen ⇒ eine einzige Zelle = exakt das alte Verhalten, Rückwärtskompatibilität).
struct MeasurementRow {
    std::string binary_id;
    bool        two_phase_valid = false;
    double      ns_per_op       = 0.0;
    double      op_insert_p50   = 0.0;
    double      op_lookup_p50   = 0.0;
    double      op_erase_p50    = 0.0;
    double      op_scan_p50     = 0.0;
    double      op_rmw_p50      = 0.0;
    // Zell-Dimensionen (REV-DATA-07; Reihenfolge = kCellDims):
    std::string workload;      // WIDE-Spalte "workload" (Lastprofil-Name, "-" = alter fixer Workload)
    std::string working_set;   // WIDE-Spalte "working_set_n" (als String — reiner Schlüsselvergleich)
    std::string platform;      // WIDE-Spalte "platform"
    std::string build_version; // WIDE-Spalte "build_version"
    std::string series;        // WIDE-Spalte "series"
    std::string setting_norep; // WIDE-Spalte "setting" OHNE "repetition.repetition_index=N"-Segment

    /// Kanonischer Zell-Schlüssel (Stratum) dieser Zeile.
    [[nodiscard]] std::string cell_key() const {
        return workload + "|" + working_set + "|" + platform + "|" + build_version + "|" + series + "|" + setting_norep;
    }
};

/// (REV-DATA-07) Die Zell-Dimensionen in kanonischer Reihenfolge (fürs Manifest / die Dokumentation).
inline constexpr std::string_view kCellDims = "workload|working_set_n|platform|build_version|series|setting";

// ── Strategy: Ranking-Kriterium (austauschbar) ────────────────────────────────────────────────────
// Eine Metrik zieht aus einer Zeile genau EINEN Vergleichswert (ns). Kleiner = besser.
// 0 (=Metrik vom Lastprofil nicht ausgeübt) wird im Aggregator als „nicht vorhanden" behandelt.
enum class Metric { ns_per_op, insert, lookup, erase, scan, rmw };

[[nodiscard]] inline std::string metric_name(Metric m) {
    switch (m) {
        case Metric::ns_per_op: return "ns_per_op";
        case Metric::insert: return "insert";
        case Metric::lookup: return "lookup";
        case Metric::erase: return "erase";
        case Metric::scan: return "scan";
        case Metric::rmw: return "rmw";
    }
    return "ns_per_op";
}

[[nodiscard]] inline std::optional<Metric> metric_from_string(std::string_view s) {
    if (s == "ns_per_op") return Metric::ns_per_op;
    if (s == "insert") return Metric::insert;
    if (s == "lookup") return Metric::lookup;
    if (s == "erase") return Metric::erase;
    if (s == "scan") return Metric::scan;
    if (s == "rmw") return Metric::rmw;
    return std::nullopt;
}

/// RankingCriterion (Strategy): liefert den Metrik-Vergleichswert einer Zeile (ns) oder 0 (= n/a).
struct RankingCriterion {
    Metric metric = Metric::ns_per_op;

    [[nodiscard]] double value_of(MeasurementRow const& r) const {
        switch (metric) {
            case Metric::ns_per_op: return r.ns_per_op;
            case Metric::insert: return r.op_insert_p50;
            case Metric::lookup: return r.op_lookup_p50;
            case Metric::erase: return r.op_erase_p50;
            case Metric::scan: return r.op_scan_p50;
            case Metric::rmw: return r.op_rmw_p50;
        }
        return r.ns_per_op;
    }
};

// ── Aggregat je binary_id für ein Kriterium (Median nearest-rank über two_phase_valid) ────────────
struct RankedBinary {
    std::string binary_id;
    double      median_value = 0.0; // ns (kleiner = besser)
    std::size_t samples      = 0;   // Anzahl gültiger, nicht-n/a-Zeilen
    std::size_t cells        = 0;   // (REV-DATA-07) Anzahl abgedeckter Mess-Zellen (== erwartetes Raster)
};

/// CSV-Parser (header-getrieben, ';'-getrennt; Reihenfolge-/Breite-agnostisch wie csv_to_latex).
/// Gibt Anzahl gelesener Datenzeilen zurück, -1 bei Fehler.
/// (REV-DATA-04, WP-5 2026-07-16): Zahlfelder werden STRIKT geparst (std::from_chars, voller Token-Verbrauch,
/// std::isfinite) — "12junk"/nan/inf/-inf und leere Pflichtwerte (ns_per_op) verwerfen die GANZE Zeile mit
/// Diagnose statt als 12/NaN ins Ranking zu fließen (NaN verletzte die strikte schwache Ordnung von sort;
/// "12junk" wurde von stod still auf 12 gekürzt). Optionale op_*-Felder: leer ⇒ 0 (= n/a, Bestands-Semantik),
/// nicht-leer ⇒ strikt. `reject_diags` (optional) sammelt je verworfener Zeile eine Diagnose.
[[nodiscard]] int parse_measurement_csv(std::filesystem::path const& in, std::vector<MeasurementRow>& out_rows,
                                        std::vector<std::string>* reject_diags = nullptr);

/// Rangbildung: je binary_id über NUR two_phase_valid-Zeilen mit Wert>0. Aufsteigend sortiert (Index 0 = beste).
/// (REV-DATA-07, WP-5 2026-07-16) STRATIFIZIERT je Mess-Zelle (cell_key = kCellDims): das erwartete Raster ist
/// die VEREINIGUNG aller beobachteten Zellen; ein Kandidat, der NICHT alle Zellen abdeckt, wird DISQUALIFIZIERT
/// (Diagnose in `excluded`, falls != nullptr) — ein Binary, das in schweren Zellen fehlt, kann nicht mehr allein
/// aus leichten Restzellen einen besseren Median bekommen. Aggregation = Median der Zell-Mediane (jede Zelle
/// zählt gleich, unabhängig von der Wiederholungszahl). Ohne Zell-Spalten (alte CSVs) gibt es genau eine Zelle
/// ⇒ identisches Verhalten wie zuvor. Strategy = crit.
[[nodiscard]] std::vector<RankedBinary> rank_binaries(std::vector<MeasurementRow> const& rows,
                                                      RankingCriterion const&            crit,
                                                      std::vector<std::string>*          excluded = nullptr);

// ── Repository: binary_id → reale perm.dll im tiere/-Baum ────────────────────────────────────────
/// Kapselt die Auflösung. tiere_dir = build/thesis_tiere/tiere. Liefert das Verzeichnis (mit perm.dll)
/// oder nullopt. Lange IDs: Match über `*_<fnv1a-hex>`-Suffix; kurze IDs: exakter sanitisierter Name.
class TiereDllRepository {
public:
    explicit TiereDllRepository(std::filesystem::path tiere_dir) : tiere_dir_(std::move(tiere_dir)) {}

    [[nodiscard]] std::optional<std::filesystem::path> resolve_dir(std::string const& binary_id) const;

    [[nodiscard]] std::filesystem::path const& root() const noexcept { return tiere_dir_; }

private:
    std::filesystem::path tiere_dir_;
};

// ── Builder: das ausgelieferte Artefakt schrittweise zusammenbauen ────────────────────────────────
struct ShippedArtifact {
    std::string           binary_id;
    std::string           metric;
    double                median_value = 0.0;
    std::size_t           samples      = 0;
    std::filesystem::path source_dll;        // gefundene reale perm.dll
    std::filesystem::path shipped_dll;       // Kopie im out_dir
    std::filesystem::path manifest_path;     // geschriebenes Manifest
    std::string           dll_build_version; // Inhalt des .version-Sidecars (System-Provenienz, z.B. cowfix-v1)
    std::string           dll_algos;         // Inhalt des .algos-Sidecars (Organ-Provenienz, algo_sig; leer wenn keins)
};

/// ShippedArtifactBuilder (Builder): validierter Zusammenbau (Kopie → Version → Manifest).
/// artifact_name = Basisname des Versand-Artefakts (z.B. "best_lookup"); out_dir wird angelegt.
class ShippedArtifactBuilder {
public:
    ShippedArtifactBuilder(std::filesystem::path out_dir, std::string artifact_name)
        : out_dir_(std::move(out_dir)), artifact_name_(std::move(artifact_name)) {}

    /// Baut + schreibt das Artefakt. error gefüllt bei Fehlschlag → nullopt.
    [[nodiscard]] std::optional<ShippedArtifact>
    build(RankedBinary const& winner, Metric metric, std::filesystem::path const& source_dir, std::string& error) const;

private:
    std::filesystem::path out_dir_;
    std::string           artifact_name_;
};

} // namespace comdare::cache_engine::best_binary

#endif // COMDARE_CACHE_ENGINE_BEST_BINARY_SELECTOR_HPP
