#pragma once
// E-24 C5 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 6.2
// FK-8) -- die D1-MELDUNG DER GATTUNGS-BAUPFADE.
//
// WAS FK-8 LOEST. Bis zum E-24-Fenster war der Bau-Pfad gattungs-BLIND: er baut SearchAlgorithm-Binaries,
// und die Gattung kommt in ihm nicht vor (Erhebung 04.08.: `grep -rn "AnatomyGenus" builder/
// build_orchestrator/ profile_facade/` -> 0 Treffer). Mit diesem Fenster werden die vier Container-
// Gattungen baubar, und damit entsteht eine Bau-ENTSCHEIDUNG, die es vorher nicht gab:
//   (1) Ist die verlangte Gattung in DIESEM Bau ueberhaupt gebunden (Komposition + Anatomie da)?
//   (2) Passt die Aritaet der gelieferten Permutation zum Slot-Satz genau dieser Gattung?
// Beide Fragen sind Planer-/Compile-Zeit-Urteile (D1): ein erkennbarer, klassifizierbarer Zustand, KEIN
// Absturz -- das Experiment baut und misst die uebrigen Permutationen weiter (axis_error.hpp Kopf-Doku).
//
// WARUM DAS KEINE ERFUNDENE FRAGE IST (beides am Ist belegt, nicht angenommen):
//   zu (1): anatomy_base.hpp:43 fuehrt `AnatomyGattung::Graph = 2`, aber gattung_of() bildet KEINE
//           Ebene-2-Gattung auf Graph ab (anatomy_base.hpp:102-110) -- es gibt zu dieser Gattung heute
//           keine Tier-Unterklasse und keine Bau-Bindung. Die Graph-Gattung kommt per Owner-Entscheid Q5
//           NACH der Abgabe (Bauplan Paragraf 1.7). Ein Bau-Wunsch auf Graph hat am Ist NICHTS, worin er
//           sich melden koennte -- weder eine Fehlerklasse noch eine Zeile.
//   zu (2): die vier Kompositions-Fabriken brechen bei falscher Aritaet COMPILE-hart
//           (set_permutation_engine.hpp:46 `== 13`, sequence_:44 `== 9`, adapter_:58 `== 11`,
//           view_:44 `== 5`). Auf der HOST-/Planer-Seite, wo eine Achsen-Liste erst zur Laufzeit
//           entsteht, gibt es dieses Urteil bisher gar nicht.
//
// FORM = das gebaute Repo-Muster, nicht ein neues: `std::optional<CompilerCompilerErrorClass>` als
// Gate-Rueckgabe, exakt wie measurement/simd_build_gate.hpp `admit_organ_on_machine` (:160-165), das der
// build_orchestrator an der Bau-Delegation bereits konsumiert (build_orchestrator.hpp:478-486 ->
// `r.outcome = std::unexpected(BuildError{*gate_err})`). Ein abgelehnter Baupfad traegt zusaetzlich
// BuildCellStatus::NichtGebaut -- die FK-1-Marker-Zeile statt einer verschwiegenen Luecke
// (cache_engine_builder_iterator.hpp:1590-1603 ist die Stelle, die genau so eine Zeile baut).
//
// SINGLE SOURCE DER SLOT-ZAHLEN ist GenusBindingTraits<G>::slot_count -- die Laufzeit-Tabelle unten wird
// COMPILE-TIME daraus gefuellt, nie handgepflegt. Die Cross-Pins am Dateiende belegen, dass beide Sichten
// (compile-time Bindung, runtime Tabelle) nicht auseinanderlaufen koennen. GenusBindingTraits selbst wird
// NICHT angefasst (TABU-Registry; Q-8-persistence_target-Zeilen bleiben byte-gleich).
//
// STAND DES VERBRAUCHERS (deklariert, nicht verschwiegen): der RUNTIME-Verbraucher dieses Gates ist die
// gattungs-aware Modul-Quellen-Emission. Die existiert am a-Teil noch nicht -- der Emitter-Pfad ist
// SA-implizit (builder/codegen/adhoc_emitter.hpp + profile_facade/lazy_adhoc_source_gen.hpp haengen an
// kCompositionAxisNames). Das Gate steht deshalb VOR seinem Emitter, so wie simd_build_gate vor seinem
// ersten required-Flag stand: compile-time konsumiert (die Cross-Pins unten), runtime durch die Test-TU
// an ECHTEN Werten gefahren, und bereit fuer die Emitter-Naht des b-Teils. Ein Fenster-Nachzuegler, der
// die Klassen erst spaeter brauchte, waere nach C11/G8 gesperrt -- deshalb liegt die Taxonomie HIER.
//
// ABI-NEUTRAL (a-Teil): header-only, constexpr, reine Host-Seite. Kein Byte an abi_adapter.hpp, an
// Wire-PODs, an abi/*_decl.hpp oder an einer Stempel-/Fingerprint-Flaeche.

