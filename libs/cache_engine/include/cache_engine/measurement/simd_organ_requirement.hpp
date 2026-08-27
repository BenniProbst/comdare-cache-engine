// measurement/simd_organ_requirement.hpp -- zentrale Organ-required-SIMD-Flag-Deklaration (Bau Section 40.a-E4).
//
// Der Deklarations-Hook je Organ-Achsen-Klasse fuer HART benoetigte SIMD-Flags (Default LEER). Ein Organ, das
// eine Auspraegung NUR mit einem bestimmten Flag ueberhaupt bauen kann, traegt es hier ein; solange die Menge
// LEER ist (heutiger Stand ALLER Organe), ist das Bau-Gate NotApplicable/inert -> Ist-Verhalten byte-identisch.
// Die erste echte required-Deklaration aktiviert die Freigabe-Kopplung (Section 37: Organ <= Maschinen-Signatur)
// OHNE weiteren Umbau -- die Aggregation aus der Organ-Signatur (aggregate_required_for_axes) und das Gate
// (simd_build_gate.hpp) stehen bereit.
//
// VERORTUNG: zentral in measurement/ (bei Katalog/Sinnhaftigkeit/Signatur), aus demselben Layering-Grund wie
// simd_organ_sensibility.hpp -- topics/ darf nicht auf cache_engine/measurement rueck-abhaengen. Die Organ-
// Klassen werden per NAME-String referenziert (keine Organ-Header-Inklusion). NIE binary_id.
//
// Metaprog: POD-Deskriptoren + constexpr/inline-Freifunktionen, keine vtable, kein std::variant.
//
// E-10/ORG-19 SCHRITT 2b (26.08.2026, Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19 + FIX-7(a)),
// supersedierend zum Kopf oben: die Aggregation ist jetzt ein constexpr-KERN ueber
// (achse,wert)-string_view-Paare, register-parametrisiert ueber die Organ-META-META-Tabelle
// (organ_meta_meta_requirement.hpp, 18+1-Schnitt H-23 C.1) -- er vereinigt required_of(achse) UND
// required_of_meta_meta(meta_meta_for_pair(achse, wert)), cpuinfo-dedupliziert. Die bestehende
// Laufzeit-Signatur aggregate_required_for_axes(span<pair<string,string> const>) bleibt als Huelle
// (FIX-7(a): sie IST bereits span-basiert; der constexpr-Kern ist der eigentliche Umbau). NEU
// aggregate_meaningful_for_axes analog (Sinnhaftigkeits-OBERGRENZE per Binary statt global;
// Hook 2/3 wird in Schritt 3 per Binary verdrahtet). Der D-4-Beweis, dass eine NICHT-leere
// Meta-Meta-Zeile PER BINARY wirkt, steht unten am Kern (detail::probe_per_binary_aggregation_
// ist_exakt, 2c-Probe-Register) und ist die Grundlage des C-3a-UMSCHLAGS (Schritt 3).

#pragma once

#include <cache_engine/measurement/organ_meta_meta_requirement.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp>
#include <cache_engine/measurement/simd_organ_sensibility.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::measurement {

// Eine Zeile der required-Deklaration: Organ-Klasse (per Name) -> HART benoetigte Flags (Default leer).
struct OrganSimdRequirement {
    std::string_view                 organ_class;
    std::span<SimdFeatureFlag const> required; // LEER = keine harte Anforderung (Gate NotApplicable)
};

// Default-Leermenge (ein Organ opt-in't, indem es hier eine eigene nicht-leere Menge einsetzt).
inline constexpr std::array<SimdFeatureFlag, 0> kRequiredNone{};

