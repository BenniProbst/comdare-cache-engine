#pragma once
// mess/steuer_dock.hpp -- DER STEUERKANAL PLANER -> CEB: die SECHS STEUERDOCKS.
//
// == DERSELBE DEFEKT EINE EBENE HOEHER (Owner-KERN 09.08.2026, woertlich) ==========================
//     "Es betrifft also nicht nur den Mess-Kanal zwischen CEB und Tier-Binary variadisch, sondern
//      auch den STEUERUNGSKANAL zwischen Planer und CEB, der sich ebenfalls VARIADISCH AN DIE
//      3 FAKULTAET vorhandenen CEB Versionen jeweils anpassen muss, damit hat der Planer 6 STEUERDOCKS
//      -- fuer jede CEB einen, der GENAU AUF VORHANDENE EINKOMPILIERTE MESSEINRICHTUNGEN PASST und
//      auch NUR BESTIMMTE STEUERBEFEHLE FREIGIBT, DIE TATSAECHLICH EXISTIEREN LAUT PLAN."
//
// DIE SYMMETRIE, die diese Datei mit mess_naht.hpp teilt:
//
//     PLANER --- Steuerkanal (variadisch, 6 Steuerdocks) ---> CEB
//                                                              |
//                                                   Mess-Kanal (variadisch, Visitor
//                                                   am Genus-Interface)
//                                                              v
//                                                         TIER-BINARY
//
// BEIDE fehlten aus DEMSELBEN Grund: variadische Template-Variablen wurden nicht per
// Metaprogrammierung durch die jeweilige Interface-Kaskade gereicht. Deshalb behebt diese Datei den
// oberen Kanal mit DEMSELBEN Mittel wie der untere -- derselbe MK-Typ, dieselbe Faltung. Zwei
// verschiedene Mechanismen fuer dasselbe Problem waeren zwei Gelegenheiten zum Auseinanderlaufen.
//
// == WAS "NUR EXISTIERENDE BEFEHLE" HIER HEISST ====================================================
// Steuerbefehle sind TYPEN (GoF Command), und das Angebot des Docks IST DIE FALTUNG SELBST: erlaubt
// ist, was aus den einkompilierten Instrumenten gefaltet wird, plus die Grundbefehle. Ein Befehl,
// dessen Instrument nicht in MK steckt, scheitert am `requires` -- BEIM UEBERSETZEN DES PLANERS,
// nicht beim Senden.
//
// Der Unterschied zur naheliegenden Bauart (Enum + Laufzeit-Pruefung "kennt die CEB diesen Befehl?")
// ist der Zeitpunkt: eine Laufzeit-Pruefung kann vergessen werden, und wenn sie vergessen wird,
// faehrt eine Kampagne los, die einen Befehl schickt, den niemand ausfuehrt. Ein Angebot, das aus der
// Konfiguration GEFALTET ist, hat keine zweite Liste, die driften koennte.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: dass es GENAU SECHS Docks gibt (static_assert unten -- der NENNER im Code) und dass
//              ein nicht angebotener Befehl nicht sendbar ist.
//   ZUSICHERT NICHT: den vollstaendigen Steuerkanal. Typestate-Fenster, fd-Kanarie und die
//              Rechtsakte (Freigabe/Durchsetzung) sind Paket P2 und ausdruecklich NICHT hier -- was
//              hier steht, ist die Dock-Maschinerie samt ihrer Zahl, nicht der Betrieb.
//
// ASCII-only.

#include <mess/konfiguration.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::mess {

// ==================================================================================================
// DAS BEFEHLS-ANGEBOT -- gefaltet, nicht gepflegt
// ==================================================================================================

namespace detail {

/// Steckt der Befehl B in der BefehlListe L?
template <class B, class L>
struct BefehlIn : std::false_type {};
template <class B, class... Bs>
struct BefehlIn<B, BefehlListe<Bs...>> : std::bool_constant<(std::is_same_v<B, Bs> || ...)> {};

/// Die Grundbefehle kann JEDES Dock -- sie haengen an keinem Instrument, sondern am Lebenszyklus.
template <class B>
inline constexpr bool ist_grundbefehl =
    std::is_same_v<B, AnschliessenBefehl> || std::is_same_v<B, StartBefehl> || std::is_same_v<B, EndeBefehl>;

/// Die FALTUNG: erlaubt ist, was die einkompilierten Instrumente anbieten.
template <class B, class MK>
struct BefehlGefaltet : std::false_type {};
template <class B, InstrumentConcept... Is>
struct BefehlGefaltet<B, Konfiguration<Is...>> : std::bool_constant<(BefehlIn<B, typename Is::befehle>::value || ...)> {
};

} // namespace detail

