// steuerdock.hpp -- B-14 DIE SECHS STEUERDOCKS Planer<->CEB (S-8-Kopf, T-NEU-6).
// =============================================================================================
// TRAEGER-STUFE 1 (PLANNER), Include-Wurzel <traeger/planner/...> -- Neubau nach DESIGN #29
// Par. 4.3; zieht nur std + die eigene Stufe (kein TRAEGER-BRUECKE-Marker noetig).
//
// DER VERTRAG (Owner-KERN 09.08., Ledger Z.17140-17206; Wellenplan Par. 19.2 B-14):
//   Der Steuerungskanal Planer->CEB passt sich VARIADISCH an die 3! vorhandenen CEB-Versionen
//   an. Der Planer haelt darum SECHS STEUERDOCKS -- je CEB-Version eines --, und jedes Dock
//     * passt COMPILE-HART GENAU auf die tatsaechlich EINKOMPILIERTEN Messeinrichtungen
//       dieser CEB (nicht auf die moeglichen), und
//     * gibt NUR Steuerbefehle frei, die laut Plan tatsaechlich existieren: ein Befehl, den
//       diese CEB nicht kennt, ist am Dock NICHT AUFRUFBAR (kein Laufzeit-Fehler -- kein Code).
//   Der Kanal traegt ZWEI RECHTSAKTE, und die Begriffe sind NICHT austauschbar:
//     FREIGABE der System-Achse   (Systemachsen werden freigegeben)
//     DURCHSETZUNG der Organ-Achse (Organachsen werden durchgesetzt)
//   RELEASE NUR GESAMMELT: Logging CEB -> Planer-CLI erfolgt GESAMMELT, VOR oder NACH der
//   Gesamt-Messung -- nie waehrend (ein Sendevorgang erzeugt genau die Latenz, die gemessen
//   wird; kontaminierte Daten sind die unheilbare Klasse).
//
// ABGRENZUNGEN, DAMIT KEINE ALTE VERWECHSLUNG WIEDER EINZIEHT:
//   * KON41-03 ("VOELLIG FALSCH"): STEUERDOCKS != MESS-PERMUTATIONEN. Der Mess-Nenner (einst
//     "32 aus 5 Schaltern", seit 15.08. DYNAMISCH, nur S-19 rechnet ihn) ist von der Dock-Zahl
//     ENTKOPPELT. Diese Datei traegt darum KEINEN Mess-Nenner und importiert keinen.
//   * KON34-03: die 6 zaehlt REIHENFOLGEN der Tooling-Haupt-Kette {Wallclock, Makro, Mikro}
//     am Steuerkanal -- nicht Instrument-Belegungen. Der dormante System-B-Bestand
//     (Code/02_messung_driver mess::Konfiguration + steuer_dock.hpp:229-241, super-Repo) traegt
//     dieselbe 3!-Zaehlung als anzahl==6-Riegel; seine Umstellung (Typliste = statische
//     Anordnungs-Freigabe, "anzahl==6 faellt") ist der C-7(a)-Strang (#24/B4) im super-Repo
//     und wird HIER weder vorweggenommen noch beruehrt.
//   * R-1/OV-10 (OFFENE OWNER-FRAGE, Ledger): Teilmengen-Lesart "max. 4 erreichbar" vs. dieser
//     6er-KERN. C-6 mandatiert die 6; gebaut ist Lesart 2 des KERNs (die Docks decken auch die
//     unerreichbaren Varianten ab = VOLLSTAENDIGKEIT AM DOCK, kein Gate-Modell-Umbau). Faellt
//     der Owner-Entscheid anders, ist die Registry-Aufzaehlung der EINE Ort der Zahl.
//
// ASCII-only, Zeilen <= 120.

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::traeger::planner {

// =====================================================================================
// DIE TOOLING-HAUPT-KETTE -- der Gegenstand, dessen 3! Anordnungen die CEB-Versionen sind.
// =====================================================================================

enum class ToolingInstrument : unsigned char {
    Wallclock = 0, // CEB-Stufe (misst UM die Tier-Arbeit herum)
    Makro     = 1, // Tier-Stufe, Makro-Messung
    Mikro     = 2, // Tier-Stufe, Mikro-Messung
};

// Die kanonische Grund-Anordnung (Basis der Permutations-Aufzaehlung). Diese Liste ist die
// EINZIGE Instrument-Aufzaehlung dieser Datei -- die Dock-Zahl ENTSTEHT aus ihr (3! = 6),
// sie steht nirgends als Literal in der Registry.
inline constexpr std::array<ToolingInstrument, 3> kToolingHauptKette = {
    ToolingInstrument::Wallclock,
    ToolingInstrument::Makro,
    ToolingInstrument::Mikro,
};

[[nodiscard]] constexpr std::string_view tooling_instrument_token(ToolingInstrument i) noexcept {
    switch (i) {
        case ToolingInstrument::Wallclock: return "wallclock";
        case ToolingInstrument::Makro: return "makro";
        case ToolingInstrument::Mikro: return "mikro";
    }
    return "?"; // unerreichbar bei gueltigem Enum; kein throw in constexpr-Pfad noetig
}

// EINE CEB-Version = EINE Anordnung der Tooling-Haupt-Kette (KON34-03: permutiert wird die
// ORDNUNG einer vollzaehligen Kette, nie eine Teil-Belegung).
struct CebVersion {
    std::array<ToolingInstrument, 3> anordnung = kToolingHauptKette;

    [[nodiscard]] constexpr bool operator==(CebVersion const&) const noexcept = default;

    // Deterministisches Kennzeichen (z. B. "wallclock>makro>mikro") -- fuer Logs und Registry-Schluessel.
    [[nodiscard]] std::string kennzeichen() const {
        std::string s;
        for (std::size_t i = 0; i < anordnung.size(); ++i) {
            if (i != 0) { s += '>'; }
            s += tooling_instrument_token(anordnung[i]);
        }
        return s;
    }
};

// =====================================================================================
// DIE ZWEI RECHTSAKTE -- getrennte Typen OHNE gemeinsame Basis und OHNE Konversion:
// eine Freigabe KANN nicht als Durchsetzung reisen und umgekehrt (Owner: "nicht austauschbar").
// =====================================================================================

struct FreigabeSystemAchse {
    std::string system_achse; // WELCHE System-Achse freigegeben wird (Freigabe des Experiment-Baumes)
};

struct DurchsetzungOrganAchse {
    std::string organ_achse; // WELCHE Organ-Achse durchgesetzt wird
};

// =====================================================================================
// RELEASE NUR GESAMMELT -- die Mess-Integritaets-Regel als Typform.
// Das Enum kennt VOR und NACH der Gesamt-Messung. Ein "WAEHREND" ist NICHT REPRAESENTIERBAR --
// die verbotene Form hat keinen Wert, den man versehentlich waehlen koennte.
// =====================================================================================

enum class ReleaseFenster : unsigned char {
    VorGesamtMessung  = 0,
    NachGesamtMessung = 1,
};

// Der gesammelte Release: die GESAMTE Log-Sammlung CEB -> Planer-CLI in EINEM Akt, mit benanntem
// Fenster. Es gibt bewusst KEINE Einzel-Sende-API an den Docks -- wer released, released alles.
struct GesammelterRelease {
    ReleaseFenster           fenster = ReleaseFenster::NachGesamtMessung;
    std::vector<std::string> log_zeilen; // die gesammelten CEB-Logzeilen (Reihenfolge = Entstehungsfolge)
};

// =====================================================================================
// DAS STEUERDOCK -- variadischer Kern. Die Template-Parameter SIND die einkompilierten
// Messeinrichtungen dieser CEB: das Dock einer anders bestueckten CEB ist ein ANDERER TYP.
// =====================================================================================

template <class Einrichtung, class... Einkompiliert>
inline constexpr bool ist_einkompiliert = (std::same_as<Einrichtung, Einkompiliert> || ...);

template <class... Einkompiliert>
class Steuerdock {
public:
    constexpr explicit Steuerdock(CebVersion version) noexcept : version_(version) {}

    [[nodiscard]] constexpr CebVersion const& version() const noexcept { return version_; }

    // "passt genau auf vorhandene einkompilierte Messeinrichtungen": compile-time-Antwort, ob
    // dieses Dock die Einrichtung E anbietet. KEIN Laufzeit-Lookup.
    template <class E>
    [[nodiscard]] static constexpr bool bietet_an() noexcept {
        return ist_einkompiliert<E, Einkompiliert...>;
    }

    // RECHTSAKT 1: FREIGABE der System-Achse -- nur fuer einkompilierte Einrichtungen aufrufbar.
    // Ein Befehl an eine fremde Einrichtung ist KEIN Fehlerpfad, sondern KEIN CODE (requires).
    template <class E>
        requires(ist_einkompiliert<E, Einkompiliert...>)
    constexpr void freigabe(FreigabeSystemAchse const& akt) {
        protokoll_.push_back("freigabe:" + akt.system_achse);
    }

    // RECHTSAKT 2: DURCHSETZUNG der Organ-Achse -- dieselbe Passform-Regel. Es gibt ABSICHTLICH
    // keine Ueberladung freigabe(DurchsetzungOrganAchse) / durchsetzung(FreigabeSystemAchse):
    // die Rechtsakte sind nicht austauschbar, der Tausch ist ein Compile-Fehler.
    template <class E>
        requires(ist_einkompiliert<E, Einkompiliert...>)
    constexpr void durchsetzung(DurchsetzungOrganAchse const& akt) {
        protokoll_.push_back("durchsetzung:" + akt.organ_achse);
    }

    // RELEASE NUR GESAMMELT: nimmt den EINEN gesammelten Akt entgegen. Einzelzeilen-Senden
    // existiert an diesem Typ nicht (keine API dafuer -- unrepresentierbar statt verboten).
    void release(GesammelterRelease const& gesammelt) {
        for (auto const& zeile : gesammelt.log_zeilen) { protokoll_.push_back("release:" + zeile); }
    }

    [[nodiscard]] std::vector<std::string> const& protokoll() const noexcept { return protokoll_; }

private:
    CebVersion               version_;
    std::vector<std::string> protokoll_; // S-8-Kopf: Schablonen-Protokoll; echte Naht-Anbindung = S-9/S-10
};

// =====================================================================================
// DIE REGISTRY -- zaehlt die CEB-Versionen deterministisch auf (lexikografische Permutation
// der Grund-Anordnung). Die SECHS ist hier ERGEBNIS, nie Eingabe: kein 6-Literal im Code.
// =====================================================================================

class SteuerdockRegistry {
public:
    // Alle CEB-Versionen in deterministischer (lexikografischer) Reihenfolge. Zwei Aufrufe
    // liefern byte-gleiche Folgen -- der Contract-Test prueft genau das plus den fremden
    // Nenner 3! (er zaehlt selbst per std::next_permutation, nicht ueber diese Funktion).
    [[nodiscard]] static std::vector<CebVersion> alle_ceb_versionen() {
        std::array<ToolingInstrument, 3> a = kToolingHauptKette;
        std::sort(a.begin(), a.end()); // lexikografischer Start der Aufzaehlung
        std::vector<CebVersion> out;
        do { out.push_back(CebVersion{a}); } while (std::next_permutation(a.begin(), a.end()));
        return out;
    }

    [[nodiscard]] static std::size_t dock_zahl() { return alle_ceb_versionen().size(); }
};

} // namespace comdare::traeger::planner