// Zentrale required-Deklaration: HEUTE traegt JEDES Organ die leere Menge (byte-neutral).
// B03/K06 (2026-08-21, Nenner-Fix): die Tabelle fuehrt ALLE 18 Organ-Haupt-Achsen in der
// kCompositionAxisNames-Reihenfolge (FK4-Slot-Ordnung 0..17). Bis dahin standen hier NEUN Namen
// in "Sinnhaftigkeits"-Reihenfolge, darunter "scoring" -- KEINE Kompositions-Achse (Geist, 0
// Konsumenten, entfernt) -- und es fehlten zehn echte Achsen. Der alte Nenner konnte "leer, weil
// nichts deklariert" nicht von "leer, weil nicht gefuehrt" unterscheiden. Der Positions-Pin
// gegen die Registry lebt in test_simd_organ_achsen_deckung.cpp (measurement darf builder nicht
// sehen); HIER wacht die Zaehlung + Inertheit. Alle Mengen LEER -> byte-/golden-neutral.
inline constexpr std::array<OrganSimdRequirement, 18> kSimdOrganRequirement{{
    {"search_algo", kRequiredNone},
    {"cache_traversal", kRequiredNone},
    {"mapping", kRequiredNone},
    {"path_compression", kRequiredNone},
    {"node_type", kRequiredNone},
    {"memory_layout", kRequiredNone},
    {"allocator", kRequiredNone},
    {"prefetch", kRequiredNone},
    {"concurrency", kRequiredNone},
    {"serialization", kRequiredNone},
    {"value_handle", kRequiredNone},
    {"index_organization", kRequiredNone},
    {"io_dispatch", kRequiredNone},
    {"migration_policy", kRequiredNone},
    {"filter", kRequiredNone},
    {"queuing_q1", kRequiredNone},
    {"queuing_q2", kRequiredNone},
    {"persistence_target", kRequiredNone},
}};

// Die harten required-Flags einer Organ-Klasse (leer, wenn nicht deklariert).
[[nodiscard]] constexpr std::span<SimdFeatureFlag const> required_of(std::string_view organ_class) noexcept {
    for (auto const& e : kSimdOrganRequirement)
        if (e.organ_class == organ_class) return e.required;
    return {};
}

// Deklariert IRGENDEIN Organ harte required-Flags? (Aktivierungs-Indikator; heute false -> Gate global inert.)
[[nodiscard]] constexpr bool any_organ_declares_required() noexcept {
    for (auto const& e : kSimdOrganRequirement)
        if (!e.required.empty()) return true;
    return false;
}

// -- Aggregations-KERN (E-10 Schritt 2b, constexpr): EIN Rechenweg fuer Produktions- UND -----------
// -- 2c-Probe-Register (D-4) -- die Laufzeit-Huellen unten delegieren hierher. ---------------------
// Ergebnis-Menge als POD mit Katalog-Kapazitaet: die cpuinfo-Deduplikation plus die within-catalog-
// Wachen beider Register (organ_meta_meta_requirement.hpp) plus catalog_ids_unique() (23 distinkte
// Ids) deckeln jede Vereinigung auf kSimdFeatureFlagCatalog.size() -- ein Ueberlauf ist damit per
// Konstruktion unerreichbar (und braeche in constant evaluation compile-hart, kein stiller Skip).
struct SimdFlagMenge {
    std::array<SimdFeatureFlag, kSimdFeatureFlagCatalog.size()> flags{};
    std::size_t                                                 count{0};

    [[nodiscard]] constexpr bool enthaelt_cpuinfo(std::string_view cpuinfo) const noexcept {
        for (std::size_t i = 0; i < count; ++i)
            if (flags[i].cpuinfo == cpuinfo) return true;
        return false;
    }
    [[nodiscard]] constexpr bool                             empty() const noexcept { return count == 0; }
    [[nodiscard]] constexpr std::span<SimdFeatureFlag const> als_span() const noexcept {
        return std::span{flags}.first(count);
    }
    constexpr void fuege_dedupliziert_hinzu(std::span<SimdFeatureFlag const> menge) noexcept {
        for (auto const& f : menge)
            if (!enthaelt_cpuinfo(f.cpuinfo)) flags[count++] = f;
    }
};

// Per-Binary-Aggregation (KERN): aus der Organ-Signatur (den (achse,wert)-Paaren einer Tier-Binary)
// die Vereinigung der harten required-Flags -- required_of(achse) der 18er-Tabelle VEREINIGT mit
// required_of_meta_meta der Zeile, auf die das Traeger-Paar aufloest (18+1; cpuinfo-dedupliziert).
// Der erste Achsen-Name eines Paares ist die Organ-Klasse (deckungsgleich zu den Sinnhaftigkeits-/
// required-Keys); der WERT waehlt seit E-10 die Meta-Meta-Zeile mit aus (vorher unbenutzt).
// HEUTE stets leer (weder Organ noch Meta-Meta deklariert required) -> Gate NotApplicable -> byte-neutral.
[[nodiscard]] constexpr SimdFlagMenge
aggregate_required_for_axes_kern(std::span<std::pair<std::string_view, std::string_view> const> axes,
                                 std::span<OrganMetaMetaRequirement const> meta_meta_register) noexcept {
    SimdFlagMenge out{};
    for (auto const& [axis, value] : axes) {
        out.fuege_dedupliziert_hinzu(required_of(axis));
        out.fuege_dedupliziert_hinzu(
            required_of_meta_meta_in(meta_meta_register, meta_meta_for_pair_in(meta_meta_register, axis, value)));
    }
    return out;
}

