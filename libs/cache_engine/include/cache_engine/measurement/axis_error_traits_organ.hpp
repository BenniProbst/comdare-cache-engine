#pragma once
// measurement/axis_error_traits_organ.hpp -- FK-4 (A15, ORG-18): der Fehlerraum der 18 ORGAN-SLOTS,
// gefuehrt in DERSELBEN AxisErrorTraits-Familie wie FK-3.
//
// -- WAS FK-4 IST UND WORIN ES SICH VON FK-5 UNTERSCHEIDET (die Frage, an der der Posten haengt) ------
// FK-5 (gelandet, f4ce89fe) haengt `error_classes()` an die 18 Organ-Haupt-Achsen-CRTP-BASEN. Sein
// Schluessel ist der C++-TYP: `SearchAlgoBase<V>` sagt, was ein Such-ALGORITHMUS hervorbringen kann.
// FK-4 fuehrt denselben Fehlerraum am SLOT -- an der POSITION T0..T17. Das ist nicht dieselbe Aussage
// in zwei Schreibweisen, sondern die zweite Haelfte: binary_id, Stempel, Segment-Zeiten und CSV-Spalten
// sind ALLE ueber die Slot-POSITION indiziert, nicht ueber den Typ (kCompositionAxisNames-Reihenfolge;
// seg_ns[18] in measurable_workload.hpp:101; axis_stats in observable_tier.hpp:45). Wer eine Zelle in
// Spalte T12 vor sich hat, hat eine POSITION und keinen Typ -- und konnte bisher nicht fragen, welcher
// Fehlerraum zu ihr gehoert.
//
// GENAU DIESE LUECKE HAT DAS A15-DESIGN ALS OFFEN AUSGEWIESEN, Befund B5 verbatim: "FK-4-Schluessel-
// Mechanik offen: die 18 Organ-Slots sind POSITIONEN (kCompositionAxisNames/kOrganAxisCount=18,
// anatomy_version_stamp.hpp:37), keine Typen; worauf AxisErrorTraits<...> je Slot spezialisiert wird
// (Tag-Typen? per-Achse-Registry-Namespaces?) ist unbenannt -- die Wache gegen kOrganAxisCount ist ohne
// Schluessel-Typen nicht baubar." Dieser Header BENENNT den Schluessel: OrganSlot<N>, ein reiner
// CT-Tag ohne Speicher und ohne Laufzeit-Spur.
//
// -- WARUM DER SCHLUESSEL EIN EIGENER TAG IST UND NICHT DIE CRTP-BASIS (Layering, am Ist erhoben) -----
// Der naheliegende Schluessel waere die Organ-CRTP-Basis selbst (K5-Vorschlag des Designs: "die 18
// per-Achse-Registry-Tags aus axes/*/axis_*_registry.hpp"). Das geht hier NICHT, und zwar aus genau dem
// Grund, aus dem FK-5 seinen Header ins topics/-Dach gelegt hat: measurement/ zieht am Ist WEDER axes/
// NOCH abi/ -- eigene, vollstaendige Erhebung ueber alle #include-Zeilen in
// include/cache_engine/measurement/*.hpp (nicht nur die <>-Schreibweise: die relativen "../../../"-
// Includes sind mitgezaehlt, sonst haette man topics/axis.hpp uebersehen). Die 18 Basen hier
// hereinzuziehen waere eine Baseline-Layering-Aenderung als Nebenwirkung einer Fehlerklassen-
// Deklaration -- dieselbe Kante, die FK-5 ausdruecklich NICHT gezogen hat.
//
// -- WIE DIE EINE WAHRHEITSQUELLE EINE BLEIBT (drei Rueckbindungen, alle verwacht) -------------------
// Dieser Header fuehrt drei Dinge, die anderswo schon stehen: die 18 Slot-NAMEN, die ZAHL 18 und die
// FEHLERRAEUME. Keines davon darf eine zweite Wahrheit werden. Die Rueckbindung steht NICHT als Zusage
// im Kommentar, sondern in der Cross-Layer-Test-TU tests/unit/test_a15_fk4_organ_slot_traits.cpp --
// einer TU, die alle Schichten sehen darf (Praezedenz woertlich: FK-5s test_e24_c9_fk5_fehlerraum.cpp
// und davor test_w10_c4_zellwert_naht.cpp, "Ohne diese Zeile stuende die Gleichheit nur als Zusage im
// Kommentar."). Verwacht werden dort:
//   (1) slot_name[i] == experiment::kCompositionAxisNames[i], fuer alle 18, IN DIESER REIHENFOLGE;
//   (2) kOrganSlotCount == abi::kOrganAxisCount == kCompositionAxisNames.size();
//   (3) der D2-Satz JEDES Slots == der error_classes()-Satz der zugehoerigen FK-5-CRTP-Basis.
// (3) ist die eigentliche Anti-Parallelstruktur-Wache: ohne sie waeren FK-4 und FK-5 zwei Behauptungen
// ueber denselben Sachverhalt, und zwei Behauptungen ueber dieselbe Sache driften.
//
// -- KEINE ZWEITE ETIKETTEN-SPUR ---------------------------------------------------------------------
// FK-5 fuehrt seine Klassen als string_view-Etiketten (topics/ kann measurement/ nicht sehen und muss
// spiegeln). FK-4 liegt IN measurement/ und fuehrt sie deshalb als SampleStatus -- die Quelle selbst,
// kein Spiegel. Die Bruecke zwischen beiden Formen ist die eine kanonische Funktion
// sample_status_label(); die TU laeuft ueber sie.
//
// ASCII-only.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/axis_error_traits.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// Die Zahl der Organ-Slots. SPIEGEL von abi::kOrganAxisCount (anatomy_version_stamp.hpp:39) und von
/// experiment::kCompositionAxisNames.size() (axis_path_serialization.hpp:40). Ein Spiegel ist er, weil
/// measurement/ weder abi/ noch builder/ sehen darf (s. Kopf); dass er stimmt, ist in der Test-TU
/// verwacht und nicht zugesichert.
inline constexpr std::size_t kOrganSlotCount = 18;