#include "genus_binding_traits.hpp" // GenusBindingTraits<G> (READ-ONLY, TABU) + GenusBound<G> + cea

#include <cache_engine/measurement/axis_error.hpp> // CompilerCompilerErrorClass / BuildCellStatus / Etiketten

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

namespace cem = ::comdare::cache_engine::measurement;

/// kGenusBuildSlotCounts -- die Slot-Zahl je Ebene-2-Gattung, COMPILE-TIME aus der Bau-Bindung gezogen.
/// Index == Enum-Wert (SearchAlgorithm=0 .. View=4). Handgepflegte Zahlen stuenden hier falsch: sie
/// wuerden beim naechsten Slot-Umbau still auseinanderlaufen, und genau das faengt der Cross-Pin unten.
inline constexpr std::array<std::size_t, 5> kGenusBuildSlotCounts = {
    GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>::slot_count,
    GenusBindingTraits<cea::AnatomyGenus::Set>::slot_count,
    GenusBindingTraits<cea::AnatomyGenus::Sequence>::slot_count,
    GenusBindingTraits<cea::AnatomyGenus::Adapter>::slot_count,
    GenusBindingTraits<cea::AnatomyGenus::View>::slot_count,
};

/// genus_is_build_bound(g) -- traegt die Gattung in DIESEM Bau eine Bau-Bindung? Am Ist: alle FUENF
/// Ebene-2-Gattungen ja (GenusBound<G> ist fuer alle fuenf erfuellt, genus_binding_traits.hpp:165-171).
/// Die Funktion ist damit fuer jeden GUELTIGEN Enum-Wert wahr -- sie ist die Wache gegen den UNGUELTIGEN:
/// eine Gattungs-Kennung, die aus einer XML-/Planer-Quelle stammt, ist ein int, kein Beweis.
[[nodiscard]] constexpr bool genus_is_build_bound(cea::AnatomyGenus g) noexcept {
    auto const idx = static_cast<std::size_t>(g);
    return idx < kGenusBuildSlotCounts.size() && kGenusBuildSlotCounts[idx] > 0;
}

/// genus_build_slot_count(g) -- der Slot-Satz der Gattung, 0 fuer eine ungebundene/ungueltige Kennung.
[[nodiscard]] constexpr std::size_t genus_build_slot_count(cea::AnatomyGenus g) noexcept {
    auto const idx = static_cast<std::size_t>(g);
    return idx < kGenusBuildSlotCounts.size() ? kGenusBuildSlotCounts[idx] : std::size_t{0};
}

/// admit_genus_build_path(g) -- (1) darf fuer diese Gattung ueberhaupt gebaut werden?
/// LEER = ja. Sonst D1 GattungsBindungFehlt (Log, Experiment baut die uebrigen Permutationen weiter).
[[nodiscard]] constexpr std::optional<cem::CompilerCompilerErrorClass>
admit_genus_build_path(cea::AnatomyGenus g) noexcept {
    if (!genus_is_build_bound(g)) return cem::CompilerCompilerErrorClass::GattungsBindungFehlt;
    return std::nullopt;
}

/// admit_gattung_build_path(gattung) -- dieselbe Frage eine Ebene HOEHER (Ebene 1, Aussen-Interface).
/// Graph hat am Ist KEINE Tier-Unterklasse (gattung_of bildet keine darauf ab) und damit keinen Baupfad;
/// per Owner-Entscheid Q5 kommt die Gattung NACH der Abgabe. Bis dahin ist ein Graph-Bau-Wunsch ein
/// DEKLARIERTER D1-Zustand statt eines stillen Nichts -- der Enumerator selbst bleibt unangetastet (TABU).
[[nodiscard]] constexpr std::optional<cem::CompilerCompilerErrorClass>
admit_gattung_build_path(cea::AnatomyGattung gattung) noexcept {
    for (std::size_t i = 0; i < kGenusBuildSlotCounts.size(); ++i)
        if (cea::gattung_of(static_cast<cea::AnatomyGenus>(i)) == gattung && kGenusBuildSlotCounts[i] > 0)
            return std::nullopt;
    return cem::CompilerCompilerErrorClass::GattungsBindungFehlt;
}

