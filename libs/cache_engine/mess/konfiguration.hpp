#pragma once
// mess/konfiguration.hpp -- DIE WURZEL: die VARIADISCHE MESS-TEMPLATE-VARIABLE, die durch die
// GESAMTE Gattung+Genus-Kaskade gereicht wird (Owner-KERN 09.08.2026).
//
// == DER DEFEKT, DEN DIESE DATEI BEHEBT (Owner 09.08.2026, woertlich) ===============================
//     "Die VARIADISCHE TEMPLATE VARIABLE(N) der Messung fuer Observable Tier wird NICHT ueber
//      Gattung+Genus Interface PER METAPROGRAMMIERUNG DURCHGEREICHT und in die Genus-Implementierung
//      HINTER die Gattung+Interface Kaskade GEBAUT. Damit kommt KEIN Signal und Kommunikationskanal
//      beim Tier-Binary an."
//
// Der Owner benennt EINEN Defekt mit VIER Folgen:
//     1. das Pack wird nicht durch die Kaskade gereicht      <== DIESE DATEI ist die Wurzel
//     2. => kein Signal-/Kommunikationskanal im Tier-Binary
//     3. => globale init() der ZWEI Arenen geht nicht wie geplant
//     4. => CEB kann ueber den Observer keinen flush() beauftragen, JE ARENA GETRENNT
//     5. => am ABI-stabilen, modules-metaprogrammierten Pruefdock keine Signale
// Wer Stufe 3, 4 oder 5 EINZELN repariert, baut an Symptomen. Diese Datei ist Stufe 1.
//
// == WAS EINE "MESS-KONFIGURATION" IST -- UND WARUM SIE EIN TYP IST =================================
// MK ist KEIN Laufzeitwert und KEIN Makro-Satz, sondern ein TYP: eine geordnete Typliste von
// Instrumenten. Das hat drei Konsequenzen, die keine Laufzeit-Loesung hat:
//   (a) Sie reist als GEWOEHNLICHER Template-Parameter durch jede Schicht -- Gattung, Genus,
//       Genus_impl, Hauptalgorithmus. Ein Makro reist nicht; es ist an die TU gebunden, in der es
//       definiert wurde, und genau daran scheiterte die Durchreichung bisher.
//   (b) Aus ihr faltet der Uebersetzer BEIDES: den Befehlssatz des Steuerdocks (Abschnitt STEUERSATZ)
//       und die Frage, welche Mess-Ebene ueberhaupt aufrufbar ist (EbeneEingebaut). Es gibt damit
//       keine zweite Liste, die von der ersten wegdriften koennte.
//   (c) Die LEERE Konfiguration ist ein anderer TYP, nicht derselbe Typ mit einem false-Feld. Bei
//       Leer existiert der Visitor-Parameter NICHT (s. mess_naht.hpp) -- `if (visitor)` ist damit
//       nicht verboten, sondern UNAUSDRUECKBAR.
//
// == WARUM GEORDNET (und nicht std::tuple-egal) ====================================================
// Owner 09.08.2026: der Steuerungskanal "muss sich VARIADISCH AN DIE 3 FAKULTAET vorhandenen CEB
// Versionen jeweils anpassen, damit hat der Planer 6 STEUERDOCKS". 3! = 6 ist eine Zahl ueber
// PERMUTATIONEN, nicht ueber Teilmengen (2^3 = 8, davon nicht-leer 7 -- beides ist nicht 6). Eine
// ungeordnete Menge kann 6 nicht erzeugen. Die POSITION ist deshalb Teil der Identitaet, und tag()
// faltet sie mit ein: <Wallclock, Makro> und <Makro, Wallclock> sind VERSCHIEDENE Konfigurationen mit
// verschiedenem Tag und damit verschiedenen, nicht gegeneinander ladbaren Binaerpaaren.
//
// WAS DIE ORDNUNG PHYSISCH BEDEUTET, ist eine offene Owner-Frage (D-3, unbeantwortet seit 08.08.).
// Trag-Interpretation dieses Baus: die SCHACHTELUNGSORDNUNG der Messfuehler -- wessen EIN/AUS-Klammer
// wen umschliesst, und damit, wessen Overhead in wessen Messwert faellt. Die Maschinerie ist gegen
// die Antwort INVARIANT: eine geordnete Typliste drueckt Permutationen UND Teilmengen aus. Faellt die
// Antwort anders aus, aendert sich EINE Alias-Zeile (CebVersionen in steuer_dock.hpp), kein Bauteil.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: dass eine Ebene ohne einkompiliertes Instrument NICHT AUFRUFBAR ist (EbeneEingebaut
//              als requires-Klausel an jeder Steuerzeile) und dass zwei verschieden konfigurierte
//              Binaries verschiedene tag() tragen.
//   ZUSICHERT NICHT: dass der Tag auch GEPRUEFT wird -- das ist Aussage des Docks (steuer_dock.hpp)
//              und der Abnahme, nicht dieser Deklaration. Eine Kennung, die niemand vergleicht, ist
//              Zierde; der Vergleich steht dort, wo geladen wird.
//
// ASCII-only.

