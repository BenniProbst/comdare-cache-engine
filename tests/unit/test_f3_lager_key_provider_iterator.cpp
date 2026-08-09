// test_f3_lager_key_provider_iterator.cpp -- F3-Testschuld (Ledger D-04, :3720-3721; Zusage
// "vor dem naechsten Lager-Increment"): der ITERATOR-LAUF mit dem EINEN REALEN bestand_key_of.
//
// STEMPEL-HOMONYMIE (T2-A/F4-NB3, 2026-08-06): dieser Lauf faehrt BEWUSST OHNE cfg.batch_plan_datei --
// der Plan-Stempel `batchplan-v3|...|bau=` aus slice_plan_stamp ist hier inert und wird NICHT geprueft.
// Geprueft wird der LAGER-/SIDECAR-Stempel: `.fingerprint`-Blattinhalt -> make_fingerprint_key_fn ->
// Lager-Eintrag. Zwei verschiedene Dinge mit demselben Wort; die Verwechslung der beiden hat den
// F4-NB2-Fehlschluss ausgeloest ("den Stempel liest ohnehin niemand" -- gemeint war der eine, gelesen
// wurde der andere).
//
// DIE LUECKE, die diese TU schliesst (am Ist erhoben, 2026-08-03):
//   * test_g3_artifact_cache_transport.cpp deckt make_fingerprint_key_fn ISOLIERT ab (Sidecar da /
//     fehlt / leer / '\n' / CRLF / 127 / 129 / nicht-hex) -- aber nie im Lauf.
//   * test_tp1_planer_filter_iterator.cpp Fall (8) deckt den Verwurf an der FUNKTIONS-NAHT ab
//     (mess_pfad_synchron_push direkt, mit synthetischen 128-hex-Keys), und sein Fall (5) setzt
//     cfg.bestand_key_of auf eine nullopt-Lambda.
//   => Der reale Provider lief in KEINEM Iterator-Test. Genau die Kette
//        .fingerprint-Sidecar (write_fingerprint_sidecar) -> make_fingerprint_key_fn ->
//        LagerRunState::observe -> werfender CachePushFn -> AsyncPushPump::failed_dirs ->
//        bestand_eintragspfad -> discard_fresh_with_pfad_prefix -> flush
//      war unbelegt. Sie ist der Weg, den der Produktions-Aufrufer (super main.cpp, cache_push +
//      bestand_key_of) im BAU-Lauf nimmt.
//
// WAS HIER BEWIESEN WIRD (tragend, kein Smoke; OE-B-Dummy-Lager im temp-Verzeichnis):
//   (F3-a) SCHREIBER UND LESER TEILEN EINE WAHRHEIT: der Orchestrator legt je gebauter Binary das
//          `.fingerprint`-Sidecar real auf die Platte, und make_fingerprint_key_fn liest genau
//          diese Datei zurueck -- 128 hex fuer X/Y, 129 Byte (mit '\n') fuer Z, 127 fuer W.
//   (F3-b) WURF -> VERWURF: der Push der Binary X wirft den REALEN Kontrakt-Typ
//          at::ArtefaktPushFehler; nach dem Pump-Drain fliegt ihr vorgemerkter Eintrag ueber den
//          bestand_eintragspfad-Praefix aus fresh_ ("NICHT registriert"), waehrend die Binary Y
//          mit gelungenem Push registriert wird -- gezielter Ausschluss, keine Pauschal-Enteignung.
//   (F3-c) TP1-F1-TRENNERSEMANTIK AM ECHTEN PROVIDER-WEG: die Stems der beiden Binaries kollidieren
//          als NACKTE Praefixe ("...cl_line_1" ist Praefix von "...cl_line_16"). Nur der Vergleich
//          MIT Trenner laesst Y stehen. Der Testfall belegt den Verlust literal: ohne '/' haette
//          derselbe Praefix Y mitgerissen.
//   (F3-d) AUF-A3-TRIM IM LAUF: die Binary Z traegt ein Sidecar mit '\n'-Schwanz (129 Byte). Ihr
//          Lager-Eintrag steht mit dem GETRIMMTEN 128-hex im geteilten Dokument -- ohne den Trim
//          waere sie still (DedupOutcome::no_key) aus dem Lager gefallen.
//   (F3-e) 127-ZEICHEN-SIDECAR -> no_key OHNE VERWURF: die Binary W erzeugt gar keinen Eintrag
//          (laenge_verstoss) -- sie wird weder registriert noch verworfen; die Verwurf-Zahl bleibt
//          exakt 1 (nur X), und der flush meldet neu=2.
//   (F3-f) FOLGELAUF gegen DASSELBE Dokument: Y wird uebersprungen (Bestand), X wird ERNEUT GEBAUT
//          -- der stille Verlustpfad (Folgelauf skippt eine nirgends existierende Binary) ist
//          geschlossen. Z/W sind konservative Misses (die Presence-Naht loest den Fingerprint ueber
//          die FingerprintFn auf, un-getrimmt) -> ein verschenkter Skip, nie ein Verlust.
//
// Deterministisch, ohne minio/mc: FakeStore-Transport (in-Memory, Muster test_g3_takeover_sweep),
// Stub-Compile ohne echte DLLs (perm.dll entsteht nie -- die Sidecars aber sehr wohl, und genau die
// liest der Provider). Build: plain main (KEIN gtest), Return 0/1 -- Registrierung nach dem
// test_tp1_planer_filter_iterator-Muster (schwerer Host-Treiber-Header + Boost::mp11).

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include <builder/bestandslog/fingerprint_key_source.hpp>     // der EINE reale bestand_key_of-Provider
#include <builder/build_orchestrator/fingerprint_sidecar.hpp> // die EINE Suffix-Wahrheit (AUF-A2)

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace bl = ::comdare::cache_engine::builder::bestandslog;
namespace at = ::comdare::cache_engine::builder::artifact_transport;
namespace fs = std::filesystem;

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

