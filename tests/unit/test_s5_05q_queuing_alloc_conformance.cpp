// A8-S5 Familie 05_write_path_io -- FAMILIEN-KONFORMITAETS-WACHE, UND ZUGLEICH DIE HEILUNG DES
// NACHWEIS-UNIVERSUMS (Truth-Check-Auflage 3, wf_94ca9f27 R1).
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5.
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp.
//
// ===================================================================================================
// WARUM DIESE TU ANDERS GEBAUT IST ALS IHRE FUENF GESCHWISTER
// ===================================================================================================
// Der S5-Abschnitts-Truth-Check hat GENAU HIER die Luecke gefunden, und sie war keine
// Nachlaessigkeit, sondern ein STRUKTURFEHLER des Nachweises: der bisherige 05-Nachweis
// (test_s5_04_execution_alloc_conformance.cpp, Kommentar-Block) greppte
//
//     axes/io_dispatch/  axes/persistence_target/
//
// und meldete "LEER -> Familie 05 traegt keinen Default-Allokator-Container". Beide Aussagen stimmen.
// Die SCHLUSSFOLGERUNG stimmte trotzdem nicht, weil das UNIVERSUM falsch war: die Familie 05 hat
// laut ihrer drift-bewachten EINZIGQUELLE VIER Achsen
//
//     builder/bestandslog/lager_baum_writer.hpp -> kOrganGruppe05
//     = {queuing_q1, queuing_q2, io_dispatch, persistence_target}
//
// und die beiden ersten haben ueberhaupt keinen axes/-Ordner: sie leben ausschliesslich in der
// topics/-Doppelwurzel. Ein grep ueber "axes/<achse>/" konnte sie also gar nicht sehen -- er lief
// gegen zwei leere Pfade und war deshalb "gruen". Das ist die Lehre "gruene Tests zementieren alte
// Ordnung" in Reinform: nicht der Test war falsch, sondern die stille Annahme darueber, wo eine
// Achse wohnt.
//
// DIESE TU MACHT DIE ANNAHME UNMOEGLICH -- in drei Stufen, die alle compile-hart sind:
//
//   (S1) DAS UNIVERSUM KOMMT AUS DER EINZIGQUELLE. Die Achsen-Namen werden NICHT hier
//        aufgeschrieben, sondern aus kOrganGruppe05 gelesen. Wer die Gruppe umbenennt, erweitert
//        oder kuerzt, bricht diese TU.
//   (S2) JEDE Achse der Gruppe MUSS eine Deckungs-Eintragung haben, und jede Deckungs-Eintragung
//        MUSS auf eine Achse der Gruppe zeigen. Beide Richtungen sind static_assert -- eine
//        einseitige Pruefung waere genau der Fehler von vorhin (Deckung ohne Vollstaendigkeit).
//   (S3) DIE WURZEL WIRD NICHT ANGENOMMEN, SONDERN VOM ORGAN ERFRAGT. Jede Deckungs-Eintragung
//        traegt die erwartete Quell-Wurzel, und die Pruefung vergleicht sie gegen das, was die
//        ORGANE SELBST ueber ihren Header sagen (COMDARE_DEFINE_ORGAN_LOCATION -> header_include).
//        Wandert eine Achse zwischen axes/ und topics/, faellt die Wache -- statt still an der
//        neuen Wurzel vorbeizulaufen.
//
// ===================================================================================================
// WAS SONST NOCH GEPINNT WIRD (das gewohnte Gate-Muster)
// ===================================================================================================
//   * FORM-AUSWEIS je Organ (A heap-frei / B ueber die Allokator-Achse) -- literal ausgegeben.
//   * ANTI-LEERLAUF: jede Typ-Liste > 0 (eine leere Liste macht jede Alles-Aussage wahr).
//   * VERDRAHTUNGS-BELEG (die von s5_family_alloc_conformance.hpp:31 geforderte Zusatz-Aussage fuer
//     Form B): Form B prueft nur, dass ein allocator_type DEKLARIERT ist. Diese TU belegt zur
//     LAUFZEIT, dass die Allokation auch WIRKLICH dort laeuft -- der Achsen-Zaehler jedes einzelnen
//     Q1-Organs muss nach echten put()-Operationen > 0 sein. An dieser Aussage waere der Alt-Stand
//     GEFALLEN (dort war der Zaehler strukturell 0, weil std::allocator daran vorbeilief).
//   * COW-/REBIND-BELEG am kopierten Organ: die Kopie alloziert ueber ihre EIGENE Strategie, und der
//     Memento-restore verhindert, dass die Kopier-Allokation als Messwert der Quelle erscheint.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-Wachen.

