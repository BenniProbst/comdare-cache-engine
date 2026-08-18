#pragma once
// hybrid/hybrid_binary_proxy.hpp -- HY-A2 (F8-MINIMAL-DoD): der PROXY einer Hybrid-Tier-Binary.
//
// WAS ER IST: die eine Klasse, die eine Hybrid-.so nach aussen zu einer ganz normalen Tier-Binary
// macht. Sie erfuellt IAnatomyBase, haelt intern das Dock-Array (HY-A1) und leitet den Antrieb an
// das Ziel durch, das an einem Dock steckt. Der Host merkt davon nichts -- und genau das ist die
// Zusage.
//
// ============================================================================================
// WEG C, UND WARUM genus() NICHT DEN REROUTE-GENUS LIEFERT
// ============================================================================================
// Owner-Entscheid E-1 final (09.08.2026), am Enum selbst ausformuliert (anatomy_base.hpp:135-169):
//   KLASSIFIKATIONS-Ebene (Weg A) -- AnatomyGenus::FunctionInterfaceReroute. Sie sortiert die
//     Hybrid-.so im Lagerbaum und benennt die ART des Reroutes. Eigenschaft des ARTEFAKTS.
//   INTERFACE-Ebene (Weg C)       -- genus() liefert das GEERBTE ZIEL-Genus (z.B. SearchAlgorithm).
//     Der Pass-through ist nach aussen transparent.
// Der naheliegende Fehler waere, aus genus() den Reroute-Wert zurueckzugeben: dann suchte der Host
// ein Pruef-Dock fuer einen Genus, der keines hat, und die Registry muesste von 5 auf 6 wachsen.
// SIE BLEIBT 5. Diese Datei ist die Stelle, an der das entschieden wird, deshalb steht die
// Begruendung hier und nicht nur im Ledger.
//
// ============================================================================================
// DIE CT-SPERRE: EIN NICHT DEKLARIERTES ZIEL BRICHT LAUT
// ============================================================================================
// Ein Reroute-Ziel ist NICHT jeder Genus, den das Enum kennt. Zwei Klassen fallen weg:
//   (a) Klassifikations-Genera (heute: FunctionInterfaceReroute selbst). Ein Hybrid, der auf einen
//       Hybrid-Genus umleitet, haette kein ABI-Interface am Ende der Kette -- Gate S1,
//       hybrid_status_kein_zielfaehiges_genus.
//   (b) Genera, die dieser Bau nicht deklariert hat. Der F8-Minimal-Schnitt deklariert GENAU EINEN
//       (SearchAlgorithm); ein zweiter kommt hinzu, indem jemand unten eine Spezialisierung
//       schreibt -- nicht, indem er einen Enum-Wert einsetzt und hofft.
// Beide Faelle brechen COMPILE-TIME mit benanntem Text. Das ist die Doktrin "erst laute
// Compile-Fehler, dann verschieben": ein nicht deklariertes Ziel darf nicht erst zur Laufzeit als
// leeres Dock auffallen, denn dort sieht es aus wie "noch nicht bestueckt" und nicht wie "falsch".

#include "heuristik_adapter_klassifikation.hpp"  // ist_abi_sichtbares_genus / ist_klassifikations_genus
#include "heuristik_adapter_synthese_matrix.hpp" // kHybridNodeObergrenzeDefault (Dock-Programm-Deckel)
#include "hybrid_dock_array.hpp"                 // DockArray + StatischeDockArrayPolicy
#include "hybrid_dock_contract.hpp"              // DockContractDescriptor + hybrid_status_*
#include "hybrid_dock_factory.hpp"               // DockArray<Policy>::attach (Definition)
#include "hybrid_pruef_dock.hpp"                 // StandardHybridDock

#include "anatomy/observable_tier.hpp" // IObservableTier (der Antrieb am Dock)

#include "anatomy/anatomy_base.hpp" // IAnatomyBase / AnatomyGenus / AnatomyGattung

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE ZIEL-DEKLARATION -- die CT-Sperre in ihrer positiven Form
// ------------------------------------------------------------------------------------------

