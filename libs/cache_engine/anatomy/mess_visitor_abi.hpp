#pragma once
// anatomy/mess_visitor_abi.hpp -- FLAECHE 3: DER MEASUREMENT-DURCHSTICH AM GENUS-INTERFACE
//                                 = DIE MESS-NAHT (NAHT-1, Owner-KERN 09.08.2026).
//
// ------------------------------------------------------------------------------------------------
// FLAECHE-3-VEREINIGUNG (KON25-02) -- EIN GEGENSTAND, ZWEI HISTORISCHE NAMEN
// ------------------------------------------------------------------------------------------------
// Owner 11.08.2026: "Ja genau, das ist Flaeche 3 und beide Konzepte muessen vereint werden."
//
// Das Drei-Flaechen-Modell (KON16-06):
//   FLAECHE 1  Genus-Interface einer Tier-Binary    abstract factory, Laufzeit
//   FLAECHE 2  der STEMPEL                          compile time factory, ABI-stabil
//   FLAECHE 3  der MEASUREMENT-DURCHSTICH           GENAU DIESE Naht (IMessVisitor)
//
// Flaeche 3 existiert, DAMIT Gattungs- und Genus-Interfaces UNVERAENDERT bleiben: die Messung
// quert als eigener Durchstich (tier_measure_accept + IMessVisitor), statt Mess-Felder oder
// Mess-Methoden in die fachlichen Interfaces zu draengen. Es gibt KEINEN zweiten Mechanismus:
// "measurement-Durchstich" (Plan-/Ledger-Vokabular, KON16-06) und "Mess-Visitor-Naht / NAHT-1"
// (Bau-Vokabular vom 09.08.2026) bezeichnen DIESELBE Flaeche -- dieser Header IST sie. Wer nach
// dem einen Begriff sucht, muss den anderen finden; deshalb stehen beide in diesem Kopf.
// Die Deckung der Gate-Kombination CEB=AUS/Tier=AN belegt am Objekt
// tests/unit/test_flaeche3_deckung_ceb_aus_tier_an.cpp (KON25-02-Deckungsluecke, Task #20).
//
// ------------------------------------------------------------------------------------------------
// DER KERN, WOERTLICH, UND WAS ER VERWIRFT
// ------------------------------------------------------------------------------------------------
// Owner 09.08.2026: "Derzeit FEHLT DIE MESSEINRICHTUNG ZWISCHEN CEB UND TIER-BINARY AM
// GENUS-INTERFACE, daher kann auch NICHTS GEMESSEN WERDEN, der Ansatz mit den SIDECARS IST FALSCH.
// Es muss stattdessen je aktiviert/deaktiviert der Messeinrichtungen in CEB und Tier-Binary AM
// INTERFACE EIN MESS-VISITOR UEBERGEBEN WERDEN oder bei deaktiviert eben nicht."
//
// VERWORFEN ist damit der POD-PULL: der Host griff mit tier_observe(Snapshot*) von aussen in das
// Tier hinein und schabte einen flachen Zustand ab. Das ist ein SIDECAR -- ein Betriebsmuster, das
// NEBEN dem Gegenstand haengt und ihn beobachtet. Es hat drei Eigenschaften, die genau die teuerste
// Fehlerklasse dieses Projekts erzeugen:
//   (1) Es misst die NACHBARSCHAFT, nicht den Gegenstand: was gemessen wird, entscheidet der Host
//       durch die Wahl der Felder, die er abholt -- nicht das Tier durch das, was es berichtet.
//   (2) Es hat KEINE Aktivierungs-Kopplung: der Host zieht, ob die Binary Messung eingebaut hat
//       oder nicht, und bekommt im zweiten Fall Nullen, die wie Messwerte aussehen. Ein richtiges
//       Messgeraet am falschen Gegenstand faellt nie auf -- es gibt nichts, was klappern koennte.
//   (3) Es existiert auch dann, wenn es nichts tut.
//
// ------------------------------------------------------------------------------------------------
// WARUM VISITOR STATT SIDECAR (die Begruendung gehoert laut KERN in den Code-Kommentar)
// ------------------------------------------------------------------------------------------------
//   - ZERO-COST BEI DEAKTIVIERT: kein Visitor => kein Aufruf, kein Zweig, kein Zustand. Der Adapter
//     erbt IObservableTier NUR unter COMDARE_MEASUREMENT_ON; unter OFF wird tier_measure_accept gar
//     nicht uebersetzt und steht als Symbol nicht in der Binary. Ein Sidecar existiert auch dann,
//     wenn er nichts tut -- das widerspricht der Compile-Time-Doktrin des Hauses.
//   - Der Visitor ist ein LEHRBUCH-MUSTER (GoF) und passt in "nur GoF, zero-cost Metaprogrammierung".
//     Ein Sidecar ist ein Betriebsmuster, kein Entwurfsmuster.
//   - Die Aktivierung ist ZWEISEITIG: CEB UND Tier-Binary. Eine UND-Bedingung, keine einseitige
//     Konfigurationsoption. Fehlt eine Seite, existiert der Pfad NICHT (statt still zu degradieren).
//   - Er trifft den GEGENSTAND statt der Nachbarschaft: das Tier BERICHTET durch die Naht, statt
//     sich abschaben zu lassen. Was gemessen wird, entscheidet damit die Seite, die es weiss.
//
// ------------------------------------------------------------------------------------------------
// WARUM EINE vtable UND KEIN TEMPLATE
// ------------------------------------------------------------------------------------------------
// Templates queren keine dlopen-Grenze. Der GoF-Visitor ist exakt das Muster fuer diesen
// Double-Dispatch. INNERHALB jeder Binary bleibt alles monomorphisiert (MessEdge<Sink> unten,
// Concept-Guard statt Laufzeit-Switch, kein std::function); typgeloescht wird NUR am ABI-Rand,
// einmal. Ueber die Grenze gehen ausschliesslich uint64/int64 und ein Zeiger auf ein FIXES
// uint64-Array -- kein STL-Typ, keine Ausnahme, alles noexcept.
//
// ------------------------------------------------------------------------------------------------
// WARUM DIESER HEADER AUCH UNTER MESSUNG-AUS UEBERSETZT
// ------------------------------------------------------------------------------------------------
// Er traegt AUSSCHLIESSLICH Deklarationen (eine abstrakte Klasse ohne Instanz, ein Concept, ein
// Template). Davon emittiert der Uebersetzer nichts, solange nichts instanziiert wird -- die
// Zero-Cost-Zusage haengt an der VERERBUNG im Adapter (abi_adapter.hpp, #if COMDARE_MEASUREMENT_ON),
// nicht an der Sichtbarkeit dieser Datei. Ein #error hier wuerde die real erreichbare
// Release-Neutralitaets-TU (tests/unit/test_a8s4_release_pfad_neutralitaet.cpp) brechen, die den
// OFF-Zustand ECHT uebersetzt -- also genau die Wache zerstoeren, die den OFF-Pfad belegt.
// Die compile-harte Wache gegen das Einschmuggeln der Naht in ein OFF-Kompilat sitzt deshalb dort,
// wo Mess-MASCHINERIE lebt: builder/pruef_dock/genus_mess_naht.hpp (#error-Guard).
//
// SELBSTCHECK
//   ZUSICHERT: hier steht KEINE Feldzahl und KEINE Achsenzahl als Kopie -- kMessVisitorAxisFieldCount
//              wird in observable_tier.hpp per static_assert an kV3FieldCount GEBUNDEN, nicht
//              wiederholt (Abschrift-Falle: eine Zahl, die als Kopie lebt, veraltet lautlos).
//   ZUSICHERT NICHT: dass ein uebergebener Visitor auch BENUTZT wird -- das ist Aussage der
//              Abnahme (tests/unit/test_naht1_mess_visitor_biss.cpp), nicht dieser Deklaration.
//
// ASCII-only.

