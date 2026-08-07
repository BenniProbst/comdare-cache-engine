// measurement/flag_grammar_catalog.hpp -- DER KATALOG der FLAG-GRAMMATIK v2 (Bauschritt S2).
//
// Die Grammatik (measurement/algo_semver.hpp) prueft die FORM: Punkt-Notation, Klammer-Struktur,
// Token-Regeln. Sie prueft NICHT, ob ein Token ein echtes Hardware-Flag ist -- "1.0.0.x512{quatsch}" ist
// dort WOHLGEFORMT. Diese Datei traegt die zweite Haelfte: WELCHE Token es gibt und UNTER WELCHER BASIS
// jedes stehen darf. Zusammen sind sie die KATALOG-WACHE (algo_semver.hpp: flag_catalog_is_satisfied).
//
// WARUM DER KATALOG HIER LIEGT UND NICHT IN algo_semver.hpp: die Andockstelle dort sagt es verbatim --
// "der Katalog selbst gehoert nach S2 und in die Nachbarschaft von measurement/simd_feature_flag.hpp".
// Die Grammatik ist eine Aussage ueber ZEICHEN, der Katalog eine ueber HARDWARE. Zwei Wahrheiten, zwei
// Dateien, EINE Naht (der Include unten in algo_semver.hpp).
//
// QUELLE, nicht geraten: die SIMD-Recherche vom 07.08.2026 (Owner-Auftrag F-5/F-6, 6 Lenses, 108 Token,
// 32- und 64-bit), abgelegt als
//   super docs/sessions/backups/20260807-explores-und-simd-katalog/
//     SIMD-KATALOG-vollstaendig-32-und-64-bit-6-lenses.json
// Massgeblich ist dort der SYNTHESE-Knoten `result.katalog` (basen + companion + empfehlung), nicht die
// einzelnen Lenses -- die widersprechen sich an drei Stellen, und die Synthese loest sie mit Belegen auf
// (s. die Einzelnachweise an den betroffenen Zeilen unten).
//
// == DIE DREI NAMENSRAEUME, ALS DREI TABELLENFELDER ================================================
// Ein SIMD-Flag hat DREI Namen, und keine zwei sind auseinander ableitbar:
//     Faehigkeit      cpuinfo-Id (Kernel)   Compiler-Schalter    UNSER Grammatik-Token
//     SSE3            pni                   -msse3               sse3
//     SSE4.1          sse4_1                -msse4.1             sse41
//     SHA-NI          sha_ni                -msha                sha
//     BMI1            bmi1                  -mbmi                bmi1     (Ziffer nur in der cpuinfo-Id)
//     RDRAND          rdrand                -mrdrnd              rdrand   (das 'a' faellt nur im Schalter)
//     PCLMULQDQ       pclmulqdq             -mpclmul             pclmulqdq
//     AVX-512 VBMI2   avx512_vbmi2          -mavx512vbmi2        vbmi2    (unter x512)
// measurement/simd_feature_flag.hpp fuehrt die ERSTEN BEIDEN seit dem 19.07.2026 getrennt und
// ausdruecklich "NIE per String-Heuristik ineinander umgerechnet". Diese Datei folgt DEMSELBEN Muster und
// erfindet kein zweites: der Eintrag bekommt ein DRITTES Feld (token) daneben -- er LEITET es nicht ab.
// Warum der dritte Namensraum zwingend ist: "-msse4.1" traegt einen PUNKT (unser Trenner) und
// "avx512_vbmi2" einen UNTERSTRICH (Owner-F-7: "Ja wir verwenden NUR den Punkt"). Beide sind in der
// Grammatik nicht schreibbar; ohne eigenen Namensraum gaebe es fuer diese Faehigkeiten gar kein Token.
//
// == DIE VIER STRUKTURFAELLE (alle vier traegt die Tabelle, s. FlagTokenKind) =======================
//   (1) BASIS MIT SUB-LISTE          x512{f.vl.bw.dq}, x128{sse.sse2....}   BreitenBasis + BreitenSubset
//   (2) COMPANION OHNE BASIS         gfni, vaes, vpclmulqdq                 Companion
//   (3) SKALAR OHNE REGISTERBREITE   popcnt, bmi1, bmi2, abm, ...           Skalar
//   (4) OHNE REGISTERBREITE UEBERH.  mmx, mmxext, 3dnow, 3dnowext           BasislosFamilie
// (2), (3) und (4) sehen in der FORM gleich aus -- ein Token auf Tiefe 0 ohne Klammer -- und sind
// trotzdem drei verschiedene Sachen. Genau deshalb steht die Unterscheidung im KATALOG und nicht im
// Parser: die Form kann sie nicht sehen. Sie steckt hier im Feld `kind`, und `kind` ist der einzige
// Grund, warum diese Tabelle mehr ist als eine Elternteil-Pruefung.
//
// == DER OFFENE OWNER-ENTSCHEID ZU FALL (4) -- HIER NICHT ENTSCHIEDEN ==============================
// Die MMX-/3DNow!-Familie liegt auf den 64-bit-MM-Registern, die auf den x87-Stack ALIASIERT sind. Sie
// gehoert damit unter KEINE der Basen x128/x256/x512. In der Grammatik kann sie ZWEI Gestalten annehmen:
//     (a) blosses Token auf Tiefe 0   "1.0.0.c.mmx.mmxext.3dnow"
//     (b) eigene Basis mit Klammer    "1.0.0.c.x64{mmx.mmxext.3dnow}"
// Welche RICHTIG ist, ist eine Aussage ueber die Hardware-Semantik (haben MM-Register eine "Breite" im
// Sinne der Basen?) und damit ein Owner-Entscheid. DIESE WACHE NIMMT IHN NICHT VORWEG: sie akzeptiert
// BEIDE Gestalten. Technisch traegt das Feld `eltern_alternativ` -- die betroffenen Eintraege nennen
// SOWOHL die Tiefe 0 ("") ALS AUCH "x64" als zulaessige Platzierung.
// DER PREIS, offen benannt: bis zum Entscheid gibt es fuer DIESELBE Tatsache ZWEI schreibbare Formen,
// und weil der Stempel Identitaet ist, sind das ZWEI verschiedene Byte-Folgen. Das ist der Grund, warum
// der Entscheid faellig ist -- aber ihn zu RATEN waere teurer: eine falsche Basis stuende dann in jedem
// Fingerprint-Preimage, das je gebaut wird.
// MECHANISCHE MARKIERUNG: `entscheid_offen` ist bei GENAU sechs Eintraegen gesetzt, und ein
// static_assert unten zaehlt sie. Wer den Entscheid vollzieht, kommt an diesen sechs Zeilen nicht vorbei.
//
// Metaprog: reine POD-Deskriptoren (trivially-copyable), alles constexpr, kein std::variant, keine
// vtable, KEINE dynamische Allokation, keine Laufzeit-Map -- die Wache laeuft im consteval-Pfad des
// Anatomie-Fingerprints mit.

#pragma once

#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::measurement {

