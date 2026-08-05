// A8-S5 Familie 01_read_path / Sub-Scheibe 01c (die REGISTRY-ORGANE der Achse 03a) -- FAMILIEN-WACHE
// der FASSADEN-KONSTRUKTION.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 6.
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt"); Design-Entscheid abend-14 (D1);
// Manager-Entscheid 05.08. nacht-2 (D1 bleibt fuer die Vor-Anker-Strecke).
//
// WAS DIESE SCHEIBE ANDERS MACHT ALS 01a/01b/01d -- und warum das eine eigene Wache braucht:
// Jene Scheiben banden ihre Organe an einen BENANNTEN ACHSEN-DEFAULT (ExgenAllocator). Das erfuellt
// die Schnitt-Regel ("Speicher NUR ueber das Allokator-Achsen-Interface"), aber NICHT die
// Owner-Vorgabe "Option B strikt": die Komposition waehlt eine EIGENE Strategie (die ausgelieferten
// Referenz-Kompositionen fuehren durchgaengig MimallocAllocator), und solange das Organ die Wahl
// nicht uebernimmt, laeuft im selben Tier eine stille ZWEITE Strategie. 01c dreht genau das.
//
// ===================================================================================================
// SCHEIBE 2 (2026-08-05): DIE TYP-LISTE IST GEHOBEN -- ABGELEITET STATT AUFGEZAEHLT.
// ===================================================================================================
// Der Pilot-Stand nannte sein EINES Organ beim Namen (`using Fassade = lk::LinearScanSearchAlgo;`).
// Das war fuer eine Scheibe richtig und fuer vier falsch: eine Wache, die man beim Erweitern der
// Familie von Hand nachziehen MUSS, ist genau die Wache, die beim naechsten Mal vergessen wird -- und
// ihr gruener Lauf saehe unveraendert aus (die Lehre "gruene Tests zementieren alte Ordnung", nur
// eine Ebene hoeher). Diese TU leitet ihre Population deshalb aus der ACHSEN-REGISTRY ab:
//
//     MigrierteOrgane = mp_filter<traegt_migrations_ausweis, lk::AllStrategies>
//
// Das Filter-Praedikat ist der Migrations-Ausweis selbst (composable::AllocatorRebindableSearchAlgo).
// Wer in Scheibe 3/4 einen `rebind_allocator` bekommt, steht ab demselben Commit unter dieser Wache,
// ohne dass hier eine Zeile faellt. Wer ihn VERLIERT, faellt aus der Population -- deshalb steht
// darunter der Vollstaendigkeits-Pin ueber lk::EnabledStrategies (unten (0)), der genau diesen
// stillen Ausfall abfaengt: eine leere oder geschrumpfte Population koennte sonst 100% gruen melden.
//
// DIE ZWEI EBENEN, DIE HIER GEPINNT WERDEN (und warum die Trennung keine Geschmacksfrage ist):
//   IDENTITAETS-EBENE  = die namens-stabile Fassade (LinearScanSearchAlgo, InterpolationSearchAlgo,
//     EytzingerSearchAlgo, KArySearchAlgo, ...). Sie ist das Registry-Organ: ihr type_name wird von
//     tools/axis_registry_gen in die committete algorithm_profiles/cache_engine_axis_registry.xml
//     reflektiert, der F30-Guard verlangt, dass `type=` mit dem ORGAN_LOCATION-Literal beginnt, und
//     die Konsumenten-TUs nennen ihren Namen.
//   SUBSTANZ-EBENE     = der Core (parametriert) und sein Rebound-Leaf. Sie tragen den Algorithmus und
//     die Allokator-Bindung, aber KEINE Identitaet -- kein ORGAN_LOCATION, kein Registry-Eintrag.
// Die Wache prueft deshalb nicht nur "ist gebunden", sondern auch "ist NICHT vermischt": ein
// Rebound-Typ, der in die Identitaets-Ebene rutscht, drehte Emitter-Typnamen und Registry-Bytes.
//
// GEPRUEFTE EBENEN:
//   (0) POPULATIONS-PIN   -- die abgeleitete Liste ist weder leer noch geschrumpft (Anti-Vakuositaet)
//   (1) FORM-AUSWEIS      -- Fassade UND Rebound-Leaf erfuellen die Familien-Konformitaet (Form B)
//   (2) CONCEPT-BEWEIS    -- der Rebound-Leaf ist ein VOLLWERTIGES Organ, nicht nur eine Typ-Huelle
//                            (die CRTP-Guard-Kette laeuft auf BEIDEN Leaves)
//   (3) IDENTITAETS-PIN   -- Level 0: die Naht liefert am Achsen-Default die Fassade SELBST (is_same)
//   (4) DURCHBINDUNG      -- Level 1: die Naht liefert bei fremder Strategie den Rebound-Leaf
//   (5) name()-INVARIANZ  -- die T6-Wahl leckt NICHT in den serialize-/binary_id-Schluessel
//   (6) KONTRAST/LAUFZEIT -- der Speicher des Organs kommt REAL aus der T6-Wahl der KOMPOSITION
//   (7) VERHALTENS-PIN    -- der Rebind aendert die Strategie, NICHT den Algorithmus
//   (8) REGISTRY-BYTE     -- die committete XML traegt den arglosen Typ-Namen unveraendert
//   (8b) XML-ABWESENHEIT  -- die migrierten Default-OFF-Organe stehen NICHT drin (Byte-Neutralitaet der
//                            per-K-Leaf-Hebung, am Artefakt statt behauptet)
//   (9) F30-RELATION      -- "::" + type_name<S>() == ORGAN_LOCATION-Literal, am TYP statt am Artefakt:
//                            genau die Relation, die der Generator-Guard prueft, hier auch fuer die
//                            Default-OFF-Organe, deren Kante bis zur Hebung latent offen war
// (1)-(5) und (9) laufen compile-hart ueber die GANZE abgeleitete Liste; (6)-(8) laufen zur Laufzeit ueber
// dieselbe Liste (mp_for_each), (8) gefiltert auf die ENABLED-Teilmenge -- nur die steht ueberhaupt in der
// committeten XML (der Generator reflektiert Enabled*, main.cpp:207) -- und (8b) auf ihr Komplement.
//
// WARUM (6) NICHT ALS BLOSSES ">0" GEBAUT IST (02a-HERZ-Vorbild, LEDGER abend-13): ein ">0" waere auch
// am ALT-Stand gruen gewesen, sobald IRGENDEIN Achsen-Zaehler lief. Die Aussage dieser Scheibe ist
// aber eine ANDERE: der Zaehler, der sich bewegt, ist der der KOMPOSITIONS-Strategie. Die Wache
// vergleicht deshalb DREI Groessen gegeneinander -- den Zaehler der kompositions-gebundenen Instanz,
// den Zaehler der default-gebundenen Instanz und den ehrlichen Nullpunkt einer ungetriebenen Instanz --
// und verlangt, dass die beiden getriebenen ZAHLENGLEICH sind (gleicher Algorithmus, gleiches
// Wachstum) waehrend sie auf VERSCHIEDENEN Strategie-Typen liegen. Am Alt-Stand (nackter
// std::vector) waeren BEIDE 0 gewesen -- die Probe kann also beissen.
//
// GRENZE, DEKLARIERT (S5-04-Praezedenz, damit die spaetere CSV-Lektuere nicht irrefuehrt): diese
// Scheibe legt die NAHT search_allocator_statistics() je Core; sie fuehrt KEINE CSV-Spalte ein. Die
// Einsammlung + die Doppelzaehlungs-Regel (konstitutiver Store-Snapshot vs. Summe der disjunkten
// Organ-Snapshots) sind der explizite Schritt des Mess-Schnitt-Fensters VOR Messbeginn (abend-11 (a)).
//
// ANTI-ZEMENTIERUNGS-BISS (gefahren, nicht behauptet): der Pilot-Stand dieser TU wurde gegen den
// ALT-Stand des Organs (14a8cfbc) uebersetzt und schlug COMPILE-HART fehl, 21 Fehler, darunter
// "'LinearScanSearchAlgoRebound' does not name a template type". Der GEHOBENE Stand wurde zusaetzlich
// gegen den Pilot-Stand der ORGANE gefahren (8571ac01: nur linear_scan migriert, die drei anderen
// enabled-Organe nicht) und schlaegt dort COMPILE-HART am VOLLSTAENDIGKEITS-Pin fehl:
//   "error: static assertion failed: 01c VOLLSTAENDIGKEIT VERLETZT: mindestens ein ENABLED Organ der
//    Achse 03a traegt keinen Migrations-Ausweis."
// Die Ableitung kann den stillen Familien-Ausfall also SEHEN -- ohne diesen Pin waere eine
// geschrumpfte Population lautlos gruen durchgelaufen.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <anatomy/organ_location.hpp>
#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/lookup/axis_03a_search_algo_registry.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <axes/lookup/composable/traversal_for_search_algo.hpp>
#include <builder/codegen/type_name.hpp> // die F30-Relation am TYP pruefen (nur <string_view>, keine Link-Kante)
#include <compositions/art_paper_binding_reference.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace s5   = ::comdare::cache_engine::tests::s5;
namespace lk   = ::comdare::cache_engine::lookup;
namespace comp = ::comdare::cache_engine::lookup::composable;
namespace ca   = ::comdare::cache_engine::alloc;
namespace anat = ::comdare::cache_engine::anatomy;
namespace mp   = boost::mp11;