#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

// ----------------------------------------------------------------------------------------------
// Feldbreite der E1-Zeile
// ----------------------------------------------------------------------------------------------

/// Zahl der uint64-Felder EINER E1-Achsen-Zeile. Der Wert IST kV3FieldCount; observable_tier.hpp
/// bindet beide per static_assert aneinander. Hier steht er, weil dieser Header den POD nicht kennen
/// darf (er ist die reine Naht-Deklaration) -- die Bindung ersetzt die Kopie.
inline constexpr std::size_t kMessVisitorAxisFieldCount = 8;

// ----------------------------------------------------------------------------------------------
// GenusFnId -- welche Genus-Funktion den Rahmen aufspannt
// ----------------------------------------------------------------------------------------------

/// ABI-stabile Kennung der gemessenen Genus-Funktion. Der Kern-Checkpoint ist 0; die
/// Genus-SPEZIALFUNKTIONEN (die Ebene, die laut Owner-Schichtung ueber dem Kerninterface der Gattung
/// liegt und die das Pruefdock der CEB sieht) beginnen bei 1000. Damit traegt EIN ABI-Vertrag
/// spaeter auch die per-Spezialfunktion-Messung, ohne einen zweiten Major-Bump.
namespace genus_fn_id {
inline constexpr std::uint64_t kCoreCheckpoint   = 0;    ///< der Fuellstands-Checkpoint des Kerninterfaces
inline constexpr std::uint64_t kGenusSpecialBase = 1000; ///< ab hier: je Genus-Spezialfunktion eine Id
} // namespace genus_fn_id

