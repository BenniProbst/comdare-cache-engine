// measurement/flag_menge_ordnung.hpp -- die ORDNUNGS-RELATION ueber Flag-MENGEN (Bauschritt S-3a,
// 13.08.2026) und die GESCHLOSSENHEITS-WACHE der deklarierten Maschinen-Signaturen (S-3c-Anteil).
//
// DER BEFUND, DER DIESEN HEADER NOETIG MACHT -- dieselbe Fehlerklasse wie bei
// builder/bvset_teilmenge.hpp (dort GLIED [6], hier die Flag-MENGE einer AlgoSemVer): eine MENGE
// wurde bisher nur auf GLEICHHEIT verglichen (Stempel-/SHA-Weg). Gleichheit ist fuer eine Menge die
// falsche Relation:
//   * Hardware-ERWEITERUNG (ein Flag kommt hinzu): der alte Stand bleibt gueltig -- er wurde mit
//     einer TEILMENGE der heutigen Faehigkeiten gebaut.
//   * Funktions-EINSCHRAENKUNG (ein Flag faellt weg): der alte Stand ist NICHT mehr gueltig.
// Beide Faelle sehen unter Gleichheit identisch aus. Die RICHTUNG ist die Aussage -- deshalb ist
// flag_menge_ist_teilmenge AUSDRUECKLICH ASYMMETRISCH (Vorbild bvset_teilmenge.hpp:157-160).
//
// DAS ELEMENT IST DAS (token, eltern)-PAAR, nicht das Token allein. Der Katalog belegt es selbst:
// 'vnni' existiert als avx_vnni (unter x256) UND als avx512_vnni (unter x512) -- zwei CPUID-Bits.
// Ein 'vnni' unter x256 ist deshalb NIE dasselbe Element wie 'vnni' unter x512
// (flag_grammar_catalog.hpp, "IDENTITAET IST DAS PAAR").
//
// DAS X.Y.Z-TRIPEL IST NICHT TEIL DER MENGEN-RECHNUNG. Es gehoert der zweistufigen Versionierung
// (Feature/Debug-Ebene) -- dieselbe Trennung, die bvset_teilmenge.hpp mit seinem Kopf zieht
// (Versions-Zahlen sind Vorbedingung anderer Vergleiche, nie Mengen-Element).
//
// FAIL-CLOSED IN JEDEM ZWEIFEL (Vorbild bvset_teilmenge.hpp:29-31): unparsbar/Sentinel auf einer
// der Seiten => false. Ein unparsbares Literal parst auf den Sentinel (K-5), der Sentinel ist "kein
// bekannter Stand" -- ueber ihn laesst sich KEINE Teilmengen-Aussage treffen. Der Preis eines
// falschen false ist ein ueberfluessiger Neubau; der Preis eines falschen true waere ein Stand, der
// Faehigkeiten vortaeuscht.
//
// TRAVERSAL ueber die BESTEHENDE Primitive for_each_flag_node (algo_semver.hpp) -- KEIN zweiter
// Parser, keine zweite Schleifenform (Hausregel: eine Aufloesung, ein Verbraucher).
//
// DIE SIGNATUR-BRUECKE flag_menge_in_signatur uebersetzt Grammatik-Token -> SimdFeatureFlag
// AUSSCHLIESSLICH ueber die TABELLENFELDER von kFlagGrammarCatalog (cpuinfo-Spalte) -- NIE ueber
// String-Heuristik: es gibt DREI Namensraeume (Grammatik-Token, cpuinfo-Id, Compiler-Schalter), und
// keine zwei sind auseinander ableitbar (flag_grammar_catalog.hpp, Kopf).
//
// Metaprog: reines constexpr, header-only, keine Allokation, kein vtable -- dieselbe Doktrin wie
// algo_semver.hpp (die Praedikate laufen im consteval-Pfad der Wachen mit).

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/flag_grammar_catalog.hpp>
#include <cache_engine/measurement/machine_simd_signature.hpp> // die drei deklarierten CT-Signaturen (Wache unten)
#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <cstddef>
#include <span>
#include <string_view>