/// admit_genus_slot_arity(g, n) -- (2) passt die Aritaet der gelieferten Permutation zur Gattung?
/// Die Bindungs-Frage kommt ZUERST: ohne Bindung gibt es keinen Slot-Satz, gegen den man messen koennte,
/// und "Aritaet falsch" waere dann eine irrefuehrende Meldung ueber eine Gattung, die es gar nicht gibt.
[[nodiscard]] constexpr std::optional<cem::CompilerCompilerErrorClass>
admit_genus_slot_arity(cea::AnatomyGenus g, std::size_t gelieferte_aritaet) noexcept {
    if (auto const bindung = admit_genus_build_path(g)) return bindung;
    if (gelieferte_aritaet != genus_build_slot_count(g)) return cem::CompilerCompilerErrorClass::GattungsSlotAritaet;
    return std::nullopt;
}

/// GenusBuildVerdict -- das VOLLE D1-Urteil eines Gattungs-Baupfads: die Fehlerklasse (leer = zugelassen)
/// UND der Zell-Status der Permutations-Zeile. Beides zusammen ist die FK-8-Meldung: eine abgelehnte
/// Permutation bekommt einen SICHTBAREN Datensatz ("nicht_gebaut") plus die klassifizierte Log-Zeile --
/// nicht eine fehlende Zeile, die eine Auswertung nicht von "nie geplant" unterscheiden koennte
/// (dieselbe Doktrin, die A15/FK-1 fuer BuildCellStatus eingefuehrt hat).
struct GenusBuildVerdict {
    std::optional<cem::CompilerCompilerErrorClass> error_class{};
    cem::BuildCellStatus                           build_status = cem::BuildCellStatus::Gebaut;

    [[nodiscard]] constexpr bool zugelassen() const noexcept { return !error_class.has_value(); }
};

/// genus_build_verdict(g, n) -- das Urteil ueber EINEN Gattungs-Baupfad samt Zell-Status.
[[nodiscard]] constexpr GenusBuildVerdict genus_build_verdict(cea::AnatomyGenus g,
                                                              std::size_t       gelieferte_aritaet) noexcept {
    GenusBuildVerdict v;
    v.error_class  = admit_genus_slot_arity(g, gelieferte_aritaet);
    v.build_status = v.zugelassen() ? cem::BuildCellStatus::Gebaut : cem::BuildCellStatus::NichtGebaut;
    return v;
}

/// genus_build_cell(v) -- der CSV-Zell-Token der Permutations-Zeile. Bei Ablehnung "nicht_gebaut"
/// (D1-Vokabular), NIE eine Null und NIE das D2-"failed": es wurde ja nicht gemessen, sondern nicht gebaut.
[[nodiscard]] constexpr std::string_view genus_build_cell(GenusBuildVerdict const& v) noexcept {
    return cem::build_cell_status_token(v.build_status);
}

/// genus_build_log_line(v, g, n) -- die klassifizierte D1-Log-Zeile eines abgelehnten Baupfads.
/// FORM (stabil und grep-bar, Praefix aus der bestehenden D1-Politik-Single-Source):
///   "Compiler-Compiler-Fehler[<klassen_etikett>] gattung=<Name> aritaet=<n>/<soll> zelle=nicht_gebaut"
/// Bei Zulassung leer -- ein zugelassener Baupfad ist kein Ereignis und soll das Log nicht fluten.
[[nodiscard]] inline std::string genus_build_log_line(GenusBuildVerdict const& v, cea::AnatomyGenus g,
                                                      std::size_t gelieferte_aritaet) {
    if (v.zugelassen()) return {};
    std::string out;
    out.reserve(112);
    out += cem::LogAndContinueD1Policy::log_prefix();
    out += '[';
    out += cem::error_class_label(*v.error_class);
    out += "] gattung=";
    out += cea::genus_name(g);
    out += " aritaet=";
    out += std::to_string(gelieferte_aritaet);
    out += '/';
    out += std::to_string(genus_build_slot_count(g));
    out += " zelle=";
    out += genus_build_cell(v);
    return out;
}