/// OrganSlot<N> -- der SCHLUESSEL-TAG einer Organ-Position (T0..T17). Reiner Compile-Zeit-Tag: leer,
/// nie instanziiert, ohne Laufzeit-Spur. Er ist bewusst FUER JEDES N bildbar (und nicht nur fuer
/// 0..17), damit die Drift-Wache unten die Gegenrichtung ueberhaupt formulieren kann: OrganSlot<18>
/// existiert als Typ, hat aber KEINEN Traits-Eintrag -- und genau das wird geprueft.
template <std::size_t Slot>
struct OrganSlot {
    static constexpr std::size_t slot_index = Slot;
};

namespace detail {

/// DIE DREI ORGAN-EIGENEN INVARIANTEN, zusaetzlich zu den vier aus axis_error_traits.hpp.
///  (O1) NUR D2. Die A15-Realm-Zuordnung sagt woertlich "organ-Achsen -> D2 (Failed am Pruef-Dock-
///       Gate)". Ein Organ-Slot, der eine D1-Domaene fuehrte, haette ein BAU-Urteil in die Mess-Spalte
///       geholt -- und dann waere "nicht gebaut" von "gemessen und gescheitert" nicht mehr zu
///       unterscheiden. Das ist exakt die Verschmelzung, gegen die die W-4-Doktrin in axis_error.hpp
///       drei getrennte Vokabulare gebaut hat (:112-118).
///  (O2) DER REALM-BODEN. Jeder Slot fuehrt SampleStatus::Failed. Das ist keine Annahme ueber den
///       einzelnen Algorithmus, sondern eine Eigenschaft des Pruef-Dock-Gates, durch das alle laufen
///       (dieselbe Begruendung wie FK-5s kOrganAxisErrorFloor, organ_axis_error_classes.hpp:54-57).
///       Ein Slot ohne Boden behauptete, an diesem Gate nicht scheitern zu koennen.
///  (O3) EIN NAME. Ein leerer slot_name macht die Namens-Rueckbindung (1) zur Tautologie.
/// Sie ist ueber den SLOT-TYP parametrisiert, nicht ueber dessen Index. Der Unterschied ist nicht
/// kosmetisch: eine Fassung ueber `Slot::slot_index` schlaegt intern doch wieder bei
/// AxisErrorTraits<OrganSlot<N>> nach -- sie prueft dann gar nicht den hereingereichten Typ, sondern
/// einen daraus neu abgeleiteten. Genau damit waere sie fuer eine Negativ-Probe blind: ein bewusst
/// falsch deklarierter Slot-artiger Typ wuerde geprueft, indem man ihn wegwirft.
template <class Slot>
[[nodiscard]] constexpr bool organ_slot_invarianten_erfuellt() noexcept {
    auto const& dom = AxisErrorTraits<Slot>::domains;
    auto const& d2  = AxisErrorTraits<Slot>::d2_klassen;
    if (dom.size() != 1 || dom[0] != ErrorDomain::Sample) return false; // O1
    if (!satz_enthaelt(d2, SampleStatus::Failed)) return false;         // O2
    if (AxisErrorTraits<Slot>::slot_name.empty()) return false;         // O3
    return true;
}

} // namespace detail

