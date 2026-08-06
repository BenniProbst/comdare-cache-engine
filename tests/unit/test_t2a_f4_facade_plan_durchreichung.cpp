// test_t2a_f4_facade_plan_durchreichung.cpp -- T2-A/F4, Fable-Review-Befund MITTEL-1 (2026-08-06).
//
// DIE LUECKE, DIE HIER GESCHLOSSEN WIRD. Die F4-Mechanik (Batch-Plan als Dokument VOR dem Lauf +
// Phasen-Zaehler dagegen) war gebaut und auf Iterator-Ebene belegt (test_tp1_planer_filter_iterator,
// Fall 11) -- aber im PRODUKTIVEN Lauf unerreichbar: kein Host baut eine LazyRunConfig selbst, der
// produktive Weg laeuft ausschliesslich ueber die run_profile-Fassade, und die reichte das Feld
// `batch_plan_datei` nicht durch. Unbemerkt blieb das, weil KEIN Test die DURCHREICHUNG prueft: die
// Iterator-Tests setzen cfg.batch_plan_datei selbst und springen damit ueber genau das Glied hinweg,
// das fehlte. Diese Wache prueft deshalb nicht die Mechanik (die ist dort belegt), sondern die KETTE
// RunProfileArgs::batch_plan_datei -> make_cfg -> LazyRunConfig::batch_plan_datei -- und zwar an ihrer
// WIRKUNG, nicht an einer Zuweisung: liegt der Plan nach dem Lauf auf der Platte, ist er angekommen.
//
// WAS BEWIESEN WIRD (gegen das ECHTE run_profile, Stub-Compile, FakeTransport, kein minio):
//   (1) ARG GESETZT -> DIE KETTE REAGIERT AUF GENAU DIESEN PFAD: der Lauf nennt die uebergebene
//       Zeichenkette woertlich in seiner Ansage, samt bezifferter Begruendung. Keine andere Stelle des
//       Laufs kennt diesen String -- er kann nur ueber RunProfileArgs::batch_plan_datei -> make_cfg ->
//       LazyRunConfig::batch_plan_datei dorthin gekommen sein.
//   (2) ARG LEER -> NICHTS: derselbe Lauf ohne den Pfad legt weder Plan noch Zaehler an und sagt auch
//       nichts an -- das Opt-in-Muster (PlanPersistenz::aktiv()==false => inert) bleibt erhalten.
//   (3) DIE WIRKUNG UEBER ZWEI LAEUFE: der zweite Lauf gegen dieselbe Ablage ERBT NICHTS und baut
//       ehrlich neu (frisches Ausgabe-Verzeichnis, sichtbar gefuellt), endet aber REGULAER -- die
//       Ablage-Ebene ist Buchhaltung und bricht keinen Bau ab.
//
// T2-A/F4-NB3 AUFLAGE 2 (2026-08-06) -- WARUM (1) UND (3) HEUTE ANDERS LAUTEN ALS BIS GESTERN.
// Bis hierher pinnten sie "Plan und Zaehler liegen da" und "der Folgelauf kompiliert nichts mehr". Beide
// Zusagen hielten nur, weil dieser Lauf OHNE COMDARE_BESTANDSLOG lief: es gab gar keinen Fingerprint-
// Provider, der Plan-Stempel trug das Wort `ohne-anker`, und die Ablage entstand trotzdem. Genau diese
// Ordnung benennt Befund 1 als Defekt (ein ankerloser Stempel ist fuer JEDEN Bau-Stand derselbe, und der
// Zaehler-Leser nimmt ihn an). Mit dem gesetzten Opt-in zeigt sich die zweite Haelfte am Objekt: die EINE
// Binary dieses Profils ist NICHT MATERIALISIERBAR, ihr Fingerprint ist absichtlich der LEERE String
// (lazy_adhoc_source_gen.hpp:367), und fuer so ein Atom ist dll_is_current per Konstruktion blind. Die
// Plan-Ablage bleibt deshalb FAIL-CLOSED inert. Das ist kein Verlust der Wache, sondern ihr Ergebnis --
// und die DURCHREICHUNG, ihr eigentlicher Gegenstand, ist dabei SCHAERFER belegt als zuvor: die Ansage
// traegt den Pfad woertlich, statt dass irgendeine Datei irgendwo erscheint.
//
// WO DIE ABLAGE-MECHANIK BELEGT IST (unveraendert, nur nicht hier): Plan-Dokument, Phasen-Zaehler und
// Praefix-Resume stehen dort unter Beweis, wo die Atome eine pruefbare Identitaet haben --
// test_tp1_planer_filter_iterator, Faelle (11a)-(11c2) am echten run_planer_driven_provision und
// (11l-b)/(11l-f) am vollen run_lazy_static_then_dynamic.
//
// HISTORIE (T2-A/F4-BILANZ, 2026-08-06): Fall (3) stand einmal als STOLPERDRAHT im Baum -- ein
// VOLLSTAENDIG plan-resumierter provision-only-Lauf endete mit exit_code=1, weil die uebersprungenen
// FUEHRENDEN Faecher den Slice-Loop nie erreichten und niemand ihre Atome buchte. Das ist geheilt
// (Buchung + [BILANZ-TESTAT] mit plan_skip=) und im Iterator-Test belegt; hier ist der Fall inzwischen
// ein anderer, weil dieses Profil gar keinen Resume-Anspruch erwerben kann.
//
// PROFIL: planner_thesis_min.profile.xml -- ohne <axis_sweeps> und ohne <sota_series>, also GENAU EIN
// Selektions-Pass. Das ist Absicht: eine Ablage je Lauf traegt heute genau dort (s. die Grenze am
// RunProfileArgs-Feld); ein Mehr-Pass-Profil wuerde die Ablage zwischen den Paessen umschreiben und
// dieser Test wuerde eine Eigenschaft messen, die die Kette nicht zusagt.
//
// Build: plain main (KEIN gtest), Return 0/1 -- Muster test_tp1_planer_filter_iterator.

