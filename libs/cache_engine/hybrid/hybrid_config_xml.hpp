#pragma once
// HY-A3 -- DER PARSER der <hybrid_tier>-Sektion: der ZWEIWEIG-Schalter und der XML-Override des
// Dock-Deckels.
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH
// ============================================================================================
// Owner-E1 (02.08.2026): "Die hybrid-Tier-Binaries fahren eine Zwischenloesung zwischen statischen
// Pruef-Docks und austauschbaren plain Tier-Binaries je Pruefdock, das ist eine XML Konfiguration
// auf Wunsch des anwenders in der Auswertungsphase."
//
// KON42-01 (Default-Doktrin): der constexpr-Default lebt im Planer, JEDE XML-Eingabe ueberschreibt
// ihn. Fuer max_docks ist dieser Parser genau der angekuendigte Mechanismus -- der Kopf von
// heuristik_adapter_synthese_matrix.hpp nennt ihn seit dem 13.08. namentlich als HY-A3-Posten.
//
// ============================================================================================
// DER ZWEIWEIG -- was `enabled` wirklich entscheidet
// ============================================================================================
// Je Kette gilt die XOR-Regel (NT 05.08.-mittag-7): ENTWEDER der direkte Tier-Aufbau ODER der
// Hybrid-Mehrfach-Aufbau, nie beides. Die beiden Ausgaenge sind sachlich verschieden
// (Ledger-Stichwort "zwei Ausgaenge", ueber den Bauplan Abschnitt II belegt): Stufe 1 = homogene Last,
// EINE beste Binary je Schnitt, direkt drangehaengt --
// kein Hybrid noetig. Stufe 2 = gemischte Lasten, multiple optimale Binaries je Last-Kanal, der
// Hybrid waehlt zur Laufzeit.
//
// DAS IST KEIN SCHALTER FUER GESCHMACK, SONDERN EINER FUER DIE BINARY-MENGE: der Hybrid-Zweig
// MULTIPLIZIERT sie (Owner-GO-3 08.08.2026, ueber den Bauplan Abschnitt II belegt). Deshalb ist der
// Default AUS und die Abwesenheit der
// Sektion kein Fehler -- wer den Zweig will, sagt es.
//
// ============================================================================================
// FAIL-CLOSED, DURCHGEHEND -- und warum das hier nicht verhandelbar ist
// ============================================================================================
// Jedes unbekannte Token ist ein benannter Fehler, KEIN Default. Das Haus hat die Gegenprobe
// bezahlt: run_methodology_for_ids liess bis zur Welle B/1 einen Tippfehler still auf measure
// zurueckfallen und loeste eine Messung aus, die niemand angefordert hatte
// (run_methodology_registry.hpp:170-173). Hier waere die Folge groesser: ein still abgeschalteter
// Hybrid-Zweig liefert eine vollstaendige, plausible, FALSCHE Messreihe.
//
// UNTERSCHIEDEN wird trotzdem zwischen ABWESENHEIT und FALSCHHEIT -- das eine ist eine Aussage,
// das andere ein falsch geschriebener Wunsch. Fehlt die Sektion, gilt der direkte Zweig (kein
// Fehler); steht dort `enabled="yes"`, ist es einer.
//
// ============================================================================================
// WAS DIESER PARSER BEWUSST NICHT TUT
// ============================================================================================
// (1) KEINE XSD-AENDERUNG. Der XSD-Typ HybridTierType bleibt Kommentar-Reserve (soll_design
//     Abschnitt 4); die Validierung lebt per Haus-Doktrin im C++-Validator. Die XSD liegt zudem im
//     SUPER-Repo und wird von dieser Stufe nicht editiert.
// (2) KEIN ANSCHLUSS AN validate_profile. Der ist die Naht zum Bestands-Ladeweg und gehoert in
//     denselben Schritt wie die Registry-Seite.
// (3) KEIN REGISTRY-EINTRAG IN DIESER DATEI. [NACHGEFUEHRT 18.08.2026, A2.5/seg1-04: hier stand
//     bis zum #15-Bruch "Er haengt an der offenen Owner-Frage E-6 ('22->23') ... lieber offen als
//     falsch". E-6 ist per KON118 BEANTWORTET: die "22->23" war ein PHANTOM-NENNER -- es stand nie
//     eine Achsen-Registry auf 22 (AllStrategies = 22 TYPEN, Achsen-Aggregat = 26,
//     cache_engine_axis_registry.xml = 18 axis, system_axis_registry.xml = 3 axis). Der ECHTE
//     Registry-Anteil war kGenusBuildSlotCounts 5->6, und der IST GEBAUT: genus_build_admission.hpp
//     traegt den sechsten Eintrag (Reroute-Genus, Aritaet = Dock-Deckel aus der Hybrid-Bindung)
//     samt static_assert size() == 6.] Dieser Parser traegt weiterhin keinen Eintrag -- die
//     Registry-Wahrheit wohnt in der Admission, nicht im XML-Leser.
// (4) KEINE eigene axis_error-Domaene. Der Bauplan nennt sie als HY-A3-Stichwort und benennt
//     zugleich, dass die Fehlerklassen-Vollstaendigkeit je Algorithmus heute ungedeckt ist
//     (organ_axis_error_classes.hpp:82-84). Diese Stufe fuehrt ihre Fehler im errno-Stil der
//     Nachbarschicht; die Domaenen-Anbindung ist als offene Naht benannt, nicht vorgetaeuscht.
//
// @autoritaet Owner-Entscheid E1 02.08.2026 + KON42-01 + KON28-03
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 4
// @muster profile_facade/planner/experiment_dock_payload.hpp:204-232 (common-DOM, optional-Rueckgabe)