#include "s5_family_alloc_conformance.hpp"

// (S1) DIE EINZIGQUELLE des Familien-Universums -- nur GELESEN.
#include <builder/bestandslog/lager_baum_writer.hpp>

// Die vier Achsen-Registries der Gruppe. DASS es genau diese vier sein muessen, prueft (S2).
#include <axes/io_dispatch/axis_io_registry.hpp>
#include <axes/persistence_target/axis_persistence_target_registry.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>

#include <anatomy/organ_location.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace mp   = boost::mp11;
namespace s5   = ::comdare::cache_engine::tests::s5;
namespace lb   = ::comdare::cache_engine::builder::bestandslog;
namespace q1   = ::comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2   = ::comdare::cache_engine::queuing::axis_q2_queuing;
namespace iod  = ::comdare::cache_engine::io_dispatch;
namespace pers = ::comdare::cache_engine::persistence_target;
namespace an   = ::comdare::cache_engine::anatomy;

namespace {

// ===================================================================================================
// (S1) DAS UNIVERSUM -- aus der drift-bewachten Einzigquelle, nicht aus dieser Datei
// ===================================================================================================
constexpr auto kFamilie05 = lb::kOrganGruppe05;

static_assert(kFamilie05.size() == 4,
              "S5-05q: die Familie 05_write_path_io hat nicht mehr 4 Achsen. Das ist KEIN Testfehler -- "
              "es heisst, dass das Nachweis-Universum dieser TU nicht mehr vollstaendig ist. Deckung "
              "unten nachziehen, NICHT diese Zahl anpassen.");
static_assert(lb::organ_gruppen_achsen(4).size() == kFamilie05.size(),
              "S5-05q: die Gruppen-Zugriffsfunktion und das Gruppen-Array sind auseinandergelaufen.");

// ===================================================================================================
// (S2)+(S3) DIE DECKUNG -- je Achse: Name, erwartete Quell-Wurzel, und die Typ-Liste, die sie deckt
// ===================================================================================================
// Der Name ist der SCHLUESSEL in die Einzigquelle; die Wurzel ist die Aussage, die frueher STILL war.
struct AchsenDeckung {
    std::string_view achse;  ///< MUSS in kOrganGruppe05 stehen (sonst bricht (S2))
    std::string_view wurzel; ///< erwartetes Praefix von header_include der Organe dieser Achse
};

constexpr AchsenDeckung kDeckungQ1{"queuing_q1", "topics/queuing/"};
constexpr AchsenDeckung kDeckungQ2{"queuing_q2", "topics/queuing/"};
constexpr AchsenDeckung kDeckungIo{"io_dispatch", "axes/io_dispatch/"};
constexpr AchsenDeckung kDeckungPt{"persistence_target", "axes/persistence_target/"};

constexpr std::array<AchsenDeckung, 4> kDeckungen{{kDeckungQ1, kDeckungQ2, kDeckungIo, kDeckungPt}};

[[nodiscard]] constexpr bool achse_in_familie(std::string_view a) noexcept {
    for (auto const& n : kFamilie05)
        if (n == a) return true;
    return false;
}
[[nodiscard]] constexpr bool achse_gedeckt(std::string_view a) noexcept {
    for (auto const& d : kDeckungen)
        if (d.achse == a) return true;
    return false;
}
[[nodiscard]] constexpr bool jede_deckung_zeigt_in_die_familie() noexcept {
    for (auto const& d : kDeckungen)
        if (!achse_in_familie(d.achse)) return false;
    return true;
}
[[nodiscard]] constexpr bool jede_familien_achse_ist_gedeckt() noexcept {
    for (auto const& n : kFamilie05)
        if (!achse_gedeckt(n)) return false;
    return true;
}

// BEIDE Richtungen. Die erste allein war der Fehler des Alt-Nachweises: er deckte zwei Achsen
// korrekt ab und schwieg ueber die anderen zwei.
static_assert(jede_deckung_zeigt_in_die_familie(),
              "S5-05q: eine Deckungs-Eintragung nennt eine Achse, die NICHT in kOrganGruppe05 steht -- "
              "die Wache prueft dann etwas, das gar nicht zur Familie gehoert.");
static_assert(jede_familien_achse_ist_gedeckt(),
              "S5-05q: eine Achse der Familie 05_write_path_io hat KEINE Deckungs-Eintragung. GENAU DAS "
              "war die vom Abschnitts-Truth-Check gefundene Luecke (queuing_q1/q2 ohne axes/-Ordner): "
              "der Nachweis lief gegen ein zu kleines Universum und war deshalb gruen.");
static_assert(kDeckungen.size() == kFamilie05.size(),
              "S5-05q: Deckungs- und Familien-Groesse stimmen nicht ueberein (doppelte Deckung?).");

// ===================================================================================================
// DIE TYP-LISTEN DER VIER ACHSEN -- aus den Registries, NICHT handgepflegt
// ===================================================================================================
using Q1Strategien = q1::AllStrategies;
using Q2Policies   = q2::AllPolicies;
using IoStrategien = iod::AllIos;
using PtTargets    = pers::AllTargets;

static_assert(mp::mp_size<Q1Strategien>::value > 0, "S5-05q: Q1-Registry-Liste LEER (Anti-Leerlauf).");
static_assert(mp::mp_size<Q2Policies>::value > 0, "S5-05q: Q2-Registry-Liste LEER (Anti-Leerlauf).");
static_assert(mp::mp_size<IoStrategien>::value > 0, "S5-05q: io_dispatch-Registry-Liste LEER (Anti-Leerlauf).");
static_assert(mp::mp_size<PtTargets>::value > 0, "S5-05q: persistence_target-Registry-Liste LEER (Anti-Leerlauf).");

// DIE eigentliche Familien-Aussage, auf ALLEN VIER Achsen (frueher: auf zweien).
static_assert(s5::family_alloc_conform_v<Q1Strategien>,
              "S5-05q: ein Organ der Achse queuing_q1 fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Q2Policies>,
              "S5-05q: ein Organ der Achse queuing_q2 fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<IoStrategien>,
              "S5-05q: ein Organ der Achse io_dispatch fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<PtTargets>,
              "S5-05q: ein Organ der Achse persistence_target fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");

// ---------------------------------------------------------------------------------------------------
// Q2 + io_dispatch + persistence_target tragen die MINIMAL-FORM (A): sie halten ueberhaupt keinen
// dynamischen Zustand. Das ist hier eine AUSSAGE, kein Zufall -- deshalb wird sie EINZELN gepinnt und
// nicht nur ueber das ODER-Praedikat mitgenommen. Faellt eines dieser Organe kuenftig auf einen
// Container zurueck, bricht DIESE Zeile (und nicht erst irgendein Messwert).
// ---------------------------------------------------------------------------------------------------
template <class T>
using is_heap_free = mp::mp_bool<s5::HeapFreeOrgan<T>>;

static_assert(mp::mp_all_of<Q2Policies, is_heap_free>::value,
              "S5-05q: eine Flush-Policy der Achse queuing_q2 ist nicht mehr heap-frei. Q2 traegt die "
              "MINIMAL-FORM (nur Skalare + Zeitpunkte); ein Container dort waere ein neues Organ und "
              "braeuchte den Form-B-Schnitt samt Achsen-Bindung.");
static_assert(mp::mp_all_of<IoStrategien, is_heap_free>::value,
              "S5-05q: eine io_dispatch-Strategie ist nicht mehr heap-frei (MINIMAL-FORM verlassen).");
static_assert(mp::mp_all_of<PtTargets, is_heap_free>::value,
              "S5-05q: ein persistence_target ist nicht mehr heap-frei (MINIMAL-FORM verlassen).");

// ---------------------------------------------------------------------------------------------------
// KONTRAST-PIN: die Q1-Achse ist NICHT heap-frei -- und das MUSS so sein. Waere sie es, dann liefe die
// Familien-Aussage oben fuer Q1 ueber den Form-A-Zweig, und der Form-B-Schnitt dieser Welle waere
// unbewiesen (die Wache saehe gruen, ohne je die Achsen-Bindung angefasst zu haben).
// ---------------------------------------------------------------------------------------------------
static_assert(!mp::mp_all_of<Q1Strategien, is_heap_free>::value,
              "S5-05q: ALLE Q1-Organe sind heap-frei -- dann traegt die Familien-Aussage fuer Q1 nichts "
              "ueber den Form-B-Schnitt, und dieser Gate-Lauf waere eine Erfolgsmeldung ohne Aussage.");

// ---------------------------------------------------------------------------------------------------
// (S3) DIE WURZEL-PROBE auf Typ-Ebene: jedes Organ, das eine Lokation deklariert, muss sie unter der
// fuer seine Achse EINGETRAGENEN Wurzel melden. Organe ohne Lokation werden uebersprungen (die
// Deklaration ist optional, HasOrganLocation) -- gezaehlt wird, wie viele wirklich geprueft wurden,
// damit ein stiller Totalausfall der Probe nicht als Erfolg durchgeht.
// ---------------------------------------------------------------------------------------------------
[[nodiscard]] constexpr bool beginnt_mit(std::string_view s, std::string_view p) noexcept {
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

int g_fail          = 0;
int g_wurzel_proben = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

template <class List>
void pruefe_wurzel(AchsenDeckung const& d) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T = typename decltype(id)::type;
        if constexpr (an::HasOrganLocation<T>) {
            ++g_wurzel_proben;
            bool const ok = beginnt_mit(T::header_include, d.wurzel);
            if (!ok) {
                std::printf("  [FAIL] %.*s: Organ meldet Wurzel '%.*s', eingetragen ist '%.*s'\n",
                            static_cast<int>(d.achse.size()), d.achse.data(),
                            static_cast<int>(T::header_include.size()), T::header_include.data(),
                            static_cast<int>(d.wurzel.size()), d.wurzel.data());
                ++g_fail;
            }
        }
    });
}

