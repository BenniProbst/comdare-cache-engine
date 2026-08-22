// traeger_rakete.hpp -- TEMPLATE-METHOD-GERUEST der TRAEGER-RAKETE (S-8-Kopf, T-NEU-6; B-17-Doktrin).
// =============================================================================================
// TRAEGER-STUFE 1 (PLANNER), Include-Wurzel <traeger/planner/...> -- Neubau nach DESIGN #29
// Par. 4.3; zieht nur std (kein TRAEGER-BRUECKE-Marker noetig).
//
// DIE DOKTRIN (Owner verbatim 11.08., Ledger KON16-03/-04/KON25; Memory "KEINE YAML"):
//   "Es gibt KEINE YAML sondern der Planer emittiert direkt eine pipe oder Prozess, der den
//    build auf einer binary faehrt. Jede Traeger-Stufe emittiert die naechste direkt und unter
//    Verwendung eines zentralen Bau-Modules, welches wiederverwendbar im Builder Pattern den
//    naechsten Traeger aufbaut." -- und zur Tiefe: "die Tiefe ist nur 3; Hybrid und Tier werden
//    sequentiell auf derselben Stufe gebaut, erst Tier und spaeter Hybrid, durch die CEB."
//
//   Also:   Planer (1) --emittiert--> CEB (2) --emittiert--> Tier, DANN Hybrid (beide Stufe 3)
//   ORT != ZEIT: Tier und Hybrid stehen auf DERSELBEN Stufe (Ort), reisen aber NACHEINANDER
//   (Zeit) -- beides ist unten getrennt kodiert (emissions_folge = Zeitfolge AN einem Ort).
//
// KEINE YAML -- BY CONSTRUCTION, nicht per Verbotsschild: das Formen-Enum EmissionsForm kennt
//   NUR Prozess und Pipe. Eine YAML-Emission ist in diesem Typsystem NICHT REPRAESENTIERBAR
//   (der Contract-Test prueft per requires-Probe, dass EmissionsForm::Yaml nicht existiert).
//   Der CiYamlBuilder des Monolithen bleibt davon unberuehrt -- er ist per KON25-04 der Weg
//   fuer den LOKALEN Build ausserhalb der CI, nicht der Traeger der Ketten-Emission.
//
// TEMPLATE METHOD (GoF): das GERUEST unten ist die fixe Schrittfolge (Folge bestimmen ->
//   je Gegenstand BAUEN ueber das zentrale Bau-Modul -> VERIFIZIEREN -> weiterreichen);
//   die konkrete Stufe liefert NUR die Hooks. Das Geruest ist non-virtual und final der
//   Basisklasse -- eine Stufe kann die Reihenfolge nicht umdeuten.
//   Der Abschluss der ECHTEN Kette ist der LAGER-Fund (KON18) -- der gehoert zur S-12-Emission
//   (#3-Strang) und ist hier als Hook-Vertrag (verifiziere_emission) vorbereitet, nicht gebaut.
//
// ABGRENZUNG: dies ist das GERUEST (Typen + Ablauf-Schablone). Die realen Bau-Kommandos der
//   Stufen (CEB-Compile, Tier-DLL-Emission, Hybrid-Bau durch die CEB nach B-10) verdrahten
//   S-9/S-10/S-12 gegen genau diese Flaeche.
//
// ASCII-only, Zeilen <= 120.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::traeger::planner {

// Die drei Raketen-Stufen (Tiefe 3 -- Owner-Wort). Stufe 3 traegt Tier UND Hybrid (ORT),
// gebaut wird dort sequentiell (ZEIT, s. emissions_folge).
enum class TraegerRaketenStufe : unsigned char {
    Planer    = 1,
    Ceb       = 2,
    TierStufe = 3, // Ort "Stufe 3": Tier und Hybrid wohnen hier; der Name traegt den Ort,
                   // die ZEIT-Folge (erst Tier, dann Hybrid) steht in emissions_folge(Ceb).
};

// WAS eine Stufe emittiert (die Gegenstaende der naechsten Stufe).
enum class EmissionsGegenstand : unsigned char {
    Ceb    = 0, // von Stufe 1 (Planer) emittiert
    Tier   = 1, // von Stufe 2 (CEB) emittiert -- ZUERST
    Hybrid = 2, // von Stufe 2 (CEB) emittiert -- DANACH (B-10: beide durch die CEB, sequentiell)
};

// WIE emittiert wird: Prozess oder Pipe. KEIN Yaml-Mitglied -- das ist die Doktrin als Typform.
enum class EmissionsForm : unsigned char {
    Prozess = 0, // direkt gestarteter Bau-Prozess
    Pipe    = 1, // die Pipe = das Flaeche-1-CONTROL-INTERFACE der CEB (KON50-Praezisierung)
};

// Das Emissions-Produkt einer Stufe: der Auftrag, den das zentrale Bau-Modul ausgefuehrt hat.
struct EmissionsProdukt {
    EmissionsForm       form       = EmissionsForm::Prozess;
    EmissionsGegenstand gegenstand = EmissionsGegenstand::Ceb;
    std::string         bau_kommando; // der Prozess-/Pipe-Auftrag (Schablone; real ab S-9/S-12)
};