// Sinnhaftigkeits-OBERGRENZE per Binary (analog; E-10 Schritt 2b NEU): sensibility_of(achse) der
// 18er-Matrix VEREINIGT mit meaningful_of_meta_meta der Traeger-Zeile. Ersetzt am Dock (Schritt 3)
// die globale kSensibilityUnion-Auskunft -- Hook 2/3 wird per Binary.
[[nodiscard]] constexpr SimdFlagMenge
aggregate_meaningful_for_axes_kern(std::span<std::pair<std::string_view, std::string_view> const> axes,
                                   std::span<OrganMetaMetaRequirement const> meta_meta_register) noexcept {
    SimdFlagMenge out{};
    for (auto const& [axis, value] : axes) {
        out.fuege_dedupliziert_hinzu(sensibility_of(axis));
        out.fuege_dedupliziert_hinzu(
            meaningful_of_meta_meta_in(meta_meta_register, meta_meta_for_pair_in(meta_meta_register, axis, value)));
    }
    return out;
}

// -- Laufzeit-Huellen (bestehende Signatur BLEIBT; FIX-7(a): sie ist bereits span-basiert) ---------
[[nodiscard]] inline std::vector<SimdFeatureFlag>
aggregate_required_for_axes(std::span<std::pair<std::string, std::string> const> axes) {
    std::vector<std::pair<std::string_view, std::string_view>> sichten;
    sichten.reserve(axes.size());
    for (auto const& [axis, value] : axes) sichten.emplace_back(axis, value);
    auto const menge = aggregate_required_for_axes_kern(sichten, kOrganMetaMetaRequirement);
    auto const span_ = menge.als_span();
    return {span_.begin(), span_.end()};
}

[[nodiscard]] inline std::vector<SimdFeatureFlag>
aggregate_meaningful_for_axes(std::span<std::pair<std::string, std::string> const> axes) {
    std::vector<std::pair<std::string_view, std::string_view>> sichten;
    sichten.reserve(axes.size());
    for (auto const& [axis, value] : axes) sichten.emplace_back(axis, value);
    auto const menge = aggregate_meaningful_for_axes_kern(sichten, kOrganMetaMetaRequirement);
    auto const span_ = menge.als_span();
    return {span_.begin(), span_.end()};
}

static_assert(kSimdOrganRequirement.size() == 18,
              "B03/E-10: alle 18 Organ-Haupt-Achsen (kCompositionAxisNames-Nenner; Positions-Pin im Test) "
              "-- 18 Komposition; die +1 Meta-Meta-Zeile lebt in organ_meta_meta_requirement.hpp "
              "(18+1-Schnitt H-23 C.1, KEINE 19. Zeile hier).");
static_assert(!any_organ_declares_required()); // heutiger Stand: ALLE leer -> Gate inert -> byte-/golden-neutral

// E-10 Schritt 2: das Produktions-Aggregat ist auch UEBER die Meta-Meta-Zeile ehrlich LEER -- selbst
// ein IO-Traeger-Paar erbt heute 0 required-Flags (KON16-02 Funktions-MINIMUM; KN-1 Lesart A). Der
// RECHENWEG ist per Binary, der WERT bleibt leer = byte-neutral (N-5/B-3 des Designplans).
[[nodiscard]] constexpr bool produktions_required_aggregat_ist_heute_leer() noexcept {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> io_traeger_paare{
        {{"search_algo", "k_ary"}, {"persistence_target", "persistence_disk_writeback"}}};
    return aggregate_required_for_axes_kern(io_traeger_paare, kOrganMetaMetaRequirement).empty();
}
static_assert(produktions_required_aggregat_ist_heute_leer(),
              "E-10: weder die 18 Kompositions-Zeilen noch die disk_io-Meta-Meta-Zeile deklarieren "
              "required -- ein Wert hier waere erfunden (nie raten); erst eine echte Deklaration "
              "aktiviert das Gate, dann mit eigenem Minor. WER DIESEN ANKER NACHZIEHT (S3-Auflage): "
              "der Glied-[5]-/build_version-Suffix-Kanal traegt den Gate-Beitrag heute als per-Perm "
              "durchgereichte LEERE Invariante (profile_run_entry/experiment_run_entry perm_gate; "
              "profile_run_facade :584/:1478) -- eine echte required-Deklaration erzwingt dort die "
              "per-Binary-Glied-Bildung, sonst stempelt die Perm falsch (die per-Job-CompileFn der "
              "Fassade bricht dann fail-closed).");