/// BefehlErlaubt<B, MK> -- die Bedingung, an der ein nicht angebotener Befehl scheitert.
template <class B, class MK>
concept BefehlErlaubt = KonfigConcept<MK> && (detail::ist_grundbefehl<B> || detail::BefehlGefaltet<B, MK>::value);

// ==================================================================================================
// DAS STEUERDOCK
// ==================================================================================================

/// Was beim Anschliessen herauskommt. Fehlerklassen, keine Booleans -- ein nacktes false liesse
/// offen, WORAN es lag, und die drei Gruende verlangen drei verschiedene Reaktionen.
enum class AnschlussBefund : std::uint8_t {
    Angeschlossen          = 0, ///< Tag und Zensus stimmen -- die CEB passt auf dieses Dock
    SD_01_TagFremd         = 1, ///< die CEB traegt eine ANDERE Mess-Konfiguration -> harter Abbruch
    SD_03_ZensusAbweichung = 2, ///< Tag gleich, aber die Befehlszahl weicht ab -> zweiter, unabhaengiger Weg
};

/// Was die CEB ueber sich behauptet, wenn sie andockt.
struct CebBeschreibung {
    std::uint64_t mess_tag       = 0; ///< MK::tag() der CEB
    std::uint64_t befehls_zensus = 0; ///< wie viele Befehle sie zu kennen behauptet
};

/// SteuerDock<MK> -- EIN Dock je CEB-Version. Es passt GENAU auf die einkompilierten
/// Messeinrichtungen dieser Version, weil es aus DERSELBEN Konfiguration monomorphisiert ist.
template <KonfigConcept MK>
class SteuerDock {
public:
    using konfiguration_t = MK;

    /// Der Tag, gegen den geprueft wird. Er entsteht aus MK, nicht aus einer gepflegten Konstante.
    static constexpr std::uint64_t mess_tag = MK::tag();

    /// Der eigene Befehls-Zensus: drei Grundbefehle plus die der Instrumente.
    ///
    /// Er ist der ZWEITE, UNABHAENGIGE Weg neben dem Tag. Der Tag ist ein Hash -- er kann
    /// theoretisch kollidieren; der Zensus ist eine Zaehlung. Zwei Wege, die auf verschiedene Weise
    /// brechen, fangen mehr als einer, der zweimal befragt wird.
    static constexpr std::uint64_t befehls_zensus = 3u + (2u * MK::anzahl);

    /// Die WEICHE VOR der Messung: passt diese CEB auf dieses Dock?
    ///
    /// Es gibt KEINE Schnittmengenbildung. Ein Fremdpaar wird nicht "auf das gemeinsam Moegliche"
    /// heruntergehandelt -- eine CEB, die still weniger misst als geplant, ist die Klasse
    /// "kontaminierte Daten", und die ist unheilbar. Mismatch ist LAUT.
    [[nodiscard]] static constexpr AnschlussBefund anschliessen(CebBeschreibung const& b) noexcept {
        if (b.mess_tag != mess_tag) return AnschlussBefund::SD_01_TagFremd;
        if (b.befehls_zensus != befehls_zensus) return AnschlussBefund::SD_03_ZensusAbweichung;
        return AnschlussBefund::Angeschlossen;
    }

    /// SENDEN -- nur, was dieses Dock tatsaechlich anbietet.
    ///
    /// `requires BefehlErlaubt<B, MK>`: ein Befehl eines nicht einkompilierten Instruments ist hier
    /// ein UEBERSETZUNGSFEHLER. Das ist der Owner-Satz "NUR BESTIMMTE STEUERBEFEHLE FREIGIBT, DIE
    /// TATSAECHLICH EXISTIEREN LAUT PLAN" -- als Struktur, nicht als Absichtserklaerung.
    template <class B>
        requires BefehlErlaubt<B, MK>
    [[nodiscard]] static constexpr bool senden(B const&) noexcept {
        return true;
    }
};

// ==================================================================================================
// DIE SECHS -- ERZEUGT, NICHT GESCHRIEBEN
// ==================================================================================================
//
// Sechs von Hand geschriebene Dock-Aliase waeren sechs Gelegenheiten, eine Permutation zu vergessen
// oder doppelt zu schreiben -- und die Zahl 6 stuende dann als Kommentar daneben statt als Rechnung
// darin. Erzeugt heisst: die Zahl IST das Ergebnis der Faltung, und der static_assert prueft sie
// gegen die Owner-Vorgabe.

