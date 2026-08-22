#pragma once
// pruefling_stempel_farben -- I-5 (P-H/#89, KON112-08 (d)+(e), Mechanik ENTSCHIEDEN 2026-08-21):
// die Stempel-"FARBEN" der Paper-Prueflinge als ACHSEN-TOKENS in der ORGAN-Stempel-Zeile.
//
// OWNER-R-1 (17.08., PV-4): die Paper-Prueflinge erhalten "EIGENE VOLLE LAGER-IDENTITAET mit
// eigenen Stempeln in den 'Farben' der neuen Achsen; deren Achsen muessen ALLEN
// metaprogrammatischen Anforderungen EXAKT genuegen, sonst COMPILE-TIME-ERROR."
//
// KOLLISIONS-AUFLOESUNG (KON112-08, Design-Punkt): D1/Owner-E2 02.08. ("Merge-Zeile kann daher
// nicht existieren") vs volle Lager-Identitaet -> die Pruefling-Achswerte erscheinen als
// ACHSEN-TOKENS in der bestehenden ORGAN-Zeile ("achse=wert@X.Y.Z", compose_organ_stamp_line),
// es entsteht KEINE vierte merge-Zeile (A13-M3/C3 bleibt bindend; SotaStampLines bleibt das
// 3-Zeilen-Trio MESS/SYSTEM/ORGAN).
//
// MECHANIK: die deklarierten Farb-Tokens (paper_pruefling_registry.hpp, kPaperFarbTokens --
// heute die 3 ABSTRAKTEN Marker-Prueflinge P08/P09/P33) werden ueber DIESELBE Versions-Tabelle
// aufgeloest wie jeder adhoc-/Katalog-Stempel (build_axis_variant_version_table -> Registry-
// Wrapper name()+algo_version). Damit gilt die CT-Pflicht (e) transitiv: die Tabelle selbst
// erzwingt algo_version compile-time (axis_variant_version_table.hpp, mp_for_each-Zugriff);
// ein Farb-Token auf einen NICHT existenten Wrapper faellt als 0.0.0-Sentinel auf und ist
// ctest-VERBOTEN (test_ph89_paper_prueflinge). KEINE zweite Zeilen-Ableitung (O-8-Lehre):
// compose_organ_stamp_line ist derselbe geteilte Emitter-Helfer wie im lazy-/Katalog-Pfad.
//
// VOLLE Prueflinge (30/33): ihre Farben SIND die volle Komposition; deren Organ-Zeile haengt
// am dokumentierten SOTA-METADATEN-BLOCKER (sota_catalog.hpp Kopf sota_stamp_lines, K-3-REST)
// -> hier EHRLICH leere Organ-Zeile statt einer Phantom-Ableitung.
//
// ADDITIV: bestehende sota_stamp_lines-Aufrufer bleiben byte-identisch; paper_stamp_lines ist
// der NEUE Einstieg des (nach-Trigger verdrahteten) Paper-Experiment-Emissionspfads.
//
// C++23, header-only, ASCII-only. Registry-SCHWER (zieht via sota_catalog/Versions-Tabelle alle
// Achsen-Registries) -- nur in Facade-/Test-TUs einbinden, NIE in schlanke TUs.

#include "paper_pruefling_registry.hpp"
#include "sota_catalog.hpp" // SotaStampLines + compose_organ_stamp_line + abi::system_stamp_line

#include <boost/mp11.hpp> // mp_iota_c/mp_at_c (registered_farb_version_table)

#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace farb_token_slot_wachen {
// CT-Pflicht (e), Slot-Haelfte: jeder deklarierte Farb-Token-Achsen-Name ist ein REALER
// kCompositionAxisNames-Slot (Single-Source der 18 Slots; ein Tippfehler bricht die Kompilation,
// nicht erst die Stempel-Zeile).
[[nodiscard]] consteval bool farb_token_achsen_sind_slots() {
    for (auto const& t : kPaperFarbTokens) {
        bool gefunden = false;
        for (auto const& slot : ex::kCompositionAxisNames)
            if (t.achse == slot) gefunden = true;
        if (!gefunden) return false;
    }
    return true;
}
static_assert(farb_token_achsen_sind_slots(),
              "I-5: jeder Farb-Token muss einen realen kCompositionAxisNames-Slot benennen.");
} // namespace farb_token_slot_wachen