/// Ein Organ-Slot traegt einen vollstaendigen UND organ-konformen Fehlerraum.
template <class Slot>
concept OrganSlotMitFehlerklassen = AxisMitFehlerklassen<Slot> && requires {
    { AxisErrorTraits<Slot>::slot_name } -> std::convertible_to<std::string_view>;
} && detail::organ_slot_invarianten_erfuellt<Slot>();

namespace detail {

template <std::size_t... Is>
[[nodiscard]] constexpr bool alle_organ_slots_belegt(std::index_sequence<Is...>) noexcept {
    return (OrganSlotMitFehlerklassen<OrganSlot<Is>> && ...);
}

} // namespace detail

// ================================================================================================
//  DIE 18 SLOTS. Sie stehen hier nach FEHLERRAUM GRUPPIERT und NICHT in T0..T17-Reihenfolge -- die
//  Indizes springen deshalb (0-7, 9-11, 14-16, dann 8, 12, 13, 17). Das ist Absicht: so steht die
//  AUSNAHME (die vier n/a-Slots) beisammen und faellt beim Lesen auf, statt zwischen vierzehn
//  gleichen Zeilen zu verschwinden. Die POSITION ist im ersten Makro-Argument explizit, und dass sie
//  zu kCompositionAxisNames passt, prueft die Test-TU je Slot -- die Reihenfolge dieser Zeilen ist
//  also keine Aussage und muss keine sein.
//
//  WORAUS DIE SAETZE ERHOBEN SIND -- nichts hier ist neu entschieden: die Aufteilung ist die von
//  FK-5, das sie seinerseits am Qualitaets-Parameter-Katalog belegt hat. 14 Slots fuehren den blossen
//  Realm-Boden {Failed}; VIER fuehren zusaetzlich {NotApplicable}, weil ihre Mess-Groessen am
//  1-Thread-/In-Memory-/decide-only-Mess-Kanon STRUKTURELL leer bleiben -- und diese Leere ist ehrlich
//  "n/a" und ausdruecklich NICHT "failed". Wuerde man sie als failed fuehren, meldete die Auswertung
//  einen Defekt, wo eine bewusste Kanon-Entscheidung steht; wuerde man sie als 0 fuehren, waere es die
//  stille Null, die das ganze Framework verhindern soll.
// ================================================================================================

#define COMDARE_FK4_SLOT(IDX, NAME, ...)                                                                               \
    template <>                                                                                                        \
    struct AxisErrorTraits<OrganSlot<IDX>> {                                                                           \
        static constexpr std::string_view slot_name  = NAME;                                                           \
        static constexpr auto             domains    = std::array{ErrorDomain::Sample};                                \
        static constexpr auto             d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};                    \
        static constexpr auto             d2_klassen = std::array{__VA_ARGS__};                                        \
    }

