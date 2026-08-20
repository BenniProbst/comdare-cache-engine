// profile_facade/planner/planner_version.hpp -- statischer Selbst-Versions-Stempel des Experiment-Planers
// (Bau W12-A, Section43.b).
//
// Section43.b (User-Direktive): "Der Experiment-Planer traegt auch einen string_view-Stempel, aber nur ueber die
// eigene statische Versionierung und unter welcher ISA/OS er passt und ausgefuehrt werden kann." Der Planer
// permutiert (er ist keine Permutation) -> KEINE Achsen-Arrays, nur die Selbst-Version X.Y.Z + eine
// Ausfuehrbarkeits-Deklaration (ISA/OS). Die ISA stammt aus der Section40.a-Signatur-Domaene (hardware_isa).
//
// Ausgegeben im --dump-plan-Header (planner_version_stamp()). Rein additiv, golden-/binary_id-neutral.
// X.Y.Z voll ausgeschrieben (neue Stempel-Welt, NICHT die .algos-Sig).
//
// CX-W5 (Codex-Doppelreview 02.08.2026): der Planer-Selbst-Stempel ist ein ce-EIGENER Versionspfad und faellt
// damit unter dieselbe Owner-Grammatik wie jede Achsen-Algorithmus-Version. Zwei Owner-Direktiven greifen HIER:
//   * Owner-Q10 ("v gilt NUR fuer die Roh-Literale im Code"): das ROH-Literal kPlannerVersion traegt jetzt das
//     'v' ("1.0.0.c"). Die GERENDERTE Form (planner_version_stamp) bleibt praefixfrei "planner@1.0.0.c" -- der
//     Renderer algo_semver_string schneidet das 'v' weg (wie "v1" -> "1.0.0"). Der CX-W5-Schritt selbst war
//     byte-identisch zum vorigen Stand; das 'c' hat A13-M3/C4 nachgezogen -- das ist ein DEKLARIERTES
//     Byte-Ereignis der --dump-plan-Zeile ("planner@1.0.0" -> "planner@1.0.0.c"), kein Q10-Verstoss.
//   * Owner-Q3 (Flag-Grammatik "alle Versionen enden auf 'c'/'ce'"): die ce-Politik-Wachen aus algo_semver.hpp
//     (ce_owned_version_is_wellformed ungated + ce_owned_version_satisfies_cpu_enforce gated) sind unten genau
//     wie an jeder Registry-Variante angelegt; der Planer ist damit Teil der A13-M2/M3-Migrations-Naht
//     (algo_semver.hpp, Klasse (e)) statt sie zu umgehen.
//
// V-08R (KON101, 19.08.2026): der Planer traegt seinen EIGENEN Fingerprint -- SHA-256/64-hex ueber die
// '\n'-getrennte Identitaets-Preimage {kPlannerVersion, isa, os} (CT-SHA-256, sha256/ctsha.hpp; das
// je-Traeger-Vertragsbein Planer=64 liefert stempel_basis.hpp). Die --dump-plan-Zeile schliesst seither
// mit " sha256=<64hex>" -- ein DEKLARIERTES Byte-Ereignis dieser Zeile (Praezedenz: das 'c'-Ereignis
// oben), kein Q10-Verstoss; die golden-/binary_id-Neutralitaet des Stempels bleibt unberuehrt.

#pragma once