template <class List>
void report_list(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T       = typename decltype(id)::type;
        bool const ok = s5::FamilyAllocConform<T>;
        auto const n  = T::name();
        std::printf("  [%s] %-7s %-30.*s Form: %s\n", ok ? " ok " : "FAIL", stufe, static_cast<int>(n.size()), n.data(),
                    s5::family_alloc_form<T>());
        if (!ok) ++g_fail;
    });
}

// ===================================================================================================
// VERDRAHTUNGS-BELEG (Form-B-Grenze, s5_family_alloc_conformance.hpp:31)
// ===================================================================================================
// Form B prueft die DEKLARATION eines allocator_type. Dass die Allokation auch wirklich dort laeuft,
// ist eine LAUFZEIT-Aussage -- und genau die Aussage, an der der Alt-Stand gefallen waere.
template <class Organ>
void beleg_verdrahtung(char const* label) {
    Organ organ{};
    for (std::uint64_t i = 0; i < 512; ++i) organ.put(static_cast<typename Organ::element_type>(i * 2654435761ull));

    auto const st = organ.buffer_allocator_statistics();
    std::printf("     %-30s allocation_count=%llu  bytes=%llu  size=%zu\n", label,
                static_cast<unsigned long long>(st.allocation_count),
                static_cast<unsigned long long>(st.total_bytes_allocated), organ.size());
    if (st.allocation_count == 0) {
        std::printf("  [FAIL] %s: 512 put() und NULL Allokationen auf dem Achsen-Zaehler -- der Puffer "
                    "laeuft am Allokator-Achsen-Interface vorbei (genau der Alt-Stand).\n",
                    label);
        ++g_fail;
    }
}

