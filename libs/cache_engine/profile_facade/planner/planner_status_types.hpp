#pragma once
// profile_facade/planner/planner_status_types.hpp -- W5 (2026-08-05, Owner-R5): die FLACHEN PODs der
// status-Naht zwischen Planer-App und Fassade.
//
// WARUM EIN EIGENER HEADER: profile_run_facade.hpp ist laut eigenem Kopf-Kommentar umbrella-FREI zu halten --
// jeder Konsument dieser Fassade (auch der CEB-Treiber im super-Repo) uebersetzt, was sie inkludiert. Die
// beiden status-Naht-Typen sind reine PODs; der eigentliche Status-LESER (planner_status_reader.hpp) zieht
// dagegen Bestandslog- und Sidecar-Header und gehoert damit NICHT in die Fassaden-Kette, sondern nur in die
// App und die TU. Der Schnitt haelt diese Trennung: PODs an der Naht, Substanz dahinter.
//
// ANSPRUCHSLOS: header-only C++23, NUR stdlib, ASCII.

#include <cstddef>
#include <string>
#include <vector>

namespace comdare::cache_engine::planner {

/// Sentinel-Doktrin: ein Pflichtfeld entfaellt nie; was nicht belegt ist, sagt das mit DIESEM Wort.
inline constexpr char kMarkerUnbelegt[] = "unbelegt";

/// Suffix des Fingerprint-Sidecars (write_fingerprint_sidecar legt <binary>.fingerprint an).
inline constexpr char kFingerprintSidecarSuffix[] = ".fingerprint";

/// Die drei Datei-Namen des per-Binary-Mess-Resume-Standes (Iterator-Substanz, hier nur GELESEN).
inline constexpr char kResultCsvName[]   = "result.csv";
inline constexpr char kResultStampName[] = "result.csv.stamp";
inline constexpr char kResultStaleName[] = "result.csv.stale";

/// Maximale Verzeichnistiefe unter einem Perm-Verzeichnis. Der Leser laeuft auf Anwender-Zuruf ueber einen
/// Baum, den er nicht kontrolliert -- eine Kappe verhindert, dass ein versehentlich verschachteltes oder
/// zyklisch verlinktes Verzeichnis das Kommando festhaelt.
inline constexpr std::size_t kMaxScanTiefe = 8;

/// Maximale Zahl besuchter Verzeichnis-EINTRAEGE je Perm-Verzeichnis. Die Tiefen-Kappe allein macht den Lauf
/// NICHT bounded: ein flacher Baum mit Millionen Geschwistern liegt vollstaendig unter kMaxScanTiefe und
/// haelt das on-demand-Kommando trotzdem beliebig lange fest. Erst BEIDE Kappen zusammen loesen die
/// "bounded"-Zusage ein. Der Deckel ist grosszuegig gegen den realen Ist (ein Mess-Fenster traegt Tausende,
/// nicht Hunderttausende Eintraege) -- greift er dennoch, sagt der Bericht es LITERAL (scan_gekappt=ja)
/// statt still eine zu kleine Bilanz zu melden.
inline constexpr std::size_t kMaxScanEintraege = 200000;

// ---------------------------------------------------------------------------------------------------------------
// Die Format-Fakten der Mess-Dateien -- von der Fassade aus der Iterator-Substanz hereingereicht.
// ---------------------------------------------------------------------------------------------------------------
struct MessFormatFakten {
    std::string csv_header; ///< == lazy_csv_header() ohne abschliessende Zeilenumbrueche
    std::string rows_key;   ///< == kLazyResumeRowsKey ("|rows=")

    [[nodiscard]] bool vollstaendig() const noexcept { return !csv_header.empty() && !rows_key.empty(); }
};

// ---------------------------------------------------------------------------------------------------------------
// SOLL -- die Plan-Sicht (gefuellt von der Fassaden-Seite aus dem deterministischen Director-Walk)
// ---------------------------------------------------------------------------------------------------------------
struct PlanZelleSoll {
    std::string ceb;               ///< [a,b,c] -- die Mess-Tooling-Konfig (CEB-Typ), EIGENES Feld
    std::string ceb_slug;          ///< cmake_slug(ceb) == der Verzeichnisname unter <root>
    std::size_t perm_index = 0;    ///< 0-basierter Perm-Index des Walks (== perm<idx> im Emissions-Pfad)
    std::string zelle;             ///< [d,e,f][g,h,i] -- System-Perm + Organ-Referenz
    std::size_t plan_schritte = 0; ///< Schritte des Director-Walks unter dieser Perm (Stufe-1-Zahl)
};

struct PlanSollSicht {
    bool                       erhoben = false; ///< false = der Walk lief nicht (Profil unlesbar o.ae.)
    std::string                grund;           ///< wenn !erhoben: warum (ehrlich, nie leer bei !erhoben)
    std::string                source_kind;
    std::string                profile_id;
    std::string                profile_basename;
    std::size_t                perm_count              = 0;
    std::size_t                measurement_combo_count = 0;
    std::vector<PlanZelleSoll> zellen;
};

} // namespace comdare::cache_engine::planner