// ----------------------------------------------------------------------------------------------
// IMessVisitor -- die EINE ABI-stabile Visitor-Flaeche der Genus-Mess-Naht
// ----------------------------------------------------------------------------------------------

/// IMessVisitor -- GoF-Visitor am dlopen-Uebergang (Double-Dispatch: der Host reicht die Senke
/// hinein, das Tier ruft die passende visit_*-Methode).
///
/// VIER EBENEN, EIN VERTRAG. Die Hybrid-Ebene ist ab diesem Major Teil des Vertrags, obwohl ihr
/// Emitter erst mit HY-A2 kommt -- Reserve mit BENANNTEM Zweck (Muster: die 8er-Feldbreite des POD),
/// damit das Hybrid-Paket keinen zweiten koordinierten ABI-Bruch ueber den gesamten Bestand kostet.
///
/// REIHENFOLGE-VERTRAG je gemessenem E2-Rahmen (die Abnahme fuehrt dafuer einen Zustandsautomaten,
/// ein Ordnungsverstoss ist ein Fehler und kein Schoenheitsfehler):
///     e2_begin
///       [ hybrid_reroute ]                   (0..n, DAZWISCHENGESCHOBEN; Emitter erst HY-A2)
///       e1_axis_row(0) .. e1_axis_row(N-1)   (AUFSTEIGEND, genau einmal je Achse)
///     e2_end
///
/// NENNER-DOKTRIN: kein Ereignis traegt eine nackte Zahl. axis_count steht im begin; die
/// P-MD3-Invariante quert im end als PAAR (seg_framework_ns + seg_run_total_ns), nie als Summe --
/// so bleibt Sum(seg_ns) + seg_framework_ns == seg_run_total_ns in der Senke pruefbar.
class IMessVisitor {
public:
    virtual ~IMessVisitor() = default;

    /// E2-Rahmen auf: EIN gemessener Genus-Funktions-Durchlauf.
    /// axis_count ist der NENNER der folgenden E1-Zeilen (nicht ableiten, nicht raten).
    virtual void visit_e2_begin(std::uint64_t genus_id, std::uint64_t fn_id, std::uint64_t fill_level,
                                std::uint64_t axis_count) noexcept = 0;

    /// E1-Zeile: GENAU EINE Achse. fields zeigt auf genau kMessVisitorAxisFieldCount uint64
    /// (Schema = kV3AxisSchema, single-source in observable_tier.hpp). Der Zeiger ist NUR waehrend
    /// des Aufrufs gueltig -- die Senke kopiert, was sie behalten will.
    virtual void visit_e1_axis_row(std::uint64_t axis_idx, std::uint64_t const* fields,
                                   std::int64_t seg_ns) noexcept = 0;

