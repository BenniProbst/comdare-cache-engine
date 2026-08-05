// A8-S5 Familie 01_read_path / Sub-Scheibe 01c (die REGISTRY-ORGANE der Achse 03a) -- FAMILIEN-WACHE
// der FASSADEN-KONSTRUKTION. Diese TU ist der PILOT-Stand: sie pinnt den Zwei-Ebenen-Schnitt am
// ersten migrierten Organ (linear_scan) und ist so geschnitten, dass die Scheiben 2-4 nur ihre
// Typ-Zeilen anhaengen.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 6.
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt"); Design-Entscheid abend-14 (D1).
//
// WAS DIESE SCHEIBE ANDERS MACHT ALS 01a/01b/01d -- und warum das eine eigene Wache braucht:
// Jene Scheiben banden ihre Organe an einen BENANNTEN ACHSEN-DEFAULT (ExgenAllocator). Das erfuellt
// die Schnitt-Regel ("Speicher NUR ueber das Allokator-Achsen-Interface"), aber NICHT die
// Owner-Vorgabe "Option B strikt": die Komposition waehlt eine EIGENE Strategie (die ausgelieferten
// Referenz-Kompositionen fuehren durchgaengig MimallocAllocator), und solange das Organ die Wahl
// nicht uebernimmt, laeuft im selben Tier eine stille ZWEITE Strategie. 01c dreht genau das.
//
// DIE ZWEI EBENEN, DIE HIER GEPINNT WERDEN (und warum die Trennung keine Geschmacksfrage ist):
//   IDENTITAETS-EBENE  = die namens-stabile Fassade (LinearScanSearchAlgo). Sie ist das Registry-Organ:
//     ihr type_name wird von tools/axis_registry_gen in die committete
//     algorithm_profiles/cache_engine_axis_registry.xml reflektiert, der F30-Guard verlangt, dass
//     `type=` mit dem ORGAN_LOCATION-Literal beginnt, und 34 Test-TUs nennen ihren Namen.
//   SUBSTANZ-EBENE     = der Core (parametriert) und sein Rebound-Leaf. Sie tragen den Algorithmus und
//     die Allokator-Bindung, aber KEINE Identitaet -- kein ORGAN_LOCATION, kein Registry-Eintrag.
// Die Wache prueft deshalb nicht nur "ist gebunden", sondern auch "ist NICHT vermischt": ein
// Rebound-Typ, der in die Identitaets-Ebene rutscht, drehte Emitter-Typnamen und Registry-Bytes.
//
// GEPRUEFTE EBENEN:
//   (1) FORM-AUSWEIS      -- Fassade UND Rebound-Leaf erfuellen die Familien-Konformitaet (Form B)
//   (2) CONCEPT-BEWEIS    -- der Rebound-Leaf ist ein VOLLWERTIGES Organ, nicht nur eine Typ-Huelle
//                            (die CRTP-Guard-Kette laeuft auf BEIDEN Leaves)
//   (3) IDENTITAETS-PIN   -- Level 0: die Naht liefert am Achsen-Default die Fassade SELBST (is_same)
//   (4) DURCHBINDUNG      -- Level 1: die Naht liefert bei fremder Strategie den Rebound-Leaf
//   (5) name()-INVARIANZ  -- die T6-Wahl leckt NICHT in den serialize-/binary_id-Schluessel
//   (6) KONTRAST/LAUFZEIT -- der Speicher des Organs kommt REAL aus der T6-Wahl der KOMPOSITION
//   (7) VERHALTENS-PIN    -- der Rebind aendert die Strategie, NICHT den Algorithmus
//   (8) REGISTRY-BYTE     -- die committete XML traegt den arglosen Typ-Namen unveraendert
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
// ANTI-ZEMENTIERUNGS-BISS (gefahren, nicht behauptet -- "gruene Tests zementieren alte Ordnung"): diese
// TU wurde gegen den ALT-Stand des Organs (14a8cfbc) uebersetzt und schlug COMPILE-HART fehl, 21 Fehler,
// darunter "'LinearScanSearchAlgoRebound' does not name a template type" und die static_assert-Texte
// "die Fassade traegt keine Achsen-Bindung (Form B)" / "der Rebound-Leaf ist nicht familien-konform".
// Die Wache kann also beissen; ihr gruener Lauf ist eine Aussage, keine Erfolgsmeldung.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/lookup/axis_03a_search_algo_linear_scan.hpp>
#include <axes/lookup/axis_03a_search_algo_registry.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <compositions/art_paper_binding_reference.hpp>

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