// -- Die 14 Slots mit dem blossen Realm-Boden ----------------------------------------------------
COMDARE_FK4_SLOT(0, "search_algo", SampleStatus::Failed);
COMDARE_FK4_SLOT(1, "cache_traversal", SampleStatus::Failed);
COMDARE_FK4_SLOT(2, "mapping", SampleStatus::Failed);
COMDARE_FK4_SLOT(3, "path_compression", SampleStatus::Failed);
COMDARE_FK4_SLOT(4, "node_type", SampleStatus::Failed);
COMDARE_FK4_SLOT(5, "memory_layout", SampleStatus::Failed);
COMDARE_FK4_SLOT(6, "allocator", SampleStatus::Failed);
COMDARE_FK4_SLOT(7, "prefetch", SampleStatus::Failed);
COMDARE_FK4_SLOT(9, "serialization", SampleStatus::Failed);
COMDARE_FK4_SLOT(10, "value_handle", SampleStatus::Failed);
COMDARE_FK4_SLOT(11, "index_organization", SampleStatus::Failed);
COMDARE_FK4_SLOT(14, "filter", SampleStatus::Failed);
COMDARE_FK4_SLOT(15, "queuing_q1", SampleStatus::Failed);
COMDARE_FK4_SLOT(16, "queuing_q2", SampleStatus::Failed);

// -- Die VIER Slots mit strukturellem n/a, je am Katalog belegt (FK-5-Commit, Abschnitt "WORAUS") --
/// T8 concurrency: Qualitaets-Parameter "single-thread-0" -- am 1-Thread-Mess-Kanon bleiben die
/// Contention-Groessen strukturell leer.
COMDARE_FK4_SLOT(8, "concurrency", SampleStatus::Failed, SampleStatus::NotApplicable);
/// T12 io_dispatch: "Simulation, ehrlich gedeckelt" -- Geraete-Groessen existieren in diesem Kanon nicht.
COMDARE_FK4_SLOT(12, "io_dispatch", SampleStatus::Failed, SampleStatus::NotApplicable);
/// T13 migration_policy: "decide-only" -- ein realer Tier-Umzug findet nicht statt.
COMDARE_FK4_SLOT(13, "migration_policy", SampleStatus::Failed, SampleStatus::NotApplicable);
/// T17 persistence_target: honest-0-Klasse (memory_only) -- kein Persistenz-Ziel, also keine Zahl.
COMDARE_FK4_SLOT(17, "persistence_target", SampleStatus::Failed, SampleStatus::NotApplicable);

#undef COMDARE_FK4_SLOT

// -- Die Drift-Wachen, BEIDE Richtungen (RF-3-Lehre aus axis_error.hpp:469-471) -------------------
// Richtung 1: alle 18 Slots sind belegt und organ-konform. Faengt eine VERGESSENE Achse.
static_assert(detail::alle_organ_slots_belegt(std::make_index_sequence<kOrganSlotCount>{}),
              "FK-4: nicht alle 18 Organ-Slots tragen einen organ-konformen Fehlerraum.");
// Richtung 2: hinter dem Count liegt KEIN belegter Slot. Richtung 1 allein faengt ein ANHAENGEN nicht
// -- sie bliebe wahr, wenn jemand einen 19. Slot einzieht und kOrganSlotCount zu erhoehen vergisst.
// Das ist woertlich die Lehre, die axis_error.hpp fuer JEDE seiner Taxonomien zweimal gezogen hat.
static_assert(!OrganSlotMitFehlerklassen<OrganSlot<kOrganSlotCount>>,
              "FK-4-Drift: hinter kOrganSlotCount liegt ein belegter Organ-Slot -- der Count ist zu klein.");

} // namespace comdare::cache_engine::measurement