/// Welche SORTE Flag ein Katalog-Eintrag ist. Die Form kann das nicht sehen (s. Kopf, Faelle 2/3/4) --
/// deshalb steht es hier und nicht im Parser.
enum class FlagTokenKind : std::uint8_t {
    HardwareBasis       = 0, // c g f n -- ZIEL-HARDWARE-Klasse, KEIN SIMD (Design-Doc: CPU/GPU/FPGA/NPU)
    HardwareUnterklasse = 1, // p e -- Kern-Klasse, verengt die CPU-Nutzung; nur unter 'c'
    BreitenBasis        = 2, // x128 x256 x512 (+ x64, s. offener Entscheid) -- Registerbreite
    BreitenSubset       = 3, // Fall (1): steht in der Klammer SEINER Basis
    Companion           = 4, // Fall (2): eigenes CPUID-Bit, die Breite FOLGT der Basis
    Skalar              = 5, // Fall (3): reine GPR-Instruktion, GAR KEINE Vektorbreite
    BasislosFamilie     = 6, // Fall (4): x87-aliasierte MM-Register, unter KEINER Breiten-Basis
};

/// Stabiles Etikett je Sorte (Single-Source fuer Diagnose-Ausgaben; kein Roh-String am Emit-Ort).
[[nodiscard]] constexpr std::string_view flag_token_kind_label(FlagTokenKind k) noexcept {
    switch (k) {
        case FlagTokenKind::HardwareBasis: return "hardware_basis";
        case FlagTokenKind::HardwareUnterklasse: return "hardware_unterklasse";
        case FlagTokenKind::BreitenBasis: return "breiten_basis";
        case FlagTokenKind::BreitenSubset: return "breiten_subset";
        case FlagTokenKind::Companion: return "companion";
        case FlagTokenKind::Skalar: return "skalar";
        case FlagTokenKind::BasislosFamilie: return "basislos_familie";
    }
    return "unbekannt"; // out-of-range-Cast -> sichtbarer Default (kein UB, kein stiller Skip)
}

/// EIN Katalog-Eintrag: die drei Namensraeume + die Sorte + die zulaessige(n) Platzierung(en).
///
/// IDENTITAET IST DAS PAAR (token, eltern), NICHT das Token allein. Der Katalog belegt das an zwei
/// Stellen selbst: 'vnni' existiert als avx_vnni (unter x256) UND als avx512_vnni (unter x512), 'ifma'
/// als avx_ifma (x256) UND avx512ifma (x512). Es sind VERSCHIEDENE CPUID-Bits mit verschiedenen
/// Compiler-Schaltern -- also zwei ZEILEN, nicht eine Zeile mit zwei Eltern. Die Basis disambiguiert
/// innerhalb der Klammer; ausserhalb gaebe es keine Disambiguierung, und genau deshalb steht keines der
/// beiden als Companion da.
///
/// LEERE FELDER, ihre Bedeutung (nie "unbekannt", immer eine Aussage):
///   eltern == ""             -- das Token steht auf TIEFE 0 (kein Elternteil).
///   eltern_alternativ == ""  -- es gibt KEINE zweite zulaessige Platzierung (Normalfall).
///   cpuinfo == ""            -- ueber den heutigen Erhebungsweg (/proc/cpuinfo) NICHT signaturfaehig.
///                               Das ist eine BELEGTE Aussage der Recherche, keine Nachlaessigkeit: sie
///                               hat elf Kandidaten live gegen /proc/cpuinfo geprueft und alle elf als
///                               abwesend vermerkt. Ein Eintrag mit leerem cpuinfo kann per Definition
///                               nie ein Signatur-Match erzeugen -- er ist GRAMMATISCH gueltig und
///                               MASCHINELL nicht nachweisbar, und das darf man ihm ansehen.
///   gpp == ""                -- es gibt keinen Compiler-Schalter (c/g/f/n, p/e, die Breiten-Basen).
struct FlagCatalogEntry {
    std::string_view token;   // UNSER Grammatik-Token (der DRITTE Namensraum)
    std::string_view cpuinfo; // exakter /proc/cpuinfo-String; leer == nicht signaturfaehig
    std::string_view gpp;     // g++/clang -m<flag>; leer == kein Compiler-Schalter
    FlagTokenKind    kind;
    std::string_view eltern;                 // "" == Tiefe 0
    std::string_view eltern_alternativ{};    // NUR fuer den offenen Fall-(4)-Entscheid belegt
    bool             entscheid_offen{false}; // haengt an einem offenen Owner-Entscheid
};

