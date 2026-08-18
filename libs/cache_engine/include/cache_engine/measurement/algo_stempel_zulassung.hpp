// measurement/algo_stempel_zulassung.hpp -- S-7 (KON9-05, 13.08.2026): der Achsen-Algo-Hardware-
// Stempel X.Y.Z in voller System-Achsen-Syntax UND Semantik -- die ZULASSUNGS-BRUECKE zwischen den
// beiden per KON16-02/KON23-02 GEBAUTEN Seiten.
//
// DIE ZWEI SEITEN (O-3, je Seite beantwortet): die FREIGABE-Seite IMPLIZIERT (aktive Maschinen-
// Deklaration -> active_machine_signature, seit S-3c produktiv verdrahtet ueber
// maschinen_deklarations_naht.hpp); die COMPILE-Seite FORDERT (die Flag-Menge einer algo_version,
// ce_owned_version_is_wellformed Term (d), S-3b). Dieser Header verbindet beide: er uebersetzt die
// GEFORDERTE Menge in die Signatur-Welt und faellt das ZULASSUNGS-Urteil je Achsen-Version.
//
// SICHT UND URTEIL SIND GETRENNT -- EIN RICHTER, KEIN ZWEITER:
//   * algo_declared_flags_for_axes ist eine reine SICHT fuer Log/Tests: welche signaturfaehigen
//     Faehigkeiten fordert der Achsen-Satz? Sie uebersetzt AUSSCHLIESSLICH ueber die Tabellenfelder
//     (find_flag_catalog_entry -> cpuinfo-Spalte -> kSimdFeatureFlagCatalog) -- es gibt DREI
//     Namensraeume (Grammatik-Token, cpuinfo-Id, Compiler-Schalter), und keine zwei sind auseinander
//     ableitbar (flag_grammar_catalog.hpp, Kopf). NIE String-Heuristik.
//   * Das URTEIL lebt ausschliesslich in flag_menge_in_signatur (S-3a, flag_menge_ordnung.hpp:101):
//     stempel_zulassung_je_achse ist nur die Fehlerklassen-Huelle darum. Die SICHT entscheidet
//     NICHTS -- insbesondere zeigt sie nicht-signaturfaehige/katalogfremde Forderungen NICHT an
//     (sie KANN sie nicht zeigen: der 23er-Katalog fuehrt sie nicht), waehrend das Urteil genau
//     daran fail-closed ablehnt. Wer die Sicht als Richter missbraucht, oeffnet fail-open.
//
// FAIL-CLOSED (geerbt aus der S-3a-Primitive, flag_menge_ordnung.hpp:101-124): Sentinel/unparsbar,
// katalogfremdes Token und nicht-signaturfaehige Faehigkeit => Ablehnung HardwareErweiterungFehlt
// (axis_error.hpp:42 -- exakt die Klasse der Sache: die Maschine gibt das Geforderte nicht frei).
//
// INERTHEITS-BEWEIS (Aktivierungs-Auftrag, kein Verhaltens-Ereignis): alle 123 Bestands-Literale
// unter organ_axes/+topics/ (97x "1.0.0.c" + 2x "1.0.1.c" + 24x "1.0.2.c", gezaehlt am Objekt 5f3f26a5)
// sind nackte c-Formen -- 'c' ist Struktur-Token (Fall 1 der Bruecke), also ist JEDE davon gegen
// JEDE Signatur gedeckt, auch gegen die LEERE (CT-Batterie unten, 4x3). Die Bruecke ist nach der
// Aktivierung heute byte-/golden-neutral BY CONSTRUCTION.
//
// Metaprog: reines constexpr, header-only -- dieselbe Doktrin wie flag_menge_ordnung.hpp. Der
// Dateiname matcht KEINEN der 9 Overlay-Praefixe (overlay_source_set.hpp:177-194) -- ausserhalb
// der S-14a-Lock-Domaene, fingerprint-neutral.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_error.hpp> // CompilerCompilerErrorClass::HardwareErweiterungFehlt
#include <cache_engine/measurement/flag_grammar_catalog.hpp>
#include <cache_engine/measurement/flag_menge_ordnung.hpp>     // DER RICHTER: flag_menge_in_signatur (S-3a)
#include <cache_engine/measurement/machine_simd_signature.hpp> // die drei deklarierten CT-Signaturen
#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::measurement {