namespace detail {

// D-4-BEWEIS (E-10 Schritt 2c) am 2c-Probe-Register (organ_meta_meta_requirement.hpp): eine
// NICHT-leere Meta-Meta-Zeile wirkt PER BINARY -- Achsen-Paare MIT IO-Traeger aggregieren exakt die
// Probe-Menge {avx2}, Paare OHNE Traeger exakt LEER; meaningful analog (search_algo traegt 4
// Katalog-Flags bw/dq/f/vl, die Probe-Zeile addiert avx2 NUR am Traeger-Paar: 5 vs. 4). Diese
// Funktion ist die Grundlage des C-3a-UMSCHLAGS (Schritt 3, simd_build_gate.hpp): der globale
// Tripwire wird zur POSITIVEN Zusage gedreht, nicht geloescht (Ledger TRIPWIRE-UMSCHLAG-Muster).
[[nodiscard]] constexpr bool probe_per_binary_aggregation_ist_exakt() noexcept {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> mit_traeger{
        {{"search_algo", "k_ary"}, {"persistence_target", "persistence_disk_writeback"}}};
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> ohne_traeger{
        {{"search_algo", "k_ary"}, {"persistence_target", "persistence_memory_only"}}};
    auto const mit_req          = aggregate_required_for_axes_kern(mit_traeger, kProbeMetaMetaRequirement);
    auto const ohne_req         = aggregate_required_for_axes_kern(ohne_traeger, kProbeMetaMetaRequirement);
    auto const mit_mean         = aggregate_meaningful_for_axes_kern(mit_traeger, kProbeMetaMetaRequirement);
    auto const ohne_mean        = aggregate_meaningful_for_axes_kern(ohne_traeger, kProbeMetaMetaRequirement);
    bool const required_exakt   = mit_req.count == 1 && mit_req.flags[0].cpuinfo == kAvx2.cpuinfo //
                                  && ohne_req.count == 0;
    bool const meaningful_exakt = mit_mean.count == 5 && mit_mean.enthaelt_cpuinfo(kAvx2.cpuinfo) //
                                  && ohne_mean.count == 4 && !ohne_mean.enthaelt_cpuinfo(kAvx2.cpuinfo);
    return required_exakt && meaningful_exakt;
}
static_assert(probe_per_binary_aggregation_ist_exakt(),
              "D-4 (E-10 Schritt 2c): die required-/meaningful-Vereinigung wird PER BINARY gebildet -- "
              "Achsen MIT Meta-Meta-Traeger ergeben die Probe-Menge, OHNE ergeben leer. Bricht diese "
              "Probe, ist die per-Binary-Selektion defekt (Grundlage des C-3a-Umschlags, Schritt 3).");

} // namespace detail
static_assert(required_of("filter").empty());
static_assert(required_of("search_algo").empty());
static_assert(required_of("persistence_target").empty()); // B03: nachgezogene Achse ist gefuehrt UND leer
static_assert(required_of("scoring").empty());            // B03: der Geist ist RAUS (unbekannt -> leer)
static_assert(required_of("nicht_existente_klasse").empty());
// B03: Duplikatfreiheit der 18 Namen (die Registry-Gleichheit selbst prueft der Test):
static_assert(
    [] {
        for (std::size_t i = 0; i < kSimdOrganRequirement.size(); ++i)
            for (std::size_t j = i + 1; j < kSimdOrganRequirement.size(); ++j)
                if (kSimdOrganRequirement[i].organ_class == kSimdOrganRequirement[j].organ_class) return false;
        return true;
    }(),
    "B03: doppelter Organ-Klassen-Name in kSimdOrganRequirement.");

} // namespace comdare::cache_engine::measurement