/// DER KATALOG. Reihenfolge = Lese-Reihenfolge der Recherche-Synthese (Ziel-Hardware, Breiten-Basen mit
/// ihren Subsets, dann die drei basislosen Klassen). Sie ist stabil, weil Diagnose-Ausgaben sie zeigen.
inline constexpr std::array<FlagCatalogEntry, 62> kFlagGrammarCatalog{{
    // -- (0) ZIEL-HARDWARE: die vier Klassen der Grammatik (Design-Doc Abschnitt 1.1) ---------------
    // Das sind KEINE SIMD-Flags: sie sagen, FUER WELCHE Hardware gebaut wurde. Sie tragen deshalb
    // weder cpuinfo-Id noch Compiler-Schalter. g/f/n sind laut Design-Doc "reserviert, nicht
    // produziert" -- sie stehen trotzdem im Katalog, weil die GRAMMATIK sie fuehrt (basis := 'c' |
    // 'g' | 'f' | 'n' | ...) und die Wache die Grammatik nicht enger machen darf als sie ist.
    {"c", "", "", FlagTokenKind::HardwareBasis, ""},
    {"g", "", "", FlagTokenKind::HardwareBasis, ""}, // GPU  -- reserviert, nicht produziert
    {"f", "", "", FlagTokenKind::HardwareBasis, ""}, // FPGA -- reserviert, nicht produziert
    {"n", "", "", FlagTokenKind::HardwareBasis, ""}, // NPU  -- reserviert, nicht produziert
    // p/e sind Kern-KLASSEN und stehen NUR unter 'c' (Owner-F-2/R8). Genau diese Bedingung ist der
    // Grund, warum die Wache das ELTERN-Token braucht und nicht nur das Token: "1.0.0.p" ist eine
    // Aussage ueber nichts, "1.0.0.c{p}" eine ueber die CPU.
    {"p", "", "", FlagTokenKind::HardwareUnterklasse, "c"}, // performance core (Default: "c" == "c{p}")
    {"e", "", "", FlagTokenKind::HardwareUnterklasse, "c"}, // efficiency core (R7: NICHT MEHR experimental)

    // -- (1) DIE REGISTERBREITEN-BASEN --------------------------------------------------------------
    {"x128", "", "", FlagTokenKind::BreitenBasis, ""},
    {"x256", "", "", FlagTokenKind::BreitenBasis, ""},
    {"x512", "", "", FlagTokenKind::BreitenBasis, ""},
    // x64 EXISTIERT NUR, WEIL DER OWNER-ENTSCHEID ZU FALL (4) OFFEN IST (s. Kopf). Faellt er auf
    // "blosses Token", verschwindet diese Zeile ersatzlos; faellt er auf "eigene Basis", verlieren die
    // vier Familien-Eintraege unten ihr eltern_alternativ und tragen "x64" als eltern.
    {"x64", "", "", FlagTokenKind::BreitenBasis, "", "", true},

    // -- x128: 128-bit XMM, Legacy-SSE (66/F2/F3 0F) bzw. VEX.128 -----------------------------------
    // AUFNAHMEKRITERIUM (Synthese): das eigene CPUID-Bit kennt AUSSCHLIESSLICH eine 128-bit-Form, die
    // Breite waechst nie mit der Basis mit.
    // SYNTHESE-KORREKTUR, hier zementiert: die Lenses widersprechen sich bei aes/pclmulqdq/sha --
    // Lens 3 ordnet sie x128 zu, Lens 4 nennt sie Companion. Die Synthese loest zugunsten x128 auf,
    // mit Lens 4s EIGENEM Beleg: unter dem aes-Bit gibt es KEINE 256/512-Form (nur '66 0F38 DC
    // AESENC xmm' und 'VEX.128 VAESENC'), SHA256RNDS2 hat ueberhaupt nur die xmm-Form. Breitenfest
    // 128 IST Breiteninformation -- also gehoert es in die Klammer. Der echte Companion-Test
    // ("traegt KEINE Breiteninformation, die Breite folgt der Basis") trifft nur gfni/vaes/vpclmulqdq.
    {"sse", "sse", "-msse", FlagTokenKind::BreitenSubset, "x128"},
    {"sse2", "sse2", "-msse2", FlagTokenKind::BreitenSubset, "x128"},
    {"sse3", "pni", "-msse3", FlagTokenKind::BreitenSubset, "x128"}, // cpuinfo heisst "pni", NICHT "sse3"
    {"ssse3", "ssse3", "-mssse3", FlagTokenKind::BreitenSubset, "x128"},
    {"sse41", "sse4_1", "-msse4.1", FlagTokenKind::BreitenSubset, "x128"}, // Unterstrich vs. PUNKT
    {"sse42", "sse4_2", "-msse4.2", FlagTokenKind::BreitenSubset, "x128"},
    {"sse4a", "sse4a", "-msse4a", FlagTokenKind::BreitenSubset, "x128"}, // AMD, in Zen NICHT gestrichen
    {"aes", "aes", "-maes", FlagTokenKind::BreitenSubset, "x128"},
    {"pclmulqdq", "pclmulqdq", "-mpclmul", FlagTokenKind::BreitenSubset, "x128"}, // Schalter OHNE "qdq"
    {"sha", "sha_ni", "-msha", FlagTokenKind::BreitenSubset, "x128"},             // alle drei Namen verschieden

    // -- x256: 256-bit YMM, VEX-Kodierung ------------------------------------------------------------
    // AUFNAHMEKRITERIUM (Synthese): das eigene CPUID-Bit deckt VEX.128/VEX.256 ab, die 512-bit-Form
    // DERSELBEN Operation kommt aus einem ANDEREN Bit (AVX512F). Staerkster Beleg: VCVTPH2PS/VCVTPS2PH
    // existieren unter dem f16c-Bit nur VEX-kodiert auf XMM/YMM.
    {"avx", "avx", "-mavx", FlagTokenKind::BreitenSubset, "x256"},
    {"avx2", "avx2", "-mavx2", FlagTokenKind::BreitenSubset, "x256"},
    {"fma", "fma", "-mfma", FlagTokenKind::BreitenSubset, "x256"},
    {"f16c", "f16c", "-mf16c", FlagTokenKind::BreitenSubset, "x256"},
    {"vnni", "avx_vnni", "-mavxvnni", FlagTokenKind::BreitenSubset, "x256"}, // NICHT avx512_vnni, s.u.
    // Die naechsten sieben sind BAUBAR (gcc 15.3, -m32 und -m64 gemessen), aber die Recherche hat sie
    // live gegen /proc/cpuinfo als ABWESEND vermerkt und daraus geschlossen: ueber den heutigen
    // Erhebungsweg nicht signaturfaehig. Deshalb steht die cpuinfo-Spalte LEER -- eine geratene Id
    // waere schlimmer als eine leere: sie behauptete ein Match, das nie eintritt.
    {"ifma", "", "-mavxifma", FlagTokenKind::BreitenSubset, "x256"}, // NICHT avx512ifma, s.u.
    {"vnniint8", "", "-mavxvnniint8", FlagTokenKind::BreitenSubset, "x256"},
    {"vnniint16", "", "-mavxvnniint16", FlagTokenKind::BreitenSubset, "x256"},
    {"neconvert", "", "-mavxneconvert", FlagTokenKind::BreitenSubset, "x256"},
    // sha512/sm3/sm4 liegen laut Synthese HIER und nicht bei den Companions: ihre Traegerbreite 256 ist
    // in der Faehigkeit selbst fixiert (VEX-kodiert) -- sie folgen keiner Basis, sie SIND 256.
    {"sha512", "", "-msha512", FlagTokenKind::BreitenSubset, "x256"},
    {"sm3", "", "-msm3", FlagTokenKind::BreitenSubset, "x256"},
    {"sm4", "", "-msm4", FlagTokenKind::BreitenSubset, "x256"},

    // -- x512: 512-bit ZMM, EVEX-Kodierung, alle gegated auf avx512f (CPUID Leaf 7) -----------------
    // Das sind exakt die 14 Avx512-Eintraege von simd_feature_flag.hpp -- Zuordnung durch die Synthese
    // bestaetigt, keine Korrektur noetig. Architektonisch zentral ist 'vl': ohne vl gibt es EVEX NUR in
    // 512 bit; mit vl reicht eine x512-Basis auf xmm/ymm herunter.
    {"f", "avx512f", "-mavx512f", FlagTokenKind::BreitenSubset, "x512"},
    {"cd", "avx512cd", "-mavx512cd", FlagTokenKind::BreitenSubset, "x512"},
    {"vl", "avx512vl", "-mavx512vl", FlagTokenKind::BreitenSubset, "x512"},
    {"dq", "avx512dq", "-mavx512dq", FlagTokenKind::BreitenSubset, "x512"},
    {"bw", "avx512bw", "-mavx512bw", FlagTokenKind::BreitenSubset, "x512"},
    {"ifma", "avx512ifma", "-mavx512ifma", FlagTokenKind::BreitenSubset, "x512"},     // OHNE Unterstrich
    {"vbmi", "avx512vbmi", "-mavx512vbmi", FlagTokenKind::BreitenSubset, "x512"},     // OHNE Unterstrich
    {"vbmi2", "avx512_vbmi2", "-mavx512vbmi2", FlagTokenKind::BreitenSubset, "x512"}, // MIT Unterstrich
    {"vnni", "avx512_vnni", "-mavx512vnni", FlagTokenKind::BreitenSubset, "x512"},
    {"bitalg", "avx512_bitalg", "-mavx512bitalg", FlagTokenKind::BreitenSubset, "x512"},
    {"vpopcntdq", "avx512_vpopcntdq", "-mavx512vpopcntdq", FlagTokenKind::BreitenSubset, "x512"},
    {"vp2intersect", "avx512_vp2intersect", "-mavx512vp2intersect", FlagTokenKind::BreitenSubset, "x512"},
    {"bf16", "avx512_bf16", "-mavx512bf16", FlagTokenKind::BreitenSubset, "x512"},
    {"fp16", "avx512_fp16", "-mavx512fp16", FlagTokenKind::BreitenSubset, "x512"},

    // -- (2) COMPANION: die Breite FOLGT der Basis (Synthese "KLASSE A") ---------------------------
    // Der Test, den NUR diese drei bestehen: EIN CPUID-Bit deckt mehrere Breiten ab, die erreichte
    // Breite kommt aus der Basis. Musterfall gfni (GF2P8AFFINEQB): Legacy-SSE '66 0F3A CE' (128, sogar
    // OHNE AVX), VEX.128/256 -> "AVX GFNI", EVEX.512 -> "AVX512F GFNI", EVEX.128/256 -> "AVX512VL
    // GFNI". Es gibt KEIN separates avx512-gfni-Bit -- damit ist die Owner-Notation
    // "...x512{f.vl.bw.dq}.gfni" durch die ISA GEDECKT, und ein "x512{gfni}" waere falsch, weil es
    // dafuer kein CPUID-Bit gibt. GENAU DAS setzt die eltern-Spalte "" hier durch.
    {"gfni", "gfni", "-mgfni", FlagTokenKind::Companion, ""},
    {"vaes", "vaes", "-mvaes", FlagTokenKind::Companion, ""},
    {"vpclmulqdq", "vpclmulqdq", "-mvpclmulqdq", FlagTokenKind::Companion, ""},

    // -- (3) SKALAR: GAR KEINE Vektorbreite (Synthese "KLASSE B") ----------------------------------
    // Reine GPR-Instruktionen. Sie stehen wie die Companions ohne Klammer, aber aus dem
    // ENTGEGENGESETZTEN Grund (Companion: die Breite kommt von aussen; Skalar: es gibt keine Breite).
    // Deshalb sind es zwei `kind`-Werte und nicht einer.
    {"popcnt", "popcnt", "-mpopcnt", FlagTokenKind::Skalar, ""},
    {"bmi1", "bmi1", "-mbmi", FlagTokenKind::Skalar, ""}, // cpuinfo MIT Ziffer, Schalter OHNE
    {"bmi2", "bmi2", "-mbmi2", FlagTokenKind::Skalar, ""},
    {"abm", "abm", "-mabm", FlagTokenKind::Skalar, ""}, // dasselbe Bit wird als lzcnt gefuehrt, s. Reserve
    {"movbe", "movbe", "-mmovbe", FlagTokenKind::Skalar, ""},
    {"adx", "adx", "-madx", FlagTokenKind::Skalar, ""},
    {"rdrand", "rdrand", "-mrdrnd", FlagTokenKind::Skalar, ""}, // cpuinfo MIT 'a', Schalter OHNE
    {"rdseed", "rdseed", "-mrdseed", FlagTokenKind::Skalar, ""},
    // 3dnowprefetch: MEINE EINORDNUNG, und sie ist ausdruecklich als solche markiert. Der
    // Synthese-Katalog fuehrt das Token GAR NICHT (weder unter basen noch unter companion); Lens 1
    // nennt es "companion"; der Vollausbau-Beweis in algo_semver.hpp gruppiert es bei der
    // MMX-Familie. Ich ordne es als SKALAR ein, weil PREFETCH/PREFETCHW auf KEINEM Vektorregister
    // arbeitet -- weder auf MM noch auf XMM -- und die Breite damit weder folgt (Companion) noch
    // aliasiert ist (Fall 4). Es ist zugleich der einzige 3DNow!-Rest, der in modernen AMD- UND
    // Intel-CPUs weiterlebt und fuer eine Cache-Engine unmittelbar einschlaegig ist. Weil das eine
    // SETZUNG ist und keine Katalog-Aussage, traegt der Eintrag entscheid_offen.
    {"3dnowprefetch", "3dnowprefetch", "-mprfchw", FlagTokenKind::Skalar, "", "", true},

    // -- (4) OHNE REGISTERBREITE UEBERHAUPT: die MMX-/3DNow!-Familie -------------------------------
    // 64-bit-MM-Register, ALIASIERT auf den x87-FP-Stack. Sie gehoeren unter KEINE der Basen
    // x128/x256/x512. DIE PLATZIERUNG IST DER OFFENE OWNER-ENTSCHEID (s. Kopf): jeder dieser vier
    // Eintraege nennt DIE TIEFE 0 ("") UND "x64" als zulaessig, damit beide Gestalten heute tragen und
    // keine praejudiziert wird.
    // ABGRENZUNG, die die Recherche mitliefert: die Synthese empfiehlt diese Familie ausdruecklich
    // NICHT zur Aufnahme in den Produktiv-Katalog ("auf keiner erreichbaren Maschine wahr"). Sie steht
    // hier trotzdem, weil der VOLLAUSBAU-Beweis in algo_semver.hpp sie fuehrt und der Owner-Entscheid
    // zu ihrer Gestalt sonst gar nicht formulierbar waere. Sie ist Vokabular, keine Bau-Empfehlung.
    {"mmx", "mmx", "-mmmx", FlagTokenKind::BasislosFamilie, "", "x64", true},
    {"mmxext", "mmxext", "-m3dnowa", FlagTokenKind::BasislosFamilie, "", "x64", true},
    {"3dnow", "3dnow", "-m3dnow", FlagTokenKind::BasislosFamilie, "", "x64", true},
    // 3dnowext teilt sich den Compiler-Schalter -m3dnowa mit mmxext (GCC schaltet mit "Enhanced
    // 3DNow!" beides zugleich). Die cpuinfo-Ids sind verschieden -- das ist der Beleg dafuer, dass die
    // gpp-Spalte NICHT eindeutig sein muss und die cpuinfo-Spalte sehr wohl.
    {"3dnowext", "3dnowext", "-m3dnowa", FlagTokenKind::BasislosFamilie, "", "x64", true},
}};