/// (a) DIE SICHT: die signaturfaehige Forderungs-MENGE eines (achse, version_literal)-Satzes --
/// als Eintraege des 23er-Signatur-Katalogs (kSimdFeatureFlagCatalog), dedupliziert ueber die
/// cpuinfo-Id. REINE SICHT fuer Log/Tests; das URTEIL lebt ausschliesslich in
/// flag_menge_in_signatur (kein zweiter Richter, s. Kopf).
///
/// Uebersetzungsweg je Flag-Knoten, NUR ueber Tabellenfelder: (token, eltern) ->
/// find_flag_catalog_entry -> kFlagGrammarCatalog[i].cpuinfo -> kSimdFeatureFlagCatalog-Eintrag.
/// Knoten ohne Katalog-Eintrag, Struktur-Token (leere gpp-Spalte), Faehigkeiten ohne cpuinfo-Id
/// und cpuinfo-Ids ausserhalb des 23er-Katalogs erscheinen NICHT in der Sicht -- genau diese
/// Faelle behandelt das Urteil fail-closed (die Sicht zeigt, was die Signatur-Welt DARSTELLEN
/// kann, nicht, was zulaessig ist). Leere Literale tragen nichts bei (kein Versions-Traeger).
[[nodiscard]] constexpr std::vector<SimdFeatureFlag>
algo_declared_flags_for_axes(std::span<std::pair<std::string_view, std::string_view> const> achsen_versionen) {
    std::vector<SimdFeatureFlag> out;
    for (auto const& [achse, literal] : achsen_versionen) {
        (void)achse; // Provenienz des Aufrufers; die Mengen-Vereinigung braucht nur das Literal
        if (literal.empty()) continue;
        AlgoSemVer const v = parse_algo_semver(literal);
        for_each_flag_node(v,
                           [&out](std::string_view token, std::uint8_t /*tiefe*/, std::string_view eltern) constexpr {
                               std::size_t const i = find_flag_catalog_entry(token, eltern);
                               if (i == kNoFlagCatalogEntry)
                                   return; // katalogfremd: nicht darstellbar (Urteil: fail-closed)
                               FlagCatalogEntry const& e = kFlagGrammarCatalog[i];
                               if (e.gpp.empty() || e.cpuinfo.empty()) return; // Struktur-Token / nicht signaturfaehig
                               for (SimdFeatureFlag const& f : kSimdFeatureFlagCatalog) {
                                   if (f.cpuinfo != e.cpuinfo) continue;
                                   bool schon = false;
                                   for (SimdFeatureFlag const& g : out)
                                       if (g.cpuinfo == f.cpuinfo) schon = true;
                                   if (!schon) out.push_back(f); // Union, dedupliziert ueber cpuinfo
                                   return;
                               }
                           });
    }
    return out;
}

/// (b) DAS URTEIL JE ACHSEN-VERSION: nullopt == zugelassen (die Flag-Menge der Version ist durch
/// die Signatur gedeckt), sonst HardwareErweiterungFehlt. Reine Huelle um den EINEN Richter
/// flag_menge_in_signatur (S-3a) -- fail-closed inkl. Sentinel/katalogfremd/nicht-signaturfaehig
/// ist dort geerbt, hier wird NICHTS zweitgeprueft.
[[nodiscard]] constexpr std::optional<CompilerCompilerErrorClass>
stempel_zulassung_je_achse(AlgoSemVer const& v, std::span<SimdFeatureFlag const> signatur) noexcept {
    if (flag_menge_in_signatur(v, signatur)) return std::nullopt;
    return CompilerCompilerErrorClass::HardwareErweiterungFehlt;
}

// =================================================================================================
// (c) COMPILE-ZEIT-BATTERIE: der Beweis, nicht die Behauptung
// =================================================================================================

namespace detail {
/// Batterie-Kurzform: ist das Literal gegen die Signatur ZUGELASSEN?
[[nodiscard]] constexpr bool stempel_zugelassen(std::string_view                 literal,
                                                std::span<SimdFeatureFlag const> signatur) noexcept {
    return !stempel_zulassung_je_achse(parse_algo_semver(literal), signatur).has_value();
}
inline constexpr std::span<SimdFeatureFlag const> kLeereSignatur{};
} // namespace detail