/// COW-/REBIND-BELEG am kopierten Organ (Memento-Muster) -- DIFFERENTIELL, und zwar mit Absicht.
///
/// Der naheliegende Test ("die Kopie hat einen Zaehlerstand > 0") BEISST NICHT: der Copy-Ctor kopiert
/// die Strategie mitsamt ihrer Statistik und stellt sie per restore_statistics wieder her -- der
/// Zaehler der Kopie ist danach IMMER > 0, ganz unabhaengig davon, ueber welchen Adapter ihr Container
/// wirklich alloziert. Ein solcher Test waere eine Erfolgsmeldung ohne Aussage.
///
/// Diese Probe misst deshalb, WOHIN DIE NAECHSTE ALLOKATION FAELLT:
///   (a) MEMENTO: die blosse Vollkopie darf den Zaehler der QUELLE nicht bewegen (die transiente
///       Kopier-Allokation ist keine Messgroesse der Quelle).
///   (b) REBIND: put() NUR AUF DER KOPIE muss den Zaehler der KOPIE erhoehen und den der QUELLE in
///       Ruhe lassen. Haette die Kopie den Adapter der Quelle geerbt (die COW-Falle), waere es genau
///       umgekehrt -- und bei zerstoerter Quelle waere es ein Zugriff auf eine tote Strategie.
template <class Organ>
void beleg_cow_rebind(char const* label) {
    Organ quelle{};
    for (std::uint64_t i = 0; i < 256; ++i) quelle.put(static_cast<typename Organ::element_type>(i));
    auto const q_vor_kopie = quelle.buffer_allocator_statistics().allocation_count;

    Organ      kopie        = quelle;
    auto const q_nach_kopie = quelle.buffer_allocator_statistics().allocation_count;
    auto const k_nach_kopie = kopie.buffer_allocator_statistics().allocation_count;

    // Last NUR auf der Kopie -- die Quelle wird ab hier nicht mehr angefasst.
    for (std::uint64_t i = 0; i < 4096; ++i) kopie.put(static_cast<typename Organ::element_type>(0xA5A5'0000ull + i));
    auto const q_nach_last = quelle.buffer_allocator_statistics().allocation_count;
    auto const k_nach_last = kopie.buffer_allocator_statistics().allocation_count;

    std::printf("     %-24s Quelle %llu -> %llu -> %llu | Kopie %llu -> %llu\n", label,
                static_cast<unsigned long long>(q_vor_kopie), static_cast<unsigned long long>(q_nach_kopie),
                static_cast<unsigned long long>(q_nach_last), static_cast<unsigned long long>(k_nach_kopie),
                static_cast<unsigned long long>(k_nach_last));

    if (q_nach_kopie != q_vor_kopie) {
        std::printf("  [FAIL] %s: (a) die Vollkopie hat den Zaehler der QUELLE bewegt -- restore_statistics "
                    "(Memento) greift nicht, die Kopier-Pollution erschiene als Messwert der Quelle.\n",
                    label);
        ++g_fail;
    }
    if (k_nach_last <= k_nach_kopie) {
        std::printf("  [FAIL] %s: (b) 4096 put() auf der KOPIE haben ihren eigenen Achsen-Zaehler nicht "
                    "bewegt -- ihr Container alloziert nicht ueber ihre eigene Strategie.\n",
                    label);
        ++g_fail;
    }
    if (q_nach_last != q_nach_kopie) {
        std::printf("  [FAIL] %s: (b) put() auf der KOPIE hat den Zaehler der QUELLE bewegt -- die Kopie "
                    "haelt noch den Adapter der Quelle (die COW-Falle; bei zerstoerter Quelle waere das "
                    "ein Zugriff auf eine tote Strategie).\n",
                    label);
        ++g_fail;
    }
    if (kopie.size() <= quelle.size()) {
        std::printf("  [FAIL] %s: die Kopie ist nach 4096 zusaetzlichen put() nicht groesser als die Quelle "
                    "-- die beiden Organe teilen sich offenbar den Zustand.\n",
                    label);
        ++g_fail;
    }
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 05_write_path_io -- Allokator-Achsen-Konformitaet (4/4 Achsen) ==\n");

    std::printf("-- (S1) Universum aus der Einzigquelle (lager_baum_writer.hpp kOrganGruppe05) --\n");
    for (auto const& n : kFamilie05) std::printf("     Achse: %.*s\n", static_cast<int>(n.size()), n.data());
    std::printf("     Gruppen-Name: %.*s\n", static_cast<int>(lb::kOrganGruppenNamen[4].size()),
                lb::kOrganGruppenNamen[4].data());

    std::printf("-- (S2) Deckung beidseitig (jede Achse gedeckt, jede Deckung in der Familie) --\n");
    tr("(S2) jede Achse der Familie hat eine Deckungs-Eintragung", jede_familien_achse_ist_gedeckt());
    tr("(S2) jede Deckungs-Eintragung zeigt auf eine Achse der Familie", jede_deckung_zeigt_in_die_familie());

    std::printf("-- (S3) Quell-Wurzel wird vom ORGAN erfragt, nicht angenommen --\n");
    pruefe_wurzel<Q1Strategien>(kDeckungQ1);
    pruefe_wurzel<Q2Policies>(kDeckungQ2);
    pruefe_wurzel<IoStrategien>(kDeckungIo);
    pruefe_wurzel<PtTargets>(kDeckungPt);
    std::printf("     geprueft: %d Organ-Lokationen\n", g_wurzel_proben);
    tr("(S3) es wurden ueberhaupt Lokationen geprueft (kein stiller Totalausfall der Probe)", g_wurzel_proben > 0);

    std::printf("-- Form-Ausweis je Organ (A heap-frei / B ueber die Allokator-Achse) --\n");
    std::printf("   queuing_q1 (%zu Organe, topics/-Doppelwurzel):\n",
                static_cast<std::size_t>(mp::mp_size<Q1Strategien>::value));
    report_list<Q1Strategien>("Q1");
    std::printf("   queuing_q2 (%zu Organe, topics/-Doppelwurzel):\n",
                static_cast<std::size_t>(mp::mp_size<Q2Policies>::value));
    report_list<Q2Policies>("Q2");
    std::printf("   io_dispatch (%zu Organe):\n", static_cast<std::size_t>(mp::mp_size<IoStrategien>::value));
    report_list<IoStrategien>("IO");
    std::printf("   persistence_target (%zu Organe):\n", static_cast<std::size_t>(mp::mp_size<PtTargets>::value));
    report_list<PtTargets>("PT");

    std::printf("-- VERDRAHTUNGS-BELEG: der Achsen-Zaehler jedes Q1-Puffers unter echter Last --\n");
    beleg_verdrahtung<q1::FIFOQueueBuffer>("FIFOQueueBuffer (deque)");
    beleg_verdrahtung<q1::LIFOStackBuffer>("LIFOStackBuffer (vector)");
    beleg_verdrahtung<q1::AppendOnlyBuffer>("AppendOnlyBuffer (vector)");
    beleg_verdrahtung<q1::DeltaChainBuffer>("DeltaChainBuffer (vector)");
    beleg_verdrahtung<q1::SkiplistBuffer>("SkiplistBuffer (set-Knoten)");
    beleg_verdrahtung<q1::PriorityHeapBuffer>("PriorityHeapBuffer (heap-Traeger)");
    beleg_verdrahtung<q1::TombstoneBuffer>("TombstoneBuffer (optional-Slots)");
    beleg_verdrahtung<q1::EpochBuffer>("EpochBuffer (2 Epochen)");
    beleg_verdrahtung<q1::BatchedInsertBuffer>("BatchedInsertBuffer (2 Ebenen)");
    beleg_verdrahtung<q1::CopyOnWriteBuffer>("CopyOnWriteBuffer (allocate_shared)");
    beleg_verdrahtung<q1::BoundedRingBuffer>("BoundedRingBuffer (Ring)");
    beleg_verdrahtung<q1::LockFreeSPSCBuffer>("LockFreeSPSCBuffer (Ring)");
    beleg_verdrahtung<q1::LockFreeMPMCBuffer>("LockFreeMPMCBuffer (Zell-Array)");
    beleg_verdrahtung<q1::OriginalLockFreeMpmcConcurrentQueue>("OriginalConcurrentQueue (Zell-Array)");

    std::printf("-- COW-/REBIND-BELEG am kopierten Organ (Memento) --\n");
    beleg_cow_rebind<q1::FIFOQueueBuffer>("FIFOQueueBuffer");
    beleg_cow_rebind<q1::SkiplistBuffer>("SkiplistBuffer");
    beleg_cow_rebind<q1::PriorityHeapBuffer>("PriorityHeapBuffer");
    beleg_cow_rebind<q1::BatchedInsertBuffer>("BatchedInsertBuffer");
    beleg_cow_rebind<q1::CopyOnWriteBuffer>("CopyOnWriteBuffer");

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Achsen der Familie: %zu   davon gedeckt: %zu   Organe gesamt: %zu\n", kFamilie05.size(),
                kDeckungen.size(),
                static_cast<std::size_t>(mp::mp_size<Q1Strategien>::value + mp::mp_size<Q2Policies>::value +
                                         mp::mp_size<IoStrategien>::value + mp::mp_size<PtTargets>::value));

    std::printf("== test_s5_05q_queuing_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