/// WARUM ES EINE RESERVE-TABELLE GIBT UND KEINEN KOMMENTAR.
/// Ein Token, das die Wache ablehnt, kann aus zwei Gruenden abgelehnt werden: es ist NIE VORGESEHEN,
/// oder es ist BEWUSST DRAUSSEN. Der Unterschied ist fuer den, der spaeter davorsteht, der ganze
/// Unterschied -- und in einem Kommentar findet er ihn nicht. Diese Tabelle macht die zweite Klasse
/// abfragbar (flag_reserve_beleg) und, wichtiger, macht ihre DISJUNKTHEIT zum Katalog compile-time
/// pruefbar: niemand kann ein Reserve-Token stillschweigend zulassen, ohne dass ein static_assert
/// bricht. Die Wache selbst konsultiert diese Tabelle NICHT -- Reserve heisst abgelehnt.
enum class FlagReserveGrund : std::uint8_t {
    NichtBaubar       = 0, // kein Compiler der Projekt-Toolchain akzeptiert den Schalter (gemessen)
    KeinEigenesBit    = 1, // kein eigenes CPUID-Bit -> ein Signatur-Match koennte nie treffen
    BasisStrittig     = 2, // die Quellen ordnen es WIDERSPRUECHLICH ein -- eine Wahl waere geraten
    KeineBreitenBasis = 3, // arbeitet auf Registern ausserhalb von x128/x256/x512
    AusserhalbPark    = 4, // existiert und ist baubar, aber die Schnittmenge mit dem Messpark ist leer
};

struct FlagReserveEntry {
    std::string_view token;
    FlagReserveGrund grund;
    std::string_view beleg; // die Quelle, die den Grund traegt -- nie eine blosse Behauptung
};

