#pragma once
// hybrid/hybrid_stempel_kette.hpp -- G3/A-03: die RT-SEITE der SHA256-Verkettung (KON47-02) und
// DIE INVARIANTE RT <= CT (KON45-01(6)).
//
// ============================================================================================
// WAS DIESE DATEI IST -- die zweite Haelfte der Komposit-Zeile
// ============================================================================================
// Die CT-Haelfte lebt beim Grammatik-Eigentuemer (abi/anatomy_fingerprint.hpp): das Glied [9]
// (KompositMapGlied), DIE EINE Bildung (hybrid_komposit_map_bilden) und DIE EINE Lesung
// (komposit_map_wert_bei). Sie friert zur Compile-Zeit ein, WELCHE Tier-Binaries an WELCHEN
// Zellen stecken: {stufen_id -> anatomy_name_hex des Tiers}.
//
// Die RT-Haelfte ist der Owner-Kern KON47-02 (12.08.2026), woertlich: "laesst sich durch caching
// der vollen Stempel der Tier-Binaries an den Pruefdocks zum init des Hybrids mit den Tier-Binary
// modules auch von der CEB ueber die Flaeche 2 zur Laufzeit abfragen." Der Cache selbst liegt am
// DockSlot (hybrid_dock_array.hpp: stempel/stufen_id, stempel_binden/stempel_von); HIER liegt die
// WACHE darueber, dass beide Haelften EINE Wahrheit erzaehlen.
//
// ============================================================================================
// DIE INVARIANTE, EXAKT: RT <= CT -- Teilmenge, nicht Gleichheit
// ============================================================================================
// Die Laufzeit-Belegung darf WENIGER Zellen tragen als die CT-Map (ein Hybrid kann unterbestueckt
// laufen -- dieselbe Richtung, die hybrid_binary_proxy.hpp:172-175 fuer die Slot-ZAHL festhaelt:
// "sie darf nie mehr"). Sie darf aber NIE eine Zelle tragen, die die CT-Map nicht kennt, und nie
// ein ANDERES Tier an einer bekannten Zelle: beides hiesse, die einkompilierte Identitaet (das
// Preimage-Glied [9], also der Fingerprint UND der Name des Hybrids) luegt ueber das, was zur
// Laufzeit tatsaechlich steckt -- die Falscher-Skip-Klasse, gegen die das ganze Glied gebaut ist.
//
// Drei benannte Verstoesse (hybrid_dock_contract.hpp, Codes 13-15):
//   rt_bindung_unvollstaendig -- Antrieb XOR Stempel am Slot: der Init ist halb. KON47-02 bindet
//       das Caching AN den Init; nach dessen Ende ist ein halber Slot ein Fehler, kein Zustand.
//   rt_ct_key_fehlt           -- die belegte stufen_id steht nicht in der CT-Map (auch: ueber dem
//       Key-Deckel -- ein solcher Key KANN in keiner grammatischen Map stehen, der Fall ist
//       derselbe Fehler und kein eigener).
//   rt_ct_wert_differiert     -- Key da, Wert anders: am Dock steckt ein anderes Tier als das
//       eingefrorene (oder der POD traegt keinen lesbaren Namen).
//
// ============================================================================================
// FORMNEUTRALITAET (F2-Vorlage P5) -- warum hier KEINE Hash-Laenge steht
// ============================================================================================
// Der Vergleich laeuft Byte fuer Byte zwischen zwei Sichten DERSELBEN Quelle-Art: name_line des
// gecachten POD gegen den Wert der CT-Map. Beide tragen die Form, die die EINE Grammatik-Wache
// (komposit_glied_ist_grammatisch) und der EINE Namens-Hash (anatomy_name_hex) festlegen -- heute
// SHA-256/64-hex (KON103-03). Dreht die offene P5-Frage die Form, drehen Quelle und Map an ihren
// Eigentuemern; dieser Vergleich wandert unveraendert mit. Eine hier festgenagelte 64 waere eine
// ZWEITE Form-Wahrheit gewesen.
//
// @doku Wellenplan 19.1 A-03 + KON45-01(6) + KON47-02 + V-04R (Ebenen-Wrap/Flaeche-2-Durchreichung)

#include "hybrid_dock_array.hpp"    // DockArray/DockSlot -- der Cache-Traeger
#include "hybrid_dock_contract.hpp" // hybrid_status_* -- die EINE Status-Heimat