#include "anatomy/anatomy_base.hpp"
#include "heuristik_adapter_klassifikation.hpp"
#include "heuristik_adapter_synthese_matrix.hpp" // kHybridNodeObergrenzeDefault
#include "hybrid_dock_contract.hpp"

#include <serialization/xml_config_parser/xml_reader.hpp> // comdare::common::xml::parse_document / XmlNode

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE GELESENE KONFIGURATION
// ------------------------------------------------------------------------------------------

/// HybridTierConfig -- was die <hybrid_tier>-Sektion aussagt.
struct HybridTierConfig {
    /// DER ZWEIWEIG-SCHALTER. false == diese Kette faehrt den direkten Tier-Zweig.
    bool enabled = false;
    /// Die Gattung der Huelle. Ein Hybrid je Gattung (Owner-GO Q6), deshalb genau EIN Wert.
    anatomy::AnatomyGenus genus = anatomy::AnatomyGenus::SearchAlgorithm;
    /// true == StatischeDockArrayPolicy (Compile-Zeit-Kapazitaet), false == RuntimeDockArrayPolicy.
    bool storage_statisch = true;
    /// Der wirksame Deckel dieser Kette. Default == kHybridNodeObergrenzeDefault, XML ueberschreibt.
    std::size_t max_docks = kHybridNodeObergrenzeDefault;
    /// Die Bestueckung. Leer ist zulaessig (eine deklarierte, aber unbestueckte Sektion).
    std::vector<DockContractDescriptor> docks;
};

/// faehrt_hybrid_zweig -- die XOR-Frage in einer Funktion. Sie existiert, damit der Aufrufer nicht
/// selbst `cfg.enabled` interpretiert und die Regel an N Stellen nachbaut.
[[nodiscard]] constexpr bool faehrt_hybrid_zweig(HybridTierConfig const& c) noexcept { return c.enabled; }

/// Das Parse-Ergebnis: Wert ODER Grund. Ein nacktes std::optional traegt den GRUND nicht -- und
/// "nicht da" saehe dann genauso aus wie "kaputt". Genau diese Unterscheidung entscheidet aber, ob
/// eine Bestands-XML weiterlaeuft oder der Lauf abbricht.
struct HybridTierParseErgebnis {
    std::optional<HybridTierConfig> config;
    int                             status = hybrid_status_ok;

    [[nodiscard]] bool                    has_value() const noexcept { return config.has_value(); }
    [[nodiscard]] HybridTierConfig const& operator*() const noexcept { return *config; }
    [[nodiscard]] HybridTierConfig const* operator->() const noexcept { return &*config; }
};

/// Der Grund des letzten Parse-Vorgangs. hybrid_status_ok bei Erfolg UND bei ABWESENHEIT der
/// Sektion -- Abwesenheit ist kein Fehlerzustand.
[[nodiscard]] constexpr int letzter_parse_status(HybridTierParseErgebnis const& e) noexcept { return e.status; }

// ------------------------------------------------------------------------------------------
// (2) TOKEN-AUFLOESUNG -- jede aus IHRER Einzelquelle
// ------------------------------------------------------------------------------------------