namespace comdare::cache_engine::measurement {

namespace detail {
/// Steht das ELEMENT (token unter eltern) in der Flag-Menge von v? Die eine Mengen-Zugehoerigkeit,
/// auf der beide Relationen dieses Headers stehen -- ueber for_each_flag_node, nie ueber einen
/// zweiten Zerleger.
[[nodiscard]] constexpr bool flag_menge_enthaelt(AlgoSemVer const& v, std::string_view token,
                                                 std::string_view eltern) noexcept {
    bool gefunden = false;
    for_each_flag_node(v, [&gefunden, token, eltern](std::string_view t, std::uint8_t /*tiefe*/,
                                                     std::string_view e) constexpr noexcept {
        if (t == token && e == eltern) gefunden = true;
    });
    return gefunden;
}
} // namespace detail

/// Ist die Flag-MENGE von `a` eine TEILMENGE der von `b`? Element == (token, eltern)-Paar.
///
/// AUSDRUECKLICH NICHT SYMMETRISCH -- das ist der ganze Zweck (s. Kopf): teilmenge(A, A+X) ist true
/// (Erweiterung), teilmenge(A+X, A) ist false (Einschraenkung). Wer hier versehentlich Gleichheit
/// einsetzt, macht beide Faelle wieder ununterscheidbar.
///
/// FAIL-CLOSED: Sentinel (auch der einer unparsbaren Eingabe) auf EINER der Seiten => false.
/// Das X.Y.Z-Tripel geht NICHT in die Rechnung ein.
[[nodiscard]] constexpr bool flag_menge_ist_teilmenge(AlgoSemVer const& a, AlgoSemVer const& b) noexcept {
    if (a.is_sentinel() || b.is_sentinel()) return false; // fail-closed: kein bekannter Stand
    bool teilmenge = true;
    for_each_flag_node(a, [&teilmenge, &b](std::string_view token, std::uint8_t /*tiefe*/,
                                           std::string_view eltern) constexpr noexcept {
        if (!detail::flag_menge_enthaelt(b, token, eltern)) teilmenge = false;
    });
    return teilmenge;
}

/// DIE SIGNATUR-BRUECKE: ist die Flag-MENGE der Version durch die deklarierte Maschinen-Signatur
/// GEDECKT? Uebersetzung Grammatik-Token -> SimdFeatureFlag ueber die TABELLENFELDER des Katalogs
/// (cpuinfo-Spalte), NIE ueber String-Heuristik.
///
/// DIE DREI FAELLE JE KNOTEN, jeder eine Aussage der Tabelle selbst:
///   (1) STRUKTUR-Token (kein Compiler-Schalter: c, p, e, die Breiten-Basen, m64) -- sagt Ziel-
///       Hardware/Kern-Klasse/Registerdatei, KEINE CPUID-Faehigkeit: zulassungs-neutral.
///   (2) FAEHIGKEITS-Token MIT cpuinfo-Id -- die Signatur muss GENAU diese Id fuehren, sonst false.
///   (3) FAEHIGKEITS-Token OHNE cpuinfo-Id ("ueber den heutigen Erhebungsweg nicht signaturfaehig",
///       Katalog x256-Block) ODER katalogfremd -- ein Match kann NIE eintreten: false, fail-closed.
///       Neutral zu sein waere fail-open: eine Forderung, die niemand pruefen kann, wuerde ueberall
///       durchgewunken.
///
/// Der SENTINEL ist false (kein bekannter Stand); die LEERE Flag-Menge einer echten Version fordert
/// nichts und ist damit auch gegen die leere Signatur gedeckt.
[[nodiscard]] constexpr bool flag_menge_in_signatur(AlgoSemVer const&                v,
                                                    std::span<SimdFeatureFlag const> signatur) noexcept {
    if (v.is_sentinel()) return false; // fail-closed: kein bekannter Stand
    bool gedeckt = true;
    for_each_flag_node(v, [&gedeckt, signatur](std::string_view token, std::uint8_t /*tiefe*/,
                                               std::string_view eltern) constexpr noexcept {
        std::size_t const i = find_flag_catalog_entry(token, eltern);
        if (i == kNoFlagCatalogEntry) {
            gedeckt = false; // katalogfremd: nie zulassbar (Fall 3)
            return;
        }
        FlagCatalogEntry const& e = kFlagGrammarCatalog[i];
        if (e.gpp.empty()) return; // Struktur-Token (Fall 1): zulassungs-neutral
        if (e.cpuinfo.empty()) {
            gedeckt = false; // nicht signaturfaehige Faehigkeit (Fall 3): fail-closed
            return;
        }
        bool gefunden = false;
        for (SimdFeatureFlag const& f : signatur)
            if (f.cpuinfo == e.cpuinfo) gefunden = true;
        if (!gefunden) gedeckt = false; // Fall 2: die Signatur fuehrt die Id nicht
    });
    return gedeckt;
}

// =================================================================================================
// DIE GESCHLOSSENHEITS-WACHE (S-3c): deklarierte Signaturen sind voraussetzungs-vollstaendig
// =================================================================================================

/// Wie viele Ketten der Tabelle nennen ein Voraussetzungs-Flag, das der 23er-Signatur-Katalog
/// (kSimdFeatureFlagCatalog) NICHT fuehrt? Das ist die deklarierte WERKZEUG-GRENZE: heute genau
/// vaes->aes und vpclmulqdq->pclmulqdq (aes/pclmulqdq sind x128-Vokabular der GRAMMATIK, aber keine
/// Eintraege des Signatur-Katalogs). Diese Ketten kann keine Signatur zusagen; die Wache unten
/// nimmt sie deshalb BENANNT aus -- nicht still. Ein ADDITIVER Katalog-Zuwachs (aes/pclmulqdq in
/// kSimdFeatureFlagCatalog, nur MIT den Schwester-Asserts dort) macht die Zahl hier LAUT falsch und
/// zieht die Wache damit automatisch nach.
[[nodiscard]] constexpr std::size_t ketten_ausserhalb_signatur_vokabular() noexcept {
    std::size_t n = 0;
    for (FlagVoraussetzungsKette const& k : kFlagVoraussetzungsKetten) {
        std::size_t const vi = find_flag_catalog_entry(k.vor_token, k.vor_eltern);
        if (vi == kNoFlagCatalogEntry) continue; // von flag_voraussetzungs_ketten_wohlgeformt ausgeschlossen
        std::string_view const cpuinfo = kFlagGrammarCatalog[vi].cpuinfo;
        if (cpuinfo.empty() || !is_known_simd_flag(cpuinfo)) ++n;
    }
    return n;
}

/// Ist die Signatur VORAUSSETZUNGS-GESCHLOSSEN? Fuer jedes deklarierte Flag, dessen Grammatik-
/// Element eine Kette traegt, muss auch das Voraussetzungs-Flag DEKLARIERT sein (prod1 traegt
/// avx512f UND alle Subsets explizit -- genau diese Vollstaendigkeit friert die Wache ein, damit
/// eine kuenftige unvollstaendige Deklaration LAUT wird statt still eine Freigabe zu verzerren).
///
/// Die Uebersetzung Signatur-Flag -> Grammatik-Element laeuft ueber die cpuinfo-SPALTE des Katalogs
/// (find_flag_catalog_entry_by_cpuinfo) -- Tabelle, nie Heuristik. Ein Signatur-Flag OHNE
/// Grammatik-Gegenstueck waere eine Drift (grammar_catalog_covers_simd_feature_catalog haelt die
/// Bruecke dicht) und faellt hier fail-closed.
///
/// KETTEN AUSSERHALB DES SIGNATUR-VOKABULARS (s. ketten_ausserhalb_signatur_vokabular): die Grenze
/// ist BENANNT und static_assert-gefroren; solche Ketten sind hier ausgenommen, weil keine Signatur
/// sie erfuellen KANN -- nicht, weil sie nicht gelten.
[[nodiscard]] constexpr bool signatur_ist_voraussetzungs_geschlossen(std::span<SimdFeatureFlag const> sig) noexcept {
    for (SimdFeatureFlag const& f : sig) {
        std::size_t const i = find_flag_catalog_entry_by_cpuinfo(f.cpuinfo);
        if (i == kNoFlagCatalogEntry) return false; // Signatur-Flag ohne Grammatik-Element: Drift, fail-closed
        FlagCatalogEntry const& e = kFlagGrammarCatalog[i];
        std::size_t const       k = find_flag_voraussetzung(e.token, e.eltern);
        if (k == kKeineVoraussetzungsKette) continue;
        std::size_t const vi =
            find_flag_catalog_entry(kFlagVoraussetzungsKetten[k].vor_token, kFlagVoraussetzungsKetten[k].vor_eltern);
        if (vi == kNoFlagCatalogEntry) return false; // von der Tabellen-Wache ausgeschlossen; doppelt dicht
        std::string_view const vor_cpuinfo = kFlagGrammarCatalog[vi].cpuinfo;
        if (vor_cpuinfo.empty() || !is_known_simd_flag(vor_cpuinfo)) continue; // deklarierte Werkzeug-Grenze
        bool gefunden = false;
        for (SimdFeatureFlag const& g : sig)
            if (g.cpuinfo == vor_cpuinfo) gefunden = true;
        if (!gefunden) return false;
    }
    return true;
}

// =================================================================================================
// COMPILE-ZEIT-BATTERIE: der Beweis, nicht die Behauptung
// =================================================================================================

// -- Die Ordnungs-Relation: asymmetrisch, (token, eltern)-genau, fail-closed ----------------------
static_assert(flag_menge_ist_teilmenge(parse_algo_semver("1.0.0.c.x512{f.vl}"),
                                       parse_algo_semver("1.0.0.c.x512{f.vl.bw}")));
static_assert(!flag_menge_ist_teilmenge(parse_algo_semver("1.0.0.c.x512{f.vl.bw}"),
                                        parse_algo_semver("1.0.0.c.x512{f.vl}")));
static_assert(!flag_menge_ist_teilmenge(parse_algo_semver("1.0.0.c.x256{vnni}"),
                                        parse_algo_semver("1.0.0.c.x512{f.vnni}")));
// Eltern-Genauigkeit auch dann, wenn rechts ALLE Token der linken Seite stehen (vnni nur unter dem
// falschen Elternteil) -- die Zeile, an der eine Eltern-ignorierende Mutation reissen MUSS.
static_assert(!flag_menge_ist_teilmenge(parse_algo_semver("1.0.0.c.x256{vnni}"),
                                        parse_algo_semver("1.0.0.c.x256.x512{f.vnni}")));
static_assert(flag_menge_ist_teilmenge(parse_algo_semver("2.0.0.c"), parse_algo_semver("1.0.0.c{p}")));
static_assert(!flag_menge_ist_teilmenge(AlgoSemVer{}, parse_algo_semver("1.0.0.c")));
static_assert(!flag_menge_ist_teilmenge(parse_algo_semver("1.0.0.c"), AlgoSemVer{}));

// -- Die Signatur-Bruecke: Tabelle statt Heuristik, Struktur neutral, Rest fail-closed ------------
static_assert(flag_menge_in_signatur(parse_algo_semver("1.0.0.c.x512{f.vl}"), Prod1Zen5Signature::signature()));
static_assert(!flag_menge_in_signatur(parse_algo_semver("1.0.0.c.x512{f}"), Prod2RaptorLakeSignature::signature()));
static_assert(flag_menge_in_signatur(parse_algo_semver("1.0.0.c{p.e}"), Prod2RaptorLakeSignature::signature()));
static_assert(flag_menge_in_signatur(parse_algo_semver("1.0.0.c.x256{vnni}"), Prod2RaptorLakeSignature::signature()));
static_assert(!flag_menge_in_signatur(parse_algo_semver("1.0.0.c.x512{vnni}"), Prod2RaptorLakeSignature::signature()));
static_assert(!flag_menge_in_signatur(parse_algo_semver("1.0.0.c.x256{vnniint8}"), Prod1Zen5Signature::signature()));
static_assert(!flag_menge_in_signatur(parse_algo_semver("1.0.0.c.quatsch"), Prod1Zen5Signature::signature()));
static_assert(!flag_menge_in_signatur(AlgoSemVer{}, Prod1Zen5Signature::signature()));

// -- Die Geschlossenheits-Wache: die drei deklarierten Signaturen sind dicht ----------------------
static_assert(signatur_ist_voraussetzungs_geschlossen(Prod1Zen5Signature::signature()),
              "prod1 deklariert avx512f UND alle Subsets explizit -- eine kuenftige Deklaration, die "
              "ein Subset ohne sein Fundament fuehrt, MUSS hier brechen.");
static_assert(signatur_ist_voraussetzungs_geschlossen(Prod2RaptorLakeSignature::signature()));
static_assert(signatur_ist_voraussetzungs_geschlossen(OdroidGracemontSignature::signature()));
static_assert(ketten_ausserhalb_signatur_vokabular() == 2,
              "Die deklarierte WERKZEUG-GRENZE: genau vaes->aes und vpclmulqdq->pclmulqdq nennen ein "
              "Flag ausserhalb des 23er-Signatur-Katalogs. Waechst der Katalog additiv um "
              "aes/pclmulqdq (nur MIT seinen Schwester-Asserts), sinkt diese Zahl -- dann gehoert die "
              "Ausnahme hier entfernt, nicht die Zahl angepasst.");

} // namespace comdare::cache_engine::measurement