namespace {

// ---------------------------------------------------------------------------------------------
// DIE T6-WAHL DER KOMPOSITION -- ABGELEITET, NICHT HINGESCHRIEBEN.
// Der Kontrast-Allokator kommt aus einer AUSGELIEFERTEN Referenz-Komposition. Haende weg von
// "MimallocAllocator" als Literal: wechselt die Komposition ihre T6-Wahl, soll diese Wache die
// NEUE Wahl pruefen, nicht eine historische.
// ---------------------------------------------------------------------------------------------
using KompositionsAllokator  = ::comdare::cache_engine::compositions::ArtPaperBindingComposition::allocator;
using AchsenDefaultAllokator = ca::ExgenAllocator;

// Die Probe ist nur dann eine Probe, wenn die Komposition ueberhaupt etwas ANDERES will als der
// Achsen-Default. Waeren beide gleich, liefe (4)/(6) auf Level 0 und behauptete nichts.
static_assert(!std::is_same_v<KompositionsAllokator, AchsenDefaultAllokator>,
              "01c-Gate VAKUOOS: die Referenz-Komposition fuehrt denselben Allokator wie der Achsen-Default -- "
              "dann prueft der Kontrast-Beweis keinen Rebind mehr. Andere Komposition waehlen.");

// =============================================================================================
// DIE ABLEITUNG -- die Population dieser Wache ist die Achsen-Registry, gefiltert auf den
// Migrations-Ausweis. Sie waechst mit Scheibe 3/4 von selbst mit.
// =============================================================================================
template <class S>
using traegt_migrations_ausweis = mp::mp_bool<comp::AllocatorRebindableSearchAlgo<S, KompositionsAllokator>>;

template <class S>
using ist_disabled = mp::mp_bool<!S::enabled>;

using MigrierteOrgane     = mp::mp_filter<traegt_migrations_ausweis, lk::AllStrategies>;
using MigriertUndEnabled  = mp::mp_filter<lk::is_enabled, MigrierteOrgane>;
using MigriertUndDisabled = mp::mp_filter<ist_disabled, MigrierteOrgane>;

// =============================================================================================
// (0) POPULATIONS-PIN -- die Anti-Vakuositaets-Wache der ABLEITUNG.
//     Ohne sie koennte diese TU bei einem stillen Familien-Ausfall (rebind_allocator verschwindet)
//     ueber eine LEERE Liste laufen und "alles gruen" melden. Der Pin ist bewusst NICHT als feste
//     Zahl gebaut (die muesste jede Scheibe nachziehen), sondern als AUSSAGE ueber die Achse.
// =============================================================================================
static_assert(mp::mp_size<MigrierteOrgane>::value > 0,
              "01c-Gate TOT: kein einziges Organ der Achse 03a traegt einen Migrations-Ausweis -- die abgeleitete "
              "Population ist leer und alle Ebenen dieser Wache liefen vakuoos gruen.");

/// DER VOLLSTAENDIGKEITS-PIN. Genau die ENABLED-Organe der Achse stehen in der committeten
/// Registry-XML (der Generator reflektiert Enabled*, axis_registry_gen main.cpp:207) und liegen damit
/// auf dem Mess-Pfad. Ein nicht migriertes Organ DARUNTER waere exakt die stille zweite Strategie, die
/// 01c abstellt. Die Aussage ist derivations-symmetrisch -- sie nennt kein Organ und keine Zahl.
static_assert(mp::mp_size<MigriertUndEnabled>::value == mp::mp_size<lk::EnabledStrategies>::value,
              "01c VOLLSTAENDIGKEIT VERLETZT: mindestens ein ENABLED Organ der Achse 03a traegt keinen "
              "Migrations-Ausweis. Es faellt damit aus dieser Wache HERAUS (die Population ist ueber den "
              "Ausweis gefiltert) und liefe im selben Tier mit einer zweiten, stillen Allokator-Strategie "
              "-- Owner-KERN LEDGER 04.08. abend-11 'Option B strikt'.");

// =============================================================================================
// DER VERTRAG JE ORGAN -- compile-hart, einmal je Typ der abgeleiteten Population instanziiert.
// Die Ebenen (1)-(5) stehen hier zusammen, damit eine Verletzung den TYP im Diagnose-Text nennt
// (mp_all_of instanziiert das Praedikat fuer JEDES Element; es gibt kein Kurzschliessen).
// =============================================================================================
template <class S>
struct FassadenVertrag {
    using DefaultAllokator = typename S::allocator_type;
    using Rebound          = comp::search_algo_for_composition_t<S, KompositionsAllokator>;