/// RerouteZiel<G> -- primaer UNDEFINIERT. Wer ein Ziel-Genus benutzt, das hier keine
/// Spezialisierung hat, bekommt einen Fehler an der Instanziierungsstelle. Die Sperre ist damit
/// eine Eigenschaft des TYPSYSTEMS, nicht eines Laufzeit-if: sie kann nicht uebersprungen,
/// vergessen oder mit einem Flag abgeschaltet werden.
template <anatomy::AnatomyGenus G>
struct RerouteZiel;

/// DIE ZWEI deklarierten plain-Tier-Ziele des F8-Minimal-Schnitts. Jede Zeile ist der sichtbare
/// Beleg, dass jemand die Zulaessigkeit GEPRUEFT hat, statt sie zu unterstellen.
///
/// WARUM ES ZWEI SIND UND NICHT EINS: mit nur einer Spezialisierung waere nicht unterscheidbar, ob
/// die Sperre wirklich AUSWAEHLT oder ob der Bau schlicht nur einen einzigen Fall kennt. Erst das
/// Paar zeigt, dass der Mechanismus eine ECHTE Teilmenge bildet -- zwei zulaessige Genera stehen
/// drei unzulaessigen gegenueber (Sequence, Adapter, View).
///
/// WARUM GERADE DIESE ZWEI: SearchAlgorithm ist die Gattung Map (18 Organ-Achsen, das Haupt-Tier),
/// Set die Container-Gattung. Ein Ziel-Paar aus VERSCHIEDENEN Gattungen belegt, dass der Reroute
/// nicht an einer Gattung klebt -- ein Paar aus zwei Container-Genera haette das offen gelassen.
template <>
struct RerouteZiel<anatomy::AnatomyGenus::SearchAlgorithm> {
    static constexpr anatomy::AnatomyGenus genus     = anatomy::AnatomyGenus::SearchAlgorithm;
    static constexpr std::string_view      ziel_name = "SearchAlgorithm";
};

template <>
struct RerouteZiel<anatomy::AnatomyGenus::Set> {
    static constexpr anatomy::AnatomyGenus genus     = anatomy::AnatomyGenus::Set;
    static constexpr std::string_view      ziel_name = "Set";
};

/// reroute_ziel_deklariert<G>() -- die LESBARE Fassung derselben Frage. Sie existiert, damit die
/// Fehlermeldung des Proxys sagen kann, WAS fehlt; ohne sie meldete nur "incomplete type".
template <anatomy::AnatomyGenus G>
[[nodiscard]] consteval bool reroute_ziel_deklariert() noexcept {
    return requires { RerouteZiel<G>::genus; };
}

// DIE ZWEI ZULAESSIGEN --------------------------------------------------------------------------
static_assert(reroute_ziel_deklariert<anatomy::AnatomyGenus::SearchAlgorithm>(),
              "HY-A2: SearchAlgorithm ist das erste deklarierte Ziel des F8-Minimal-Schnitts.");
static_assert(reroute_ziel_deklariert<anatomy::AnatomyGenus::Set>(),
              "HY-A2: Set ist das zweite deklarierte Ziel des F8-Minimal-Schnitts.");

// DIE UNZULAESSIGEN -- vollzaehlig aufgezaehlt, damit die Sperre MESSBAR trennt ------------------
// Ohne diese Zeilen bewiese der Bau nur, dass zwei Faelle gehen; er sagte nichts darueber, ob die
// UEBRIGEN gesperrt sind. Ein Praedikat, das auf immer-wahr degeneriert, liesse jeden Enum-Wert
// durch und faellt genau hier auf.
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::FunctionInterfaceReroute>(),
              "HY-A2/Gate S1: der Reroute-Genus selbst ist NIE ein Ziel -- ein Hybrid auf einen "
              "Hybrid haette am Ende der Kette kein ABI-Interface.");
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::View>(),
              "HY-A2: die CT-Sperre unterscheidet noch -- View ist ABI-sichtbar, aber von DIESEM Bau "
              "nicht als Reroute-Ziel deklariert.");
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::Sequence>(),
              "HY-A2: Sequence ist ABI-sichtbar, aber nicht als Reroute-Ziel deklariert.");
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::Adapter>(),
              "HY-A2: Adapter ist ABI-sichtbar, aber nicht als Reroute-Ziel deklariert.");

