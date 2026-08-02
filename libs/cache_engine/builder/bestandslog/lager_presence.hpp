#pragma once
// lager_presence.hpp -- G3 / #46b Lagerhaltung, Folge-A (T3-Naht).
//
// Die NAHT zwischen dem geladenen Lager-Index (LagerRunState, I1) und dem Miss-Scan des Planers
// (PresenceFn, I1b). Sie beantwortet genau eine Frage: "liegt die Binary zum view-Index i schon im
// Lager?" -- und zwar OHNE dass eine der beteiligten Schichten die andere kennenlernt:
//
//   view_index  --(IndexKeyFn)-->  128-hex-Fingerprint  --(+ Zelle, LagerRunState)-->  present?
//
// SEAM-ONLY: der Iterator und die StaticBinaryView werden NICHT angefasst. make_index_key_fn ist ueber
// den View-Typ TEMPLATISIERT und duck-typed (verlangt nur View::operator[](i) mit einem .binary_id-
// Member) -> dieser Header inkludiert experiment_tree.hpp nicht, zieht die Baum-Schicht nicht in die
// Bestandslog-Abhaengigkeiten und bleibt gegen jede kompatible View testbar. Die Aufloesung ist
// CT-rein: der Typ steht am Instanziierungspunkt fest, es gibt keinen Runtime-Switch und keine vtable.
//
// §66-N3-Reinheit: hier findet ausschliesslich RT->RT statt. binary_id (Runtime-String) -> Fingerprint
// (Runtime-String) -> Zell-Koordinaten (Runtime-Strings) -> bool. Nichts davon wird zu einem CT-Typ
// hochgezogen; die einzige CT-Groesse ist der View-TYP selbst, und der kommt von oben herein.
//
// KONSERVATIVER MISS (die tragende Regel): jede Unaufloesbarkeit -- kein Host-Provider gesetzt, der
// Provider liefert nullopt (kein .fingerprint-Sidecar), das Hex ist ungueltig -- ergibt present==false,
// also "fehlt". Das ist die sichere Seite: ein falsches "fehlt" kostet einen ueberfluessigen Bau, ein
// falsches "vorhanden" wuerde eine Binary unterschlagen. Seit Lager-TP1(B)/G-A2 ist die PresenceFn
// der BAU-FILTER des Planer-getriebenen Slice-Baus (bekannte Bestands-Treffer werden VOR dem
// Bau-Versuch uebersprungen, LEDGER:3397); dll_is_current bleibt die zweite, lokale
// Verteidigungslinie fuer alles, was gebaut wird -- der konservative Miss traegt genau deshalb: er
// kann nie eine fehlende Binary unterschlagen, nur einen Skip verschenken.
// PRAEZISIERUNG (TP1-N2/B-1): "present == vorhanden" stuetzt sich auf die SCHREIB-DISZIPLIN der
// Registrierungs-Seite -- ein Bestands-Eintrag entsteht erst NACH dem Push-Drain und unter
// Ausschluss gescheiterter Pushes (Iterator: bestandslog_flush nach push_pump->close(),
// discard_fresh_with_pfad_prefix). Nur weil dort nichts Unbestaetigtes ins Dokument gelangt, darf
// der Filter hier einem Eintrag glauben, ohne den Store selbst zu befragen.
//
// DEFAULT-NEUTRAL: ohne BinaryIdKeyFn liefert make_index_key_fn immer nullopt, damit meldet die
// PresenceFn durchweg "fehlt", damit ist missing_count == Fenstergroesse -- exakt das Verhalten des
// Ist-Pfades ohne Lager (byte-identisch, die Byte-Wachen bleiben ohne Erwartungs-Update gruen).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), stdlib. std::function ist hier zulaessig:
// die Naht laeuft VOR dem Bau (Planer-Thread), nie im gemessenen Hot-Path.

#include "builder_registration.hpp" // LagerRunState::lager_contains (T2)
#include "planer_driven_build.hpp"  // PresenceFn (die Ziel-Signatur des Planer-Miss-Scans)

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

// Host-Provider: binary_id -> 128-hex-Fingerprint. Das ist DERSELBE Provider-Gedanke wie I2s
// FingerprintFn (der Host komponiert ihn aus den Stempel-Zeilen bzw. liest das .fingerprint-Sidecar);
// diese Naht bleibt dadurch stamp-/env-blind. nullopt = kein Fingerprint bekannt.
using BinaryIdKeyFn = std::function<std::optional<std::string>(std::string const& binary_id)>;

// Dieselbe Frage, nur ueber den view_index gestellt -- die Form, die der Planer-Miss-Scan braucht.
using IndexKeyFn = std::function<std::optional<std::string>(std::size_t view_index)>;

// Hebt einen BinaryIdKeyFn auf die view_index-Ebene: i -> view[i].binary_id -> Fingerprint.
//
// Duck-typed ueber View (nur operator[](std::size_t) mit .binary_id-Member noetig). Leerer Provider ->
// immer nullopt (default-neutral). Die View wird per REFERENZ gehalten: sie muss die zurueckgegebene
// Funktion ueberleben (im Iterator lebt sie ueber den ganzen provision-Lauf -- laenger als der Planer).
template <class View>
[[nodiscard]] IndexKeyFn make_index_key_fn(View const& view, BinaryIdKeyFn by_binary_id) {
    return [&view, by_binary_id = std::move(by_binary_id)](std::size_t i) -> std::optional<std::string> {
        if (!by_binary_id) return std::nullopt; // kein Provider -> nichts aufloesbar (default-neutral)
        return by_binary_id(view[i].binary_id);
    };
}

// Baut die PresenceFn: present(i) == (Fingerprint zu i aufloesbar) UND (Tupel aus Fingerprint und Zelle
// liegt im geladenen Lager). Jede Unaufloesbarkeit -> false (konservativer Miss, s. Kopf).
//
// zelle sind die drei Zell-Koordinaten DIESES Bau-Laufs (§62-NACHTRAG-4). Sie sind je Lauf KONSTANT --
// combo/opt/simd stecken in der per-Perm build_version bzw. der Prozess-Umgebung, waehrend der Lauf
// eine Perm ueber die ganze View baut. Der Lager-Index dagegen traegt ALLE Zellen: erst die Abfrage
// entscheidet, welche gemeint ist. Genau deshalb ist die Zelle ein Parameter dieser Fabrik und kein
// Feld des Index.
//
// Der Zustand wird per REFERENZ gehalten (er lebt im Iterator ueber den ganzen Bau). lager_contains ist
// thread-sicher (T2) -> die zurueckgegebene Funktion darf aus dem Planer-Thread laufen, waehrend die
// Build-Worker observe() rufen.
[[nodiscard]] inline PresenceFn make_lager_presence(LagerRunState const& state, IndexKeyFn key_of,
                                                    ZellKoordinaten zelle) {
    return [&state, key_of = std::move(key_of), zelle = std::move(zelle)](std::size_t i) -> bool {
        if (!key_of) return false; // kein Schluessel-Weg -> konservativer Miss
        auto const hex = key_of(i);
        if (!hex) return false; // Fingerprint unbekannt -> konservativer Miss
        return state.lager_contains(*hex, zelle);
    };
}

} // namespace comdare::cache_engine::builder::bestandslog