    // ---- (1) FORM-AUSWEIS: beide Ebenen sind familien-konform, und zwar ueber Form B. ----
    static_assert(s5::FamilyAllocConform<S>, "01c: die Fassade ist nicht familien-konform.");
    static_assert(s5::FamilyAllocConform<Rebound>, "01c: der Rebound-Leaf ist nicht familien-konform.");
    static_assert(s5::AxisAllocatorBoundOrgan<S>, "01c: die Fassade traegt keine Achsen-Bindung (Form B).");
    static_assert(s5::AxisAllocatorBoundOrgan<Rebound>,
                  "01c: der Rebound-Leaf traegt keine Achsen-Bindung (Form B).");
    // Die Bindung ist die RICHTIGE -- nicht irgendeine.
    static_assert(std::is_same_v<DefaultAllokator, AchsenDefaultAllokator>,
                  "01c: die Fassade haengt nicht mehr am benannten Achsen-Default.");
    static_assert(std::is_same_v<typename Rebound::allocator_type, KompositionsAllokator>,
                  "01c DURCHBINDUNG VERFEHLT: der Rebound-Leaf traegt nicht die T6-Wahl der Komposition.");

    // ---- (2) CONCEPT-BEWEIS: der Rebound-Leaf ist ein vollwertiges Organ (CRTP-Guard-Kette auf ----
    //          BEIDEN Leaves; R2 des Design-Risikoblatts: ein falscher Self-Parameter braeche sie LEISE).
    static_assert(lk::concepts::SearchAlgoVariant<Rebound>,
                  "01c: der Rebound-Leaf erfuellt SearchAlgoVariant nicht -- CRTP-Self-Vertrag gebrochen.");
    static_assert(lk::concepts::CacheEngineSearchAlgoPermutationStrategy<Rebound>,
                  "01c: der Rebound-Leaf ist nicht permutations-faehig.");
    static_assert(lk::concepts::DensityClassifiedStrategy<Rebound>,
                  "01c: der Rebound-Leaf traegt die Dichte-Klassifikation nicht mehr.");

