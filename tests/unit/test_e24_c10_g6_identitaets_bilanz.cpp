// test_e24_c10_g6_identitaets_bilanz -- E-24 C10 (c) / Gate G6, Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 5.2: "+ceb=-Diff literal / Stempel-Preimage-
// UNVERAENDERT-Beweis (OverlayHash identisch fuer ein Referenz-SA-Binary ohne Code-Aenderung) /
// CRC64 unveraendert".
//
// DIE AUSSAGE VON G6: der Major-Bump C8 bewegt GENAU EINE Identitaets-Ebene -- den Objekt-Store-Key
// (+ceb=) und die Lade-Akzeptanz -- und laesst die beiden ANDEREN Ebenen unberuehrt: den A13/W10-
// STEMPEL (SHA512-Preimage der Tier-Binaries) und den golden-/binary_id-ANKER (CRC64). Genau diese
// Trennung ist die Voraussetzung dafuer, dass am Fenster-Ende EIN Anker-Vollzug reicht und nicht zwei
// (Bauplan Paragraf 5.1/5.2, Risiko R1 "Anker-Doppelung").
//
// WAS EINE BILANZ VON EINER BEHAUPTUNG UNTERSCHEIDET: die drei Groessen stehen hier als LITERALE
// Zahlen/Digests, die VOR dem Bump erhoben wurden. Ein Test, der nur "Konstante == Konstante" prueft,
// wuerde jede kuenftige Drehung stillschweigend mitmachen.
//
// ERHEBUNG DER VOR-WERTE (reproduzierbar, im C10-(c)-Commit literal protokolliert): dieselbe Sonde
// gegen den Stand 19adba05 (VOR C8) und gegen HEAD uebersetzt; die Digests unten sind die Ausgabe des
// VOR-Laufs. Der Nach-Lauf liefert sie byte-gleich -- das IST der Preimage-UNVERAENDERT-Beweis, und er
// gilt, weil der ABI-Major in KEINEM der Preimage-Glieder vorkommt.
//
// [NACHGEFUEHRT 2026-08-05, O-2/C-2 (Format 2 -> 3): die beiden Digest-Literale sind in DIESEM Commit NEU
// eingefroren -- nicht weil die G6-Aussage falsch waere, sondern weil das Preimage zwei Glieder dazubekommen
// hat (Toolchain [5], bvset [6]) und das Overlay ans Ende gewandert ist. Die Bilanz-AUSSAGE ist unberuehrt:
// der C8-Major-Bump hat die Stempel-Ebene nicht bewegt, der O-2-Neuanker bewegt sie ABSICHTLICH und genau
// einmal. Die Vor-Werte stehen unten bei den Konstanten und in der git-Historie.]
//
// ABWEICHUNG GEGEN DEN BAUPLAN (deklariert, nicht stillschweigend korrigiert): Paragraf 5.1 rechnet
// den Shift als "7.1 -> 8.0". Am Objekt erhoben war der Stand VOR C8 aber "+ceb=7.2" -- zwischen dem
// Bauplan (03.08.) und C8 hat W10 den codegen-Minor von 1 auf 2 gedreht (der C8-Commit benennt den
// Reset korrekt als "2 -> 0"). Die WIRKUNG ist unveraendert (ein Shift des Objekt-Store-Key-Namensraums,
// einmalige Bucket-Invalidierung vor Voll-Bau-4); nur die Zwischenstufe der Kette lautet anders. Die
// vollstaendige Kette ist damit: 7.0 -> 7.1 (M4) -> 7.2 (W10) -> 8.0 (C8).
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_MCE24_PLAIN_TESTS -> add_test +
// COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include "builder/artifact_transport/artifact_cache.hpp" // cache_key_prefix (die EINE Key-Naht)

#include <cache_engine/abi/anatomy_fingerprint.hpp>        // Preimage-Glieder + consteval-Digest
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // COMDARE_ANATOMY_ABI_MAJOR / kCebContractCodegenMinor
#include <cache_engine/abi/anatomy_version_stamp.hpp>      // kOrganAxisCount (18 Organ-Haupt-Achsen)
#include <profile_facade/source_catalog.hpp>               // kNewGolden131072Crc64 (der golden-Anker)
#include <profile_facade/system_version_suffix.hpp>        // ceb_contract_version_text + kSuffixSegmentOrder

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace abi = comdare::cache_engine::abi;
namespace at  = comdare::cache_engine::builder::artifact_transport;
namespace pf  = comdare::cache_engine::profile_facade;
namespace tlz = comdare::cache_engine::thesis_lazy;

