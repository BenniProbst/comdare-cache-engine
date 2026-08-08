#pragma once
// tools/mess_report/skip_manifest.hpp -- A9-S4, vierter Owner-Nachtrag Punkt 2: SKIP bei
// validem Bestand fuer exakt dieselbe Binary.
//
// Owner woertlich (08.08.): "Die operation bei Validen Messdaten ist skip fuer die XLSX, sofern
// von der exakt gleichen binary gemessen wird. Jede neue Version dieser Binary erzeugt auch neue
// Messdaten fuer die geupdateten Eigenschaften der binary und behaelt die alte Version
// zusaetzlich." Binary-Identitaet = binary_id-String (dieselbe Gruppierungs-Spalte wie in
// mess_report_render.hpp -- im Erstbeleg-Archiv real eine Achsen-Kompositions-Kette, keine
// Fingerprint-Hex; die Skip-Regel bindet an die Identitaet, die dieses Werkzeug ohnehin schon
// als Gruppierungs-Schluessel kennt, nicht an eine zweite, hier neu erfundene ID).
//
// WARUM EIN SIDECAR-MANIFEST STATT DER PRODUKTIONS-FINGERPRINT-KETTE (dll_is_current,
// bestand_lager_skips, LagerBaumWriter<MessdatenRealmPolicy>): keiner dieser drei Wege hat einen
// PRODUKTIONS-Konsumenten (LagerBaumWriter wird real nirgends konstruiert, nur in Tests;
// MessdatenBaumSpec verlangt die volle Mess-/System-/18-Organ-Achsen-Belegung + einen fertigen
// 128-hex-Fingerprint -- Daten, die dieses CLI aus einer rohen Mess-CSV nicht besitzt und nicht
// neu herleiten darf, "ein zweiter Preimage waere genau die Drift, die A13-M3 geschlossen hat",
// lager_baum_writer.hpp:31-36). Ein eigenes Sidecar-Manifest ist dasselbe HAUS-MUSTER wie
// build_orchestrator/fingerprint_sidecar.hpp (ein schlankes, werkzeug-eigenes Bestandsdokument
// neben der eigentlichen Ausgabe) und bindet an GENAU die Identitaet, die dieses Werkzeug bereits
// kennt -- kein zweiter Preimage, kein Raten einer Achsen-Belegung, die es nicht hat.
//
// "CSV wird NIE verwendet" (Owner, 7x wiederholt) heisst: CSV ist kein Weg, um MESS-ERGEBNISSE zu
// erhalten. Dieses Manifest ist KEIN Mess-Ergebnis, sondern reine Werkzeug-Buchfuehrung (wie ein
// .fingerprint-Sidecar) -- es widerspricht der Direktive nicht. Es wird deshalb bewusst NUR im
// xlsx-Pfad gefuehrt: ein --format=csv-Testlauf liest/schreibt das Manifest NICHT (Testlaeufe
// duerfen den echten Skip-Zustand weder lesen noch veraendern).
//
// DIES IST EINE BEWUSSTE, VORLAEUFIGE GRENZE, KEIN BEHELFSWEG (Lead-Review 08.08., bestaetigt):
// der LagerBaumWriter<MessdatenRealmPolicy> IST der SPAETERE Zielort dieses Werkzeugs, sobald es
// die volle Mess-/System-/18-Organ-Achsen-Belegung + einen fertigen 128-hex-Fingerprint besitzt
// (z.B. weil ein vorgelagerter Schritt sie mitliefert, statt dass dieses CLI sie aus der rohen CSV
// raet). Bis dahin traegt dieses Sidecar-Manifest die Skip-Semantik. Wer dieses Manifest spaeter
// als "provisorisch, kann weg" liest, ohne den LagerBaumWriter-Weg vorher gebaut zu haben, entfernt
// die einzige Skip-Wache, die es dann gibt -- ERST der Umzug, DANN das Entfernen.
//
// ATOMAR: tmp + rename (Messdaten-nie-loeschen-Doktrin -- ein Absturz mitten im Schreiben darf
// das Manifest nicht halb-kaputt hinterlassen; bestehende Eintraege werden NIE entfernt, nur
// ergaenzt).

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::tools::mess_report {

inline constexpr std::string_view kManifestDateiname = ".mess_report_manifest";

/// Liest das Manifest. Fehlt die Datei (frisches --out, allererster Lauf) -> ehrlich leer, kein
/// Fehler.
[[nodiscard]] inline std::set<std::string> lies_manifest(std::filesystem::path const& out_dir) {
    std::set<std::string> out;
    std::ifstream         in(out_dir / std::string{kManifestDateiname}, std::ios::binary);
    if (!in.is_open()) return out;
    std::string zeile;
    while (std::getline(in, zeile)) {
        if (!zeile.empty() && zeile.back() == '\r') zeile.pop_back();
        if (!zeile.empty()) out.insert(zeile);
    }
    return out;
}

/// Ergaenzt das Manifest um `neue_binary_ids` -- additiv, bestehende Eintraege bleiben
/// unangetastet ("die alte Version bleibt zusaetzlich"). No-op bei leerer Liste (kein Anlegen
/// eines leeren Verzeichnisses/Manifests ohne echten Anlass -- lazy, wie der Owner es fuer den
/// gesamten Lager-Baum verlangt).
inline void ergaenze_manifest(std::filesystem::path const& out_dir, std::vector<std::string> const& neue_binary_ids) {
    if (neue_binary_ids.empty()) return;
    std::set<std::string> bestand = lies_manifest(out_dir);
    for (auto const& b : neue_binary_ids) bestand.insert(b);

    std::filesystem::create_directories(out_dir);
    std::filesystem::path const ziel = out_dir / std::string{kManifestDateiname};
    std::filesystem::path const tmp  = out_dir / (std::string{kManifestDateiname} + ".tmp");
    {
        std::ofstream aus(tmp, std::ios::binary | std::ios::trunc);
        for (auto const& b : bestand) aus << b << '\n';
    }
    std::error_code ec;
    std::filesystem::rename(tmp, ziel, ec);
    if (ec) { // Rename ueber Dateisystem-Grenzen kann fehlschlagen -- Kopieren + Aufraeumen als Rueckfall.
        std::filesystem::copy_file(tmp, ziel, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(tmp);
    }
}

} // namespace comdare::cache_engine::tools::mess_report