    // ---- (3) IDENTITAETS-PIN (Level 0) -- DER golden-Beweis dieser Scheibe. ----
    //      Was typ-identisch ist, kann sich in binary_id, serialize-Pfad und Registry-XML nicht bewegen.
    static_assert(std::is_same_v<comp::search_algo_for_composition_t<S, DefaultAllokator>, S>,
                  "01c LEVEL-0 VERLETZT: die Kompositions-Naht liefert am Achsen-Default NICHT die Fassade "
                  "selbst. Damit laege ein anderer Typ auf dem golden-Pfad.");
    // Gegenprobe: die Level-0-Aussage ist nicht deshalb wahr, weil die Naht IMMER die Identitaet liefert.
    static_assert(!std::is_same_v<Rebound, S>,
                  "01c-Gate TOT: die Naht liefert auch bei FREMDER Strategie die Fassade -- dann pinnt der "
                  "Level-0-Beweis nichts, weil er gar nicht unterscheiden kann.");

    // ---- (4) DURCHBINDUNG (Level 1) + EBENEN-TRENNUNG. ----
    static_assert(!comp::IsReboundSearchAlgoLeaf<S>,
                  "01c EBENEN-VERMISCHUNG: die Identitaets-Ebene traegt den Rebound-Ausweis.");
    static_assert(comp::IsReboundSearchAlgoLeaf<Rebound>, "01c: der Rebound-Leaf traegt seinen Ausweis nicht.");
    static_assert(anat::HasOrganLocation<S>,
                  "01c: die Fassade verlor ihr COMDARE_DEFINE_ORGAN_LOCATION -- der F30-Guard des Generators "
                  "haengt daran.");
    static_assert(!anat::HasOrganLocation<Rebound>,
                  "01c: der Rebound-Leaf traegt eine Organ-Lokation -- die Substanz-Ebene wuerde reflektierbar.");