/// NACHGEMESSEN, NICHT UEBERNOMMEN (07.08.2026, gcc 15.3.0 auf dieser Maschine, je -m32 und -m64,
/// headerfreie Quelle): jede Baubarkeits-Aussage der Belege unten ist hier selbst gegengeprueft worden,
/// weil sie im Code zementiert wird. Ergebnis:
///   ABGELEHNT in BEIDEN Bitbreiten: -mavx512er, -mavx512pf, -mavx5124vnniw, -mavx5124fmaps,
///                                   -mavx512bmm, -mtzcnt
///   AKZEPTIERT in beiden:           -mxop, -mfma4, -mtbm, -mlwp, -mlzcnt, -mprfchw, -m3dnowa
///   NUR -m64:                       -mapxf ("'-mapxf' is not supported for 32-bit code")
/// Das ist der Grund fuer die Trennung der Gruende: xop/fma4/tbm/lwp sind BAUBAR und trotzdem draussen
/// (kein Messpark-Schnitt), lzcnt ist BAUBAR und trotzdem draussen (kein eigenes Bit). Wer "Reserve"
/// mit "geht nicht" gleichsetzt, liegt bei fuenf der vierzehn Eintraege falsch.
inline constexpr std::array<FlagReserveEntry, 14> kFlagGrammarReserve{{
    // NICHT BAUBAR: oben selbst nachgemessen. Der Kernel fuehrt die cpuinfo-Strings teils weiter --
    // das macht sie zu historischem Vokabular, nicht zu baubaren Flags.
    {"er", FlagReserveGrund::NichtBaubar, "gcc 15.3: '-mavx512er' unrecognized (Knights-Landing-Erbe)"},
    {"pf", FlagReserveGrund::NichtBaubar, "gcc 15.3: '-mavx512pf' unrecognized (Knights-Landing-Erbe)"},
    {"4vnniw", FlagReserveGrund::NichtBaubar, "gcc 15.3: '-mavx5124vnniw' unrecognized (Knights-Mill-Erbe)"},
    {"4fmaps", FlagReserveGrund::NichtBaubar, "gcc 15.3: '-mavx5124fmaps' unrecognized (Knights-Mill-Erbe)"},
    {"bmm", FlagReserveGrund::NichtBaubar, "gcc 15.3 lehnt '-mavx512bmm' ab; AMD Zen 6, erst ab GCC 16"},
    // KEIN EIGENES CPUID-BIT: ein Eintrag dafuer koennte nie matchen -- exakt die Fehlerklasse, gegen
    // die simd_feature_flag.hpp mit is_known_simd_flag("avx512_phantom")==false gebaut ist.
    {"tzcnt", FlagReserveGrund::KeinEigenesBit,
     "Intel-Tabelle weist TZCNT der Feature-Spalte BMI1 zu; kein cpuinfo-String, kein -mtzcnt. Wer TZCNT "
     "will, fordert bmi1. Zusatzfalle: TZCNT dekodiert auf alten CPUs still als BSF mit anderer Semantik"},
    {"lzcnt", FlagReserveGrund::KeinEigenesBit,
     "IDENTISCHES CPUID-Bit wie abm (CPUID.80000001H:ECX[5]); der Kernel gibt es als 'abm' aus. Als "
     "eigenes Token entstuende ein Eintrag, den kein Signatur-Matching je trifft -- s. Katalog-Zeile abm"},
    // BASIS STRITTIG: hier eine Wahl zu treffen hiesse raten, und eine geratene Basis stuende danach in
    // jedem Fingerprint-Preimage.
    {"xop", FlagReserveGrund::BasisStrittig,
     "Lens 1 ordnet xop x128 zu, Lens 3 und Lens 4 ordnen es x256 zu (ueberwiegend 128-bit, einige "
     "FP-Befehle 256-bit). Die Synthese loest den Widerspruch NICHT auf und schliesst xop zugleich vom "
     "Produktivkatalog aus (AMD-only, mit Zen 2017 ersatzlos gestrichen)"},
    {"avx10", FlagReserveGrund::BasisStrittig,
     "STRUKTURBRUCH: gcc 15.3 warnt verbatim \"'-mavx10.1' is aliased to 512 bit since GCC14.3\", die "
     "MSVC-Primaerquelle sagt das GEGENTEIL (/arch:AVX10.1 Default-Vektorlaenge 256, erst /vlen=512 hebt "
     "an). DASSELBE Feature haette Basis x512 unter GCC und x256 unter MSVC. Zusatzfalle: die Token "
     "'avx10.1'/'avx10.2' tragen den PUNKT und sind in dieser Grammatik ohnehin nicht schreibbar"},
    // KEINE DER DREI BREITEN-BASEN: arbeitet auf Registern, die es hier nicht gibt.
    {"apx", FlagReserveGrund::KeineBreitenBasis,
     "APX erweitert die GPR-Datei (32 statt 16), keine Vektorbreite. Zugleich die EINZIGE harte "
     "32-bit-Grenze der ganzen Liste: gcc -m32 -mapxf -> \"'-mapxf' is not supported for 32-bit code\""},
    {"amx", FlagReserveGrund::KeineBreitenBasis,
     "AMX arbeitet auf 2D-Tiles (TMM), nicht auf ZMM. WARNUNG aus derselben Recherche: gcc akzeptiert "
     "-mamx-tile AUCH unter -m32 -- Schalter-Akzeptanz ist KEIN Verfuegbarkeitsbeweis"},
    // AUSSERHALB DES MESSPARKS: existiert, ist baubar, trifft aber keine erreichbare Maschine.
    {"fma4", FlagReserveGrund::AusserhalbPark,
     "AMD-only, mit Zen 2017 ersatzlos gestrichen; die Synthese schliesst fma4 ausdruecklich vom "
     "Produktivkatalog aus: die Schnittmenge mit dem lebenden Messpark ist leer"},
    {"tbm", FlagReserveGrund::AusserhalbPark, "AMD-only (Bulldozer-Familie), mit Zen 2017 ersatzlos gestrichen"},
    {"lwp", FlagReserveGrund::AusserhalbPark, "AMD-only (Bulldozer-Familie), mit Zen 2017 ersatzlos gestrichen"},
}};

// == DIE ABFRAGEN =================================================================================

inline constexpr std::size_t kNoFlagCatalogEntry = static_cast<std::size_t>(-1);

/// Der Index des Eintrags, der TOKEN unter ELTERN zulaesst -- oder kNoFlagCatalogEntry.
/// `eltern` ist "" fuer ein Token auf Tiefe 0. Ein INDEX statt eines Zeigers, weil der Aufrufer damit
/// im constexpr-Pfad rechnen kann, ohne dass eine Adresse in einen static_assert geraet (dasselbe
/// Muster wie kNoFlagParent in algo_semver.hpp).
[[nodiscard]] constexpr std::size_t find_flag_catalog_entry(std::string_view token, std::string_view eltern) noexcept {
    for (std::size_t i = 0; i < kFlagGrammarCatalog.size(); ++i) {
        FlagCatalogEntry const& e = kFlagGrammarCatalog[i];
        if (e.token != token) continue;
        if (e.eltern == eltern) return i;
        // Die zweite zulaessige Platzierung -- heute NUR der offene Fall-(4)-Entscheid. Die
        // empty()-Bedingung ist wesentlich: ohne sie liesse ein leeres eltern_alternativ jedes
        // Tiefe-0-Token unter jedem Eltern-Token durch.
        if (!e.eltern_alternativ.empty() && e.eltern_alternativ == eltern) return i;
    }
    return kNoFlagCatalogEntry;
}

/// Darf TOKEN unter ELTERN stehen? Das ist die Frage, die die Katalog-Wache je Knoten stellt.
[[nodiscard]] constexpr bool flag_token_is_admitted_under(std::string_view token, std::string_view eltern) noexcept {
    return find_flag_catalog_entry(token, eltern) != kNoFlagCatalogEntry;
}