int g_fail = 0;

template <class A, class B>
void eq(char const* w, A const& g, B const& e) {
    bool const ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << "\n         ist  = " << g << "\n";
    if (!ok) {
        std::cout << "         soll = " << e << "\n";
        ++g_fail;
    }
}

void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ---------------------------------------------------------------------------------------------------
// Die VOR-C8-Werte des Stempels. Erhoben am Stand 19adba05, BEVOR der Major gedreht wurde.
// ---------------------------------------------------------------------------------------------------

/// Referenz-"SA-Binary": ein fixes Tripel aus Organ-/System-/Mess-Zeile. Es muss kein echtes Binary
/// sein -- die Aussage ist "gleiche Eingabe, gleicher Stempel", und dafuer ist ein FESTES Tripel der
/// schaerfere Zeuge als ein wanderndes.
inline constexpr std::string_view kRefOrgan  = "search_algo=array256@1.0.0.c;memory_layout=soa@1.0.0.c";
inline constexpr std::string_view kRefSystem = "compiler=gcc@16.0.0.c;os=linux@1.0.0.c;isa=x86_64@1.0.0.c";
inline constexpr std::string_view kRefMess   = "wallclock@1.0.0.c+load_framework=intern@1.0.0.c";

/// Der Digest des Referenz-Tripels. [NEU EINGEFROREN 2026-08-05, O-2/C-2: der Preimage-Format-Bump 2 -> 3
/// (zwei zusaetzliche Glieder) verschiebt beide Digests dieser Bilanz -- deklariert und im SELBEN Commit
/// nachgezogen. Die AUSSAGE der Bilanz bleibt: der ABI-MAJOR bewegt sie nicht. Die VOR-C8-Werte
/// f8da53ce...c5af (Referenz) und fa10b791...23ae (leer) stehen in der git-Historie.]
/// [ERNEUT EINGEFROREN 2026-08-07, R-3: derselbe Vorgang aus demselben Grund -- der Format-Bump 3 -> 4
/// haengt das NEUNTE Glied an (Mess-Gates, abi/mess_gates_glied.hpp) und bewegt damit beide Digests.
/// Die AUSSAGE der Bilanz ist unveraendert und wird durch den Neu-Anker NICHT entschaerft: sie sagt,
/// dass der ABI-MAJOR die Stempel-Ebene nicht bewegt -- nicht, dass diese Ebene unbeweglich WAERE.
/// Format-3-Werte: 6667b5bf...91c6c493 (Referenz) und ec3a5ea9...c8ee9cb (leer), in der git-Historie.]
/// [ZUM DRITTEN MAL EINGEFROREN 2026-08-07, FLAG-GRAMMATIK v2 (Owner-KERN): die Versions-Schreibweise
/// des REFERENZ-TRIPELS wandert von "@1.0.0c" auf "@1.0.0.c" -- damit aendert sich die EINGABE des
/// Digests, nicht das Verfahren. Der LEER-Digest bleibt deshalb unberuehrt (er hat keine Versionen im
/// Preimage), und genau diese Asymmetrie ist der Beleg, dass hier die Zeilen gewandert sind und nicht
/// die Glied-Struktur. Format-4/R-3-Referenzwert: 24ca8905...173fd194, in der git-Historie.]
/// [ZUM VIERTEN MAL EINGEFROREN 18.08.2026, S-6a/KON45-01: Format 4 -> 5. Zwei Ursachen in EINEM Bump --
/// (a) die Kategorien-Drehung der Glieder [1][2][3] auf MESS, SYSTEM, ORGAN und (b) das zehnte Glied
/// (Hybrid-Komposit, leer, aber sein Separator zaehlt). Diesmal bewegen sich BEIDE Digests, und das ist
/// die erwartete Asymmetrie-Umkehr zum Flag-Grammatik-Dreh: dort wanderte nur der Referenzwert (die
/// EINGABE aenderte sich), hier wandert auch der leere (die GLIED-STRUKTUR aendert sich). Genau das
/// belegt, dass der Bump wirkt. Format-4-Werte: d1ddf922888c7fd1...ffbe20b0 (Referenz) und
/// 0a74581660d2dd6d...572a0a3c (leer), in der git-Historie.]
inline constexpr std::string_view kRefDigestVorC8 = "425f4e2cb427ae760281c4b8c86a87cbb1ce3cfcc7f8e2a712f4b1a7a7cf8d66"
                                                    "c0e3f62e3ba2a516f334a52c9ce91b83932ec471dc5044fbddc950e7d92a2717";