    // ---- (5) name()-INVARIANZ -- die T6-Wahl darf NIE in den serialize-/binary_id-Schluessel lecken. ----
    static_assert(comp::search_algo_name_is_allocator_invariant_v<S, DefaultAllokator>,
                  "01c name()-INVARIANZ (Level 0) verletzt.");
    static_assert(comp::search_algo_name_is_allocator_invariant_v<S, KompositionsAllokator>,
                  "01c name()-INVARIANZ (Level 1) verletzt: das kompositions-gebundene Organ traegt einen "
                  "anderen Namen -- eine mimalloc-Komposition serialisierte dann unter einem anderen "
                  "Schluessel als dieselbe Komposition mit exgen.");
    static_assert(S::family_id::value == Rebound::family_id::value,
                  "01c: family_id driftet zwischen den Ebenen.");
    static_assert(S::algo_version == Rebound::algo_version, "01c: algo_version driftet zwischen den Ebenen.");

    // ---- (9) DIE F30-RELATION -- exakt die, die der Generator prueft, hier am TYP statt am Artefakt. ----
    //      axis_registry_gen bildet `type = "::" + type_name<W>()` und verlangt, dass dieses `type` mit dem
    //      COMDARE_DEFINE_ORGAN_LOCATION-Literal BEGINNT (main.cpp:161-162/:255-263). Genau diese Relation
    //      bricht, sobald die Identitaets-Ebene ein Template wird -- dann traegt type_name Argumente, die im
    //      Literal nicht stehen koennen. Der Pin gilt fuer ALLE migrierten Organe, auch die Default-OFF
    //      per-K-Familie: DEREN Kante war bis zur 01c-Hebung latent offen (Owner-Punkt (ii), abend-14), weil
    //      sie Aliase auf eine Template-Id ohne jedes ORGAN_LOCATION waren. Hier wird sie zugehalten, BEVOR
    //      irgendwer ein Flag anschaltet -- statt sie erst am XML-Byte-Ereignis zu bemerken.
    static constexpr std::string_view kReflektiert = ::comdare::cache_engine::builder::codegen::type_name<S>();
    static_assert(kReflektiert.find('<') == std::string_view::npos,
                  "01c F30-KANTE OFFEN: der reflektierte Typ-Name der Identitaets-Ebene traegt "
                  "Template-Argumente. Beim Einschalten dieses Organs braechte `type=` in der Registry-XML die "
                  "Form -- und der F30-Guard koennte es nicht sehen, weil ein Makro-Literal nie Argumente "
                  "traegt. Die Fassade muss eine NICHT-Template-Klasse sein.");
    //      Und die Relation selbst: weil die Fassade nicht-Template ist, ist das Praefix sogar GLEICHHEIT --
    //      "::" + type_name<S>() == cpp_type_name. Das ist schaerfer als der Guard und faengt jede Drift
    //      zwischen Makro-Literal und realem Typ (Umbenennung, falscher Namensraum, Tippfehler).
    static_assert(std::string_view{S::cpp_type_name}.size() == kReflektiert.size() + 2u &&
                      std::string_view{S::cpp_type_name}.substr(2) == kReflektiert,
                  "01c F30-DRIFT: das ORGAN_LOCATION-Literal ist nicht '::' + der real reflektierte Typ-Name. "
                  "Der Generator schriebe dann ein `type=`, das nicht mit dem Literal beginnt -> F30-GUARD-BRUCH, "
                  "und es waere KEINE Datei geschrieben worden (axis_registry_gen Rueckgabe 5).");

