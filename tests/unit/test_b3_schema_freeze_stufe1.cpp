// test_b3_schema_freeze_stufe1 -- B-3 + SCHEMA-FREEZE STUFE 1 (2026-08-09), Testklasse T-4.
//
// WAS HIER GEPRUEFT WIRD -- und in welcher REIHENFOLGE, denn die Reihenfolge ist der halbe Test:
//
//   Teil 1  DER KOEDER MUSS ERST BEISSEN. Bevor irgendeine gruene Aussage ueber das echte Schema
//           fallen darf, wird die Wache an DREI absichtlich verdorbenen Spaltenlisten vorgefuehrt:
//           eine Spalte zugefuegt, eine entfernt, zwei vertauscht. Faellt sie dort nicht -- und zwar
//           mit der RICHTIGEN Klasse -- ist ihr spaeteres "unveraendert" wertlos. Ein richtiges
//           Messgeraet am falschen Gegenstand faellt nie auf; nichts klappert. Also klappern wir
//           zuerst absichtlich.
//
//           Die Koeder sind FRISCH GEWUERFELT (std::random_device), nicht fest verdrahtet: eine
//           Wache, die nur an Position 0 oder nur am Namen "foo" biss, waere gegen genau eine
//           Aenderung scharf und gegen alle anderen blind. Der Wuerfel-Startwert wird woertlich
//           ausgegeben, damit ein roter Lauf von Hand nachstellbar ist.
//
//   Teil 2  DIE GEGENPROBE. Erst jetzt wird das ECHTE lazy_csv_header() gegen die eingefrorene Liste
//           gehalten -- und zwar mit beiden NENNERN in der Ausgabe, nie als blosses "ok".
//
//   Teil 3  DIE PIPELINE-SEITE (B-3). Dieselbe Wache gegen die 16- und die 25-spaltige Pipeline-CSV.
//           Zusaetzlich der RUECKNAHME-BELEG: die vier frueheren Literale sind byte-identisch durch
//           die EINE Quelle ersetzt -- der Test traegt das alte Literal selbst noch einmal und
//           vergleicht Zeichen fuer Zeichen. Ein Konsolidieren, das die Bytes veraendert haette,
//           faellt hier auf und nicht erst in der Auswertung.
//
//   Teil 4  DIE VORAUSSETZUNGEN DER WACHE SELBST. "Umgeordnet" ist nur dann eine sinnvolle Aussage,
//           wenn kein Spaltenname doppelt vorkommt (zwei gleiche Namen zu tauschen waere keine
//           Aenderung). Das wird geprueft statt angenommen.
//
// WARUM DIE EINGEFRORENE LISTE EINE EIGENE KOPIE IST: siehe Kopf von
// cache_engine/measurement/schema_freeze.hpp -- waere sie als Alias auf den Erzeuger definiert,
// praefte die Wache eine Sache gegen sich selbst und waere per Konstruktion immer gruen.
//
// Build: Standalone int main() (kein gtest), Include-Kette wie test_a8s1_t17_vollzaehligkeit.cpp.
// ASCII-only.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header (Erzeuger WIDE)
#include <builder/measurement_snapshot.hpp>                          // serialize_measurements_*_csv
#include <cache_engine/measurement/pipeline_csv_schema.hpp>          // DIE EINE Pipeline-Spaltenliste
#include <cache_engine/measurement/schema_freeze.hpp>                // die Wache + die eingefrorenen Listen

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace ms = ::comdare::cache_engine::measurement;
namespace mb = ::comdare::cache_engine::builder;

int g_fehler = 0;

void pruefe(bool bedingung, std::string const& was) {
    if (bedingung) {
        std::cout << "  OK   " << was << "\n";
    } else {
        std::cout << "  FEHL " << was << "\n";
        ++g_fehler;
    }
}

/// Ein Name, den es im echten Schema garantiert nicht gibt -- mit Wuerfel-Anteil, damit der Koeder
/// nicht ueber die Runden hinweg derselbe ist.
std::string gewuerfelter_fremdname(std::mt19937& wuerfel) {
    std::uniform_int_distribution<int> d{100000, 999999};
    return "koeder_spalte_" + std::to_string(d(wuerfel));
}

} // namespace