    /// Vierte Ebene (Hybrid): der HY-A2-Router meldet eine Umleitung von Genus zu Genus.
    /// Noch ohne Emitter -- der Slot steht ab diesem Major, s. Reserve-Begruendung oben.
    virtual void visit_hybrid_reroute(std::uint64_t from_genus, std::uint64_t to_genus,
                                      std::int64_t decision_ns) noexcept = 0;

    /// E2-Rahmen zu: die Meta + der NENNER des Segment-Laufs.
    virtual void visit_e2_end(std::uint64_t observable_axis_count, std::uint64_t filled_axis_count,
                              std::uint64_t batches_measured, std::int64_t seg_framework_ns,
                              std::int64_t seg_run_total_ns) noexcept = 0;
};

// ----------------------------------------------------------------------------------------------
// GenusMessSink + MessEdge -- die metaprogrammierte Seite (Owner 09.08.: die Einrichtungen fuer die
// CEB muessen METAPROGRAMMIERT am Genus-Interface stehen, nicht zur Laufzeit nachgereicht werden)
// ----------------------------------------------------------------------------------------------

/// GenusMessSink -- Concept-Guard statt Laufzeit-Switch. Eine Senke ist ein GEWOEHNLICHER Typ mit
/// vier noexcept-Methoden; sie erbt NICHTS und zahlt keine vtable. Wer die Flaeche verfehlt, bekommt
/// einen Concept-Fehler an der Instanziierung -- nicht einen stillen Overload-Fehlgriff.
template <class S>
concept GenusMessSink = requires(S s, std::uint64_t u, std::uint64_t const* f, std::int64_t n) {
    { s.e2_begin(u, u, u, u) } noexcept;
    { s.e1_axis_row(u, f, n) } noexcept;
    { s.hybrid_reroute(u, u, n) } noexcept;
    { s.e2_end(u, u, u, n, n) } noexcept;
};

/// MessEdge<Sink> -- der EINE Typloeschungs-Punkt der Naht, final, je Senke monomorphisiert.
/// Er haelt eine REFERENZ auf die Senke (kein Besitz, kein std::function, keine Allokation) und
/// leitet 1:1 weiter. Genau ein virtueller Sprung je Ereignis, und der faellt am dlopen-Rand an --
/// nirgends sonst.
///
/// WARUM DIE SENKE PER REFERENZ UND DER VISITOR AUCH: es gibt keinen null-Zustand, also nichts,
/// worauf man `if` sagen koennte. Das verworfene `if (visitor)` kann strukturell nicht entstehen.
template <GenusMessSink Sink>
class MessEdge final : public IMessVisitor {
public:
    explicit MessEdge(Sink& sink) noexcept : sink_(&sink) {}

    void visit_e2_begin(std::uint64_t genus_id, std::uint64_t fn_id, std::uint64_t fill_level,
                        std::uint64_t axis_count) noexcept override {
        sink_->e2_begin(genus_id, fn_id, fill_level, axis_count);
    }
    void visit_e1_axis_row(std::uint64_t axis_idx, std::uint64_t const* fields, std::int64_t seg_ns) noexcept override {
        sink_->e1_axis_row(axis_idx, fields, seg_ns);
    }
    void visit_hybrid_reroute(std::uint64_t from_genus, std::uint64_t to_genus,
                              std::int64_t decision_ns) noexcept override {
        sink_->hybrid_reroute(from_genus, to_genus, decision_ns);
    }
    void visit_e2_end(std::uint64_t observable_axis_count, std::uint64_t filled_axis_count,
                      std::uint64_t batches_measured, std::int64_t seg_framework_ns,
                      std::int64_t seg_run_total_ns) noexcept override {
        sink_->e2_end(observable_axis_count, filled_axis_count, batches_measured, seg_framework_ns, seg_run_total_ns);
    }

private:
    Sink* sink_;
};

} // namespace comdare::cache_engine::anatomy