#include <cache_engine/abi/stempel_basis.hpp>                    // S-1: StempelBasis/StempelVertrag (Erbin PLANER)
#include <cache_engine/measurement/algo_semver.hpp>              // CX-W5: ce-Politik-Wachen + render-neutraler Semver
#include <cache_engine/measurement/hardware_isa_system_axis.hpp> // Section40.a: Amd64HostIsaAxis::host_isa()
#include <sha256/ctsha.hpp>                                      // V-08R: CT-SHA-256 (sha256 + to_hex, consteval)

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::planner {

/// Selbst-Version des Experiment-Planers, X.Y.Z (initial 1.0.0; X.Y = Feature, Z = Debug-Revision). CX-W5: das
/// ROH-Literal traegt das 'v' (Owner-Q10); A13-M3/C4 hat es auf "1.0.0.c" gezogen (Owner-Q3-Flag-Grammatik).
inline constexpr std::string_view kPlannerVersion = "1.0.0.c";

/// Ziel-ISA, unter der der Planer ausfuehrbar ist (aus der Section40.a-Signatur-Domaene; heute x86_64).
[[nodiscard]] constexpr std::string_view planner_target_isa() noexcept {
    return ::comdare::cache_engine::measurement::Amd64HostIsaAxis::host_isa();
}

/// Ziel-OS (compile-time, kein Runtime-Sniff).
[[nodiscard]] constexpr std::string_view planner_target_os() noexcept {
#if defined(_WIN32)
    return "windows";
#elif defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "macos";
#else
    return "unknown";
#endif
}

// -- V-08R (KON101): DER PLANER-FINGERPRINT -- SHA-256/64-hex, consteval ------------------------------

namespace planer_fingerprint_detail {

/// Die Preimage-Glieder in GENANNTER Reihenfolge: die drei Identitaets-Glieder, die dieser Header schon
/// traegt (Versions-/Ausfuehrbarkeits-Deklaration Section43.b). KEIN Selbstbezug: der sha selbst haengt
/// nie im Preimage.
[[nodiscard]] constexpr std::array<std::string_view, 3> preimage_glieder() noexcept {
    return {kPlannerVersion, planner_target_isa(), planner_target_os()};
}

/// '\n'-Separator nach dem Feldgrenzen-Muster anatomy_fingerprint_hex (anatomy_fingerprint.hpp:883-886):
/// leere Glieder liessen die Feldgrenze stehen, verschobene Glieder aendern den Digest.
inline constexpr char kPreimageSeparator = '\n';

/// Kein Glied darf den Separator tragen -- sonst wuerde das Preimage mehrdeutig (NB/CX-1-Doktrin).
static_assert(
    [] {
        for (std::string_view const g : preimage_glieder())
            if (g.find(kPreimageSeparator) != std::string_view::npos) return false;
        return true;
    }(),
    "V-08R: ein Preimage-Glied traegt den '\\n'-Separator -- das Preimage waere mehrdeutig");

/// consteval-Ableitung: Preimage-Bytes -> sha256 -> lowercase-hex, nullterminiert (Speicher-Muster
/// kCebFingerprintArrayFor, ceb_version_stamp.hpp:592-599 -- dort SHA-512/129, hier SHA-256/65).
[[nodiscard]] consteval std::array<char, 65> fingerprint_hex_65() {
    constexpr std::size_t kLaenge = [] {
        std::size_t n = preimage_glieder().size() - 1; // die Separatoren
        for (std::string_view const g : preimage_glieder()) n += g.size();
        return n;
    }();
    std::array<std::uint8_t, kLaenge> bytes{};
    std::size_t                       p = 0;
    for (std::string_view const g : preimage_glieder()) {
        if (p != 0) bytes[p++] = static_cast<std::uint8_t>(kPreimageSeparator);
        for (char const c : g) bytes[p++] = static_cast<std::uint8_t>(c);
    }
    auto const digest = ::comdare::cache_engine::sha256::sha256(std::span<const std::uint8_t>{bytes.data(), p});
    auto const hex    = ::comdare::cache_engine::sha256::to_hex(digest); // array<char, 64>, lowercase
    std::array<char, 65> out{};
    for (std::size_t i = 0; i < 64; ++i) out[i] = hex[i];
    out[64] = '\0';
    return out;
}

} // namespace planer_fingerprint_detail

/// Der Planer-Fingerprint: nullterminiertes Traeger-Array + die {data,64}-Sicht darauf.
inline constexpr std::array<char, 65> kPlanerFingerprintArray = planer_fingerprint_detail::fingerprint_hex_65();
inline constexpr std::string_view     kPlanerFingerprint{kPlanerFingerprintArray.data(), 64};

// -- S-1 (P4): DIE PLANER-ERBIN DES STEMPEL-VERTRAGS ---------------------------------------------------

/// Die Teile des gesamt_stempel-Kompositums in ERBIN-genannter Reihenfolge (S-6-Sperre: die Basis kennt
/// keine Ordnung). Teile 1-6 BYTE-IDENTISCH zur bisherigen planner_version_stamp()-Konkatenation: dort
/// lief kPlannerVersion durch algo_semver_string, und Render==Identitaet ist fuer dieses Literal durch die
/// Roundtrip-Wache unten (render_algo_semver(parse_algo_semver(kPlannerVersion)).view() == kPlannerVersion)
/// BEWIESEN -- das Kompositum traegt deshalb das Roh-Literal selbst. V-08R haengt die Schluss-Glieder
/// " sha256=" + kPlanerFingerprint an (6 -> 8 Teile): das DEKLARIERTE Byte-Ereignis der --dump-plan-Zeile
/// (s. Kopf) -- es erfuellt zugleich die Teilstring-Pflicht des Vertrags (stempel_basis.hpp:503-505).
[[nodiscard]] constexpr std::array<std::string_view, 8> planer_gesamt_stempel_teile() noexcept {
    return {"planner@", kPlannerVersion,     " isa=",    planner_target_isa(),
            " os=",     planner_target_os(), " sha256=", kPlanerFingerprint};
}
inline constexpr auto kPlanerGesamtStempelKompositum =
    ::comdare::cache_engine::abi::stempel_kompositum<&planer_gesamt_stempel_teile>();

/// PlanerStempel -- die Erbin des CRTP-Stempel-Vertrags fuer den Traeger PLANER (S-1). Sie DELEGIERT auf
/// die Werte, die diese Datei schon traegt (S-1 aendert KEINEN Stempel-WERT, kein X.Y.Z-Bump).
struct PlanerStempel
    : ::comdare::cache_engine::abi::StempelBasis<PlanerStempel, ::comdare::cache_engine::abi::StempelTraeger::Planer> {
    /// version_xyz (KON7-04: NUR der Planer): das EINE Roh-Literal.
    [[nodiscard]] static constexpr std::string_view version_xyz() noexcept { return kPlannerVersion; }

    /// fingerprint_sha (V-08R, KON101): die KON6-03-IST-Luecke ist GEFUELLT -- SHA-256/64-hex ueber die
    /// '\n'-getrennte Identitaets-Preimage {kPlannerVersion, isa, os} (CT-SHA-256; das 64er-PLANER-Bein
    /// des S-1-Vertrags traegt die Form, stempel_basis.hpp fingerprint_sha_hex_laenge).
    [[nodiscard]] static constexpr std::string_view fingerprint_sha() noexcept { return kPlanerFingerprint; }

    /// gesamt_stempel: das consteval-Kompositum "planner@<X.Y.Z[.flag]*> isa=<isa> os=<os> sha256=<64hex>".
    [[nodiscard]] static constexpr std::string_view gesamt_stempel() noexcept {
        return kPlanerGesamtStempelKompositum.view();
    }
};
/// DER ZWANG (S-1/P1.2): die Erbin schliesst direkt unter ihrer Definition mit dem Vertrag. Die
/// bestehenden Wachen unten bleiben VOLLSTAENDIG stehen -- die Erbin verschaerft, sie ersetzt nichts.
static_assert(
    ::comdare::cache_engine::abi::StempelVertrag<PlanerStempel, ::comdare::cache_engine::abi::StempelTraeger::Planer>,
    "S-1: PlanerStempel verletzt den Stempel-Vertrag des Traegers PLANER (Zulassungsmatrix "
    "kStempelZulassungsMatrix, stempel_basis.hpp)");

/// Die Planer-Stempel-Zeile fuer den --dump-plan-Header:
/// "planner@X.Y.Z[.flag]* isa=<isa> os=<os> sha256=<64hex>" (Schlussfeld seit V-08R, s. Kopf).
/// FLAG-GRAMMATIK v2: das 'v'-Praefix ist entfallen, rohe und gerenderte Form fallen damit zusammen --
/// algo_semver_string ist fuer ein wohlgeformtes Literal die Identitaet. S-1 (P4): die Funktion
/// DELEGIERT auf das consteval-Kompositum der Erbin, byte-identisch zur frueheren Laufzeit-Konkatenation
/// (Beweis: die Roundtrip-Wache unten haelt Render==Identitaet fest, und der Vertragstest friert das
/// Byte-Orakel mit dem Praefix-Anker ein). Der Fehl-Literal-Fang der frueheren algo_semver_string-Stufe
/// ist dabei NICHT entfallen, sondern VORVERLEGT: die static_asserts unten brechen ein Fehl-Literal
/// compile-hart, bevor irgendein Aufrufer eine Zeile sieht.
[[nodiscard]] inline std::string planner_version_stamp() { return std::string{PlanerStempel::gesamt_stempel()}; }

static_assert(kPlannerVersion == std::string_view{"1.0.0.c"});
// RENDER-TREUE (Byte-Wache): das Literal parst auf {1,0,0} mit CPU-Basis, und es rendert VERBATIM zurueck.
// Die Prueform ist bewusst der RENDER-VERGLEICH und nicht mehr ein von Hand aufgebauter AlgoSemVer{...}:
// mit der Flag-LISTE muesste ein Aggregat-Literal die interne Knoten-Darstellung nachbauen -- eine zweite
// Wahrheit ueber die Repraesentation, die bei jeder Deckel-Aenderung mitgepflegt werden muesste. Der
// Roundtrip-Vergleich sagt dasselbe, haengt aber an der ZEICHENFOLGE, die der Stempel wirklich traegt.
static_assert(::comdare::cache_engine::measurement::render_algo_semver(
                  ::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion))
                  .view() == kPlannerVersion);