/// registered_farb_version_table -- die {axis,variant->version}-Tabelle ueber die VOLLE
/// REGISTRIERTE Organ-Population (axes26_registered::R00..R26, Enabled UND Default-OFF).
///
/// WARUM REGISTRIERT STATT ENABLED (Befund am Objekt, 21.08.): der P33-Farb-Token vampir_nfp
/// ist ein Default-OFF-Registry-Eintrag (axis_06_allocator_registry.hpp "P33-VAMPIR Option A --
/// Default-OFF, end append"); in der Enabled-Tabelle (build_axis_variant_version_table) fehlt
/// er in einem Default-Bau und faellt auf den 0.0.0-Sentinel. Die LAGER-IDENTITAET des
/// Prueflings (Owner-R-1: "im Pruefling verbucht und versioniert") haengt aber am REGISTRIERTEN
/// ANGEBOT, nicht am Enable-Schalter eines konkreten Baus -- dieselbe Lesart, mit der die
/// CX-W6-Voll-Registry-Wache die Versions-Grammatik ueber die VOLLE Population erzwingt
/// (guard_all_registered_organ_versions laeuft in jedem Tabellen-Bau mit).
/// KEIN zweiter Ableitungsweg (O-8-Lehre): DIESELBEN geteilten Helfer (reflect_versions +
/// kCompositionAxisNames + AllRegisteredOrganAxisLists, deren 18er-Ordnung per static_assert an
/// kCompositionAxisNames gebunden ist) -- nur die Population ist die registrierte.
/// EMISSIONS-Zeit bleibt getrennt: der (paper_ref, table)-Overload unten nimmt weiterhin die
/// Tabelle des KONKRETEN Baus (ein Bau ohne freigeschaltete Pruefling-Achse stempelt sie
/// ehrlich als Sentinel -- genau das faengt der ctest-Sentinel-Check der Emissionsseite).
[[nodiscard]] inline std::vector<ex::AxisVariantVersion> registered_farb_version_table() {
    std::vector<ex::AxisVariantVersion> t;
    boost::mp11::mp_for_each<boost::mp11::mp_iota_c<ex::kCompositionAxisNames.size()>>([&](auto index) {
        using List = boost::mp11::mp_at_c<ex::AllRegisteredOrganAxisLists, decltype(index)::value>;
        ex::reflect_versions<List>(ex::kCompositionAxisNames[decltype(index)::value], t);
    });
    return t;
}

/// paper_farb_achsen -- die deklarierten (achse, wert)-Paare EINES Papers (leer fuer volle
/// Prueflinge und unbekannte paper_refs: kein Phantom-Token). Reihenfolge = Deklarations-
/// Reihenfolge; die kanonische Slot-Ordnung stellt compose_organ_stamp_line selbst her.
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> paper_farb_achsen(std::string_view paper_ref) {
    std::vector<std::pair<std::string, std::string>> out;
    for (auto const& t : kPaperFarbTokens)
        if (t.paper_ref == paper_ref) out.emplace_back(std::string{t.achse}, std::string{t.wert});
    return out;
}

/// paper_organ_stempel_zeile -- die ORGAN-Zeile eines Paper-Prueflings aus seinen Farb-Tokens,
/// aufgeloest ueber DIESELBE Versions-Tabelle wie alle anderen Stempel-Pfade (kein zweiter
/// Ableitungsweg). Leer, wenn das Paper keine deklarierten Farb-Tokens traegt (volle
/// Prueflinge: K-3-REST; unbekannte refs: kein Phantom).
[[nodiscard]] inline std::string paper_organ_stempel_zeile(std::string_view                           paper_ref,
                                                           std::vector<ex::AxisVariantVersion> const& table) {
    auto const achsen = paper_farb_achsen(paper_ref);
    if (achsen.empty()) return {};
    return ex::compose_organ_stamp_line(achsen, table);
}

/// paper_organ_stempel_zeile (IDENTITAETS-Form) -- die ORGAN-Zeile aus der REGISTRIERTEN
/// Population (registered_farb_version_table oben): die deklarierte Lager-Identitaet des
/// Prueflings, unabhaengig vom Enable-Schalter eines konkreten Baus. Der Sentinel-Check der
/// Tests laeuft gegen DIESE Form (ein 0.0.0 hier == der Farb-Token benennt KEINEN registrierten
/// Wrapper == Deklarations-Regression).
[[nodiscard]] inline std::string paper_organ_stempel_zeile(std::string_view paper_ref) {
    auto const achsen = paper_farb_achsen(paper_ref);
    if (achsen.empty()) return {};
    return ex::compose_organ_stamp_line(achsen, registered_farb_version_table());
}

/// paper_stamp_lines -- das VOLLE 3-Zeilen-Trio (MESS/SYSTEM/ORGAN) eines Paper-Prueflings:
/// Organ = Farb-Tokens (oben), System = abi::system_stamp_line() (dieselbe Quelle wie
/// sota_stamp_lines), Mess = die vom Aufrufer gereichte Combo-Zeile. KEINE merge-Zeile --
/// der Rueckgabetyp SotaStampLines kennt strukturell keine (Owner-E2/A13-M3-C3).
[[nodiscard]] inline SotaStampLines paper_stamp_lines(std::string_view                           paper_ref,
                                                      std::vector<ex::AxisVariantVersion> const& table,
                                                      std::string const&                         measurement_stamp) {
    return SotaStampLines{paper_organ_stempel_zeile(paper_ref, table), abi::system_stamp_line(), measurement_stamp};
}

} // namespace comdare::cache_engine::thesis_lazy