/// Der Digest des LEEREN Tripels -- der Zeuge der Glied-STRUKTUR (leere Glieder, aber Separatoren bleiben;
/// das ist der GA-01-Fix, und er wuerde bei jeder Glied-Umsortierung brechen). Genau deshalb ist er der
/// empfindlichste Wert dieser Datei: er hat sich mit dem Format-3-Bump bewegt, WEIL zwei Glieder und damit
/// zwei Separatoren dazugekommen sind -- das ist der Beweis, dass der Bump wirkt.
inline constexpr std::string_view kLeerDigestVorC8 = "e8ca5445c569ac9eb13414cb4bd23d02b5d6acf8e05eab6edc25d68144afe544"
                                                     "b9a1fca35d1bc794915609d2633c21ebe5c019212836bccc00ca72b09ba360af";

/// Der CRC64-Anker der 2^17-golden-Menge. TABU-Wert; er steht hier als Bilanz-Zeuge, nicht als zweite
/// Wahrheit -- die RECHNENDE Wache bleibt test_limits_entkopplung_vorstufe, die die 131072 ids
/// regeneriert und die CRC gegen dieselbe Konstante prueft.
inline constexpr std::uint64_t kCrc64AnkerVorC8 = 0x56F1B721C72DC10EULL;

} // namespace