namespace detail {

/// Genus-Token -> Genus. Die Auswahl kommt aus kAlleGenera und genus_name(), NICHT aus einer
/// eigenen Liste: eine zweite Aufzaehlung driftet, sobald ein Genus dazukommt -- exakt der Fehler,
/// den heuristik_adapter_klassifikation.hpp fuer die Namens-Seite behebt.
[[nodiscard]] inline std::optional<anatomy::AnatomyGenus> genus_of_token(std::string_view token) noexcept {
    if (token.empty()) return std::nullopt;
    for (auto const g : kAlleGenera) {
        if (anatomy::genus_name(g) == token) return g;
    }
    return std::nullopt;
}

/// Ein striktes bool: NUR "true"/"false". Kein "1", kein "yes", kein "True".
/// Die Wire-Form ist eng, weil jede Aufweichung eine stille Fehl-Lesart erlaubt.
[[nodiscard]] inline std::optional<bool> strenges_bool(std::string_view token) noexcept {
    if (token == "true") return true;
    if (token == "false") return false;
    return std::nullopt;
}

/// Eine strikte Groessenangabe: NUR Ziffern, kein Vorzeichen, kein Leerraum, nicht leer.
/// std::stoul waere hier falsch -- es akzeptiert "8abc" und fuehrendes Minus.
[[nodiscard]] inline std::optional<std::size_t> strenge_zahl(std::string_view token) noexcept {
    if (token.empty() || token.size() > 19) return std::nullopt;
    std::size_t wert = 0;
    for (char const c : token) {
        if (c < '0' || c > '9') return std::nullopt;
        wert = wert * 10 + static_cast<std::size_t>(c - '0');
    }
    return wert;
}

} // namespace detail

// ------------------------------------------------------------------------------------------
// (3) DER PARSER
// ------------------------------------------------------------------------------------------

/// lies_hybrid_tier_knoten -- der Kern: ein bereits gefundener <hybrid_tier>-Knoten -> Konfiguration.
/// Getrennt von der Dokument-Ebene, damit derselbe Code beide Einstiege bedient (ganzes Dokument
/// oder blosses Fragment) und die Regeln nicht zweimal existieren.
[[nodiscard]] inline HybridTierParseErgebnis lies_hybrid_tier_knoten(comdare::common::xml::XmlNode const& knoten) {
    HybridTierConfig cfg;

    // (a) enabled -- fehlend == aus (Aussage), falsch == hart (falsch geschriebener Wunsch).
    if (knoten.has_attr("enabled")) {
        auto const an = detail::strenges_bool(knoten.attr("enabled"));
        if (!an) return {std::nullopt, hybrid_status_unbekanntes_token};
        cfg.enabled = *an;
    }

    // (b) genus -- PFLICHT. Ein Hybrid-Gehaeuse ohne Gattung waere ein Gehaeuse ohne Ziel; die
    //     Gattung ist zugleich der Lager-Sortierschluessel und die Dock-Zuordnung.
    // F-5: FEHLT und FALSCH sind zwei verschiedene Anwenderfehler und bekommen zwei Codes.
    // has_attr wird hier genauso benutzt wie oben fuer enabled -- die Unterscheidung war schon
    // moeglich, sie wurde nur nicht gezogen.
    if (!knoten.has_attr("genus")) return {std::nullopt, hybrid_status_genus_fehlt};
    auto const g = detail::genus_of_token(knoten.attr("genus"));
    if (!g) return {std::nullopt, hybrid_status_unbekanntes_token};
    cfg.genus = *g;
    // Das Gate der Stufe, hier an der XML-Kante: hinter einem Klassifikations-Genus steht keine
    // plain Tier-Binary. Die Sperre darf nicht davon abhaengen, auf welchem Weg der Wert entsteht.
    if (!ist_abi_sichtbares_genus(cfg.genus)) return {std::nullopt, hybrid_status_kein_zielfaehiges_genus};

    // (c) <dock_array> -- optional, SOLANGE der Hybrid-Zweig nicht angefordert ist.
    //
    // W12-PFLICHTANGABE: wer den Hybrid-Zweig ANFORDERT (enabled="true"), MUSS max_docks
    // deklarieren. Das ist kein Widerspruch zu KON42-01 ("constexpr-Default im Planer, jede
    // XML-Eingabe ueberschreibt"), sondern dessen zweite Haelfte: der Default lebt im PLANER und
    // traegt den abgeschalteten Fall; wer den Zweig anfordert, sagt AUCH, wie breit er ihn haben
    // will. Der Grund ist die Mengenwirkung -- der Hybrid-Zweig multipliziert die Binary-Menge,
    // und die Dock-Zahl ist der Faktor. Ein stillschweigendes 32 waere hier die teuerste
    // Voreinstellung des Hauses, getroffen von niemandem.
    // Bei enabled="false" bleibt der Deckel schlicht der Default: es wird nichts gebaut, also
    // gibt es auch nichts zu deklarieren.
    if (cfg.enabled) {
        auto const* da = knoten.child("dock_array");
        if (da == nullptr || !da->has_attr("max_docks")) return {std::nullopt, hybrid_status_max_docks_fehlt};
    }
    if (auto const* da = knoten.child("dock_array"); da != nullptr) {
        if (da->has_attr("storage")) {
            std::string const s = da->attr("storage");
            if (s == "static") {
                cfg.storage_statisch = true;
            } else if (s == "runtime") {
                cfg.storage_statisch = false;
            } else {
                return {std::nullopt, hybrid_status_unbekanntes_token};
            }
        }
        if (da->has_attr("max_docks")) {
            auto const n = detail::strenge_zahl(da->attr("max_docks"));
            // 0 traegt kein Dock; ueber dem Programm-Deckel wird NICHT per XML unterlaufen. Der
            // Deckel selbst (32) bleibt zulaessig -- die Grenze ist inklusiv (KON28-03 "maximal 32").
            if (!n || *n == 0 || *n > kHybridNodeObergrenzeDefault)
                return {std::nullopt, hybrid_status_max_docks_ungueltig};
            cfg.max_docks = *n;
        }
    }

    // (d) <docks> -- die Bestueckung. Jeder Eintrag geht durch DIESELBE Deskriptor-Pruefung wie ein
    //     im Code gebauter (pruefe_dock_deskriptor) -- eine zweite Regel-Fassung hier waere die
    //     Stelle, an der XML- und Code-Weg auseinanderliefen.
    if (auto const* docks = knoten.child("docks"); docks != nullptr) {
        for (auto const* d : docks->children_named("dock")) {
            auto const vertrag = hybrid_dock_contract_lookup(d->attr("contract"));
            if (!vertrag) return {std::nullopt, hybrid_status_unbekannter_vertrag};

            DockContractDescriptor const desc{static_cast<std::uint8_t>(*vertrag), cfg.genus};
            if (int const st = pruefe_dock_deskriptor(desc); st != hybrid_status_ok) return {std::nullopt, st};
            cfg.docks.push_back(desc);
        }
    }

    // (e) Die beiden Zahlen der Sektion muessen zueinander passen. Eine Bestueckung, die den selbst
    //     gesetzten Deckel sprengt, ist in sich widerspruechlich; sie erst beim attach auffallen zu
    //     lassen hiesse, den Fehler eine Stufe zu spaet und an der falschen Stelle zu melden.
    if (cfg.docks.size() > cfg.max_docks) return {std::nullopt, hybrid_status_mehr_docks_als_deckel};

    return {std::move(cfg), hybrid_status_ok};
}