// Haus-Weg: Winkelklammern von der Include-Wurzel libs/cache_engine aus (wie
// anatomy/observable_tier.hpp im Bestand). Ein relativer Pfad braeche, sobald die Datei umzieht.
#include <builder/measure_storage/mess_arena.hpp> // MessEbene, MessCheckpointZeile (Tag-Bindung)

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::mess {

namespace ms = ::comdare::cache_engine::builder::measure_storage;

// ==================================================================================================
// BEFEHLE -- die Steuerbefehle sind TYPEN (GoF Command), nicht Enum-Werte
// ==================================================================================================
//
// Owner 09.08.2026: das Steuerdock darf "auch NUR BESTIMMTE STEUERBEFEHLE FREIGEBEN, DIE TATSAECHLICH
// EXISTIEREN LAUT PLAN". Als Enum-Wert waere jeder Befehl immer benennbar und die Pruefung eine
// Laufzeit-Abfrage, die man vergessen kann. Als TYP ist ein nicht angebotener Befehl an der
// Aufrufstelle ein UEBERSETZUNGSFEHLER -- das Angebot ist die Faltung selbst.

/// Typliste der Befehle eines Instruments. Bewusst ein eigener, leerer Traeger-Typ: er erlaubt die
/// Faltung ueber Instrumente, ohne <tuple> in jede TU zu ziehen, die nur messen will.
template <class... Bs>
struct BefehlListe {
    static constexpr std::size_t anzahl = sizeof...(Bs);
};

/// Die Grundbefehle, die JEDES Dock kann -- unabhaengig davon, welche Instrumente einkompiliert sind.
/// Sie tragen keine Mess-Semantik, sondern den Lebenszyklus des Kanals.
struct AnschliessenBefehl {};
struct StartBefehl {};
struct EndeBefehl {};

// Die instrumentgebundenen Befehle. Je Instrument sein eigener flush -- die ZWEI ARENEN werden JE
// GETRENNT beauftragt (Owner: "JE ARENA GETRENNT"), also gibt es auch zwei Befehlstypen und nie einen
// Sammelbefehl, der beide zugleich meinte.
struct FlushMessBefehl {};
struct FlushStapelBefehl {};

// ==================================================================================================
// DIE INSTRUMENTE -- je eines je Mess-Ebene
// ==================================================================================================
//
// Ein Instrument ist die kleinste einkompilierbare Mess-Einrichtung. Es traegt drei Dinge und sonst
// nichts: WELCHE Ebene es bedient, WELCHE Steuerbefehle es dem Dock hinzufuegt, und eine stabile
// KENNUNG fuer den Tag. Es traegt ausdruecklich KEINEN Zustand -- der liegt im Instrument-Objekt
// (checkpoint_measure.hpp), nicht im Konfigurations-Typ.

/// Wallclock -- die VERGLEICHS-Ebene (MessEbene::Compare): grobe Aussenmessung eines ganzen Laufs.
struct Wallclock {
    static constexpr ms::MessEbene ebene   = ms::MessEbene::Compare;
    static constexpr std::uint32_t kennung = 0x57434C4Bu; // 'WCLK'
    using befehle                          = BefehlListe<FlushMessBefehl, FlushStapelBefehl>;
};

/// Makro -- die Batch-Ebene (MessEbene::Macro): EIN Paar je Batch im Hauptalgorithmus.
struct Makro {
    static constexpr ms::MessEbene ebene   = ms::MessEbene::Macro;
    static constexpr std::uint32_t kennung = 0x4D414B52u; // 'MAKR'
    using befehle                          = BefehlListe<FlushMessBefehl, FlushStapelBefehl>;
};

/// Mikro -- die Achsen-Ebene (MessEbene::Micro): EIN Paar je Achsen-Aufruf.
///
/// Das ist die Ebene, die die Achsen ueberhaupt erst einzeln messbar macht -- und sie ist der Grund,
/// warum die Mess-Naht am GENUS-Interface sitzt und nicht tiefer: der Hauptalgorithmus RECHNET NICHT,
/// er ORCHESTRIERT Achsen. Zwischen seinen Achsen-Aufrufen liegt die einzige Stelle, an der sich eine
/// einzelne Achse ohne Verfaelschung klammern laesst.
struct Mikro {
    static constexpr ms::MessEbene ebene   = ms::MessEbene::Micro;
    static constexpr std::uint32_t kennung = 0x4D494B52u; // 'MIKR'
    using befehle                          = BefehlListe<FlushMessBefehl, FlushStapelBefehl>;
};

/// Was ein Instrument sein muss. Concept-Guard statt Vertrauen: wer die Flaeche verfehlt, faellt an
/// der Konfigurations-Instanziierung auf und nicht erst am ersten Checkpoint.
template <class I>
concept InstrumentConcept = requires {
    { I::ebene } -> std::convertible_to<ms::MessEbene>;
    { I::kennung } -> std::convertible_to<std::uint32_t>;
    typename I::befehle;
};

static_assert(InstrumentConcept<Wallclock>, "Wallclock muss die Instrument-Flaeche erfuellen");
static_assert(InstrumentConcept<Makro>, "Makro muss die Instrument-Flaeche erfuellen");
static_assert(InstrumentConcept<Mikro>, "Mikro muss die Instrument-Flaeche erfuellen");

// ==================================================================================================
// DER TAG -- die ABI-Kennung EINER Monomorphisierung
// ==================================================================================================
//
// ABI-VERTRAG dieser Naht ist: IDENTISCHE MONOMORPHISIERUNG. Templates queren keine dlopen-Grenze;
// was quert, ist eine je MK monomorphisierte Flaeche. Der Tag attestiert, dass beide Seiten dieselbe
// meinen. Er faltet drei Dinge, weil drei Dinge die Monomorphisierung bestimmen:
//   (1) die Instrumente MIT IHRER POSITION -- die Ordnung ist Identitaet, s. Kopf
//   (2) den ABI-Major -- ein Bruch der Naht-Signatur muss den Tag aendern, auch bei gleicher Liste
//   (3) sizeof(MessCheckpointZeile) -- die Zeilenbreite ist das Format der uebertragenen Nutzlast.
//       Sie steht hier NICHT als Zahl, sondern als sizeof: eine Zahl, die als Kopie lebt, veraltet
//       lautlos (Abschrift-Falle). Waechst die Zeile, aendert sich der Tag von selbst.
//
// FNV-1a, 64 Bit: keine Krypto-Anforderung (das ist kein Sicherheitsmerkmal, sondern eine
// Verwechslungs-Sperre), aber eine, die bei Positionstausch zuverlaessig auseinanderlaeuft.

inline constexpr std::uint64_t kMessAbiMajor = 1u;

namespace detail {

inline constexpr std::uint64_t kFnvOffset = 1469598103934665603ull;
inline constexpr std::uint64_t kFnvPrime  = 1099511628211ull;

[[nodiscard]] constexpr std::uint64_t fnv_schritt(std::uint64_t h, std::uint64_t wert) noexcept {
    for (int byte = 0; byte < 8; ++byte) {
        h ^= (wert >> (byte * 8)) & 0xFFull;
        h *= kFnvPrime;
    }
    return h;
}

} // namespace detail

// ==================================================================================================
// DIE KONFIGURATION -- die variadische Template-Variable selbst
// ==================================================================================================

/// Konfiguration<Is...> -- GEORDNETES Instrument-Pack. Der EINE Typparameter, der durch die gesamte
/// Kaskade reist (Gattung -> Genus -> Genus_impl -> Hauptalgorithmus).
///
/// SELBSTCHECK: diese Klasse hat KEIN Datenmitglied und KEINEN Konstruktor. Sie wird nie instanziiert
/// -- sie ist ausschliesslich Traeger von Typ-Information. Wer hier ein Feld einbaut, hat aus einer
/// Uebersetzungszeit-Entscheidung einen Laufzeit-Zustand gemacht und damit den Defekt reproduziert,
/// um dessentwillen diese Datei existiert.
template <InstrumentConcept... Is>
struct Konfiguration {
    /// Wie viele Instrumente einkompiliert sind. DER NENNER jeder Aussage ueber diese Konfiguration.
    static constexpr std::size_t anzahl = sizeof...(Is);

    /// Traegt diese Konfiguration das Instrument I?
    template <class I>
    static constexpr bool traegt = (std::is_same_v<I, Is> || ...);

    /// Der ABI-Tag dieser Monomorphisierung. consteval: er MUSS zur Uebersetzungszeit entstehen --
    /// ein zur Laufzeit gerechneter Tag koennte von einer Konfiguration abweichen, die schon
    /// einkompiliert ist, und genau das soll strukturell unmoeglich sein.
    [[nodiscard]] static consteval std::uint64_t tag() noexcept {
        std::uint64_t h        = detail::kFnvOffset;
        h                      = detail::fnv_schritt(h, kMessAbiMajor);
        h                      = detail::fnv_schritt(h, sizeof(ms::MessCheckpointZeile));
        std::uint64_t position = 0;
        // Die POSITION geht MIT ein -- deshalb ist <A,B> nicht <B,A>. Der Faltungsausdruck laeuft in
        // Deklarationsreihenfolge (Komma-Faltung ueber den Pack), die Reihenfolge ist also die des
        // Packs und nicht die des Uebersetzers.
        ((h = detail::fnv_schritt(h, (static_cast<std::uint64_t>(Is::kennung) << 8) | position), ++position), ...);
        return h;
    }
};

/// Die LEERE Konfiguration -- "Messung aus". Ein eigener Typ, kein Flag.
using Leer = Konfiguration<>;

/// Die volle Konfiguration des Hauses in kanonischer Ordnung.
using Voll = Konfiguration<Wallclock, Makro, Mikro>;

namespace detail {
template <class T>
struct IstKonfiguration : std::false_type {};
template <InstrumentConcept... Is>
struct IstKonfiguration<Konfiguration<Is...>> : std::true_type {};

// Die Ebenen-Zugehoerigkeit als Traits-Faltung. Sie steht HIER und nicht unten beim Concept, weil ein
// Concept seinen Traits nicht vorausgehen darf -- ein Vorwaertsverweis waere hier kein Stilfehler,
// sondern ein Uebersetzungsfehler.
template <class MK, ms::MessEbene E>
struct EbeneIn : std::false_type {};
template <ms::MessEbene E, InstrumentConcept... Is>
struct EbeneIn<Konfiguration<Is...>, E> : std::bool_constant<((Is::ebene == E) || ...)> {};
} // namespace detail

/// KonfigConcept -- was als MK durch die Kaskade reisen darf. Ohne diesen Guard koennte jeder
/// beliebige Typ an der MK-Stelle stehen und die Kaskade uebersetzte weiter, bis tief unten etwas
/// fehlt; mit ihm faellt die Verwechslung an der obersten Schicht auf.
template <class MK>
concept KonfigConcept = detail::IstKonfiguration<MK>::value;

/// aktiv<MK> -- die EINE Frage, an der die Doppelform jeder Genus-Funktion haengt.
template <KonfigConcept MK>
inline constexpr bool aktiv = (MK::anzahl > 0u);

static_assert(!aktiv<Leer>, "die leere Konfiguration ist per Definition inaktiv");
static_assert(aktiv<Voll>, "die volle Konfiguration ist per Definition aktiv");

/// EbeneEingebaut<MK, E> -- ist fuer die Mess-Ebene E ueberhaupt ein Instrument einkompiliert?
///
/// DAS IST DIE WACHE GEGEN DIE STILLE NULL. Ohne sie koennte ein Hauptalgorithmus einen
/// Mikro-Checkpoint setzen, obwohl die gebaute CEB-Version gar keine Mikro-Messung traegt: der Aufruf
/// liefe ins Leere und die Reihe enthielte ehrliche Nullen, die wie Messwerte aussehen. Mit ihr ist
/// derselbe Aufruf ein UEBERSETZUNGSFEHLER -- die Fehlerklasse "richtiges Messgeraet am falschen
/// Gegenstand" kann nicht entstehen, weil sie nicht uebersetzt.
template <class MK, ms::MessEbene E>
concept EbeneEingebaut = KonfigConcept<MK> && detail::EbeneIn<MK, E>::value;

static_assert(EbeneEingebaut<Voll, ms::MessEbene::Micro>, "Voll traegt die Mikro-Ebene");
static_assert(!EbeneEingebaut<Konfiguration<Wallclock>, ms::MessEbene::Micro>,
              "eine Konfiguration ohne Mikro-Instrument darf die Mikro-Ebene NICHT anbieten");
static_assert(!EbeneEingebaut<Leer, ms::MessEbene::Compare>, "die leere Konfiguration bietet keine Ebene an");

// DIE ORDNUNG IST IDENTITAET -- compile-hart, nicht als Absichtserklaerung. Genau diese Zusicherung
// traegt die Owner-Zahl 3! = 6: waere sie falsch, kollabierten die sechs Permutationen auf eine
// einzige Konfiguration und die sechs Steuerdocks waeren sechsmal dasselbe Dock.
static_assert(Konfiguration<Wallclock, Makro>::tag() != Konfiguration<Makro, Wallclock>::tag(),
              "vertauschte Instrumente muessen verschiedene Tags tragen -- sonst ist die Ordnung keine Identitaet");
static_assert(Voll::tag() != Leer::tag(), "voll und leer duerfen nie denselben Tag tragen");
static_assert(Konfiguration<Wallclock>::tag() != Konfiguration<Makro>::tag(),
              "verschiedene Instrumente muessen verschiedene Tags tragen");

} // namespace comdare::cache_engine::mess