/// Kennt der Katalog das Token UEBERHAUPT (an irgendeiner Stelle)? Trennt die zwei Ablehnungsgruende
/// "unbekanntes Token" und "bekanntes Token an falscher Stelle" -- fuer Diagnose, nicht fuer die Wache.
[[nodiscard]] constexpr bool flag_token_is_known(std::string_view token) noexcept {
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog)
        if (e.token == token) return true;
    return false;
}

/// Der Eintrag zu einer cpuinfo-Id -- oder kNoFlagCatalogEntry. Die LEERE Id trifft NIE (sie bedeutet
/// "nicht signaturfaehig" und ist kein Schluessel); ohne diese Zeile faende jede Suche den ersten
/// Eintrag ohne cpuinfo-Id.
[[nodiscard]] constexpr std::size_t find_flag_catalog_entry_by_cpuinfo(std::string_view cpuinfo) noexcept {
    if (cpuinfo.empty()) return kNoFlagCatalogEntry;
    for (std::size_t i = 0; i < kFlagGrammarCatalog.size(); ++i)
        if (kFlagGrammarCatalog[i].cpuinfo == cpuinfo) return i;
    return kNoFlagCatalogEntry;
}

/// Das Grammatik-Token zu einer cpuinfo-Id (leer == unbekannt). Die EINE erlaubte Richtung der
/// Uebersetzung zwischen zwei Namensraeumen: ueber die TABELLE, nie ueber eine String-Umformung.
[[nodiscard]] constexpr std::string_view flag_token_for_cpuinfo(std::string_view cpuinfo) noexcept {
    std::size_t const i = find_flag_catalog_entry_by_cpuinfo(cpuinfo);
    return (i == kNoFlagCatalogEntry) ? std::string_view{} : kFlagGrammarCatalog[i].token;
}

/// Steht das Token in der RESERVE, und mit welchem Beleg? Leer == nicht in der Reserve. Damit
/// unterscheidet eine Fehlermeldung "nie vorgesehen" von "bewusst draussen, hier ist der Grund".
[[nodiscard]] constexpr std::string_view flag_reserve_beleg(std::string_view token) noexcept {
    for (FlagReserveEntry const& r : kFlagGrammarReserve)
        if (r.token == token) return r.beleg;
    return {};
}

[[nodiscard]] constexpr bool flag_token_is_reserve(std::string_view token) noexcept {
    for (FlagReserveEntry const& r : kFlagGrammarReserve)
        if (r.token == token) return true;
    return false;
}

// == DIE WOHLGEFORMTHEIT DES KATALOGS SELBST (alles compile-time) =================================
// Eine Wache ist nur so gut wie ihre Tabelle. Diese Praedikate pruefen die TABELLE, nicht die Eingabe.

/// Jedes Token ist nicht leer, und jede Sorte, die einen Compiler-Schalter haben MUSS, hat einen.
/// (Die Ziel-Hardware-Klassen c/g/f/n, die Kern-Klassen p/e und die Breiten-BASEN haben bewusst
/// keinen -- sie sind Struktur, nicht Befehlssatz.)
[[nodiscard]] constexpr bool flag_catalog_entries_are_nonempty() noexcept {
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog) {
        if (e.token.empty()) return false;
        bool const traegt_schalter = e.kind == FlagTokenKind::BreitenSubset || e.kind == FlagTokenKind::Companion ||
                                     e.kind == FlagTokenKind::Skalar || e.kind == FlagTokenKind::BasislosFamilie;
        if (traegt_schalter && e.gpp.empty()) return false;
        if (!traegt_schalter && !e.gpp.empty()) return false; // Struktur-Token traegt NIE einen Schalter
    }
    return true;
}

/// Das PAAR (token, eltern) ist eindeutig. Das Token allein ist es NICHT -- 'vnni' und 'ifma' stehen
/// je zweimal (x256 und x512), und genau das ist der Grund, warum die Wache das Elternteil braucht.
[[nodiscard]] constexpr bool flag_catalog_pairs_unique() noexcept {
    for (std::size_t i = 0; i < kFlagGrammarCatalog.size(); ++i)
        for (std::size_t j = i + 1; j < kFlagGrammarCatalog.size(); ++j)
            if (kFlagGrammarCatalog[i].token == kFlagGrammarCatalog[j].token &&
                kFlagGrammarCatalog[i].eltern == kFlagGrammarCatalog[j].eltern)
                return false;
    return true;
}

/// Die cpuinfo-Id ist eindeutig, WO SIE GESETZT IST. Sie ist die Identitaet im Signatur-Matching
/// (SimdFeatureFlag::operator== vergleicht nur cpuinfo) -- zwei Eintraege mit derselben Id waeren zwei
/// Wahrheiten ueber dieselbe Maschinen-Eigenschaft. Leere Ids sind ausgenommen, sie sind kein Schluessel.
[[nodiscard]] constexpr bool flag_catalog_cpuinfo_unique() noexcept {
    for (std::size_t i = 0; i < kFlagGrammarCatalog.size(); ++i) {
        if (kFlagGrammarCatalog[i].cpuinfo.empty()) continue;
        for (std::size_t j = i + 1; j < kFlagGrammarCatalog.size(); ++j)
            if (kFlagGrammarCatalog[i].cpuinfo == kFlagGrammarCatalog[j].cpuinfo) return false;
    }
    return true;
}

/// Jedes genannte Eltern-Token existiert selbst im Katalog. Ohne diese Wache koennte eine Zeile auf eine
/// Basis zeigen, die es nicht gibt -- das Token waere dann UNERREICHBAR und die Tabelle behauptete eine
/// Zulassung, die nie greift.
[[nodiscard]] constexpr bool flag_catalog_parents_resolve() noexcept {
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog) {
        if (!e.eltern.empty() && !flag_token_is_known(e.eltern)) return false;
        if (!e.eltern_alternativ.empty() && !flag_token_is_known(e.eltern_alternativ)) return false;
        if (!e.eltern_alternativ.empty() && e.eltern_alternativ == e.eltern) return false; // waere ein Duplikat
    }
    return true;
}

/// Die ZWEITE Platzierung gibt es NUR beim offenen Fall-(4)-Entscheid. Diese Wache haelt fest, dass
/// niemand die Mehrdeutigkeit auf andere Eintraege ausdehnt, ohne es zu merken.
[[nodiscard]] constexpr bool flag_catalog_alternative_only_for_open_decision() noexcept {
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog)
        if (!e.eltern_alternativ.empty() && (e.kind != FlagTokenKind::BasislosFamilie || !e.entscheid_offen))
            return false;
    return true;
}

/// Wie viele Eintraege haengen an einem offenen Owner-Entscheid? Die Zahl ist unten static_assert-fest:
/// wer den Entscheid vollzieht, kommt an genau diesen Zeilen nicht vorbei.
[[nodiscard]] constexpr std::size_t flag_catalog_offene_entscheide() noexcept {
    std::size_t n = 0;
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog)
        if (e.entscheid_offen) ++n;
    return n;
}