using Fassade    = lk::LinearScanSearchAlgo;
using ReboundK   = comp::search_algo_for_composition_t<Fassade, KompositionsAllokator>;
using ReboundExg = lk::LinearScanSearchAlgoRebound<AchsenDefaultAllokator>;

// =============================================================================================
// (1) FORM-AUSWEIS -- beide Ebenen erfuellen die Familien-Konformitaet, und zwar ueber Form B.
// =============================================================================================
static_assert(s5::FamilyAllocConform<Fassade>, "01c: die Fassade ist nicht familien-konform.");
static_assert(s5::FamilyAllocConform<ReboundK>, "01c: der Rebound-Leaf ist nicht familien-konform.");
static_assert(s5::AxisAllocatorBoundOrgan<Fassade>, "01c: die Fassade traegt keine Achsen-Bindung (Form B).");
static_assert(s5::AxisAllocatorBoundOrgan<ReboundK>, "01c: der Rebound-Leaf traegt keine Achsen-Bindung (Form B).");
// Die Bindung ist die RICHTIGE -- nicht irgendeine: der Leaf traegt die Wahl der Komposition.
static_assert(std::is_same_v<typename Fassade::allocator_type, AchsenDefaultAllokator>,
              "01c: die Fassade haengt nicht mehr am benannten Achsen-Default.");
static_assert(std::is_same_v<typename ReboundK::allocator_type, KompositionsAllokator>,
              "01c DURCHBINDUNG VERFEHLT: der Rebound-Leaf traegt nicht die T6-Wahl der Komposition.");

// =============================================================================================
// (2) CONCEPT-BEWEIS -- der Rebound-Leaf ist ein vollwertiges Organ (CRTP-Guard-Kette auf beiden
//     Leaves; R2 des Design-Risikoblatts: ein falscher Self-Parameter braeche die Kette LEISE).
// =============================================================================================
static_assert(lk::concepts::SearchAlgoVariant<ReboundK>);
static_assert(lk::concepts::CacheEngineSearchAlgoPermutationStrategy<ReboundK>);
static_assert(lk::concepts::DensityClassifiedStrategy<ReboundK>);
static_assert(comp::AllocatorRebindableSearchAlgo<Fassade, KompositionsAllokator>,
              "01c: die Fassade traegt keinen Migrations-Ausweis mehr -- die Naht faele still auf Identitaet "
              "zurueck und die Durchbindung waere eine Behauptung.");

// =============================================================================================
// (3) IDENTITAETS-PIN (Level 0) -- DER golden-Beweis dieser Scheibe.
//     Was typ-identisch ist, kann sich in binary_id, serialize-Pfad und Registry-XML nicht bewegen.
// =============================================================================================
static_assert(std::is_same_v<comp::search_algo_for_composition_t<Fassade, AchsenDefaultAllokator>, Fassade>,
              "01c LEVEL-0 VERLETZT: die Kompositions-Naht liefert am Achsen-Default NICHT die Fassade selbst. "
              "Damit laege ein anderer Typ auf dem golden-Pfad.");
// Gegenprobe zur Level-0-Aussage: sie ist nicht deshalb wahr, weil die Naht IMMER die Identitaet
// liefert -- bei fremder Strategie liefert sie eben NICHT die Fassade.
static_assert(!std::is_same_v<ReboundK, Fassade>,
              "01c-Gate TOT: die Naht liefert auch bei FREMDER Strategie die Fassade -- dann pinnt der "
              "Level-0-Beweis nichts, weil er gar nicht unterscheiden kann.");