    static constexpr bool ok = true;
};

template <class S>
using erfuellt_fassaden_vertrag = mp::mp_bool<FassadenVertrag<S>::ok>;

static_assert(mp::mp_all_of<MigrierteOrgane, erfuellt_fassaden_vertrag>::value,
              "01c: mindestens ein migriertes Organ verletzt den Zwei-Ebenen-Vertrag (der konkrete "
              "static_assert oben nennt den Typ).");

// =============================================================================================
// PER-K-ZUSATZ: der Rebind darf die Strategie aendern, NIE den Such-Pfad.
// Der Pin steht HIER und nicht in composable/traversal_for_search_algo.hpp, weil jener Header seine
// Algo-Typen nur vorwaerts deklariert und keine Allokator-Strategie kennt -- ein axis_06-Include waere
// dort eine neue Kante in einem sehr breit gezogenen Header (Hotfix-Lehre cda964e0). Diese TU fuehrt
// axis_06 ohnehin. Rand-Aritaeten stellvertretend: alle vier laufen durch DENSELBEN Core.
// =============================================================================================
static_assert(std::is_same_v<comp::traversal_for_search_algo_t<lk::KArySearchAlgoK2>,
                             comp::traversal_for_search_algo_t<
                                 lk::KArySearchAlgoKRebound<2u, KompositionsAllokator>>>,
              "01c per-K: die kompositions-gebundene Form K=2 traegt ein ANDERES Traversal-Organ als ihre "
              "Fassade -- dieselbe Aritaet maesse dann je nach T6-Wahl ueber zwei verschiedene Such-Pfade.");
static_assert(std::is_same_v<comp::traversal_for_search_algo_t<lk::KArySearchAlgoK16>,
                             comp::traversal_for_search_algo_t<
                                 lk::KArySearchAlgoKRebound<16u, KompositionsAllokator>>>,
              "01c per-K: Traversal-Organ-Drift zwischen Fassade und Rebound-Leaf bei K=16.");
// Und die Gegenprobe, damit der Vergleich nicht bloss "beide void" behauptet.
static_assert(!std::is_same_v<comp::traversal_for_search_algo_t<lk::KArySearchAlgoK2>, void> &&
                  !std::is_same_v<comp::traversal_for_search_algo_t<lk::KArySearchAlgoK2>,
                                  comp::traversal_for_search_algo_t<lk::KArySearchAlgoK16>>,
              "01c per-K-Gate TOT: das Traversal-Mapping liefert void oder fuer K=2 und K=16 dasselbe Organ -- "
              "dann pinnt der Gleichheits-Vergleich oben nichts.");

int fehler = 0;

void pruefe(bool ok, char const* was) {
    std::printf("  [%s] %s\n", ok ? "OK" : "FEHLER", was);
    if (!ok) ++fehler;
}

/// Genau dieselbe Op-Folge auf beiden Ebenen -- der Vergleich ist nur dann eine Aussage, wenn die
/// Last identisch ist.
template <class Organ>
void treibe(Organ& o, std::uint32_t n) {
    for (std::uint32_t i = 0; i < n; ++i) {
        o.insert(static_cast<typename Organ::key_type>(i), static_cast<typename Organ::value_type>(i) * 3u + 1u);
    }
}

constexpr std::uint32_t kN = 512;

/// (6) KONTRAST-BEWEIS + (7) VERHALTENS-PIN -- generisch je Organ der abgeleiteten Population.
template <class S>
void pruefe_organ_zur_laufzeit() {
    using Rebound = comp::search_algo_for_composition_t<S, KompositionsAllokator>;

    std::printf("  --- %s (Fassade %s | Rebound %s)\n", S::name().data(), s5::family_alloc_form<S>(),
                s5::family_alloc_form<Rebound>());

    Rebound am_kompositions_zaehler{};
    S       am_achsen_default{};

    treibe(am_kompositions_zaehler, kN);
    treibe(am_achsen_default, kN);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Drei Groessen gegeneinander: kompositions-gebunden / default-gebunden / ehrlicher Nullpunkt.
    Rebound ungetrieben{}; // NICHT angefasst

    auto const k_stat = am_kompositions_zaehler.search_allocator_statistics();
    auto const d_stat = am_achsen_default.search_allocator_statistics();
    auto const n_stat = ungetrieben.search_allocator_statistics();

    std::printf("      Allokationen: Komposition=%llu | Achsen-Default=%llu | Nullpunkt=%llu\n",
                static_cast<unsigned long long>(k_stat.allocation_count),
                static_cast<unsigned long long>(d_stat.allocation_count),
                static_cast<unsigned long long>(n_stat.allocation_count));

    pruefe(k_stat.allocation_count > 0,
           "das kompositions-gebundene Organ alloziert REAL ueber die T6-Wahl der Komposition");
    pruefe(k_stat.total_bytes_allocated > 0, "und es sind echte Bytes, nicht nur Zaehl-Ereignisse");
    // Der eigentliche Kontrast: gleiche Last, gleiche Zahlen -- auf VERSCHIEDENEN Strategie-Typen.
    // Das trennt "die Achse zaehlt" von "die Achse zaehlt das RICHTIGE".
    pruefe(k_stat.allocation_count == d_stat.allocation_count,
           "gleiche Op-Folge => zahlengleiches Wachstum auf beiden Ebenen (der Rebind aendert die Strategie, "
           "nicht den Allokations-Pfad)");
    pruefe(k_stat.total_bytes_allocated == d_stat.total_bytes_allocated,
           "auch byte-gleich -- keine stille Layout-/Wachstums-Aenderung durch den Rebind");
    // Ehrlicher Nullpunkt: ohne Last KEINE Allokation. Ohne diese Zeile koennte der Zaehler auch
    // konstruktions-bedingt stehen und der ">0"-Beweis waere geschenkt.
    pruefe(n_stat.allocation_count == 0, "ungetriebenes Organ meldet ehrliche 0 (kein Konstruktions-Rauschen)");

    // Die Snapshots sind DISJUNKT (jede Instanz haelt ihre eigene Strategie-Instanz) -- Voraussetzung
    // der Doppelzaehlungs-Regel des Mess-Schnitt-Fensters, hier belegt statt behauptet.
    Rebound zweite_instanz{};
    treibe(zweite_instanz, kN / 2u);
    pruefe(zweite_instanz.search_allocator_statistics().allocation_count <
               am_kompositions_zaehler.search_allocator_statistics().allocation_count,
           "zwei Instanzen derselben gebundenen Form zaehlen DISJUNKT (eigene Strategie-Instanz je Organ)");
#else
    std::printf("      (Statistik-Pfad aus -- Kontrast-Beweis uebersprungen, Verhaltens-Pin laeuft)\n");
#endif

    // (7) VERHALTENS-PIN -- der Rebind aendert die Strategie, NICHT den Algorithmus. Ohne diese Probe
    //     koennte die Durchbindung typ-korrekt und trotzdem semantisch falsch sein.
    bool alle_gleich = true;
    for (std::uint32_t i = 0; i < kN + 64u; ++i) {
        auto const k = static_cast<typename S::key_type>(i);
        if (am_kompositions_zaehler.lookup(k) != am_achsen_default.lookup(k)) alle_gleich = false;
    }
    pruefe(alle_gleich, "Antworten der beiden Ebenen ueber Treffer UND Fehlschlaege identisch");
    pruefe(am_kompositions_zaehler.occupied_count() == am_achsen_default.occupied_count(), "Belegung identisch");
    pruefe(am_kompositions_zaehler.erase(static_cast<typename S::key_type>(7)) ==
               am_achsen_default.erase(static_cast<typename S::key_type>(7)),
           "erase-Semantik identisch");
}

/// (8) REGISTRY-BYTE-BEWEIS am committeten Artefakt -- je ENABLED Organ der Population.
/// Der Roundtrip-Test (test_axis_registry_roundtrip) beweist "XML == Reflektion des Codes". DIESE
/// Probe beweist die andere Haelfte: dass die reflektierte Form die ARGLOSE Fassaden-Form ist. Ein
/// Template-Kopf am Wrapper wuerde hier Template-Argumente in `type=` bringen -- genau die Kante, die
/// die Fassaden-Konstruktion vermeidet.
template <class S>
void pruefe_registry_zeile(std::string const& xml) {
    std::string const fq          = std::string{S::cpp_type_name};
    auto const        kurz_pos    = fq.rfind("::");
    std::string const kurz        = (kurz_pos == std::string::npos) ? fq : fq.substr(kurz_pos + 2);
    std::string const erwartet    = std::string{"type=\""} + fq + "\"";
    std::string const erwartet_wr = std::string{"wrapper=\""} + kurz + "\"";
    std::string const schluessel  = std::string{"name=\""} + std::string{S::name()} + "\"";

    auto const pos = xml.find(schluessel);
    pruefe(pos != std::string::npos, "der Baustein steht in der committeten XML");
    if (pos == std::string::npos) return;

    auto const        zeilen_ende = xml.find('\n', pos);
    std::string const zeile       = xml.substr(pos, zeilen_ende - pos);
    std::printf("      XML-Zeile: %s\n", zeile.c_str());
    pruefe(zeile.find(erwartet) != std::string::npos,
           "type= traegt exakt das ORGAN_LOCATION-Literal der Fassade (F30-Guard-Beziehung)");
    pruefe(zeile.find('<') == std::string::npos,
           "type= traegt KEINE Template-Argumente -- die Fassade ist nicht-Template geblieben");
    pruefe(zeile.find(erwartet_wr) != std::string::npos, "wrapper= unveraendert (short_name der Fassade)");
}

/// (8b) DIE ANDERE HAELFTE DES XML-BEWEISES -- die ABWESENHEIT der Default-OFF-Organe.
/// Die per-K-Leaf-Hebung (Owner-Punkt (ii)) hat vier Typen von Template-Id-Aliassen zu echten
/// Registry-Organ-Klassen gemacht. Die Behauptung "das bewegt die committete XML um NULL Byte" haengt
/// an genau EINER Bedingung: sie sind Default-OFF, und der Generator reflektiert nur Enabled*. Statt
/// diese Bedingung zu behaupten, wird sie hier AM ARTEFAKT geprueft -- ihr name() darf in der
/// committeten XML nicht vorkommen. Faellt die Probe, ist ein per-K-Flag angeschaltet worden und die
/// XML muesste regeneriert werden (Owner-Sache, nacht-2: XML = Rueckfrage-Gate).
template <class S>
void pruefe_registry_abwesenheit(std::string const& xml) {
    std::string const schluessel = std::string{"name=\""} + std::string{S::name()} + "\"";
    pruefe(xml.find(schluessel) == std::string::npos,
           "Default-OFF-Organ steht NICHT in der committeten XML (Hebung byte-neutral)");
    pruefe(xml.find(std::string{S::cpp_type_name}) == std::string::npos,
           "und auch sein Typ-Name taucht dort nirgends auf");
}

} // namespace