/// Katalog und Reserve sind DISJUNKT. Ein Token kann nicht zugleich zugelassen und begruendet
/// abgelehnt sein -- und genau dieser Fall entstuende still, wenn jemand ein Reserve-Token in den
/// Katalog naehme, ohne es aus der Reserve zu nehmen.
[[nodiscard]] constexpr bool flag_catalog_and_reserve_disjoint() noexcept {
    for (FlagReserveEntry const& r : kFlagGrammarReserve) {
        if (r.token.empty() || r.beleg.empty()) return false; // eine Reserve ohne Beleg ist eine Behauptung
        if (flag_token_is_known(r.token)) return false;
    }
    for (std::size_t i = 0; i < kFlagGrammarReserve.size(); ++i)
        for (std::size_t j = i + 1; j < kFlagGrammarReserve.size(); ++j)
            if (kFlagGrammarReserve[i].token == kFlagGrammarReserve[j].token) return false;
    return true;
}

// -- DIE DRIFT-BRUECKE ZU simd_feature_flag.hpp ---------------------------------------------------
// Es gibt ab jetzt ZWEI Tabellen ueber dieselben Flags: die alte (cpuinfo + Compiler-Schalter, fuer das
// Signatur-Matching und das Bau-Gate) und diese (drei Namensraeume + Platzierung, fuer die Grammatik).
// Zwei Tabellen sind eine Drift-Naht -- deshalb steht hier eine WACHE und kein Kommentar:
//   * JEDER der 23 Eintraege von kSimdFeatureFlagCatalog hat hier ein Gegenstueck mit DERSELBEN
//     cpuinfo-Id, und die Compiler-Schalter stimmen ueberein. Wer dort ein Flag ergaenzt, ohne ihm hier
//     ein Token zu geben, bricht compile-time.
// Die UMGEKEHRTE Richtung ist bewusst NICHT gefordert: dieser Katalog ist echt groesser (62 gegen 23
// Eintraege), weil er x128, die x256-Wurzel, sechs Skalar-Luecken und die basislose Familie mitfuehrt.
// Ein Zwang zur Gleichmaechtigkeit hiesse, den alten Katalog aus einer Grammatik-Aenderung heraus zu
// erweitern -- das ist ein eigener Bauschritt mit eigenen Folgen (Signatur-Erhebung, Bau-Gate).
[[nodiscard]] constexpr bool grammar_catalog_covers_simd_feature_catalog() noexcept {
    for (SimdFeatureFlag const& f : kSimdFeatureFlagCatalog) {
        std::size_t const i = find_flag_catalog_entry_by_cpuinfo(f.cpuinfo);
        if (i == kNoFlagCatalogEntry) return false;
        if (kFlagGrammarCatalog[i].gpp != f.gpp) return false;
    }
    return true;
}

// == COMPILE-ZEIT-BATTERIE: der Beweis, nicht die Behauptung =======================================

static_assert(kFlagGrammarCatalog.size() == 62,
              "Die Zahl ist am echten Katalog gerechnet: 4 Ziel-Hardware + 2 Kern-Klassen + 4 "
              "Breiten-Basen + 10 x128 + 12 x256 + 14 x512 + 3 Companion + 9 Skalar + 4 basislos.");
static_assert(kFlagGrammarReserve.size() == 14);
static_assert(flag_catalog_entries_are_nonempty());
static_assert(flag_catalog_pairs_unique());
static_assert(flag_catalog_cpuinfo_unique());
static_assert(flag_catalog_parents_resolve());
static_assert(flag_catalog_alternative_only_for_open_decision());
static_assert(flag_catalog_and_reserve_disjoint());
static_assert(grammar_catalog_covers_simd_feature_catalog(),
              "Ein Flag in simd_feature_flag.hpp ohne Grammatik-Token (oder mit abweichendem "
              "Compiler-Schalter) waere genau die Drift, gegen die diese Bruecke gebaut ist.");

// Der offene Owner-Entscheid, mechanisch markiert: x64 + mmx/mmxext/3dnow/3dnowext + 3dnowprefetch.
static_assert(flag_catalog_offene_entscheide() == 6,
              "GENAU sechs Eintraege haengen am offenen Entscheid: die Basis 'x64', die vier Eintraege der "
              "MMX-/3DNow!-Familie und meine Einordnung von '3dnowprefetch' als Skalar. Wer einen siebten "
              "markiert oder einen wegnimmt, hat den Entscheid beruehrt und muss es begruenden.");

// -- DIE DREI NAMENSRAEUME, je EINZELN belegt -- keiner ist aus einem anderen ableitbar ------------
static_assert(find_flag_catalog_entry("sse3", "x128") != kNoFlagCatalogEntry);
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sse3", "x128")].cpuinfo == std::string_view{"pni"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sse3", "x128")].gpp == std::string_view{"-msse3"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sse41", "x128")].cpuinfo == std::string_view{"sse4_1"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sse41", "x128")].gpp == std::string_view{"-msse4.1"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sha", "x128")].cpuinfo == std::string_view{"sha_ni"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("sha", "x128")].gpp == std::string_view{"-msha"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("bmi1", "")].cpuinfo == std::string_view{"bmi1"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("bmi1", "")].gpp == std::string_view{"-mbmi"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("rdrand", "")].gpp == std::string_view{"-mrdrnd"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("pclmulqdq", "x128")].gpp == std::string_view{"-mpclmul"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("vbmi2", "x512")].cpuinfo ==
              std::string_view{"avx512_vbmi2"}); // MIT Unterstrich
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("vbmi", "x512")].cpuinfo ==
              std::string_view{"avx512vbmi"}); // OHNE Unterstrich
// Die Uebersetzung laeuft ueber die TABELLE, nie ueber eine String-Umformung.
static_assert(flag_token_for_cpuinfo("pni") == std::string_view{"sse3"});
static_assert(flag_token_for_cpuinfo("avx512_vp2intersect") == std::string_view{"vp2intersect"});
static_assert(flag_token_for_cpuinfo("sha_ni") == std::string_view{"sha"});
static_assert(flag_token_for_cpuinfo("gibtsnicht").empty());
static_assert(flag_token_for_cpuinfo("").empty()); // die leere Id ist KEIN Schluessel

// -- DIE TOKEN-KOLLISION, die den Elternteil ueberhaupt noetig macht -------------------------------
// 'vnni' und 'ifma' existieren je ZWEIMAL, mit verschiedenen CPUID-Bits und Compiler-Schaltern.
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("vnni", "x256")].cpuinfo == std::string_view{"avx_vnni"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("vnni", "x512")].cpuinfo == std::string_view{"avx512_vnni"});
static_assert(find_flag_catalog_entry("vnni", "x256") != find_flag_catalog_entry("vnni", "x512"));
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("ifma", "x256")].gpp == std::string_view{"-mavxifma"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("ifma", "x512")].gpp == std::string_view{"-mavx512ifma"});
// ... und KEINES der beiden steht auf Tiefe 0: ausserhalb der Klammer gaebe es keine Disambiguierung.
static_assert(!flag_token_is_admitted_under("vnni", ""));
static_assert(!flag_token_is_admitted_under("ifma", ""));
static_assert(!flag_token_is_admitted_under("vnni", "x128"));