/// kDeklarierteRerouteZielListe -- die GESCHLOSSENE Whitelist der deklarierten Ziele, als DATEN.
/// A2.5-Fix A-F6 (Review #15): vorher stand hier ein nacktes `= 2` mit einem `== 2`-Assert
/// dagegen -- eine Zahl, die sich selbst bestaetigte (Tautologie; Wegwerf-Mutation 18.08.2026:
/// drittes Ziel + entfernter Negativ-Assert blieben gruen, die Zahl log). Jetzt ist die LISTE die
/// Quelle: Praedikat und Anzahl fallen aus ihr heraus, und die Deckungs-Wache unten haelt sie
/// gegen die TATSAECHLICHEN RerouteZiel-Spezialisierungen -- in BEIDE Richtungen.
inline constexpr std::array<anatomy::AnatomyGenus, 2> kDeklarierteRerouteZielListe{
    anatomy::AnatomyGenus::SearchAlgorithm, anatomy::AnatomyGenus::Set};

/// Das PRAEDIKAT aus der Liste -- die Werteform der Frage, deren Typform
/// reroute_ziel_deklariert<G>() ist. detail::enthaelt kommt aus der Klassifikations-Einzelquelle.
[[nodiscard]] constexpr bool reroute_ziel_gelistet(anatomy::AnatomyGenus g) noexcept {
    return detail::enthaelt(kDeklarierteRerouteZielListe, g);
}

/// kDeklarierteRerouteZiele -- die Abnahme-Zahl der Ziel-Deklaration, aus der Liste ABGELEITET
/// statt daneben gepflegt. Sie steht hier und nicht im Test, damit ein Ziel-Zuwachs am
/// EIGENTUEMER auffaellt und nicht erst im Lauf.
inline constexpr std::size_t kDeklarierteRerouteZiele = kDeklarierteRerouteZielListe.size();

namespace detail {
/// true gdw. fuer JEDEN Genus-Wert der Einzelquelle gilt: RerouteZiel-Spezialisierung vorhanden
/// == in der Whitelist gelistet. NICHT tautologisierbar: die linke Seite fragt das TYPSYSTEM
/// (existiert die Spezialisierung?), die rechte die DATEN (steht G in der Liste?) -- eine dritte
/// Spezialisierung ohne Listen-Eintrag bricht hier, ein Listen-Eintrag ohne Spezialisierung ebenso.
template <std::size_t... I>
[[nodiscard]] consteval bool reroute_ziel_liste_deckt_deklarationen(std::index_sequence<I...>) noexcept {
    return ((reroute_ziel_deklariert<kAlleGenera[I]>() == reroute_ziel_gelistet(kAlleGenera[I])) && ...);
}
} // namespace detail

static_assert(detail::reroute_ziel_liste_deckt_deklarationen(std::make_index_sequence<kAlleGenera.size()>{}),
              "HY-A2/F8-DoD: Whitelist und RerouteZiel-Spezialisierungen sind auseinandergelaufen. "
              "Wer ein Ziel aufnimmt oder streicht, zieht Spezialisierung, Listen-Eintrag und den "
              "zugehoerigen Negativ-static_assert im SELBEN Commit nach.");

static_assert(kDeklarierteRerouteZiele == 2,
              "HY-A2/F8-DoD: GENAU ZWEI plain-Tier-Ziele (SearchAlgorithm, Set). Die Zahl ist aus "
              "der Whitelist ABGELEITET -- sie bewegt sich nur mit einem Listen-Eintrag, und den "
              "haelt die Deckungs-Wache darueber gegen die echten Spezialisierungen.");

// ------------------------------------------------------------------------------------------
// (2) DER CT-SLOT-DECKEL DES REROUTE-GENUS
// ------------------------------------------------------------------------------------------