namespace detail {

/// Eine Typliste (Traeger fuer die Permutations-Rechnung) und eine Menge solcher Listen.
template <class... Ts>
struct Liste {};
template <class... Ls>
struct ListenSatz {
    static constexpr std::size_t anzahl = sizeof...(Ls);
};

template <class A, class B>
struct SatzVerbinden;
template <class... As, class... Bs>
struct SatzVerbinden<ListenSatz<As...>, ListenSatz<Bs...>> {
    using typ = ListenSatz<As..., Bs...>;
};

/// Alle Einfuegungen von T in eine Liste -- T wandert von vorne nach hinten durch alle Positionen.
template <class T, class Vorne, class Hinten>
struct EinfuegeHelfer;
template <class T, class... Ps>
struct EinfuegeHelfer<T, Liste<Ps...>, Liste<>> {
    using typ = ListenSatz<Liste<Ps..., T>>;
};
template <class T, class... Ps, class S0, class... Ss>
struct EinfuegeHelfer<T, Liste<Ps...>, Liste<S0, Ss...>> {
    using hier = ListenSatz<Liste<Ps..., T, S0, Ss...>>;
    using rest = typename EinfuegeHelfer<T, Liste<Ps..., S0>, Liste<Ss...>>::typ;
    using typ  = typename SatzVerbinden<hier, rest>::typ;
};
template <class T, class L>
struct AlleEinfuegungen {
    using typ = typename EinfuegeHelfer<T, Liste<>, L>::typ;
};

/// T in JEDE Liste des Satzes an JEDER Position einfuegen.
template <class T, class Satz>
struct SatzDurchsetzen;
template <class T>
struct SatzDurchsetzen<T, ListenSatz<>> {
    using typ = ListenSatz<>;
};
template <class T, class P0, class... Ps>
struct SatzDurchsetzen<T, ListenSatz<P0, Ps...>> {
    using hier = typename AlleEinfuegungen<T, P0>::typ;
    using rest = typename SatzDurchsetzen<T, ListenSatz<Ps...>>::typ;
    using typ  = typename SatzVerbinden<hier, rest>::typ;
};

/// Die Permutationen einer Liste: n! Stueck.
template <class L>
struct Permutationen;
template <>
struct Permutationen<Liste<>> {
    using typ = ListenSatz<Liste<>>;
};
template <class T0, class... Ts>
struct Permutationen<Liste<T0, Ts...>> {
    using rest = typename Permutationen<Liste<Ts...>>::typ;
    using typ  = typename SatzDurchsetzen<T0, rest>::typ;
};

/// Aus jeder Permutations-Liste wird eine Konfiguration und daraus ein Dock.
template <class Satz>
struct ZuDocks;
template <class... Ls>
struct ZuDocks<ListenSatz<Ls...>> {
    template <class L>
    struct EineKonfig;
    template <class... Is>
    struct EineKonfig<Liste<Is...>> {
        using typ = Konfiguration<Is...>;
    };
    using konfigurationen               = ListenSatz<typename EineKonfig<Ls>::typ...>;
    static constexpr std::size_t anzahl = sizeof...(Ls);
};

} // namespace detail

/// DIE SECHS CEB-VERSIONEN: alle Permutationen der drei Instrumente.
using CebPermutationen = detail::Permutationen<detail::Liste<Wallclock, Makro, Mikro>>::typ;
using CebVersionen     = detail::ZuDocks<CebPermutationen>::konfigurationen;

/// DER NENNER IM CODE -- Owner 09.08.2026: "3 FAKULTAET ... damit hat der Planer 6 STEUERDOCKS".
static_assert(detail::ZuDocks<CebPermutationen>::anzahl == 6,
              "Owner 09.08.2026: 3 Fakultaet = 6 Steuerdocks -- der NENNER steht im Code, nicht im Kommentar");

/// Und die Gegenprobe, dass die Rechnung eine ist: 2! = 2, 1! = 1, 0! = 1.
static_assert(detail::Permutationen<detail::Liste<Wallclock, Makro>>::typ::anzahl == 2,
              "zwei Instrumente ergeben zwei Permutationen -- sonst rechnet die Faltung nicht");
static_assert(detail::Permutationen<detail::Liste<Wallclock>>::typ::anzahl == 1);
static_assert(detail::Permutationen<detail::Liste<>>::typ::anzahl == 1);

} // namespace comdare::cache_engine::mess