#include "profile_run_entry.hpp" // die geprueften Glieder: RunProfileArgs + make_cfg (run_profile)

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstddef>
#include <cstdint>
#include <cstdlib> // T2-A/F4-NB3: ::setenv -- das COMDARE_BESTANDSLOG-Opt-in dieses Laufs
#include <filesystem>
#include <sstream> // T2-A/F4-NB3: der cerr-Fang -- die Ansage der Kette ist hier der Beleg
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_F4_THESIS_MIN
#error "COMDARE_F4_THESIS_MIN must point to tests/unit/thesis_tiere/planner_thesis_min.profile.xml"
#endif

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace fs  = std::filesystem;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == static_cast<A>(want));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

// In-Memory-Transport (Muster test_tp1_planer_filter_iterator): das Bestandslog muss AKTIV sein, sonst
// bleibt planer_driven_active false und der planer-getriebene Pfad -- der einzige, der den Plan legt --
// wird gar nicht betreten.
struct FakeStore {
    std::map<std::string, std::string> objs;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            objs[k] = c;
            return true;
        };
        t.remove = [this](std::string const& k) -> bool {
            objs.erase(k);
            return true;
        };
        t.stat = [this](std::string const& k) -> std::optional<bl::ObjectStat> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return bl::ObjectStat{.size = it->second.size(), .mtime_epoch_s = 0};
        };
        return t;
    }
};

constexpr char const* kDocKey = "bestandslog/test_f4_facade.xml";

// Die Lauf-Args der Fassade. NUR `batch_plan_datei` und das Ausgabe-Verzeichnis unterscheiden die Faelle
// -- alles andere ist zwischen ihnen gleich, damit die Zurechnung eindeutig bleibt.
tlz::RunProfileArgs mach_args(FakeStore& store, fs::path const& lauf, fs::path const& plan) {
    // Das Lauf-Verzeichnis legt der HOST an, nicht run_profile (es oeffnet die CSV und bricht sonst mit
    // "CSV nicht oeffenbar" ab) -- hier steht der Test in der Rolle des Hosts.
    std::error_code ec;
    fs::create_directories(lauf, ec);
    tlz::RunProfileArgs a;
    a.profile_path = fs::path{COMDARE_F4_THESIS_MIN};
    a.out_csv      = lauf / "measurements.csv";
    a.src_dir      = lauf / "src";
    a.dll_dir      = lauf / "dll";
    // Stub-Compile: der Bau-Erfolg ist hier nicht die Frage, die Ablage ist es. 0 == Erfolg.
    a.compile           = [](ex::BuildJob const&) -> int { return 0; };
    a.provision_only    = true; // planer_driven_active = bestandslog_active UND provision_only
    a.run_sota_series   = false;
    a.max_binaries      = 1;
    a.cores_per_build   = 1;
    a.build_parallelism = 1;
    // Das Bestandslog scharf, aber ohne Lager-Inhalt: der Key-Provider antwortet nie -> jede Binary gilt
    // als fehlend -> die Selektion wird wirklich gebaut (und der Zaehler traegt sie).
    a.bestand_transport = store.transport();
    a.bestand_doc_key   = kDocKey;
    a.bestand_key_of    = [](fs::path const&) -> std::optional<std::string> { return std::nullopt; };
    a.batch_plan_datei  = plan; // DAS gepruefte Glied (leer im Negativ-Fall)
    return a;
}