/// kRerouteGenusCtSlotCount -- die COMPILE-ZEIT-Slot-Zahl der Gattung HeuristikAdapter/Genus
/// FunctionInterfaceReroute, wie sie das Bau-Gate (builder/experiment_tree/genus_build_admission.hpp)
/// konsumiert.
///
/// WARUM SIE DER DOCK-DECKEL IST UND KEINE ACHSEN-ZAHL: die fuenf ABI-sichtbaren Genera zaehlen hier
/// ihre KOMPOSITIONS-Slots (SearchAlgorithm 18 Organ-Achsen, Set 13, ...). Ein Reroute-Genus hat
/// keine eigene Komposition -- er hat DOCKS. Seine Bau-Aritaet ist deshalb die Zahl der Docks, die
/// eine Hybrid-Binary tragen darf, und das ist der Programm-Deckel kHybridNodeObergrenzeDefault
/// (32, KON28-03, Owner 12.08. "maximal 32").
///
/// RT <= CT, und das ist die Richtung, auf die es ankommt: diese Zahl ist die OBERGRENZE. Eine
/// konkrete Hybrid-Binary bestueckt weniger (der F8-Minimal-Schnitt genau EINS); sie darf nie mehr.
/// Die Laufzeit-Seite dieser Invariante wacht DockArray::attach mit hybrid_status_array_voll -- sie
/// steht dort und nicht hier, weil nur dort beide Mengen zugleich sichtbar sind.
inline constexpr std::size_t kRerouteGenusCtSlotCount = kHybridNodeObergrenzeDefault;

static_assert(kRerouteGenusCtSlotCount == 32,
              "HY-A3: die CT-Slot-Zahl des Reroute-Genus IST der Dock-Programm-Deckel. Wer ihn "
              "bewegt, bewegt die Bau-Aritaet der Gattung mit -- und muss den Cross-Pin in "
              "genus_build_admission.hpp im SELBEN Commit nachziehen.");

// ------------------------------------------------------------------------------------------
// (3) DER PROXY -- DELEGATION AUF DEN ADAPTER, NICHT NEBEN IHN
// ------------------------------------------------------------------------------------------
//
// DER BEFUND, DER DIESEN SCHNITT ERZWUNGEN HAT (18.08.2026, am Compiler gemessen): eine Klasse, die
// IAnatomyBase erbt, muss die VOLLE IExecutionEngine-Flaeche bedienen -- engine_name,
// lifecycle_state, warm_up, run, reset, shutdown (execution_engine_base.hpp:103-118; engine_kind()
// ist in IAnatomyBase final und faellt weg). Der erste Entwurf liess sie offen und war abstrakt.
//
// DIE FALSCHE HEILUNG WAERE, SIE HIER NACHZUBAUEN. Fuer plain Tiere beantwortet sie der
// SearchAlgorithmAbiAdapter (abi_adapter.hpp:428-443), und zwar mit einer ausdruecklichen Zusage:
// "die Lifecycle-Methoden setzen NUR state_ (kein eigener Preheat)" (:392). Ein zweiter, hier
// geschriebener Lebenszyklus waere die ZWEITE WAHRHEIT, gegen die K2 gebaut ist: zwei Stellen, die
// "warm_up" beantworten, laufen auseinander -- und an warm_up haengt die Mess-Semantik (E-WARMUP:
// ausfuehren -> zurueckrollen -> warm wiederholen). Ein Hybrid, der seinen eigenen Warmup-Zustand
// fuehrt, meldete "warm", waehrend sein Ziel kalt ist.
//
// DESHALB: DELEGATION. Der Proxy haelt das gebundene Ziel als IAnatomyBase* (dasselbe Objekt, das
// auch am Dock als IObservableTier* haengt -- der Adapter erbt beide) und reicht JEDE Frage 1:1
// durch. Seine EIGENLEISTUNG ist genau zweierlei und sonst nichts:
//   (a) die Reroute-BUCHFUEHRUNG (welches Ziel steckt an welchem Dock),
//   (b) die Identitaets-Antwort, die NICHT delegiert werden darf: composition_name/paper_id nennen
//       den HYBRID, nicht sein Ziel -- sonst waere die Hybrid-Binary im Log ununterscheidbar von
//       einem plain Tier, und genau diese Unterscheidbarkeit ist der Zweck der Gattung.
// genus() dagegen WIRD delegiert: das ist Weg C. Die Antwort ist damit per Konstruktion das geerbte
// Ziel-Genus und kann gar nicht der Reroute-Wert sein.
//
// DER UNGEBUNDENE ZUSTAND ist ein ehrlicher Zustand, kein Absturz: solange kein Ziel steckt, liefert
// der Proxy benannte Sentinels ("hybrid_ungebunden", Idle, 0 Organe). Ein Proxy, der stattdessen
// dereferenzierte, braeche zwischen attach und Bindung -- also in genau dem Fenster, das das
// Dock-Design ausdruecklich vorsieht (StandardHybridDock::ist_bereit trennt "Slot belegt" von
// "Ziel gebunden").