// ---------------------------------------------------------------------------------------------------
// CROSS-PINS: die Laufzeit-Tabelle IST die compile-time-Bindung (sie kann nicht daneben laufen).
// ---------------------------------------------------------------------------------------------------
static_assert(kGenusBuildSlotCounts.size() == 5, "Ebene 2 hat fuenf Tier-Unterklassen (anatomy_base.hpp:78-84).");
static_assert(genus_build_slot_count(cea::AnatomyGenus::SearchAlgorithm) ==
              GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>::slot_count);
static_assert(genus_build_slot_count(cea::AnatomyGenus::Set) == GenusBindingTraits<cea::AnatomyGenus::Set>::slot_count);
static_assert(genus_build_slot_count(cea::AnatomyGenus::Sequence) ==
              GenusBindingTraits<cea::AnatomyGenus::Sequence>::slot_count);
static_assert(genus_build_slot_count(cea::AnatomyGenus::Adapter) ==
              GenusBindingTraits<cea::AnatomyGenus::Adapter>::slot_count);
static_assert(genus_build_slot_count(cea::AnatomyGenus::View) ==
              GenusBindingTraits<cea::AnatomyGenus::View>::slot_count);
// Und die Bindung selbst ist fuer alle fuenf erfuellt -- das Gate ist gegenueber dem heutigen Bestand
// INERT (es lehnt keine gebaute Gattung ab), genau wie simd_build_gate es gegenueber den heutigen Organen ist.
static_assert(GenusBound<cea::AnatomyGenus::SearchAlgorithm> && GenusBound<cea::AnatomyGenus::Set> &&
              GenusBound<cea::AnatomyGenus::Sequence> && GenusBound<cea::AnatomyGenus::Adapter> &&
              GenusBound<cea::AnatomyGenus::View>);
static_assert(!admit_genus_build_path(cea::AnatomyGenus::Set).has_value());
static_assert(!admit_genus_build_path(cea::AnatomyGenus::View).has_value());

// Die beiden Ebene-1-Aussagen: Container und Map sind baubar, Graph ist es NICHT (E-24 C7-1:
// die Map-Gattung hiess bis hier nach ihrem einzigen Genus SearchAlgorithm).
static_assert(!admit_gattung_build_path(cea::AnatomyGattung::Container).has_value());
static_assert(!admit_gattung_build_path(cea::AnatomyGattung::Map).has_value());
static_assert(admit_gattung_build_path(cea::AnatomyGattung::Graph) ==
                  cem::CompilerCompilerErrorClass::GattungsBindungFehlt,
              "Graph hat am Ist keine Tier-Unterklasse (Q5, nach Abgabe) -- ein Bau-Wunsch darauf ist ein "
              "DEKLARIERTER D1-Zustand, kein stilles Nichts.");

// Die Aritaets-Aussage je Gattung -- gegen die echte Slot-Zahl, nicht gegen eine Zahl im Test.
static_assert(!admit_genus_slot_arity(cea::AnatomyGenus::Set, genus_build_slot_count(cea::AnatomyGenus::Set)));
static_assert(admit_genus_slot_arity(cea::AnatomyGenus::Set, genus_build_slot_count(cea::AnatomyGenus::Sequence)) ==
                  cem::CompilerCompilerErrorClass::GattungsSlotAritaet,
              "Ein Sequence-Achsensatz an der Set-Gattung ist ein Aritaets-Fehler, kein Bindungs-Fehler.");
static_assert(genus_build_verdict(cea::AnatomyGenus::Set, 0).build_status == cem::BuildCellStatus::NichtGebaut);
static_assert(genus_build_cell(genus_build_verdict(cea::AnatomyGenus::Set, 0)) == std::string_view{"nicht_gebaut"});
static_assert(genus_build_verdict(cea::AnatomyGenus::Set, genus_build_slot_count(cea::AnatomyGenus::Set))
                      .build_status == cem::BuildCellStatus::Gebaut,
              "Ein zugelassener Baupfad bleibt byte-identisch zum Bestand (Gebaut == 0, Default-Init).");
// Die C5-Haelften bleiben getrennt: der Bau-Zell-Token ist NIE der Mess-Zell-Token.
static_assert(genus_build_cell(genus_build_verdict(cea::AnatomyGenus::Set, 0)) !=
                  cem::sample_status_token(cem::SampleStatus::Failed),
              "FK-8 meldet 'nicht gebaut', FK-7 meldet 'gemessen und gescheitert' -- nie dasselbe Wort.");

} // namespace comdare::cache_engine::builder::experiment