#include <cache_engine/abi/anatomy_fingerprint.hpp>        // komposit_map_wert_bei + Key-Deckel
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // AnatomyVersionLines (volle Deklaration)

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::hybrid {

/// Das Ergebnis der Invarianten-Pruefung: der ERSTE Verstoss mit seinem Slot, oder ok.
/// Ein struct statt int + Out-Parameter: der Slot ist Teil der Antwort, nicht Beiwerk -- ohne ihn
/// meldete die Wache "irgendwo", und der Anwender suchte an 32 Docks.
struct RtCtPruefergebnis {
    int         status = hybrid_status_ok; ///< hybrid_status_ok oder der erste Verstoss (13..15)
    std::size_t slot   = 0;                ///< Slot des Verstosses; bei ok ohne Aussage
};

/// hybrid_rt_ct_invariante_pruefen(ct_komposit_glied, docks) -- DIE WACHE der Invariante RT <= CT.
///
/// ct_komposit_glied ist die CT-Wahrheit des PRUEFLINGS -- als PARAMETER, nicht als Header-Zugriff
/// auf kHybridKompositGlied: das Define gehoert der jeweiligen Uebersetzungseinheit der
/// Hybrid-Binary; ein Header, der es selbst laese, truege in jeder anderen TU eine andere
/// "Wahrheit" (genau die Zwei-Wahrheiten-Klasse, die die K-1-Traeger schliessen). Der Aufrufer
/// reicht seine Zeile herein -- im Modul kHybridKompositGlied, im Test die Probe-Map.
///
/// WANN RUFEN: am ENDE des Hybrid-Init (nach dock_anlegen + beiden Bind-Aspekten je Dock) und vor
/// jeder Identitaets-Auskunft ueber Flaeche 2. Unbelegte Slots sind KEIN Verstoss (Teilmenge);
/// vollstaendig ungebundene, aber belegte Slots ebenfalls nicht (ein angelegtes, noch leeres Dock
/// ist der dokumentierte Zwischenzustand von attach).
///
/// KEIN noexcept, und das ist ehrlich: der Key-Text-Weg der EINEN Lesung wirft designgemaess ueber
/// dem Key-Deckel (FAIL-LOUD der Grammatik). Diese Wache PRUEFT den Deckel vorab und meldet
/// rt_ct_key_fehlt statt zu werfen -- aber der Vertrag "wirft nie" gehoert der Grammatik, nicht
/// dieser Signatur; ein noexcept hier wuerde bei einer kuenftigen Grammatik-Aenderung still zu
/// std::terminate eskalieren statt laut nicht zu uebersetzen.
template <class Policy>
[[nodiscard]] RtCtPruefergebnis hybrid_rt_ct_invariante_pruefen(std::string_view          ct_komposit_glied,
                                                                DockArray<Policy> const& docks) {
    for (std::size_t i = 0; i < docks.kapazitaet(); ++i) {
        DockSlot const* const s = docks.slot(i);
        if (s == nullptr) continue; // unbelegt: RT ist Teilmenge, fehlende Zellen sind erlaubt
        bool const hat_antrieb = s->antrieb != nullptr;
        bool const hat_stempel = s->stempel != nullptr;
        if (hat_antrieb != hat_stempel) return {hybrid_status_rt_bindung_unvollstaendig, i};
        if (!hat_stempel) continue; // belegt, aber beidseitig ungebunden: attach-Zwischenzustand
        if (s->stufen_id > abi::kAnatomyFingerprintKompositKeyDeckel)
            return {hybrid_status_rt_ct_key_fehlt, i}; // kann in keiner grammatischen Map stehen
        std::string_view const ct_wert = abi::komposit_map_wert_bei(ct_komposit_glied, s->stufen_id);
        if (ct_wert.empty()) return {hybrid_status_rt_ct_key_fehlt, i};
        if (s->stempel->name_line == nullptr) return {hybrid_status_rt_ct_wert_differiert, i};
        std::string_view const rt_name{s->stempel->name_line, static_cast<std::size_t>(s->stempel->name_len)};
        if (rt_name != ct_wert) return {hybrid_status_rt_ct_wert_differiert, i};
    }
    return {hybrid_status_ok, 0};
}

} // namespace comdare::cache_engine::hybrid