// In-Memory-Transport (Muster test_g3_takeover_sweep, ohne Fehler-Skript).
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

constexpr char const* kDocKey = "bestandslog/test_f3_bestand.xml";

// cerr-Fang fuer die Testat-Zeilen (Muster test_g3_takeover_sweep). Faengt auch die Zeilen des
// Push-Thread-Umfelds: der rdbuf-Tausch wirkt global, und der Pump ist beim Auswerten gejoint.
class CerrCapture {
public:
    CerrCapture() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrCapture(CerrCapture const&)            = delete;
    CerrCapture& operator=(CerrCapture const&) = delete;
    ~CerrCapture() { std::cerr.rdbuf(alt_); }

    [[nodiscard]] std::string text() const { return puffer_.str(); }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

// VIER statische Blaetter (1x4) + eine dynamische Dimension. Die Werte der zweiten Achse sind mit
// Absicht "1"/"16": nach der Stem-Sanitisierung (orch_make_stem: jedes Nicht-alnum -> '_') ist der
// Stem von Blatt 0 ein NACKTES Praefix des Stems von Blatt 1 -- die TP1-F1-Klasse auf Ordner-Ebene.
ex::ExperimentTree make_tree(std::shared_ptr<ex::ExperimentNodeFactory> const& f) {
    ex::ExperimentTree t{f};
    t.build({
        ex::AxisLevel{"traversal", {"ART"}, true, "", ""},
        ex::AxisLevel{"node.cl_line", {"1", "16", "2", "3"}, true, "", ""},
        ex::AxisLevel{"concurrency", {"1"}, false, "thread_count", ""},
    });
    return t;
}

// Gemeinsame Lauf-Konfiguration (frisches Ausgabe-Verzeichnis je Lauf). build_version in der
// Flag-Grammatik v2; sie kann hier nie einen Skip ausloesen, weil der Stub-Compile gar keine
// perm.dll erzeugt und dll_is_current deren Existenz verlangt -- die EINZIGE Skip-Quelle ist damit
// der Lager-Bau-Filter (saubere Zurechnung).
ex::LazyRunConfig make_cfg(FakeStore& store, fs::path const& out) {
    ex::LazyRunConfig cfg;
    cfg.source_dir         = out / "src";
    cfg.output_dir         = out / "dll";
    cfg.build_version      = "1.0.0.c";
    cfg.per_binary_subdirs = true;
    cfg.build_parallelism  = 1;
    cfg.provision_only     = true;
    cfg.max_binaries       = 4;
    cfg.bestand_transport  = store.transport();
    cfg.bestand_doc_key    = kDocKey;
    cfg.bestand_owner_uuid = "uuid-f3-anker";
    cfg.bestand_maschine   = "prodX";
    cfg.bestand_zelle      = bl::ZellKoordinaten{.combo = "default", .opt = "O2", .simd = "avx2"};
    // DER PUNKT DIESER TU: der EINE reale Provider statt einer Test-Lambda.
    cfg.bestand_key_of = bl::make_fingerprint_key_fn();
    cfg.marker_kontext = ex::MarkerKontext{"amd", "[O2,avx2][default]", "[all]"};
    return cfg;
}

// Liefert den Bestands-Eintrag zu einem store-relativen Pfad (nullopt = nicht im Dokument).
[[nodiscard]] std::optional<bl::BestandEintrag> eintrag_zu(bl::BestandslogDocument const& doc,
                                                           std::string const&             pfad) {
    auto const it = std::find_if(doc.bestand.begin(), doc.bestand.end(),
                                 [&pfad](bl::BestandEintrag const& e) { return e.pfad == pfad; });
    if (it == doc.bestand.end()) return std::nullopt;
    return *it;
}

[[nodiscard]] std::string datei_inhalt(fs::path const& p) {
    std::ifstream     f{p, std::ios::binary};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

int main() {
    std::cout << "==== F3/D-04: Iterator-Lauf mit dem REALEN bestand_key_of (.fingerprint-Sidecar) ====\n";

    auto                 factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree   tree    = make_tree(factory);
    ex::StaticBinaryView view    = tree.static_binary_view();
    check_eq("Vorbedingung: 4 statische Blaetter", view.size(), std::size_t{4});

    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_f3_lager_key";
    std::error_code ec;
    fs::remove_all(base, ec);

    auto compile_stub = [](ex::BuildJob const&) -> int { return 0; };
    auto gen_stub     = [](std::string const&) { return std::string{"// f3-anker-stub\n"}; };
    auto ram_stub     = []() -> std::uint64_t { return ~std::uint64_t{0}; };

    std::vector<std::size_t> const alle{0, 1, 2, 3};

    // Die vier Rollen (Index -> Stem -> Sidecar-Inhalt, den der Orchestrator BYTEGENAU schreibt):
    //   X = 0 ("...cl_line_1")  128 hex        -> Push WIRFT   -> muss verworfen werden
    //   Y = 1 ("...cl_line_16") 128 hex        -> Push gelingt -> muss registriert werden
    //   Z = 2 ("...cl_line_2")  128 hex + '\n' -> Push gelingt -> registriert MIT getrimmtem Key
    //   W = 3 ("...cl_line_3")  127 hex        -> Push gelingt -> no_key: kein Eintrag, kein Verwurf
    std::string const stem_x = ex::orch_make_stem(view[0].binary_id, view[0].index);
    std::string const stem_y = ex::orch_make_stem(view[1].binary_id, view[1].index);
    std::string const stem_z = ex::orch_make_stem(view[2].binary_id, view[2].index);
    std::string const stem_w = ex::orch_make_stem(view[3].binary_id, view[3].index);
    std::string const hex_x  = std::string(128, 'a');
    std::string const hex_y  = std::string(128, 'b');
    std::string const hex_z  = std::string(128, 'c');
    std::string const roh_w  = std::string(127, 'd'); // laenge_verstoss (bewusst 127)
    std::string const pfad_x = stem_x + "/perm.dll";
    std::string const pfad_y = stem_y + "/perm.dll";
    std::string const pfad_z = stem_z + "/perm.dll";

    // (F3-c) Vorbedingung der Trenner-Semantik: OHNE sie waere der Fall leer. Der Stem von X ist ein
    // nacktes Praefix des Stems von Y -- ein starts_with(stem_x) haette Y mitgerissen.
    check_true("(F3-c) Vorbedingung: stem_x ist NACKTES Praefix von stem_y (Kollisions-Lage besteht)",
               stem_y.starts_with(stem_x) && stem_y != stem_x);
    check_true("(F3-c) Beleg des Verlusts: der Eintragspfad von Y beginnt mit dem Praefix von X",
               pfad_y.starts_with(stem_x));
    check_true("(F3-c) MIT Trenner trifft derselbe Praefix Y NICHT", !pfad_y.starts_with(stem_x + "/"));

    // Der Fingerprint-Provider des Hosts (I2): binary_id -> Sidecar-Inhalt. Der Orchestrator schreibt
    // den Rueckgabewert BYTEGENAU (write_fingerprint_sidecar) -- ein '\n'-behafteter Wert erzeugt also
    // exakt die 129-Byte-Datei, die ein fremdes Werkzeug hinterliesse (AUF-A3, Befund B26).
    // Nach dem Aufbau nur noch LESEND benutzt -> aus den Build-Workern gefahrlos.
    std::map<std::string, std::string> fp_by_id;
    fp_by_id[view[0].binary_id]   = hex_x;
    fp_by_id[view[1].binary_id]   = hex_y;
    fp_by_id[view[2].binary_id]   = hex_z + "\n";
    fp_by_id[view[3].binary_id]   = roh_w;
    ex::FingerprintFn const fp_fn = [&fp_by_id](std::string const& binary_id) -> std::string {
        auto const it = fp_by_id.find(binary_id);
        return (it == fp_by_id.end()) ? std::string{} : it->second;
    };

    FakeStore store; // EIN Lager ueber beide Laeufe -- der Folgelauf liest, was der erste schrieb

    // == LAUF 1: Bau mit werfendem Push fuer X ================================================
    {
        ex::LazyRunConfig cfg      = make_cfg(store, base / "lauf1");
        cfg.bestand_fingerprint_fn = fp_fn;

        // Der werfende Transport: der REALE Kontrakt-Typ (artifact_cache.hpp:153), nicht irgendeine
        // Exception -- genau der, den push_tier_binary seit TP1FK1-B2 (ce c7a6ec20) wirft.
        // Der Pump hat EINEN Worker-Thread und ist beim Auswerten gejoint (close() joint) -> die
        // Liste braucht keine eigene Sperre.
        std::vector<std::string> gepusht;
        ex::CachePushFn const    push = [&gepusht, &stem_x](fs::path const& bin_dir, std::string const&) {
            gepusht.push_back(bin_dir.filename().string());
            if (bin_dir.filename().string() == stem_x)
                // cppcheck-suppress throwInEntryPoint // FP: der Wurf IST der Testfall, der Pump faengt ihn
                throw at::ArtefaktPushFehler{"Store-Push simuliert fehlgeschlagen", bin_dir,
                                             "tier/v1.0.0c/" + stem_x + "/perm.dll", 1};
        };
        cfg.cache_push = push;

        ex::BuildSelection sel;
        sel.indices    = alle;
        sel.provenance = "explicit";

        std::string       spur;
        ex::LazyRunResult r;
        {
            CerrCapture fang;
            r    = ex::run_lazy_static_then_dynamic(tree, sel, compile_stub, gen_stub, ram_stub, cfg);
            spur = fang.text();
        }

        check_eq("(F3-a) alle 4 Blaetter frisch gebaut (leeres Lager)", r.built_new, std::size_t{4});
        check_eq("(F3-a) kein Lager-Skip im ERSTEN Lauf", r.bestand_lager_skips, std::size_t{0});
        check_eq("(F3-b) jede gebaute Binary wurde EINMAL gepusht", gepusht.size(), std::size_t{4});

        // (F3-a) Der Schreiber hat real geschrieben -- und der Leser liest genau diese Dateien.
        fs::path const side_x = ex::fingerprint_sidecar_path(cfg.output_dir / stem_x / "perm.dll");
        fs::path const side_y = ex::fingerprint_sidecar_path(cfg.output_dir / stem_y / "perm.dll");
        fs::path const side_z = ex::fingerprint_sidecar_path(cfg.output_dir / stem_z / "perm.dll");
        fs::path const side_w = ex::fingerprint_sidecar_path(cfg.output_dir / stem_w / "perm.dll");
        check_true("(F3-a) das .fingerprint-Sidecar von X liegt real auf der Platte", fs::exists(side_x, ec));
        check_eq("(F3-a) X: 128 Byte auf der Platte", datei_inhalt(side_x).size(), std::size_t{128});
        check_eq("(F3-a) Y: 128 Byte auf der Platte", datei_inhalt(side_y).size(), std::size_t{128});
        check_eq("(F3-a) Z: 129 Byte auf der Platte (mit '\\n'-Schwanz)", datei_inhalt(side_z).size(),
                 std::size_t{129});
        check_eq("(F3-a) W: 127 Byte auf der Platte (laenge_verstoss)", datei_inhalt(side_w).size(), std::size_t{127});
        // Und der EINE reale Provider gibt daraus genau die Lager-Schluessel zurueck, mit denen der
        // Lauf gerade gearbeitet hat (kein zweiter Rechenweg im Test).
        auto const key_of = bl::make_fingerprint_key_fn();
        auto const kx     = key_of(cfg.output_dir / stem_x / "perm.dll");
        auto const ky     = key_of(cfg.output_dir / stem_y / "perm.dll");
        auto const kz     = key_of(cfg.output_dir / stem_z / "perm.dll");
        auto const kw     = key_of(cfg.output_dir / stem_w / "perm.dll");
        check_true("(F3-a) Provider liefert X den 128-hex-Schluessel", kx.has_value() && *kx == hex_x);
        check_true("(F3-a) Provider liefert Y den 128-hex-Schluessel", ky.has_value() && *ky == hex_y);
        check_true("(F3-d) Provider TRIMMT den '\\n'-Schwanz von Z", kz.has_value() && *kz == hex_z);
        check_true("(F3-e) Provider lehnt das 127-Zeichen-Sidecar von W ab (nullopt)", !kw.has_value());

        // (F3-b/e) Die Testat-Zeilen: GENAU EIN Verwurf (X), und der flush registriert 2 (Y+Z).
        check_true("(F3-b) der Ausschluss ist beziffert (Nie-stumm)",
                   spur.find("NICHT registriert") != std::string::npos);
        check_true("(F3-b/e) genau EIN Eintrag wegen EINEM Push-Fehler verworfen",
                   spur.find("[bestandslog] warn: 1 Eintraege wegen 1 Push-Fehler(n) NICHT registriert") !=
                       std::string::npos);
        check_true("(F3-e) der flush registriert 2 Eintraege (Y+Z; W hatte nie einen)",
                   spur.find("[bestandslog] lager=0 hits=0 neu=2") != std::string::npos);

        // (F3-b/c/d/e) Das GETEILTE Dokument -- die Wahrheit, die der Folgelauf liest.
        auto const raw = store.objs.find(kDocKey);
        check_true("(F3-b) Dokument im Store", raw != store.objs.end());
        auto const doc = (raw == store.objs.end()) ? std::nullopt : bl::parse_bestandslog(raw->second);
        check_true("(F3-b) Dokument parsebar", doc.has_value());
        if (doc) {
            check_eq("(F3-e) genau ZWEI Bestands-Eintraege (Y+Z)", doc->bestand.size(), std::size_t{2});
            check_true("(F3-b) X (Push warf) steht NICHT im geteilten Dokument", !eintrag_zu(*doc, pfad_x));
            auto const ey = eintrag_zu(*doc, pfad_y);
            auto const ez = eintrag_zu(*doc, pfad_z);
            check_true("(F3-c) Y steht drin -- der '=1'-Praefix hat '=16' NICHT mitgerissen", ey.has_value());
            check_true("(F3-c) und traegt den Schluessel aus SEINEM Sidecar", ey && ey->key_sha512 == hex_y);
            check_true("(F3-d) Z steht drin", ez.has_value());
            check_true("(F3-d) und traegt den GETRIMMTEN 128-hex-Schluessel (kein '\\n' im Lager-Key)",
                       ez && ez->key_sha512 == hex_z && ez->key_sha512.size() == 128);
            check_true("(F3-e) W (127-Zeichen-Sidecar) hat gar keinen Eintrag erzeugt",
                       !eintrag_zu(*doc, stem_w + "/perm.dll"));
        }
    }

    // == LAUF 2 (F3-f): DERSELBE Store, frisches Ausgabe-Verzeichnis -- was skippt der Filter? ==
    //    Der Beweis, dass der Verwurf WIRKT: X ist nicht im Lager, also baut dieser Lauf X erneut.
    //    Stuende X faelschlich im Dokument, wuerde er hier uebersprungen -- der stille Verlustpfad.
    {
        ex::LazyRunConfig cfg      = make_cfg(store, base / "lauf2");
        cfg.bestand_fingerprint_fn = fp_fn;
        cfg.cache_push             = [](fs::path const&, std::string const&) {}; // diesmal gelingt jeder Push

        std::vector<std::size_t> konfig_cursor; // Fenster-Positionen der WIRKLICH gebauten Binaries
        cfg.progress_sink = [&konfig_cursor](ex::ProgressDelta const& d) {
            if (!d.done) konfig_cursor.push_back(d.cursor);
        };

        ex::BuildSelection sel;
        sel.indices    = alle;
        sel.provenance = "explicit";

        std::string       spur;
        ex::LazyRunResult r2;
        {
            CerrCapture fang;
            r2   = ex::run_lazy_static_then_dynamic(tree, sel, compile_stub, gen_stub, ram_stub, cfg);
            spur = fang.text();
        }

        check_eq("(F3-f) genau EIN Lager-Skip (Y)", r2.bestand_lager_skips, std::size_t{1});
        check_eq("(F3-f) drei Binaries neu gebaut (X, Z, W)", r2.built_new, std::size_t{3});
        // Die Erwartung folgt aus den ROLLEN oben, nicht aus dem Lauf: 0=X (verworfen -> nicht im
        // Lager -> muss neu), 2=Z und 3=W (fuer die Presence-Naht unaufloesbar, s. u.), und 1=Y
        // faellt als einziger Bestands-Treffer heraus.
        std::vector<std::size_t> const erwartet{0, 2, 3};
        check_true("(F3-f) X wird ERNEUT GEBAUT, Y uebersprungen (Cursor 0,2,3)", konfig_cursor == erwartet);
        // Ehrliche Grenze, damit die 3 nicht als Zufall gelesen wird: die Presence-Naht loest den
        // Fingerprint ueber die FingerprintFn auf (lager_presence.hpp), NICHT ueber das Sidecar --
        // sie trimmt also nicht. Z ("...\n") und W (127) sind dort unaufloesbar und damit KONSERVATIVE
        // MISSES: ein verschenkter Skip, nie ein Verlust (der Lager-Eintrag von Z bleibt bestehen).
        check_true("(F3-f) Z wurde erneut gebaut, aber NICHT doppelt registriert (lager_hit)",
                   spur.find("[bestandslog] lager=2 hits=1 neu=1") != std::string::npos);

        auto const raw = store.objs.find(kDocKey);
        auto const doc = (raw == store.objs.end()) ? std::nullopt : bl::parse_bestandslog(raw->second);
        check_true("(F3-f) Dokument parsebar", doc.has_value());
        if (doc) {
            check_eq("(F3-f) jetzt DREI Eintraege -- X ist nachgetragen, kein Duplikat", doc->bestand.size(),
                     std::size_t{3});
            check_true("(F3-f) X steht nach dem gelungenen Push drin", eintrag_zu(*doc, pfad_x).has_value());
        }
    }

    std::cout << (g_fail == 0 ? "F3_LAGER_KEY_OK\n" : "F3_LAGER_KEY_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