// =============================================================================================
// (4) DURCHBINDUNG (Level 1) + EBENEN-TRENNUNG.
// =============================================================================================
static_assert(std::is_same_v<ReboundK, lk::LinearScanSearchAlgoRebound<KompositionsAllokator>>,
              "01c: die Naht liefert bei fremder Strategie nicht den Rebound-Leaf des Organs.");
static_assert(!comp::IsReboundSearchAlgoLeaf<Fassade>,
              "01c EBENEN-VERMISCHUNG: die Identitaets-Ebene traegt den Rebound-Ausweis.");
static_assert(comp::IsReboundSearchAlgoLeaf<ReboundK>, "01c: der Rebound-Leaf traegt seinen Ausweis nicht.");
// Der Rebound-Leaf ist KEIN Registry-Organ und darf deshalb keine Organ-Lokation tragen -- sonst
// koennte er in die Registry-Reflektion geraten.
static_assert(::comdare::cache_engine::anatomy::HasOrganLocation<Fassade>,
              "01c: die Fassade verlor ihr COMDARE_DEFINE_ORGAN_LOCATION -- der F30-Guard des Generators haengt "
              "daran.");
static_assert(!::comdare::cache_engine::anatomy::HasOrganLocation<ReboundK>,
              "01c: der Rebound-Leaf traegt eine Organ-Lokation -- die Substanz-Ebene wuerde reflektierbar.");

// =============================================================================================
// (5) name()-INVARIANZ -- die T6-Wahl darf NIE in den serialize-/binary_id-Schluessel lecken.
// =============================================================================================
static_assert(comp::search_algo_name_is_allocator_invariant_v<Fassade, AchsenDefaultAllokator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(comp::search_algo_name_is_allocator_invariant_v<Fassade, KompositionsAllokator>,
              "01c name()-INVARIANZ (Level 1) verletzt: das kompositions-gebundene Organ traegt einen anderen "
              "Namen -- eine mimalloc-Komposition serialisierte dann unter einem anderen Schluessel als "
              "dieselbe Komposition mit exgen.");
static_assert(Fassade::family_id::value == ReboundK::family_id::value, "01c: family_id driftet zwischen den Ebenen.");
static_assert(Fassade::algo_version == ReboundK::algo_version, "01c: algo_version driftet zwischen den Ebenen.");

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

} // namespace