template <anatomy::AnatomyGenus ZielGenus, std::size_t MaxDocks = 1>
class HybridBinaryProxy final : public anatomy::IAnatomyBase {
    static_assert(reroute_ziel_deklariert<ZielGenus>(),
                  "HY-A2: dieses Reroute-Ziel ist NICHT deklariert. Zulaessig sind ausschliesslich "
                  "Genera mit einer RerouteZiel<G>-Spezialisierung in diesem Header. Ein "
                  "Klassifikations-Genus (FunctionInterfaceReroute) ist NIE zulaessig -- er haette "
                  "am Ende der Reroute-Kette kein ABI-Interface (Gate S1). Wer ein neues Ziel "
                  "braucht, schreibt die Spezialisierung; er setzt nicht den Enum-Wert ein.");
    static_assert(MaxDocks >= 1 && MaxDocks <= kRerouteGenusCtSlotCount,
                  "HY-A2: die Dock-Zahl liegt ausserhalb von [1, Programm-Deckel]. Null Docks waere "
                  "eine Hybrid-Binary ohne Ziel -- eine Huelle, die sich wie ein Tier anfuehlt.");
    // A2.5-Fix 6 (Review #15): der F8-Schnitt SAGT ehrlich, was er traegt. EIN ziel_-Feld traegt
    // die GESAMTE Delegation (genus/engine_name/lifecycle/organ_count); bei MaxDocks > 1 waeren
    // last-bind-wins beim Binden und die Komplett-Nullung beim Loesen EINES Slots still falsch.
    // Mehr-Dock-Delegation (slotbezogene Basiszeiger) ist das HY-B/W3-Design -- wer sie braucht,
    // baut sie DORT und loest diesen Pin als bewusste Quittung, nicht als Reibung.
    static_assert(MaxDocks == 1, "HY-A2/F8: Mehr-Dock-Delegation ist nicht definiert -- EIN ziel_-Feld traegt die "
                                 "gesamte Delegation, mehr Docks waeren still falsch (last-bind-wins beim Binden, "
                                 "Komplett-Nullung beim Loesen). Mehr-Dock = HY-B/W3, slotbezogene Basiszeiger.");

public:
    using Policy = StatischeDockArrayPolicy<MaxDocks>;

    HybridBinaryProxy() noexcept = default;

    // -- IExecutionEngine: 1:1 DELEGATION, kein eigener Zustand --------------------------------