int main() {
    // -- Wuerfel: frisch, und der Startwert steht woertlich im Protokoll ----------------------------
    std::random_device              rd;
    std::mt19937::result_type const startwert = rd();
    std::mt19937                    wuerfel{startwert};
    std::cout << "test_b3_schema_freeze_stufe1 -- Koeder-Startwert = " << startwert
              << " (rot? mit genau diesem Wert nachstellen)\n\n";

    // -- Das IST des WIDE-Schemas, einmal erhoben --------------------------------------------------
    std::string const              wide_kopf = ex::lazy_csv_header();
    std::vector<std::string> const wide_ist  = ms::spalten_aus_kopfzeile(wide_kopf, ms::kWideCsvTrenner);
    std::vector<std::string> const wide_soll = ms::als_spaltenliste(ms::kWideSchemaFreezeStufe1);

    std::cout << "Teil 0 -- ERHEBUNG\n";
    std::cout << "  lazy_csv_header() liefert " << wide_ist.size() << " Spalten (Trenner '" << ms::kWideCsvTrenner
              << "'), eingefroren sind " << wide_soll.size() << ".\n";
    pruefe(!wide_kopf.empty() && wide_kopf.back() == '\n',
           "die WIDE-Kopfzeile endet auf '\\n' (Form gehoert zum Vertrag)");
    pruefe(wide_ist.size() >= 2, "die WIDE-Kopfzeile hat ueberhaupt Spalten (sonst waere alles Folgende leer)");

    // -- Teil 4 vorgezogen: die Voraussetzung der Umordnungs-Aussage --------------------------------
    std::cout << "\nTeil 4 (vorgezogen) -- VORAUSSETZUNG DER WACHE\n";
    pruefe(!ms::hat_doppelte_spalten(wide_soll),
           "die eingefrorene WIDE-Liste ist duplikatfrei (sonst ist 'umgeordnet' nicht eindeutig)");
    pruefe(!ms::hat_doppelte_spalten(wide_ist), "die erzeugte WIDE-Liste ist duplikatfrei");

    // =============================================================================================
    // Teil 1 -- DER KOEDER MUSS ERST BEISSEN (drei Aenderungsarten, je frisch gewuerfelt)
    // =============================================================================================
    std::cout << "\nTeil 1 -- BISSPROBEN (die Wache muss FALLEN, bevor ihr Gruen etwas wert ist)\n";

    // (A) ZUFUEGEN: ein frischer Fremdname an frisch gewuerfelter Position.
    {
        std::uniform_int_distribution<std::size_t> pos{0, wide_soll.size()};
        std::size_t const                          p      = pos(wuerfel);
        std::string const                          name   = gewuerfelter_fremdname(wuerfel);
        std::vector<std::string>                   koeder = wide_soll;
        koeder.insert(koeder.begin() + static_cast<std::ptrdiff_t>(p), name);

        auto const b = ms::pruefe_schema_freeze(koeder, wide_soll);
        std::cout << "  Koeder A: \"" << name << "\" an Position " << p << " eingefuegt\n";
        pruefe(!b.identisch, "A/zugefuegt: die Wache meldet NICHT identisch");
        pruefe(b.zugefuegt.size() == 1 && b.zugefuegt.front() == name,
               "A/zugefuegt: sie benennt genau die zugefuegte Spalte");
        pruefe(b.entfernt.empty(), "A/zugefuegt: sie behauptet nicht zusaetzlich ein Entfernen");
        pruefe(b.ist_spalten == wide_soll.size() + 1 && b.soll_spalten == wide_soll.size(),
               "A/zugefuegt: beide Nenner stehen im Befund");
        std::cout << ms::schema_freeze_bericht(b, "WIDE/Bissprobe-A");
    }

    // (B) ENTFERNEN: eine frisch gewuerfelte bestehende Spalte faellt weg.
    {
        std::uniform_int_distribution<std::size_t> pos{0, wide_soll.size() - 1};
        std::size_t const                          p      = pos(wuerfel);
        std::string const                          name   = wide_soll[p];
        std::vector<std::string>                   koeder = wide_soll;
        koeder.erase(koeder.begin() + static_cast<std::ptrdiff_t>(p));

        auto const b = ms::pruefe_schema_freeze(koeder, wide_soll);
        std::cout << "  Koeder B: \"" << name << "\" (Position " << p << ") entfernt\n";
        pruefe(!b.identisch, "B/entfernt: die Wache meldet NICHT identisch");
        pruefe(b.entfernt.size() == 1 && b.entfernt.front() == name,
               "B/entfernt: sie benennt genau die verlorene Spalte");
        pruefe(b.zugefuegt.empty(), "B/entfernt: sie behauptet nicht zusaetzlich ein Zufuegen");
        pruefe(b.ist_spalten + 1 == b.soll_spalten, "B/entfernt: der Nenner faellt um genau eins");
        std::cout << ms::schema_freeze_bericht(b, "WIDE/Bissprobe-B");
    }

    // (C) UMORDNEN: zwei frisch gewuerfelte, VERSCHIEDENE Positionen tauschen. Die Spaltenzahl bleibt
    //     gleich -- genau der Fall, den eine blosse Zaehl-Wache durchwinken wuerde.
    {
        std::uniform_int_distribution<std::size_t> pos{0, wide_soll.size() - 1};
        std::size_t                                i = pos(wuerfel);
        std::size_t                                j = pos(wuerfel);
        while (j == i) j = pos(wuerfel);
        std::vector<std::string> koeder = wide_soll;
        std::swap(koeder[i], koeder[j]);

        auto const b = ms::pruefe_schema_freeze(koeder, wide_soll);
        std::cout << "  Koeder C: Positionen " << i << " (\"" << wide_soll[i] << "\") und " << j << " (\""
                  << wide_soll[j] << "\") vertauscht\n";
        pruefe(!b.identisch, "C/umgeordnet: die Wache meldet NICHT identisch");
        pruefe(b.ist_spalten == b.soll_spalten,
               "C/umgeordnet: die Spaltenzahl ist unveraendert -- eine Zaehl-Wache saehe hier nichts");
        pruefe(b.zugefuegt.empty() && b.entfernt.empty(),
               "C/umgeordnet: weder zugefuegt noch entfernt (reine Permutation)");
        pruefe(b.umgeordnet.size() == 2, "C/umgeordnet: genau die zwei getauschten Positionen sind benannt");
        std::cout << ms::schema_freeze_bericht(b, "WIDE/Bissprobe-C");
    }

    // (D) Zusatz-Bissprobe gegen die Trenner-Blindheit: derselbe Kopf mit dem FALSCHEN Trenner
    //     zerlegt ergibt genau eine Riesen-"Spalte" -- die Wache muss auch das melden.
    {
        std::vector<std::string> const falsch = ms::spalten_aus_kopfzeile(wide_kopf, ',');
        auto const                     b      = ms::pruefe_schema_freeze(falsch, wide_soll);
        pruefe(!b.identisch, "D/Trenner: mit dem falschen Trenner zerlegt faellt die Wache ebenfalls");
        pruefe(b.ist_spalten != b.soll_spalten, "D/Trenner: der Nenner zeigt den Unterschied an");
    }

    // =============================================================================================
    // Teil 2 -- DIE GEGENPROBE am echten Schema (erst JETZT darf gruen etwas bedeuten)
    // =============================================================================================
    std::cout << "\nTeil 2 -- GEGENPROBE (echtes lazy_csv_header() gegen den Stufe-1-Freeze)\n";
    {
        auto const b = ms::pruefe_schema_freeze(wide_ist, wide_soll);
        std::cout << ms::schema_freeze_bericht(b, "WIDE/E4-Mess-CSV");
        pruefe(b.identisch, "WIDE: die erzeugte Spaltenliste ist deckungsgleich mit dem Stufe-1-Freeze");
        pruefe(b.soll_spalten == ms::kWideSchemaFreezeSpalten && b.ist_spalten == ms::kWideSchemaFreezeSpalten,
               "WIDE: beide Nenner sind " + std::to_string(ms::kWideSchemaFreezeSpalten));
    }

    // =============================================================================================
    // Teil 3 -- DIE PIPELINE-SEITE (B-3: vier Literale -> eine Quelle)
    // =============================================================================================
    std::cout << "\nTeil 3 -- PIPELINE-SCHEMA (B-3)\n";
    {
        std::vector<std::string> const ist16 =
            ms::spalten_aus_kopfzeile(ms::pipeline16_csv_header(), ms::kPipelineCsvTrenner);
        std::vector<std::string> const soll16 = ms::als_spaltenliste(ms::kPipeline16FreezeStufe1);
        auto const                     b16    = ms::pruefe_schema_freeze(ist16, soll16);
        std::cout << ms::schema_freeze_bericht(b16, "PIPELINE-16");
        pruefe(b16.identisch, "PIPELINE-16: erzeugte Liste == Stufe-1-Freeze");
        pruefe(b16.ist_spalten == 16, "PIPELINE-16: Nenner ist 16");

        std::vector<std::string> const ist25 =
            ms::spalten_aus_kopfzeile(ms::pipeline_voll_csv_header(), ms::kPipelineCsvTrenner);
        std::vector<std::string> const soll25 = ms::als_spaltenliste(ms::kPipelineVollFreezeStufe1);
        auto const                     b25    = ms::pruefe_schema_freeze(ist25, soll25);
        std::cout << ms::schema_freeze_bericht(b25, "PIPELINE-VOLL");
        pruefe(b25.identisch, "PIPELINE-VOLL: erzeugte Liste == Stufe-1-Freeze");
        pruefe(b25.ist_spalten == 25, "PIPELINE-VOLL: Nenner ist 25");

        // Praefix-Eigenschaft: die volle Sicht ist ein echtes Superset der Pipeline-Sicht. Ohne sie
        // waeren es zwei Schemata und die Stufe 04/05 laese die falschen Positionen.
        bool praefix = ist25.size() > ist16.size();
        for (std::size_t i = 0; praefix && i < ist16.size(); ++i) praefix = (ist25[i] == ist16[i]);
        pruefe(praefix, "PIPELINE: die volle Sicht traegt die 16 als echtes Praefix");
    }

    // -- RUECKNAHME-BELEG: byte-identisch zu den vier frueheren Literalen ---------------------------
    std::cout << "\nTeil 3b -- RUECKNAHME-BELEG (Konsolidierung ohne Byte-Aenderung)\n";
    {
        // Das WOERTLICHE Literal, wie es bis zum 09.08. an vier Stellen stand
        // (measurement_snapshot.hpp:223, execution_engine/src/result_aggregator.cpp:63,
        //  apps/f15_compare/main.cpp:496 -- und als Praefix in measurement_snapshot.hpp:192).
        std::string const alt16 = "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
                                  "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
                                  "coherence_invalidations,energy_micro_joules,"
                                  "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag\n";
        std::string const alt25 = "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
                                  "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
                                  "coherence_invalidations,energy_micro_joules,"
                                  "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag,"
                                  "search_insert,search_lookup,search_hit,search_miss,search_erase,"
                                  "search_peak_occupancy,pmc_available,branch_misses,throughput_ops_per_sec\n";
        pruefe(ms::pipeline16_csv_header() == alt16,
               "die EINE Quelle liefert die 16-Spalten-Kopfzeile Zeichen fuer Zeichen wie zuvor");
        pruefe(ms::pipeline_voll_csv_header() == alt25,
               "die EINE Quelle liefert die 25-Spalten-Kopfzeile Zeichen fuer Zeichen wie zuvor");

        // Und der Beleg am echten Schreiber, nicht nur an der Kopfzeilen-Funktion: die beiden
        // Serializer muessen mit genau diesen Kopfzeilen beginnen.
        std::vector<mb::ComdareMeasurementSnapshotV1> const leer{};
        std::vector<std::string> const                      keine{};
        pruefe(mb::serialize_measurements_pipeline16_csv(leer, keine, keine) == alt16,
               "serialize_measurements_pipeline16_csv beginnt unveraendert");
        pruefe(mb::serialize_measurements_csv(leer, keine, keine) == alt25,
               "serialize_measurements_csv beginnt unveraendert");
    }

    // -- Bissprobe auf der Pipeline-Seite, damit auch dort kein blindes Gruen entsteht -------------
    {
        std::vector<std::string>                   koeder = ms::als_spaltenliste(ms::kPipeline16FreezeStufe1);
        std::uniform_int_distribution<std::size_t> pos{0, koeder.size() - 1};
        std::size_t                                i = pos(wuerfel);
        std::size_t                                j = pos(wuerfel);
        while (j == i) j = pos(wuerfel);
        std::swap(koeder[i], koeder[j]);
        auto const b = ms::pruefe_schema_freeze(koeder, ms::als_spaltenliste(ms::kPipeline16FreezeStufe1));
        pruefe(!b.identisch && b.umgeordnet.size() == 2,
               "PIPELINE-16/Bissprobe: zwei vertauschte Spalten werden erkannt");
    }

    std::cout << "\n";
    if (g_fehler == 0) {
        std::cout << "ALLE OK -- Schema-Freeze Stufe 1 haelt: WIDE " << ms::kWideSchemaFreezeSpalten
                  << " Spalten, PIPELINE " << ms::kPipeline16FreezeSpalten << " / " << ms::kPipelineVollFreezeSpalten
                  << " Spalten.\n";
        return 0;
    }
    std::cout << "FEHLER: " << g_fehler << " Pruefung(en) gefallen (Koeder-Startwert " << startwert << ").\n";
    return 1;
}
