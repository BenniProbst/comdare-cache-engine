// tests/unit/test_w10_c4_zellwert_naht.cpp -- W10-C4 (Bauplan-Dossier 20260803, Sektion 2): das ORAKEL der
// Naht-SCHARFSCHALTUNG und des Manager-Entscheids W10-M2.
//
// W10-C1 hat die Zellwert-Grammatik gebaut, C2/C3 haben die beiden Rechen-Stellen (consteval-Makro und
// Laufzeit-Zwilling) darauf gelegt -- alles GOLDEN-NEUTRAL, weil ohne Define nichts passiert. C4 ist der
// Commit, an dem das Define ENTSTEHT. Diese TU bewacht genau die vier Aussagen, die dabei neu sind und die
// man sonst nur im Bau-Log "sieht":
//
//   (A) WERTFORM + DEFINE-ARGUMENT je Route (no_extension/avx2/avx512) -- LITERAL. Das ist der Test-seitige
//       Zwilling des rsp-Zeilen-Diffs (Praezedenz der C-3a-Auflage: eine Identitaets-Aenderung muss sichtbar
//       sein, nicht bloss behauptet).
//   (B) DISKRIMINIERUNG -- die vervollstaendigte System-Zeile und ihr SHA-512 unterscheiden sich zwischen zwei
//       OS-FAMILIEN und zwischen zwei ISA-Zellen, und sind fuer dieselbe Zelle gleich. Das ist der
//       W10-Abnahme-Beweis: die Kollision "linux-Bau == macos-Bau" ist mechanisch tot.
//   (C) FAIL-CLOSED -- eine Zelle ohne Achsen-Glied (riscv64) faellt auf den `na`-Sentinel, und `na` ist am
//       Praedikat erkennbar. Der Rueckschrieb-Stopp haengt an DIESEM Praedikat (profile_run_entry).
//   (D) W10-M2 -- das +ceb=-Glied steht in der Perm-build_version (vorher gar nicht) UND der Objekt-Store-Key
//       traegt es GENAU EINMAL (Dedupe statt Doppel-Provenienz).
//       PIN-BEWEGUNG E-24 C8 (deklariert, nicht still): dieser Block fuehrte den Contract-Minor an DREI Stellen
//       als Literal ".2". Das war ein ZWEITER literaler Wert-Pin neben dem einen designierten
//       (test_v41_anatomy_module_abi.cpp, R5D_CebContract) -- der Decl-Header sagt ausdruecklich "der EINE
//       literale Pin dort ist die Gegenprobe". Mit dem Major-Bump 7->8 und dem Minor-RESET 2->0 waeren die drei
//       Stellen rot geworden, ohne dass diese TU je etwas ueber den ZAHLENWERT beweisen wollte: ihr Gegenstand
//       ist die C4-VERDRAHTUNG (Glied vorhanden, in bindender Ordnung, im Store-Key genau einmal). Sie ziehen
//       den Wert deshalb ab jetzt aus der Konstante; der Wert-Pin bleibt an seiner einen designierten Stelle.
//
// Alles laeuft gegen die REALEN Funktionen der Produktions-Naht, nie gegen eine Nachbildung.
//
// KEIN Compiler-Subprozess: dass die Backslash-Anfuehrungszeichen dieses Arguments die gcc-Response-Datei
// unbeschadet passieren, ist eine Eigenschaft der TOOLCHAIN und wird am realen Probe-Bau belegt (Commit-Text),
// nicht in einer Unit-TU nachgestellt -- eine TU, die g++ aufruft, prueft den Compiler, nicht diesen Code.

#include <builder/artifact_transport/artifact_cache.hpp>   // W10-M2: cache_key_prefix-Dedupe (die Store-Key-Naht)
#include <cache_engine/abi/anatomy_fingerprint.hpp>        // Preimage-Ordnung + System-Glied-Index
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // E-24 C8: Contract-Minor + Historien-Freezes DIREKT
#include <cache_engine/abi/anatomy_version_stamp.hpp>      // system_stamp_line(): das Ist der System-Zeile
#include <cache_engine/abi/system_cell_values.hpp>         // C1: Vervollstaendiger + na-Praedikat
#include <system_axes/simd_sub_axis.hpp>                   // die simd-Ids der drei Routen (nie als Literal)
#include <profile_facade/system_cell_values_naht.hpp>      // C4: die Wertform-/Define-Naht selbst
#include <profile_facade/system_version_suffix.hpp>        // Segment-Ordnung + ceb_contract_version_text
#include <sha512/ctsha512.hpp>                             // der Digest-Beweis (dieselbe Primitive wie die Naht)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {

namespace pf  = ::comdare::cache_engine::profile_facade;
namespace cea = ::comdare::cache_engine::abi;
namespace cm  = ::comdare::cache_engine::measurement;
namespace at  = ::comdare::cache_engine::builder::artifact_transport;

int  g_fail = 0;
void check(char const* what, bool ok, std::string const& detail = {}) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) {
        if (!detail.empty()) std::cout << "        " << detail << "\n";
        ++g_fail;
    }
}
void check_eq(char const* what, std::string const& got, std::string const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n        got  = '" << got << "'\n";
    if (!ok) {
        std::cout << "        want = '" << want << "'\n";
        ++g_fail;
    }
}

[[nodiscard]] std::size_t count_occurrences(std::string const& hay, std::string_view needle) {
    std::size_t n = 0;
    for (std::size_t pos = hay.find(needle); pos != std::string::npos; pos = hay.find(needle, pos + needle.size())) ++n;
    return n;
}

/// Der 128-hex-Fingerprint ueber die Glied-Folge -- EXAKT der Weg des Laufzeit-Zwillings
/// (lazy_adhoc_fingerprint_for): dieselbe Glied-Quelle, derselbe Separator, dieselbe Primitive. Nachgebaut
/// waere er wertlos; hier ruft der Test die realen Bausteine.
[[nodiscard]] std::string fingerprint_of_cell(std::string const& zellwerte) {
    std::string const system =
        cea::complete_system_stamp_line(cea::system_stamp_line(), cea::SystemCellValues{zellwerte});
    auto const        glieder = cea::anatomy_fingerprint_glieder("ORGAN-FIX", system, "MESS-FIX");
    std::string const preimage =
        cea::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const hex = ::comdare::cache_engine::sha512::to_hex(digest);
    return std::string(hex.data(), hex.size());
}

} // namespace