    [[nodiscard]] std::string_view engine_name() const noexcept override {
        return ziel_ == nullptr ? std::string_view{"hybrid_ungebunden"} : ziel_->engine_name();
    }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return ziel_ == nullptr ? ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle
                                : ziel_->lifecycle_state();
    }
    void warm_up() override {
        if (ziel_ != nullptr) ziel_->warm_up();
    }
    void run() override {
        if (ziel_ != nullptr) ziel_->run();
    }
    void reset() override {
        if (ziel_ != nullptr) ziel_->reset();
    }
    void shutdown() override {
        if (ziel_ != nullptr) ziel_->shutdown();
    }

    // -- IAnatomyBase ---------------------------------------------------------------------------

    /// WEG C: das GEERBTE Ziel-Genus -- delegiert, also per Konstruktion NIE der Reroute-Wert.
    /// Ohne Ziel die CT-Deklaration: sie ist die Zusage dieses Bau-Typs, keine Erfindung.
    [[nodiscard]] anatomy::AnatomyGenus genus() const noexcept override {
        return ziel_ == nullptr ? ZielGenus : ziel_->genus();
    }

    /// NICHT delegiert (Begruendung im Kopf): der Hybrid nennt sich selbst.
    [[nodiscard]] std::string_view composition_name() const noexcept override { return "HybridBinaryProxy"; }
    [[nodiscard]] std::string_view paper_id() const noexcept override { return "HY-A2"; }

    /// Die Organ-Zahl des ZIELS -- der Proxy hat keine eigene Anatomie. Ohne Ziel: 0, und das ist
    /// die ehrliche Antwort. Die Zahl, die das Ziel HAETTE, waere eine Behauptung ueber etwas, das
    /// nicht da ist.
    [[nodiscard]] std::size_t organ_count() const noexcept override {
        return ziel_ == nullptr ? std::size_t{0} : ziel_->organ_count();
    }

    // -- Die Reroute-Naht -----------------------------------------------------------------------

    /// dock_anlegen() -- legt das Standard-Dock an. Rueckgabe: Slot-Index (>= 0) oder -status.
    /// Der Deskriptor traegt den ZIEL-Genus, nicht den Reroute-Genus: das Dock nimmt plain Tiere auf.
    [[nodiscard]] int dock_anlegen() noexcept {
        DockContractDescriptor const desc{static_cast<std::uint8_t>(HybridDockContract::Standard), ZielGenus};
        return docks_.attach(desc);
    }

    /// ziel_binden(slot, antrieb, basis) -- DER UMSCHALTPUNKT. Beide Zeiger zeigen auf DASSELBE
    /// Objekt (der Adapter erbt IObservableTier UND IAnatomyBase); sie stehen getrennt in der
    /// Signatur, weil der Aufrufer den Cross-Cast besitzt und nicht dieser Header.
    ///
    /// nullptr in BEIDEN heisst GELOEST, nicht Fehler -- der Slot muss seinen Zeiger verlieren,
    /// BEVOR das Ziel-Modul entladen wird (destroy-vor-dlclose). Ein Dock, das einen Zeiger in ein
    /// entladenes Modul behielte, waere ein use-after-dlclose mit voll plausiblem Verhalten bis zum
    /// ersten Zugriff.
    ///
    /// A2.5-Fix 4 (Review #15): die zwei Zusagen darueber werden ERZWUNGEN, nicht nur beschrieben.
    /// (a) Ein GEMISCHTES Paar (einer null, einer nicht) ist keiner der beiden dokumentierten
    ///     Zustaende -- antrieb und ziel_ zeigten fortan auf verschiedene Wahrheiten ->
    ///     hybrid_status_bindung_inkonsistent, VOR jeder Mutation.
    /// (b) Ein basis mit FREMDEM genus() wuerde die Loader-Riegel-Garantie (Symbol == genus()) am
    ///     gebundenen Proxy nachtraeglich entwerten: genus() delegiert an ziel_, die Antwort
    ///     wechselte still auf das Fremd-Genus -> hybrid_status_kein_zielfaehiges_genus.
    [[nodiscard]] int ziel_binden(std::size_t slot, anatomy::IObservableTier* antrieb,
                                  anatomy::IAnatomyBase* basis) noexcept {
        if ((antrieb == nullptr) != (basis == nullptr)) return hybrid_status_bindung_inkonsistent;
        if (basis != nullptr && basis->genus() != ZielGenus) return hybrid_status_kein_zielfaehiges_genus;
        int const status = docks_.antrieb_binden(slot, antrieb);
        if (status != hybrid_status_ok) return status;
        ziel_ = basis;
        return hybrid_status_ok;
    }

    /// antrieb(slot) -- der OP-PFAD. Ein Zeiger-Read, kein visit, kein Cast (HY-A1-Hotpath-Anker).
    [[nodiscard]] anatomy::IObservableTier* antrieb(std::size_t slot) const noexcept {
        return docks_.antrieb_von(slot);
    }

    [[nodiscard]] bool                              ist_gebunden() const noexcept { return ziel_ != nullptr; }
    [[nodiscard]] std::size_t                       belegte_docks() const noexcept { return docks_.size(); }
    [[nodiscard]] std::size_t                       dock_kapazitaet() const noexcept { return docks_.kapazitaet(); }
    [[nodiscard]] static constexpr std::string_view reroute_ziel_name() noexcept {
        return RerouteZiel<ZielGenus>::ziel_name;
    }

private:
    DockArray<Policy>      docks_{};
    anatomy::IAnatomyBase* ziel_ = nullptr;
};

} // namespace comdare::cache_engine::hybrid