int main() {
    std::cout << "==== E-24 C10 (c) / G6: die Identitaets-Bilanz des Major-Bumps ====\n";

    // ----------------------------------------------------------------------------------------------
    // EBENE 1 -- DIE BEWEGTE: der Objekt-Store-Key (+ceb=).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Ebene 1 (BEWEGT): +ceb= / Objekt-Store-Key --\n";
    {
        std::string const ceb = pf::ceb_contract_version_text();
        std::cout << "  Kette der Shifts: 7.0 -> 7.1 (M4) -> 7.2 (W10) -> 8.0 (E-24 C8) -> 8.1 (B14-NB4) -> 9.1 "
                  << "-> 9.2 (A-14/B-5b) "
                     "(NAHT-1 d4c0b49c)\n";
        eq("der HEUTIGE +ceb=-Wert (Major aus der ABI, Minor aus kCebContractCodegenMinor)", ceb, std::string{"9.2"});
        eq("... Major-Anteil == COMDARE_ANATOMY_ABI_MAJOR", static_cast<int>(COMDARE_ANATOMY_ABI_MAJOR), 9);
        // B14-NB4: Bump 0 -> 1 unter Major 8 (Vertrags-Erweiterung kV3AxisSchema[5][5] = line_bytes).
        // Diese Fundstelle war beim Bump uebersehen worden -- gefangen vom ctest-Doppellauf, nicht vom
        // Decl-Header, der weiterhin von "dem EINEN literalen Pin" sprach.
        eq("... Minor-Anteil == kCebContractCodegenMinor (A-14/B-5b: 1 -> 2)",
           static_cast<int>(abi::kCebContractCodegenMinor), 2);
        tr("der Vor-Wert 7.2 ist NICHT mehr der Ist-Wert (der Shift hat stattgefunden)", ceb != "7.2");

        // Die EINE Key-Naht: sie MUSS genau ein +ceb=-Segment fuehren (Suffix-Wache-Regel), und dessen
        // Wert ist der heutige. Zwei Segmente waeren die W10-C4-Falle (Anhaengen statt Einfalten).
        ::unsetenv("COMDARE_MEASUREMENT_COMBO");
        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        at::ArtifactCache const cache = at::ArtifactCache::from_env();
        std::string const       bv    = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
        std::string const       key   = cache.cache_key_prefix(bv);
        std::cout << "  cache_key_prefix = " << key << "\n";
        std::size_t n   = 0;
        std::size_t pos = key.find("+ceb=");
        while (pos != std::string::npos) {
            ++n;
            pos = key.find("+ceb=", pos + 1);
        }
        eq("der Objekt-Store-Key traegt GENAU EIN +ceb=-Segment", n, std::size_t{1});
        tr("... und es traegt den heutigen Wert (+ceb=9.2)", key.find("+ceb=9.2") != std::string::npos);
        tr("... und der C8-Vor-Wert kommt nirgends mehr vor (+ceb=8.0)", key.find("+ceb=8.0") == std::string::npos);
        tr("... und der Vor-Wert kommt nirgends mehr vor (+ceb=7.2)", key.find("+ceb=7.2") == std::string::npos);

        // Die POSITION des Glieds in der deklarativen Ordnung ist unveraendert -- der Bump bewegt den
        // WERT, nicht die Segment-Ordnung (sonst waere jede Suffix-Zerlegung mitbetroffen).
        eq("die Segment-Ordnung fuehrt +ceb= unveraendert an Position 3", std::string{pf::kSuffixSegmentOrder[3]},
           std::string{"+ceb="});
    }

    // ----------------------------------------------------------------------------------------------
    // EBENE 2 -- DIE VOM ABI-MAJOR UNBEWEGTE: der A13/W10-Stempel (SHA512-Preimage).
    // R-3 (07.08.2026): "unbewegt" hiess hier immer "vom C8-Major-Dreh unbewegt" -- nie "unbeweglich".
    // Ein DEKLARIERTES Preimage-Ereignis (Format-Bump) bewegt sie sehr wohl und soll es; die Anker
    // werden dann im selben Commit nachgezogen (so geschehen 05.08. fuer 2 -> 3 und heute fuer 3 -> 4).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Ebene 2 (durch den ABI-MAJOR UNBEWEGT; R-3-Format-Bump nachgezogen): Stempel-Preimage "
                 "/ SHA512 --\n";
    {
        // S-6a/KON45-01 (18.08.2026): Format 5, ZEHN Glieder. Diese beiden Zeilen sind KEINE Aussage darueber, dass
        // die Stempel-Ebene unbeweglich waere -- sie sind der Zeuge dafuer, dass sie sich NUR durch
        // deklarierte Preimage-Ereignisse bewegt und nie durch einen ABI-Major-Dreh. Genau deshalb werden
        // sie bei jedem Format-Bump im SELBEN Commit nachgezogen (Praezedenz O-2/C-2, 05.08.).
        eq("die Preimage-FORMAT-Kennung", std::string{abi::kAnatomyFingerprintFormat},
           std::string{"fingerprint_format=5"});
        eq("die Zahl der Preimage-Glieder", abi::kAnatomyFingerprintGliedCount, std::size_t{10});
        eq("die Position der System-Zeile in der Glied-Folge", abi::kAnatomyFingerprintSystemGlied, std::size_t{2});

        // E-E (07.08.2026): das Overlay-Glied [7] wird ab hier EXPLIZIT LEER gereicht -- und die beiden
        // Digests bleiben dadurch BYTE-IDENTISCH (nachgemessen an einer Probe-TU: der Leer-Digest ist
        // unveraendert 0a74581660d2dd6d...572a0a3c). Das ist kein Ausweichen vor der Scharfschaltung,
        // sondern die Voraussetzung dafuer, dass diese Bilanz weiter etwas AUSSAGT: sie prueft, ob der
        // ABI-MAJOR die Stempel-Ebene bewegt. Liefe der Default herein, traege sie ab jetzt den
        // QUELLTEXT-Stand des Baums, bewegte sich bei jedem Commit, und die Aussage waere nicht mehr
        // messbar -- der Zeuge wuerde durch Rauschen ersetzt. Die Wirksamkeit des LIVE-Werts beweist
        // stattdessen der Bissbeweis in test_m_w12_stamp_bausteine.cpp.
        constexpr auto kRefJetzt =
            abi::anatomy_fingerprint_hex(abi::MessZeile{kRefMess}, abi::SystemZeile{kRefSystem},
                                         abi::OrganZeile{kRefOrgan}, abi::ToolchainGlied{abi::kToolchainStampGlied},
                                         abi::BvsetGlied{abi::kBuildVariantSetSignatureGlied}, abi::OverlayHash{""});
        std::string const ref_jetzt{kRefJetzt.data()};
        std::cout << "  Referenz-SA-Tripel:\n    organ  = " << kRefOrgan << "\n    system = " << kRefSystem
                  << "\n    mess   = " << kRefMess << "\n";
        eq("der Stempel des Referenz-Tripels ist IDENTISCH zum eingefrorenen Wert", ref_jetzt,
           std::string{kRefDigestVorC8});

        constexpr auto kLeerJetzt = abi::anatomy_fingerprint_hex(
            abi::MessZeile{""}, abi::SystemZeile{""}, abi::OrganZeile{""},
            abi::ToolchainGlied{abi::kToolchainStampGlied}, abi::BvsetGlied{abi::kBuildVariantSetSignatureGlied},
            abi::OverlayHash{""}); // E-E: s. oben
        std::string const leer_jetzt{kLeerJetzt.data()};
        eq("der Stempel des LEEREN Tripels ist IDENTISCH zum eingefrorenen Wert (Glied-Struktur)", leer_jetzt,
           std::string{kLeerDigestVorC8});

        // Der STRUKTURELLE Grund, warum der ABI-MAJOR diese Ebene nicht bewegt: er kommt in keinem der
        // neun Glieder vor -- SOLANGE das Toolchain-Glied nicht befuellt ist.
        //
        // [NACHGEFUEHRT 2026-08-05, O-2/C-2 -- EHRLICHE EINSCHRAENKUNG statt stiller Fortschreibung:] das
        // Toolchain-Glied [5] traegt per Bauplan ein ceb=<abi_major>.<codegen_minor>-Feld (F7-Spez (a),
        // G-C2 "heilt Fall C"). Sobald die C-3-Scheibe es injiziert, KOPPELT der ABI-Major den
        // Tier-Stempel -- und das ist gewollt: ein Contract-Bruch MUSS die Tier-Binaries invalidieren.
        // Die G6-Bilanz behaelt ihre Aussage fuer den Stand, den sie beschreibt (der C8-Bump allein hat
        // die Stempel-Ebene nicht bewegt), und diese Wache prueft ab jetzt die DEFAULT-Glied-Folge, also
        // exakt den Zustand vor der Injektion. Wer sie nach C-3 gruen halten will, muss die Bilanz neu
        // formulieren -- nicht die Wache entschaerfen.
        auto const glieder = abi::anatomy_fingerprint_glieder(
            abi::MessZeile{kRefMess}, abi::SystemZeile{kRefSystem}, abi::OrganZeile{kRefOrgan},
            abi::ToolchainGlied{abi::kToolchainStampGlied}, abi::BvsetGlied{abi::kBuildVariantSetSignatureGlied},
            abi::OverlayHash{""}); // E-E: s. oben
        auto const major_text = std::to_string(COMDARE_ANATOMY_ABI_MAJOR);
        bool       major_frei = true;
        for (auto const& glied : glieder) {
            if (glied.find("abi_major") != std::string_view::npos) major_frei = false;
            if (glied.find("ceb=") != std::string_view::npos) major_frei = false;
        }
        tr("kein Preimage-Glied der DEFAULT-Folge traegt den ABI-Major oder ein ceb=-Feld "
           "(vor der C-3-Injektion bleiben die Ebenen getrennt)",
           major_frei);
        (void)major_text;
    }

    // ----------------------------------------------------------------------------------------------
    // EBENE 3 -- DIE UNBEWEGTE: der golden-/binary_id-Anker (CRC64).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Ebene 3 (UNBEWEGT): golden-/binary_id-Anker / CRC64 --\n";
    {
        std::cout << "  kNewGolden131072Crc64 = 0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
                  << tlz::kNewGolden131072Crc64 << std::dec << std::nouppercase << "\n";
        eq("der CRC64-Anker ist unveraendert (TABU-Wert)", tlz::kNewGolden131072Crc64, kCrc64AnkerVorC8);
        // organ_count bleibt 18: der Major bewegt die binary_id-permutierende Komposition NICHT -- das
        // ist der Grund, warum es KEINE golden_fullpilot_320_binary_ids_abi7.txt gibt (anders als bei
        // den Majors 4/5/6, die jeweils an einer BEWEGTEN binary_id hingen).
        eq("die Zahl der Organ-Haupt-Achsen (binary_id-permutierend) ist unveraendert",
           static_cast<int>(abi::kOrganAxisCount), 18);
    }

    std::cout << "\n==== E-24 C10 (c) / G6: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