int main() {
    std::printf("A8-S5 01c -- FASSADEN-KONFORMITAET (Population ABGELEITET aus der Achsen-Registry)\n");
    std::printf("  AllStrategies=%zu | migriert=%zu | davon enabled=%zu | EnabledStrategies=%zu\n",
                static_cast<std::size_t>(mp::mp_size<lk::AllStrategies>::value),
                static_cast<std::size_t>(mp::mp_size<MigrierteOrgane>::value),
                static_cast<std::size_t>(mp::mp_size<MigriertUndEnabled>::value),
                static_cast<std::size_t>(mp::mp_size<lk::EnabledStrategies>::value));

    std::printf("\n(6)+(7) KONTRAST- UND VERHALTENS-BEWEIS je migriertem Organ:\n");
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, MigrierteOrgane>>([](auto id) {
        using S = typename decltype(id)::type;
        pruefe_organ_zur_laufzeit<S>();
    });

    std::printf("\n(8) REGISTRY-BYTE-BEWEIS je ENABLED Organ (nur die stehen in der XML):\n");
    {
        std::ifstream f{COMDARE_CE_AXIS_REGISTRY_XML};
        pruefe(f.good(), "committete Registry-XML lesbar");
        std::stringstream puffer;
        puffer << f.rdbuf();
        std::string const xml = puffer.str();

        mp::mp_for_each<mp::mp_transform<mp::mp_identity, MigriertUndEnabled>>([&xml](auto id) {
            using S = typename decltype(id)::type;
            std::printf("  --- %s\n", S::name().data());
            pruefe_registry_zeile<S>(xml);
        });

        std::printf("\n(8b) XML-ABWESENHEITS-BEWEIS je migriertem Default-OFF-Organ (%zu Stueck --\n"
                    "     das ist die Byte-Neutralitaet der per-K-Leaf-Hebung, am Artefakt statt behauptet):\n",
                    static_cast<std::size_t>(mp::mp_size<MigriertUndDisabled>::value));
        mp::mp_for_each<mp::mp_transform<mp::mp_identity, MigriertUndDisabled>>([&xml](auto id) {
            using S = typename decltype(id)::type;
            std::printf("  --- %s (%s)\n", S::name().data(), S::cpp_type_name.data());
            pruefe_registry_abwesenheit<S>(xml);
        });
    }

    std::printf(fehler == 0 ? "\n01c-FASSADEN-GATE: BESTANDEN\n" : "\n01c-FASSADEN-GATE: FEHLGESCHLAGEN (%d)\n",
                fehler);
    return fehler == 0 ? 0 : 1;
}