int main() {
    std::printf("A8-S5 01c -- FASSADEN-KONFORMITAET (Pilot linear_scan)\n");
    std::printf("  Identitaets-Ebene : %s (name=\"%s\")\n", Fassade::cpp_type_name.data(), Fassade::name().data());
    std::printf("  Form Fassade      : %s\n", s5::family_alloc_form<Fassade>());
    std::printf("  Form Rebound-Leaf : %s\n", s5::family_alloc_form<ReboundK>());

    constexpr std::uint32_t kN = 512;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // =========================================================================================
    // (6) KONTRAST-BEWEIS -- der Speicher kommt REAL aus der T6-Wahl der Komposition.
    //     Drei Groessen gegeneinander: kompositions-gebunden / default-gebunden / Nullpunkt.
    // =========================================================================================
    ReboundK am_kompositions_zaehler{};
    Fassade  am_achsen_default{};
    ReboundK ungetrieben{}; // ehrlicher Nullpunkt -- NICHT angefasst

    treibe(am_kompositions_zaehler, kN);
    treibe(am_achsen_default, kN);

    auto const k_stat = am_kompositions_zaehler.search_allocator_statistics();
    auto const d_stat = am_achsen_default.search_allocator_statistics();
    auto const n_stat = ungetrieben.search_allocator_statistics();

    std::printf("  Allokationen: Kompositions-Strategie=%llu | Achsen-Default=%llu | Nullpunkt=%llu\n",
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

    // Die Snapshots sind DISJUNKT (jede Instanz haelt ihre eigene Strategie-Instanz) -- das ist die
    // Voraussetzung der Doppelzaehlungs-Regel des Mess-Schnitt-Fensters, hier belegt statt behauptet.
    ReboundK zweite_instanz{};
    treibe(zweite_instanz, kN / 2u);
    pruefe(zweite_instanz.search_allocator_statistics().allocation_count <
               am_kompositions_zaehler.search_allocator_statistics().allocation_count,
           "zwei Instanzen derselben gebundenen Form zaehlen DISJUNKT (eigene Strategie-Instanz je Organ)");
#else
    ReboundK am_kompositions_zaehler{};
    Fassade  am_achsen_default{};
    treibe(am_kompositions_zaehler, kN);
    treibe(am_achsen_default, kN);
    std::printf("  (Statistik-Pfad aus -- Kontrast-Beweis uebersprungen, Verhaltens-Pin laeuft)\n");
#endif

    // =========================================================================================
    // (7) VERHALTENS-PIN -- der Rebind aendert die Strategie, NICHT den Algorithmus.
    //     Ohne diese Probe koennte die Durchbindung typ-korrekt und trotzdem semantisch falsch sein.
    // =========================================================================================
    {
        bool alle_gleich = true;
        for (std::uint32_t i = 0; i < kN + 64u; ++i) {
            auto const k = static_cast<Fassade::key_type>(i);
            auto const a = am_kompositions_zaehler.lookup(k);
            auto const b = am_achsen_default.lookup(k);
            if (a != b) alle_gleich = false;
        }
        pruefe(alle_gleich, "Antworten der beiden Ebenen ueber Treffer UND Fehlschlaege identisch");
        pruefe(am_kompositions_zaehler.occupied_count() == am_achsen_default.occupied_count(), "Belegung identisch");
        pruefe(am_kompositions_zaehler.erase(static_cast<Fassade::key_type>(7)) ==
                   am_achsen_default.erase(static_cast<Fassade::key_type>(7)),
               "erase-Semantik identisch");
    }

    // =========================================================================================
    // (8) REGISTRY-BYTE-BEWEIS am committeten Artefakt.
    //     Der Roundtrip-Test (test_axis_registry_roundtrip) beweist "XML == Reflektion des Codes".
    //     DIESE Probe beweist die andere Haelfte: dass die reflektierte Form die ARGLOSE Fassaden-
    //     Form ist. Ein Template-Kopf am Wrapper wuerde hier Template-Argumente in `type=` bringen --
    //     genau die Kante, die die Fassaden-Konstruktion vermeidet.
    // =========================================================================================
    {
        std::ifstream f{COMDARE_CE_AXIS_REGISTRY_XML};
        pruefe(f.good(), "committete Registry-XML lesbar");
        std::stringstream puffer;
        puffer << f.rdbuf();
        std::string const xml = puffer.str();

        std::string const erwartet_typ = std::string{"type=\""} + std::string{Fassade::cpp_type_name} + "\"";
        auto const        pos          = xml.find("name=\"linear_scan\"");
        pruefe(pos != std::string::npos, "der linear_scan-Baustein steht in der committeten XML");
        if (pos != std::string::npos) {
            auto const        zeilen_ende = xml.find('\n', pos);
            std::string const zeile       = xml.substr(pos, zeilen_ende - pos);
            std::printf("  XML-Zeile: %s\n", zeile.c_str());
            pruefe(zeile.find(erwartet_typ) != std::string::npos,
                   "type= traegt exakt das ORGAN_LOCATION-Literal der Fassade (F30-Guard-Beziehung)");
            pruefe(zeile.find('<') == std::string::npos,
                   "type= traegt KEINE Template-Argumente -- die Fassade ist nicht-Template geblieben");
            pruefe(zeile.find("wrapper=\"LinearScanSearchAlgo\"") != std::string::npos,
                   "wrapper= unveraendert (short_name der Fassade)");
        }
    }

    std::printf(fehler == 0 ? "01c-FASSADEN-GATE: BESTANDEN\n" : "01c-FASSADEN-GATE: FEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