// T2-A/F4-NB3: der cerr-Fang. Die Ansage der Kette IST hier der Beleg (s. Kopf), also muss der Test sie
// lesen koennen -- Muster CerrCapture aus test_tp1_planer_filter_iterator.
class CerrFang {
public:
    CerrFang() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrFang(CerrFang const&)            = delete;
    CerrFang& operator=(CerrFang const&) = delete;
    ~CerrFang() { std::cerr.rdbuf(alt_); }

    [[nodiscard]] std::string text() const { return puffer_.str(); }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

} // namespace

int main() {
    std::error_code ec;
    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_t2a_f4_facade";
    fs::remove_all(base, ec); // Reste eines Vorlaufs -- (2) prueft ABWESENHEIT und darf nichts erben
    fs::create_directories(base, ec);

    std::cout << "== T2-A/F4: die Plan-Ablage reist durch die run_profile-Fassade ==\n";

    // T2-A/F4-NB3 -- DAS OPT-IN, VON DEM DIESER TEST ABHAENGT, STEHT JETZT LITERAL DA (s. Kopf).
    // Ohne COMDARE_BESTANDSLOG=true bleibt lazy_fingerprint leer, cfg.bestand_fingerprint_fn ebenfalls,
    // und die Plan-Ablage ist seit NB3 fail-closed INERT -- es entstuende gar kein Plan. Der Test hat
    // diese Abhaengigkeit bis heute stillschweigend AUSGENUTZT (sein Vergleichs-Stempel war ankerlos).
    ::setenv("COMDARE_BESTANDSLOG", "true", 1);
    // Und die ZWEITE Haelfte derselben Vorbedingung (T2-C, profile_run_entry.hpp:438): ist die
    // Realversion des Tier-Treibers nicht sondierbar, faellt der Provider ebenfalls weg. Das ist eine
    // Eigenschaft der UMGEBUNG, keine des geprueften Glieds -- deshalb steht sie hier als eigene Zeile
    // und nicht als stille Annahme unter einem Plan-Fehlschlag.
    check_true("(0) Vorbedingung: die Tier-Realversion ist erhebbar (sonst kein Fingerprint-Anker)",
               ::comdare::cache_engine::profile_facade::tier_realversion_ist_bekannt());

    // ============================================================================================
    // T2-A/F4-NB3 AUFLAGE 2 -- WAS DIESER TEST AB HIER BELEGT, UND WARUM ER ES ANDERS BELEGT.
    //
    // AM OBJEKT GEMESSEN (2026-08-06): die EINE Binary dieses Profils ist NICHT MATERIALISIERBAR --
    // lazy_adhoc_macro_args_for findet fuer ihren binary_id nicht alle 17 Kompositions-Achsen in der
    // Enabled-Tabelle, und lazy_adhoc_fingerprint_for gibt dafuer absichtlich den LEEREN String zurueck
    // ("nicht materialisierbar -> keine DLL -> kein Fingerprint", lazy_adhoc_source_gen.hpp:367). Das ist
    // fuer dieses Profil auch richtig: der Test baut mit einem STUB-Compile, es entsteht nie eine echte
    // .so, und dll_is_current ist fuer ein Atom mit leerer Erwartung per Konstruktion BLIND.
    //
    // FOLGE: die Plan-Ablage bleibt seit der Formwache FAIL-CLOSED INERT -- ein Stempel ueber leere
    // Identitaeten deckte ein Atom, das nie geprueft werden kann, und zwei verschiedene Bau-Staende
    // waeren wieder stempel-gleich. Der Lauf legt deshalb WEDER Plan NOCH Zaehler an.
    //
    // WAS DAS FUER DIE GEPRUEFTE KETTE HEISST: die DURCHREICHUNG bleibt vollstaendig belegt, nur an einer
    // anderen Wirkung -- der Lauf REAGIERT auf GENAU DEN uebergebenen Pfad und nennt ihn woertlich in
    // seiner Ansage; ohne das Argument gibt es weder Ansage noch Artefakt (Fall (2)). Das ist ein
    // SCHAERFERER Durchreichungs-Beleg als "irgendeine Datei erschien": die Zeile traegt die Zeichenkette.
    // Die MECHANIK der Ablage (Plan + Zaehler + Resume) ist unveraendert belegt, aber dort, wo die Atome
    // eine pruefbare Identitaet haben: test_tp1_planer_filter_iterator, Faelle (11a)-(11c2) am
    // run_planer_driven_provision und (11l-b)/(11l-f) am vollen run_lazy_static_then_dynamic.
    //
    // BIS T2-A/F4-NB3 SAH DAS HIER ANDERS AUS -- und zwar aus einem Grund, der kein Beleg war: ohne
    // COMDARE_BESTANDSLOG gab es GAR KEINEN Provider, der Stempel trug `ohne-anker`, und die Ablage
    // entstand trotzdem. Der gruene Stand pinnte damit genau die Ordnung, die Befund 1 als Defekt
    // benennt. Diese Nachfuehrung ist bewusst und nicht sein stilles Verschwinden.
    // ============================================================================================

    // -- (1) ARG GESETZT -> DIE KETTE REAGIERT AUF GENAU DIESEN PFAD.
    fs::path const plan_datei = base / "ablage" / "batch_plan.txt";
    fs::path const z_datei    = fs::path{plan_datei.string() + ".zaehler"};
    FakeStore      store1;
    std::uint64_t  lauf1_bereitgestellt = 0;
    std::string    lauf1_log;
    {
        tlz::RunProfileArgs const a = mach_args(store1, base / "lauf1", plan_datei);
        CerrFang                  fang;
        tlz::RunProfileResult const r = tlz::run_profile(a);
        lauf1_log                     = fang.text();
        lauf1_bereitgestellt          = r.any_provisioned;
        check_eq("(1) der Lauf endet regulaer", r.exit_code, 0);
        check_true("(1) und hat wirklich etwas bereitgestellt", lauf1_bereitgestellt > 0);
    }
    // (1a) DER DURCHREICHUNGS-BELEG: die Ansage nennt GENAU den uebergebenen Pfad. Keine andere Stelle
    //      dieses Laufs kennt diese Zeichenkette -- sie kann nur ueber RunProfileArgs::batch_plan_datei
    //      -> make_cfg -> LazyRunConfig::batch_plan_datei dorthin gekommen sein.
    check_true("(1a) die Kette reagiert auf den uebergebenen Pfad -- und nennt ihn woertlich",
               lauf1_log.find("plan-ablage INERT (bau-weg)") != std::string::npos &&
                   lauf1_log.find(plan_datei.string()) != std::string::npos);
    // (1b) UND SIE BEZIFFERT DEN GRUND: das eine Atom dieses Profils traegt keine pruefbare Identitaet.
    check_true("(1b) mit dem bezifferten Grund (Formwache, nicht fehlender Provider)",
               lauf1_log.find("1 von 1 Atomen liefern keine pruefbare Bau-Identitaet") != std::string::npos &&
                   lauf1_log.find("erstes: view_index 0") != std::string::npos);
    check_true("(1b) und ausdruecklich NICHT mit 'kein Fingerprint-Anker' -- der Provider IST gesetzt",
               lauf1_log.find("KEINEN Fingerprint-Anker") == std::string::npos);
    // (1c) FAIL-CLOSED: lieber keine Ablage als eine, die ein ungedecktes Atom zertifiziert.
    check_true("(1c) FAIL-CLOSED: KEIN Batch-Plan auf der Platte", !fs::exists(plan_datei, ec));
    check_true("(1c) und KEIN Phasen-Zaehler daneben", !fs::exists(z_datei, ec));
    // (1d) ALT-STAND-BISS: genau DIESER Stempel waere frueher abgelegt worden -- ankerlos, und damit fuer
    //      JEDEN Bau-Stand derselbe. max_binaries=1 => Basis-Selektion {0}.
    std::vector<std::size_t> const eins{0};
    std::string const             alt_stempel = bl::slice_plan_stamp(eins, bl::kBuildSliceGrain);
    check_true("(1d) ALT-STAND-BISS: der frueher hier abgelegte Stempel traegt |bau=ohne-anker",
               alt_stempel.find(std::string{"|bau="} + bl::kPlanOhneAnker) != std::string::npos);

    // -- (2) ARG LEER -> NICHTS. Derselbe Lauf, nur ohne Ablage-Pfad: das Opt-in-Muster bleibt inert.
    {
        fs::path const      plan_leer = base / "ablage_leer" / "batch_plan.txt";
        FakeStore           store2;
        tlz::RunProfileArgs a = mach_args(store2, base / "lauf2", plan_leer);
        a.batch_plan_datei.clear(); // DER Unterschied zu (1)
        tlz::RunProfileResult const r = tlz::run_profile(a);
        check_eq("(2) auch dieser Lauf endet regulaer", r.exit_code, 0);
        check_true("(2) KEIN Plan -- leerer Pfad heisst inert, nicht 'irgendwohin'",
                   !fs::exists(plan_leer, ec));
        check_true("(2) und kein Zaehler", !fs::exists(fs::path{plan_leer.string() + ".zaehler"}, ec));
        check_true("(2) das Ablage-Verzeichnis wird gar nicht erst angelegt",
                   !fs::exists(plan_leer.parent_path(), ec));
    }

    // -- (3) DIE WIRKUNG DER FAIL-CLOSED-LINIE: der zweite Lauf ERBT NICHTS und baut ehrlich neu.
    //    Frisches Ausgabe-Verzeichnis, damit der Bau SICHTBAR ist.
    {
        fs::path const              lauf3 = base / "lauf3";
        std::string                 log3;
        tlz::RunProfileResult       r{};
        {
            tlz::RunProfileArgs const a = mach_args(store1, lauf3, plan_datei);
            CerrFang                  fang;
            r    = tlz::run_profile(a);
            log3 = fang.text();
        }
        // (3a) KEIN PLAN-RESUME -- es gibt keinen Plan, den man erben koennte, und das ist die Zusage:
        //      ein Atom ohne pruefbare Identitaet darf NIE uebersprungen werden. Der Beleg ist die
        //      Platte: das frische Ausgabe-Verzeichnis dieses Laufs ist GEFUELLT (er hat gebaut).
        check_true("(3a) NEU: der Folgelauf baut ehrlich neu -- ohne gedeckte Identitaet kein Resume",
                   fs::exists(lauf3 / "dll", ec) && !fs::is_empty(lauf3 / "dll", ec));
        // (3b) UND ER IST EIN GUELTIGER LAUF: die Ablage-Ebene ist Buchhaltung, kein Bau-Fehler.
        check_eq("(3b) exit 0 -- die inerte Ablage bricht keinen Bau ab", r.exit_code, 0);
        check_eq("(3c) und er stellt dieselbe eine Binary bereit wie Lauf 1", r.any_provisioned,
                 lauf1_bereitgestellt);
        // (3d) GEGENPROBE ZUR HERKUNFT: gemessen hat dieser Lauf nichts (provision-only).
        check_eq("(3d) nichts gemessen (provision-only)", r.any_measured, std::uint64_t{0});
        check_eq("(3d) und nichts mess-resumiert", r.any_resumed, std::uint64_t{0});
        // (3e) UND DIE ANSAGE STEHT AUCH HIER -- der Zustand ist nicht einmalig, sondern die Lage.
        check_true("(3e) auch der Folgelauf sagt die inerte Ablage beziffert an (nie stumm)",
                   log3.find("plan-ablage INERT (bau-weg)") != std::string::npos &&
                       log3.find(plan_datei.string()) != std::string::npos);
        check_true("(3e) und es entsteht weiterhin weder Plan noch Zaehler",
                   !fs::exists(plan_datei, ec) && !fs::exists(z_datei, ec));
    }

    fs::remove_all(base, ec);
    std::cout << (g_fail == 0 ? "== ALLE PRUEFUNGEN BESTANDEN ==\n" : "== FEHLGESCHLAGEN ==\n");
    return g_fail == 0 ? 0 : 1;
}