int main() {
    std::string const isa = "x86_64";
    std::string const os  = "linux";

    std::cout << "== (A) Wertform + Define-Argument je Route (LITERAL) ==\n";
    {
        // Die drei Routen kommen aus der Achsen-Single-Source, nicht als Test-Literale -- sonst pruefte der
        // Test seine eigenen Strings statt der Achse.
        check_eq("Route no_extension: Wertform",
                 pf::compose_system_cell_values(isa, os, cm::SimdNoExtOption::simd_id()),
                 "target_isa=x86_64;operating_system=linux;simd=no_extension");
        check_eq("Route avx2: Wertform", pf::compose_system_cell_values(isa, os, cm::SimdAvx2Option::simd_id()),
                 "target_isa=x86_64;operating_system=linux;simd=avx2");
        check_eq("Route avx512: Wertform", pf::compose_system_cell_values(isa, os, cm::SimdAvx512Option::simd_id()),
                 "target_isa=x86_64;operating_system=linux;simd=avx512");
        // Das Argument, das real in der rsp-Zeile landet. Die beiden Backslash-Anfuehrungszeichen sind der Teil,
        // der das Makro zu einem STRING-LITERAL macht; ohne sie expandierte es zu nacktem Text und die
        // Tier-Uebersetzung braeche.
        check_eq(
            "Route avx512: Define-Argument",
            pf::system_cell_values_define_arg(pf::compose_system_cell_values(isa, os, cm::SimdAvx512Option::simd_id())),
            "-DCOMDARE_SYSTEM_CELL_VALUES=\\\"target_isa=x86_64;operating_system=linux;simd=avx512\\\"");
        // Die Wertform muss die C1-Diagnose bestehen -- sonst braeche sie den Tier-Bau an der Define-Naht.
        check("Wertform besteht die C1-Diagnose (ok)", cea::diagnose_system_cell_values(pf::compose_system_cell_values(
                                                           isa, os, "avx2")) == cea::SystemCellValuesDiagnose::ok);
    }
    {
        // IDENTITAET: ein Host, der die Facade-Zellen nicht verdrahtet, baut byte-identisch weiter.
        check_eq("Unbelegte ISA-Zelle => leere Wertform", pf::compose_system_cell_values("", os, "avx2"), "");
        check_eq("Unbelegte OS-Zelle => leere Wertform", pf::compose_system_cell_values(isa, "", "avx2"), "");
        check_eq("Leere Wertform => KEIN Define-Argument", pf::system_cell_values_define_arg(""), "");
        check("Leere Wertform laesst die System-Zeile byte-identisch",
              cea::complete_system_stamp_line(cea::system_stamp_line(), cea::SystemCellValues{}) ==
                  cea::system_stamp_line());
    }

    std::cout << "== (B) Diskriminierung: die B1-Kollision ist mechanisch tot ==\n";
    {
        std::string const linux_avx512  = pf::compose_system_cell_values(isa, "linux", "avx512");
        std::string const macos_avx512  = pf::compose_system_cell_values(isa, "macos", "avx512");
        std::string const arm_linux_512 = pf::compose_system_cell_values("aarch64", "linux", "avx512");
        std::string const linux_avx2    = pf::compose_system_cell_values(isa, "linux", "avx2");

        std::cout << "  [INFO] vervollstaendigte System-Zeile (linux/x86_64/avx512) =\n         '"
                  << cea::complete_system_stamp_line(cea::system_stamp_line(), cea::SystemCellValues{linux_avx512})
                  << "'\n";
        check("dieselbe Zelle => derselbe Fingerprint",
              fingerprint_of_cell(linux_avx512) == fingerprint_of_cell(linux_avx512));
        check("linux vs macos => VERSCHIEDENER Fingerprint (Owner-E3/B1)",
              fingerprint_of_cell(linux_avx512) != fingerprint_of_cell(macos_avx512));
        check("x86_64 vs aarch64 => VERSCHIEDENER Fingerprint",
              fingerprint_of_cell(linux_avx512) != fingerprint_of_cell(arm_linux_512));
        check("avx512 vs avx2 => VERSCHIEDENER Fingerprint",
              fingerprint_of_cell(linux_avx512) != fingerprint_of_cell(linux_avx2));
        check("ohne Zellwerte => VERSCHIEDEN von jedem definierten Bau",
              fingerprint_of_cell("") != fingerprint_of_cell(linux_avx512));
        // STEMPEL-KERN: der Vervollstaendiger fuegt IN Entries ein. Die Eintrags-ZAHL bleibt gleich, der
        // Meta-Meta-Anhang bleibt am Realm-Zeilen-ENDE, es entsteht kein zusaetzliches Segment.
        std::string const ist  = cea::system_stamp_line();
        std::string const voll = cea::complete_system_stamp_line(ist, cea::SystemCellValues{linux_avx512});
        // Die Eintrags-ZAHL selbst prueft der consteval-Vervollstaendiger bei JEDER Makro-Expansion nach
        // (count_stamp_entries ist consteval und damit hier nicht aufrufbar; die CT-Probe steht in der C1-TU).
        // Was diese Zeilen am LAUFZEIT-Ist beitragen, ist die Trenner-Invarianz: der Renderer reicht ';', '['
        // und ']' unveraendert durch, also kann er weder ein Segment noch eine merge-Zeile erfunden haben.
        auto const zaehle = [](std::string const& s, char c) {
            std::size_t n = 0;
            for (char const x : s)
                if (x == c) ++n;
            return n;
        };
        check("Segment-Trenner unveraendert (kein zusaetzliches Segment, keine merge-Zeile)",
              zaehle(voll, ';') == zaehle(ist, ';') && zaehle(voll, '[') == zaehle(ist, '[') &&
                  zaehle(voll, ']') == zaehle(ist, ']'));
        check("Meta-Meta-Anhang bleibt am Zeilen-ENDE", !voll.empty() && voll.back() == ']' && ist.back() == ']');
        check("die vervollstaendigte Zeile ist laenger (der Zellwert ist wirklich drin)", voll.size() > ist.size());
    }

    std::cout << "== (C) FAIL-CLOSED: na-Sentinel statt stiller Null ==\n";
    {
        // riscv64 hat heute kein Achsen-Glied (kAllTargetIsaIds) -- die Aufloesung sagt das ehrlich.
        check_eq("unbekannte Ziel-ISA => na", std::string{pf::resolve_system_cell_target_isa("riscv64")},
                 std::string{cea::kSystemCellValueNa});
        check_eq("bekannte Ziel-ISA => genau dieser Wert", std::string{pf::resolve_system_cell_target_isa("aarch64")},
                 "aarch64");
        check_eq("keine Deklaration => die CT-Zelle der Bau-Plattform",
                 std::string{pf::resolve_system_cell_target_isa("")}, std::string{pf::kSystemCellBuildIsa});
        std::string const riscv =
            pf::compose_system_cell_values(pf::resolve_system_cell_target_isa("riscv64"), os, "avx2");
        check_eq("na reist als Token in der Wertform", riscv, "target_isa=na;operating_system=linux;simd=avx2");
        check("na ist am Praedikat erkennbar (der Rueckschrieb-Stopp haengt daran)",
              cea::system_cell_values_contain_na(riscv));
        check("eine vollstaendig bestimmte Zelle traegt KEIN na",
              !cea::system_cell_values_contain_na(pf::compose_system_cell_values(isa, os, "avx2")));
        // Ein ungrammatisches Token wird zum na normalisiert, statt eine Wertform zu erzeugen, die jeden
        // Tier-Bau compile-hart braeche: GENAU EIN Fehl-Pfad.
        check("ungrammatisches simd-Token => na (ein Fehl-Pfad, kein Compiler-Rauschen)",
              cea::system_cell_values_contain_na(pf::compose_system_cell_values(isa, os, "AVX-512")));
        // Und die na-Wertform bleibt trotzdem grammatik-konform -- sonst braeche der na-Bau statt zu stempeln.
        check("die na-Wertform ist grammatik-konform",
              cea::diagnose_system_cell_values(riscv) == cea::SystemCellValuesDiagnose::ok);
    }

    std::cout << "== (D) W10-M2: Contract-Minor, Perm-Glied, Store-Key-Dedupe ==\n";
    {
        check_eq("ceb_contract_version_text setzt sich aus ABI-Major und Contract-Minor zusammen",
                 pf::ceb_contract_version_text(),
                 std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." + std::to_string(cea::kCebContractCodegenMinor));
        // Die Perm-build_version, wie beide Perm-Schleifen sie ab C4 bauen.
        pf::SystemVersionSuffixParts perm;
        perm.cxx                      = "g++-16";
        perm.opt                      = "O3";
        perm.simd                     = "avx512";
        std::string const ceb         = pf::ceb_contract_version_text();
        perm.ceb                      = ceb;
        std::string const perm_suffix = pf::compose_system_version_suffix(perm);
        std::cout << "  [INFO] Perm-Suffix = '" << perm_suffix << "'\n";
        check("die Perm-build_version traegt das +ceb=-Glied (vor W10 gar nicht)",
              count_occurrences(perm_suffix, "+ceb=") == 1u);
        check("das Glied steht in der bindenden Ordnung (nach +ext=, vor +bt=/+gate=)",
              perm_suffix.find("+ext=") < perm_suffix.find("+ceb="));

        at::ArtifactCache const cache = at::ArtifactCache::from_env(); // inert; der Key-Praefix ist davon unabhaengig
        std::string const       key_perm = cache.cache_key_prefix("m3v2" + perm_suffix);
        std::cout << "  [INFO] Store-Key-Praefix (Perm-Pfad) = '" << key_perm << "'\n";
        check("DEDUPE: der Store-Key traegt +ceb= GENAU EINMAL", count_occurrences(key_perm, "+ceb=") == 1u);
        check("der Store-Key haengt +mtool=/+mrg= weiterhin an",
              key_perm.find("+mtool=") != std::string::npos && key_perm.find("+mrg=none") != std::string::npos);
        check("der +ceb=-Wert im Key ist der der build_version (nicht ein zweiter, angehaengter)",
              key_perm.find("+ceb=" + ceb) != std::string::npos);

        // GEGENPROBE: eine build_version OHNE das Glied bekommt es unveraendert eingefaltet -- der
        // Bestands-Aufrufer (Einzel-Pfad/Fremd-Version) bleibt byte-identisch zum Vor-C4-Verhalten.
        std::string const key_ohne = cache.cache_key_prefix("m3v2+cxx=g++-16+opt=O3");
        check("ohne vorhandenes Glied wird es EINGEFALTET (Bestand byte-identisch)",
              count_occurrences(key_ohne, "+ceb=") == 1u && key_ohne.find("+ceb=" + ceb) != std::string::npos);
        // NEUER BUCKET, zwei unabhaengige Gruende -- beide werden einzeln belegt statt pauschal behauptet:
        //   (1) der WERT wanderte, also zeigt jeder Key auf einen anderen Bucket als vor dem jeweiligen Shift.
        //       Geprueft wird gegen die HISTORIEN-FREEZES (E-24 C8): keiner der drei Vorgaenger-Buckets der
        //       C'-Kette -- 7.0 (vor A13-M4), 7.1 (A13-M4), 7.2 (W10-M2) -- darf im lebenden Key stehen.
        check("NEUER BUCKET (Wert): der Key traegt den lebenden Contract-Wert und KEINEN der Vorgaenger",
              key_perm.find("+ceb=" + ceb) != std::string::npos &&
                  key_perm.find("+ceb=" + std::to_string(cea::kHostAnatomyAbiVersionAbi7.major) + ".0") ==
                      std::string::npos &&
                  key_perm.find("+ceb=" + std::to_string(cea::kHostAnatomyAbiVersionAbi7.major) + ".1") ==
                      std::string::npos &&
                  key_perm.find("+ceb=" + std::to_string(cea::kHostAnatomyAbiVersionAbi7.major) + "." +
                                std::to_string(cea::kCebContractCodegenMinorAbi7)) == std::string::npos);
        //   (2) die POSITION wanderte: das Glied steht jetzt in der bindenden Suffix-Ordnung (vor +bt=/+gate=)
        //       statt hinten angehaengt. Sichtbar wird das erst, sobald ein spaeteres Segment belegt ist --
        //       ohne +bt= faellt die angehaengte Form zufaellig mit der eingeordneten zusammen.
        pf::SystemVersionSuffixParts perm_dbg = perm;
        perm_dbg.build_type                   = "Debug";
        std::string const key_neu = cache.cache_key_prefix("m3v2" + pf::compose_system_version_suffix(perm_dbg));
        pf::SystemVersionSuffixParts perm_alt = perm_dbg;
        perm_alt.ceb                          = {}; // die Vor-C4-Perm-build_version: KEIN +ceb=-Glied
        std::string const key_alt = cache.cache_key_prefix("m3v2" + pf::compose_system_version_suffix(perm_alt));
        std::cout << "  [INFO] Debug-Zelle nachher = '" << key_neu << "'\n";
        std::cout << "  [INFO] Debug-Zelle vorher  = '" << key_alt << "'\n";
        check("NEUER BUCKET (Position): eingeordnetes Glied != angehaengtes Glied", key_neu != key_alt);
        check("beide Formen tragen +ceb= dennoch genau einmal",
              count_occurrences(key_neu, "+ceb=") == 1u && count_occurrences(key_alt, "+ceb=") == 1u);

        // Die Store-Key-Naht darf den Segment-Schluessel nicht aus profile_facade inkludieren (Layer-Richtung).
        // Dass beide Schreibweisen dennoch identisch sind, ist deshalb HIER verwacht -- diese TU darf beide Seiten
        // sehen. Ohne diese Zeile stuende die Gleichheit nur als Zusage im Kommentar.
        check_eq("artifact_transport::kCebSegmentKey == kSuffixSegmentOrder[3]", std::string{at::kCebSegmentKey},
                 std::string{pf::kSuffixSegmentOrder[3]});
    }

    std::cout << (g_fail == 0 ? "ALLE W10-C4-ORAKEL GRUEN\n" : "W10-C4-ORAKEL ROT\n");
    return g_fail == 0 ? 0 : 1;
}