// -- Die STUFENKETTE als reine Funktion: wer kommt nach wem. Terminal = leeres Optional. --
[[nodiscard]] constexpr std::optional<TraegerRaketenStufe> naechste_stufe(TraegerRaketenStufe s) noexcept {
    switch (s) {
        case TraegerRaketenStufe::Planer: return TraegerRaketenStufe::Ceb;
        case TraegerRaketenStufe::Ceb: return TraegerRaketenStufe::TierStufe;
        case TraegerRaketenStufe::TierStufe: return std::nullopt; // Tiefe 3: hier endet die Rakete
    }
    return std::nullopt;
}

// -- Die EMISSIONS-FOLGE je Stufe: WAS sie emittiert, in FIXER Reihenfolge (ZEIT-Ordnung). --
//    Planer -> [Ceb] ; Ceb -> [Tier, Hybrid] (erst Tier, spaeter Hybrid); Stufe 3 -> [] (terminal).
//    span auf statische Arrays: die Folge ist Programm-Konstante, kein Laufzeit-Zustand.
namespace detail {
inline constexpr std::array<EmissionsGegenstand, 1> kFolgePlaner = {EmissionsGegenstand::Ceb};
inline constexpr std::array<EmissionsGegenstand, 2> kFolgeCeb    = {EmissionsGegenstand::Tier,
                                                                    EmissionsGegenstand::Hybrid};
} // namespace detail

[[nodiscard]] constexpr std::span<EmissionsGegenstand const> emissions_folge(TraegerRaketenStufe s) noexcept {
    switch (s) {
        case TraegerRaketenStufe::Planer: return detail::kFolgePlaner;
        case TraegerRaketenStufe::Ceb: return detail::kFolgeCeb;
        case TraegerRaketenStufe::TierStufe: return {};
    }
    return {};
}

// =====================================================================================
// DAS ZENTRALE BAU-MODUL (Builder Pattern, wiederverwendbar) -- Owner: "unter Verwendung
// eines zentralen Bau-Modules". EINE Bau-Flaeche fuer ALLE Stufen; die Stufen reichen ihre
// Auftraege hierdurch, statt je eigene Bau-Logik zu tragen. Das Modul ist absichtlich ein
// Interface: die S-12-Emission (#3) setzt hier den realen Prozess-/Pipe-Starter ein.
// =====================================================================================

class IEmissionsBauModul {
public:
    virtual ~IEmissionsBauModul() = default;

    // Baut den naechsten Traeger fuer den benannten Gegenstand. KEINE YAML-Form moeglich --
    // die Rueckgabe traegt EmissionsForm, und das Enum kennt nur Prozess/Pipe.
    [[nodiscard]] virtual EmissionsProdukt baue(EmissionsGegenstand gegenstand, std::string_view auftrag) = 0;
};

// Die Schablonen-Implementierung: emittiert Prozess-Auftraege (deterministisch, seiteneffektfrei).
// Real wird hier ab S-12 der Prozess/Pipe-Start verdrahtet; die Tests fahren gegen diese Form.
class ProzessBauModul final : public IEmissionsBauModul {
public:
    [[nodiscard]] EmissionsProdukt baue(EmissionsGegenstand gegenstand, std::string_view auftrag) override {
        EmissionsProdukt p;
        p.form       = EmissionsForm::Prozess;
        p.gegenstand = gegenstand;
        p.bau_kommando.assign(auftrag);
        return p;
    }
};

// =====================================================================================
// DIE TEMPLATE METHOD -- das Geruest der Rakete. CRTP: die konkrete Stufe steckt als
// <Konkret, S> fest und liefert zwei Hooks:
//     std::string bau_auftrag(EmissionsGegenstand)        -- WAS gebaut werden soll
//     void verifiziere_emission(EmissionsProdukt const&)  -- Abschluss-Pruefung (LAGER-Fund ab S-12)
// Das Geruest fixiert: Folge bestimmen -> je Gegenstand bauen -> sofort verifizieren ->
// Produkte in Folge-Reihenfolge zurueck. Eine Terminal-Stufe HAT diese Methode nicht
// (requires-Klausel): "Stufe 3 emittiert" ist ein Compile-Fehler, kein Laufzeit-Ast.
// =====================================================================================

template <class Konkret, TraegerRaketenStufe S>
class TraegerRaketenGeruest {
public:
    static constexpr TraegerRaketenStufe stufe = S;

    // DIE Template-Method. Non-virtual; die Schrittfolge ist nicht ueberschreibbar.
    [[nodiscard]] std::vector<EmissionsProdukt> emittiere_naechste(IEmissionsBauModul& bau_modul)
        requires(!emissions_folge(S).empty())
    {
        std::vector<EmissionsProdukt> produkte;
        auto const                    folge = emissions_folge(S); // FIXE Zeit-Folge (Ceb: erst Tier, dann Hybrid)
        produkte.reserve(folge.size());
        for (EmissionsGegenstand const g : folge) {
            EmissionsProdukt p = bau_modul.baue(g, self().bau_auftrag(g)); // zentrales Bau-Modul
            self().verifiziere_emission(p);                                // sofort, je Emission
            produkte.push_back(std::move(p));
        }
        return produkte;
    }

private:
    [[nodiscard]] Konkret& self() noexcept { return static_cast<Konkret&>(*this); }
};

} // namespace comdare::traeger::planner