static_assert(::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion).x == 1u);
static_assert(::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion).has_top_level_flag("c"));
// B12-Politik (ungated, immer gebaut): der Planer-Stempel ist wohlgeformt. Die flaglose Form bliebe hier
// GRAMMATISCH wohlgeformt -- den flaglosen Fall weist erst der gated Zwilling unten ab.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kPlannerVersion),
              "kPlannerVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n punkt-getrennten "
              "Flags; traegt sie Flags, MUSS 'c' darunter sein (Owner-F-10 07.08.2026) UND jedes Flag-Token "
              "muss im Katalog stehen -- unter SEINER Basis (S2-Katalog-Wache, flag_grammar_catalog.hpp)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
// SCHARFSCHALTUNG (Migrations-Naht, algo_semver.hpp Klasse (e)): der Planer-Stempel MUSS -- wie jede
// ce-Version -- die CPU-Basis tragen. Ohne diese Wache haette der Planer die Migration umgangen.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kPlannerVersion),
              "kPlannerVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version 'c' unter ihren Flags "
              "tragen (Owner-Q3 02.08.2026 / F-10 07.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif
static_assert(planner_target_isa() == std::string_view{"x86_64"});
// V-08R-WACHEN (KON101): Form-Pins des gefuellten Fingerprints direkt am Traeger-Header -- Laenge 64,
// Zeichenvorrat lowercase-hex, und die Teilstring-Selbstprobe des Kompositums (der sha reist als
// " sha256="-Schlussfeld in der --dump-plan-Zeile; Vertrags-Spiegel stempel_basis.hpp:503-505).
static_assert(PlanerStempel::fingerprint_sha().size() == 64,
              "V-08R: der Planer-Fingerprint traegt 64 hex (SHA-256-Bein des S-1-Vertrags, KON101)");
static_assert(
    [] {
        for (char const c : PlanerStempel::fingerprint_sha())
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        return true;
    }(),
    "V-08R: der Planer-Fingerprint ist lowercase-hex ([0-9a-f], sha256::to_hex-Kanon)");
static_assert(std::string_view{PlanerStempel::gesamt_stempel()}.find(PlanerStempel::fingerprint_sha()) !=
                  std::string_view::npos,
              "V-08R: das Kompositum traegt den sha als Teilstring (' sha256='-Schlussfeld)");

} // namespace comdare::cache_engine::planner