// -- DIE VIER STRUKTURFAELLE, je an REALEN Token ---------------------------------------------------
// (1) Basis mit Sub-Liste: das Subset steht NUR unter SEINER Basis.
static_assert(flag_token_is_admitted_under("x512", ""));
static_assert(flag_token_is_admitted_under("vl", "x512"));
static_assert(!flag_token_is_admitted_under("vl", "x256"));
static_assert(!flag_token_is_admitted_under("vl", "x128"));
static_assert(!flag_token_is_admitted_under("vl", "")); // 'vl' allein ist keine Aussage
static_assert(flag_token_is_admitted_under("sse2", "x128"));
static_assert(!flag_token_is_admitted_under("sse2", "x512")); // DIE Bissprobe des Auftrags
static_assert(!flag_token_is_admitted_under("sse2", "x256"));
static_assert(flag_token_is_admitted_under("avx2", "x256"));
static_assert(!flag_token_is_admitted_under("avx2", "x128"));
// (2) Companion: Tiefe 0, und AUSDRUECKLICH NICHT in der Klammer -- es gibt kein x512-gfni-Bit.
static_assert(flag_token_is_admitted_under("gfni", ""));
static_assert(!flag_token_is_admitted_under("gfni", "x512"));
static_assert(!flag_token_is_admitted_under("gfni", "x256"));
static_assert(flag_token_is_admitted_under("vaes", "") && !flag_token_is_admitted_under("vaes", "x512"));
static_assert(flag_token_is_admitted_under("vpclmulqdq", ""));
// (3) Skalar: Tiefe 0, nie in einer Klammer.
static_assert(flag_token_is_admitted_under("popcnt", "") && !flag_token_is_admitted_under("popcnt", "x512"));
static_assert(flag_token_is_admitted_under("bmi1", "") && flag_token_is_admitted_under("bmi2", ""));
static_assert(flag_token_is_admitted_under("abm", "") && flag_token_is_admitted_under("movbe", ""));
static_assert(flag_token_is_admitted_under("adx", "") && flag_token_is_admitted_under("rdrand", ""));
static_assert(flag_token_is_admitted_under("rdseed", ""));
// (4) Basislose Familie: BEIDE Gestalten tragen -- der Owner-Entscheid ist NICHT vorweggenommen.
static_assert(flag_token_is_admitted_under("mmx", ""));    // Gestalt (a): blosses Token
static_assert(flag_token_is_admitted_under("mmx", "x64")); // Gestalt (b): eigene Basis
static_assert(flag_token_is_admitted_under("mmxext", "") && flag_token_is_admitted_under("mmxext", "x64"));
static_assert(flag_token_is_admitted_under("3dnow", "") && flag_token_is_admitted_under("3dnow", "x64"));
static_assert(flag_token_is_admitted_under("3dnowext", "") && flag_token_is_admitted_under("3dnowext", "x64"));
static_assert(flag_token_is_admitted_under("x64", ""));
// ... aber sie stehen NICHT unter den drei ECHTEN Breiten-Basen. Genau das ist die Hardware-Aussage
// hinter Fall (4): MM-Register sind x87-aliasiert, sie sind keine 128/256/512-bit-Register.
static_assert(!flag_token_is_admitted_under("mmx", "x128"));
static_assert(!flag_token_is_admitted_under("mmx", "x256"));
static_assert(!flag_token_is_admitted_under("mmx", "x512"));
static_assert(!flag_token_is_admitted_under("3dnow", "x512"));
// 3dnowprefetch ist als SKALAR eingeordnet (meine Setzung, s. Katalog-Zeile) -- also NUR Tiefe 0.
static_assert(flag_token_is_admitted_under("3dnowprefetch", ""));
static_assert(!flag_token_is_admitted_under("3dnowprefetch", "x64"));

// -- DIE ZIEL-HARDWARE UND DIE KERN-KLASSEN --------------------------------------------------------
static_assert(flag_token_is_admitted_under("c", ""));
static_assert(flag_token_is_admitted_under("g", "") && flag_token_is_admitted_under("f", ""));
static_assert(flag_token_is_admitted_under("n", ""));
static_assert(flag_token_is_admitted_under("p", "c")); // Owner-R8: 'p' und 'e' NUR unter 'c'
static_assert(flag_token_is_admitted_under("e", "c"));
static_assert(!flag_token_is_admitted_under("p", "")); // 'p' allein ist eine Aussage ueber nichts
static_assert(!flag_token_is_admitted_under("e", ""));
static_assert(!flag_token_is_admitted_under("p", "g")); // und nicht unter einer anderen Ziel-Hardware
static_assert(!flag_token_is_admitted_under("p", "x512"));
static_assert(!flag_token_is_admitted_under("c", "c")); // 'c' ist nie sein eigener Sub

// -- UNBEKANNTE UND RESERVE-TOKEN ------------------------------------------------------------------
static_assert(!flag_token_is_admitted_under("quatsch", ""));
static_assert(!flag_token_is_admitted_under("quatsch", "x512")); // DIE Bissprobe des Auftrags
static_assert(!flag_token_is_known("quatsch"));
// Die Reserve wird abgelehnt -- aber MIT Grund, und der Grund ist abfragbar. Das ist der Unterschied
// zwischen "nie vorgesehen" und "bewusst draussen".
static_assert(!flag_token_is_admitted_under("xop", "x128") && !flag_token_is_admitted_under("xop", "x256"));
static_assert(!flag_token_is_admitted_under("er", "x512") && !flag_token_is_admitted_under("pf", "x512"));
static_assert(!flag_token_is_admitted_under("fma4", "x256"));
static_assert(!flag_token_is_admitted_under("tzcnt", "") && !flag_token_is_admitted_under("lzcnt", ""));
static_assert(!flag_token_is_admitted_under("amx", "") && !flag_token_is_admitted_under("apx", ""));
static_assert(flag_token_is_reserve("xop") && !flag_reserve_beleg("xop").empty());
static_assert(flag_token_is_reserve("er") && flag_token_is_reserve("bmm"));
static_assert(!flag_token_is_reserve("gfni")); // ein zugelassenes Token ist nie Reserve
static_assert(flag_reserve_beleg("gibtsnicht").empty());
// 'abm' ist zugelassen, 'lzcnt' ist Reserve -- dasselbe CPUID-Bit, EIN Token. Ohne diese Trennung
// entstuende ein Eintrag, den kein Signatur-Matching je trifft.
static_assert(flag_token_is_admitted_under("abm", "") && flag_token_is_reserve("lzcnt"));
// 'bmi1' ist zugelassen, 'tzcnt' ist Reserve -- TZCNT hat kein eigenes CPUID-Bit, es IST bmi1.
static_assert(flag_token_is_admitted_under("bmi1", "") && flag_token_is_reserve("tzcnt"));

// -- DIE ETIKETTEN (Single-Source fuer Diagnose) ---------------------------------------------------
static_assert(flag_token_kind_label(FlagTokenKind::BreitenSubset) == std::string_view{"breiten_subset"});
static_assert(flag_token_kind_label(FlagTokenKind::Companion) == std::string_view{"companion"});
static_assert(flag_token_kind_label(FlagTokenKind::BasislosFamilie) == std::string_view{"basislos_familie"});
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("gfni", "")].kind == FlagTokenKind::Companion);
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("popcnt", "")].kind == FlagTokenKind::Skalar);
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("mmx", "")].kind == FlagTokenKind::BasislosFamilie);
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("f", "x512")].kind == FlagTokenKind::BreitenSubset);
static_assert(kFlagGrammarCatalog[find_flag_catalog_entry("f", "")].kind == FlagTokenKind::HardwareBasis);
// ... und die letzten beiden Zeilen sind zusammen der schaerfste Einzelbeweis dieser Datei: 'f' ist
// UNTER x512 das AVX-512-Foundation-Subset und auf Tiefe 0 die Ziel-Hardware FPGA. DASSELBE Token,
// ZWEI Bedeutungen, entschieden allein durch das Elternteil.

} // namespace comdare::cache_engine::measurement