// -- INERTHEITS-BEWEIS: die drei Bestandsformen sind UEBERALL zugelassen, auch gegen LEER (4x3) ---
static_assert(detail::stempel_zugelassen("1.0.0.c", Prod1Zen5Signature::signature()));
static_assert(detail::stempel_zugelassen("1.0.0.c", Prod2AlderLakeSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.0.c", OdroidGracemontSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.0.c", detail::kLeereSignatur));
static_assert(detail::stempel_zugelassen("1.0.1.c", Prod1Zen5Signature::signature()));
static_assert(detail::stempel_zugelassen("1.0.1.c", Prod2AlderLakeSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.1.c", OdroidGracemontSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.1.c", detail::kLeereSignatur));
static_assert(detail::stempel_zugelassen("1.0.2.c", Prod1Zen5Signature::signature()));
static_assert(detail::stempel_zugelassen("1.0.2.c", Prod2AlderLakeSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.2.c", OdroidGracemontSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.2.c", detail::kLeereSignatur));

// -- Die FORDERNDE Form faellt, wo die Freigabe fehlt: leere Signatur und prod2 (fused-off) ------
static_assert(!detail::stempel_zugelassen("1.0.0.c.x512{f}", detail::kLeereSignatur));
static_assert(!detail::stempel_zugelassen("1.0.0.c.x512{f}", Prod2AlderLakeSignature::signature()));
static_assert(detail::stempel_zugelassen("1.0.0.c.x512{f}", Prod1Zen5Signature::signature()));

// -- DAS OWNER-BEISPIEL (algo_semver.hpp:1265, volle System-Achsen-Syntax) gegen prod1: das
//    Ergebnis IST zugelassen (avx512f/vl/bw/dq und gfni stehen in der Zen-5-Signatur; c{p.e} und
//    x512 sind Struktur) -- hier festgeschrieben, damit eine Signatur-/Katalog-Drift laut wird.
static_assert(detail::stempel_zugelassen(detail::kOwnerBeispiel, Prod1Zen5Signature::signature()));
static_assert(!detail::stempel_zugelassen(detail::kOwnerBeispiel, Prod2AlderLakeSignature::signature()));
static_assert(!detail::stempel_zugelassen(detail::kOwnerBeispiel, detail::kLeereSignatur));

// -- T-4-GEGENEINGANG: der VOLLAUSBAU (algo_semver.hpp:1232, 59 Knoten) ist NICHT signatur-gedeckt
//    -- er traegt nicht-signaturfaehige Tokens (u.a. x256{vnniint8}, sm3, sm4) und faellt deshalb
//    fail-closed SELBST gegen die reichste deklarierte Signatur. Das ist der Beweis, dass die
//    Bruecke kein Positivfall-Durchwinker ist -- NICHT als Positivfall umdeuten.
static_assert(!detail::stempel_zugelassen(detail::kVollausbau, Prod1Zen5Signature::signature()));
static_assert(!detail::stempel_zugelassen(detail::kVollausbau, detail::kLeereSignatur));

// -- Die SICHT uebersetzt ueber die Tabelle (drei Namensraeume), dedupliziert ueber cpuinfo ------
namespace detail {
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 1> kSichtVnni256{
    {{"probe", "1.0.0.c.x256{vnni}"}}};
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 1> kSichtVnni512{
    {{"probe", "1.0.0.c.x512{vnni}"}}};
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 2> kSichtDedup{
    {{"a", "1.0.0.c.x512{f}"}, {"b", "1.0.0.c.x512{f.vl}"}}};
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 1> kSichtNackt{{{"a", "1.0.0.c"}}};
} // namespace detail
// vnni-Doppelgaenger: DIESELBE Token-Schreibweise, ZWEI cpuinfo-Ids -- nur die Tabelle trennt sie.
static_assert(algo_declared_flags_for_axes(detail::kSichtVnni256).size() == 1);
static_assert(algo_declared_flags_for_axes(detail::kSichtVnni256)[0].cpuinfo == std::string_view{"avx_vnni"});
static_assert(algo_declared_flags_for_axes(detail::kSichtVnni512).size() == 1);
static_assert(algo_declared_flags_for_axes(detail::kSichtVnni512)[0].cpuinfo == std::string_view{"avx512_vnni"});
// Union zweier Literale: avx512f erscheint EINMAL (Dedup ueber cpuinfo), avx512vl kommt dazu.
static_assert(algo_declared_flags_for_axes(detail::kSichtDedup).size() == 2);
// Die nackte Bestands-Form fordert nichts Darstellbares -- leere Sicht == leere Forderung.
static_assert(algo_declared_flags_for_axes(detail::kSichtNackt).empty());

} // namespace comdare::cache_engine::measurement