/// parse_hybrid_tier_xml -- ein Fragment, dessen WURZEL <hybrid_tier> ist.
[[nodiscard]] inline HybridTierParseErgebnis parse_hybrid_tier_xml(std::string_view xml) {
    std::optional<comdare::common::xml::XmlNode> const root = comdare::common::xml::parse_document(xml);
    if (!root || root->tag != "hybrid_tier") return {std::nullopt, hybrid_status_xml_nicht_wohlgeformt};
    return lies_hybrid_tier_knoten(*root);
}

/// finde_hybrid_tier_sektion -- ein GANZES Dokument; die Sektion ist ein optionales Kind der Wurzel.
///
/// ABWESENHEIT IST KEIN FEHLER. Das ist der Kern der Byte-Neutralitaets-Zusage (soll_design
/// Abschnitt 4: additives OPTIONALES Element, minOccurs=0, "damit alle Bestands-XMLs valide
/// bleiben"). Deshalb liefert der Fall {kein Wert, hybrid_status_ok} -- und NICHT einen Fehlerstatus,
/// der eine Bestands-XML abweisen wuerde.
/// Ein nicht wohlgeformtes Dokument ist dagegen sehr wohl einer.
[[nodiscard]] inline HybridTierParseErgebnis finde_hybrid_tier_sektion(std::string_view xml) {
    std::optional<comdare::common::xml::XmlNode> const root = comdare::common::xml::parse_document(xml);
    if (!root) return {std::nullopt, hybrid_status_xml_nicht_wohlgeformt};

    // Auch das Dokument SELBST darf die Sektion sein (Fragment-Aufruf ueber diesen Einstieg).
    if (root->tag == "hybrid_tier") return lies_hybrid_tier_knoten(*root);

    auto const* sektion = root->child("hybrid_tier");
    if (sektion == nullptr) return {std::nullopt, hybrid_status_ok}; // nicht da == direkter Zweig
    return lies_hybrid_tier_knoten(*sektion);
}

} // namespace comdare::cache_engine::hybrid
