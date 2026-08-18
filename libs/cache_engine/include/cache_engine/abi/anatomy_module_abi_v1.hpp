#pragma once
// V41.F.6.1.R5.D — Anatomy Module ABI v1 (Minimal Factory-Pattern) — MODUL-AUTOR-Seite.
//
// User-Direktive 2026-05-27 (Doku 14 §41.5):
// "Per Doku 14 §41.5 reicht: comdare_create_anatomy() → IAnatomyBase* + comdare_destroy_anatomy(IAnatomyBase*)."
//
// **R6 Inkrement 2b (2026-05-30) — Entkopplung:** Die leichte ABI-Schnittstelle (Factory-Deklarationen,
// Version/Magic, Interface-Typen, AnatomyAbiVersion-Helper) lebt jetzt in `anatomy_module_abi_v1_decl.hpp`
// (das der host-seitige Loader inkludiert — OHNE die schwere Adapter-Template). DIESES Header ist die
// MODUL-AUTOR-Seite: es inkludiert das Decl + die schwere `abi_adapter.hpp` + `SearchAlgorithmAnatomy` +
// `AdHocComposition` und stellt die `COMDARE_DEFINE_ANATOMY_MODULE`-Makros bereit, mit denen eine generierte
// Permutations-.cpp ihre .so/.dll-Export-Symbole materialisiert. Inhaltlich unveraendert fuer DLLs/Tests.
//
// Vorteile gegenueber module_abi_v1.hpp:
// 1. Eine .so/.dll exportiert nur 2 Symbole (Factory + Destroy), nicht 9+
// 2. Anatomy-API-Erweiterungen brechen die ABI NICHT (Virtual-Interface erweiterbar ueber neue Sub-Interfaces)
// 3. C++ RAII statt manueller Pointer-Tracking
// 4. Type-Safety: kein void*-Cast erforderlich
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §41.5 + §43; docs/architecture/24 §8.6
// @task #706 V41.F.6.1.R5.D
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]

#include "anatomy_module_abi_v1_decl.hpp" // leichte ABI-Schnittstelle (Version/Magic/Factory-Decls/Helper)
#include "anatomy_fingerprint.hpp"        // K7b-3: consteval SHA-512-Fingerprint der Stempel-Glieder (A13-M3: 3 Zeilen)
#include "anatomy_stamp_entries.hpp" // G2-1b/A4: consteval count/parse_stamp_entries + stamp_entries_ptr (Array-Form)
#include "system_cell_values.hpp"    // W10-C1: Zellwert-Define-Naht + consteval-Vervollstaendiger der System-Zeile
#include "../../../anatomy/abi_adapter.hpp" // SearchAlgorithmAbiAdapter (Makro-Materialisierung)
#include "../../../anatomy/search_algorithm_anatomy.hpp"
#include "../../../anatomy/composition_factory.hpp" // R5.G: AdHocComposition für Auto-Permutations-Codegen
#include "build_variant_inspection.hpp" // L-74a: COMDARE_DEFINE_BUILD_VARIANT_INSPECTION (BUILDVARIANT-Variante)

#include <new>

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_DEFINE_ANATOMY_MODULE — Convenience-Macro fuer Permutations-Binary
// ─────────────────────────────────────────────────────────────────────────────

/// COMDARE_DEFINE_ANATOMY_MODULE(CompositionType) generiert die 4 Pflicht-extern-C-Symbole eines
/// Anatomy-Permutations-Binary. CompositionType muss IsComposition erfuellen UND fuer SearchAlgorithm-
/// Gattung zugelassen sein.
///
/// Verwendung:
/// ```cpp
/// // generated_perm_<hash>.cpp
/// #include <cache_engine/abi/anatomy_module_abi_v1.hpp>
/// #include "art_reference.hpp"  // konkrete Composition
/// COMDARE_DEFINE_ANATOMY_MODULE(comdare::cache_engine::compositions::ArtComposition)
/// ```
#define COMDARE_DEFINE_ANATOMY_MODULE(CompositionType)                                                                 \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        using AnatomyType = ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<CompositionType>;                 \
        return new (::std::nothrow)::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType>{};         \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...) — R5.G: Materialisiert eine AUTO-ENUMERIERTE Permutation
/// (AdHocComposition) als Permutations-Binary, OHNE benannten Composition-Header. Nimmt die 17
/// Achsen-Vendor-Typen VARIADISCH (T0..T16 — 15 Such-Achsen + queuing q1/q2, Doc 30 §8.0) und baut die
/// Composition intern als Alias — das löst das Komma-im-Makro-Argument-Problem von AdHocComposition<A,B,…>.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...)                                                                       \
    using ComdareAdHocPermutationComposition = ::comdare::cache_engine::anatomy::AdHocComposition<__VA_ARGS__>;        \
    COMDARE_DEFINE_ANATOMY_MODULE(ComdareAdHocPermutationComposition)

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, <17 Anatomie-Achsen>) — L-74a: EINE DLL trägt
/// SOWOHL die 18-Slot-SearchAlgorithm-Anatomie (4 ABI-Symbole, genus()==SearchAlgorithm, organ_count()==18 -- 15
/// Such-Achsen + queuing q1/q2, Doc 30 §8.0; INC-2c/2d: telemetry+isa sind System-Achsen) ALS AUCH die
/// Build-Identität der 3 Build-Achsen (page_type/09b/12)
/// als extern-"C"-Inspection-Symbol (comdare_build_variant_inspect). Beweist Doc 27 §0.1: die 3 Build-Achsen sind
/// Build-Parameter DERSELBEN Binary (Sub/Build-Varianten DESSELBEN Algorithmus), NICHT eine eigene Gattung.
/// Reihenfolge: die 3 Build-Achsen ZUERST (named), dann die 17 Anatomie-Achsen variadisch (Komma-Problem von
/// AdHocComposition<17>). Host: genus über den Loader + Build-Identität über GetProcAddress/dlsym aus DERSELBEN .dll.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, ...)                                              \
    COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(__VA_ARGS__)                                                                   \
    COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(comdare_build_variant_inspect, PT, SE, HW)

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(ShapeType, <17 Anatomie-Achsen>) — 234-V-a (Option A,
/// User-GO 07.07.): Materialisiert eine auto-enumerierte Permutation MIT Shape-Traeger (z.B.
/// axis_btree_order::BtreeOrderKt8). Der Shape faehrt als ADAPTER-Traeger mit (2. Template-Parameter des
/// SearchAlgorithmAbiAdapter) und waehlt ueber `organ_for_search_algo_shaped<S,Shape>` das Shaped-Organ
/// der Pool-Familie -- er ist KEIN 19. Composition-Slot: die 18-Slot-ABI-Invariante, organ_count()==18,
/// alle POD-Layouts und ABI-MAJOR bleiben unveraendert. Reihenfolge nach BUILDVARIANT-Praezedenz:
/// benannter Shape ZUERST, dann die 17 Anatomie-Achsen variadisch (Komma-Problem von AdHocComposition).
/// Bewusst SELBSTSTAENDIG (4 Symbole erneut definiert statt Basis-Makro-Refactor): der Golden-Pfad
/// (COMDARE_DEFINE_ANATOMY_MODULE/_ADHOC/_BUILDVARIANT) wird nicht angefasst.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(ShapeType, ...)                                                     \
    using ComdareAdHocPermutationComposition = ::comdare::cache_engine::anatomy::AdHocComposition<__VA_ARGS__>;        \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        using AnatomyType =                                                                                            \
            ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<ComdareAdHocPermutationComposition>;              \
        return new (                                                                                                   \
            ::std::nothrow)::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType, ShapeType>{};      \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }

/// COMDARE_ANATOMY_VERSION_STAMP_M(measurement_lit, system_lit, organ_lit) -- A13-M3 (Owner-E2 02.08.2026):
/// die VOLLFORM. Materialisiert das OPTIONALE extern-"C"-Probe-Symbol comdare_anatomy_version_lines() aus DREI
/// String-Literalen -- seit S-6a in der Kategorien-Ordnung MESS, SYSTEM, ORGAN (Mess-Tooling-HAUPT-, System-
/// und Organ-Stempel; KON21-03 #87). Die frueher hier stehende 4-arg-Form
/// _MERGE ist ERSATZLOS ENTFALLEN -- "Merge Zeile kann daher nicht existieren"; stehender toter Stempel-Code
/// waere ein dritter Ableitungsweg in Wartestellung (O-8-Schritt-12-Lehre). Die Merge-DURCHFUEHRUNG bleibt
/// unberuehrt (profile_facade/merge_plan.hpp, Owner-Q2).
///
/// Die Literale werden im Modul als static constexpr char[] hinterlegt (KEIN std::string im Modul); der
/// zurueckgegebene POD traegt nur Zeiger + Laengen. KEIN Loader-Pflicht-Symbol -> KEIN ABI-Bruch.
/// Stempel-Strings sind C-literal-sicher (nur =@;.+_ [] und alnum, keine Quotes/Backslashes) --
/// A13-M2 (Owner-Q1): '[' und ']' kamen mit dem Meta-Meta-Klammer-Anhang dazu und brauchen kein Escaping.
/// K7b-3 (Section 62-B, 2026-07-22): INNEN wird zusaetzlich der SHA-512-Fingerprint materialisiert
/// (anatomy_fingerprint_hex, consteval, ueber die Glied-Folge anatomy_fingerprint_glieder) und als POD-Feld
/// sha512_line/sha512_len abgelegt. Die EINGABE bleibt reine String-Literale -> der emittierte Makro-Call ist
/// byte-identisch (golden-neutral); der Fingerprint entsteht rein in der Makro-Expansion, nicht im emittierten
/// Quelltext.
///
/// W10-C2 (Bauplan-Dossier 20260803, Sektion 2) -- DIE ZELLWERT-NAHT. Das System-Literal wird nicht mehr
/// direkt hinterlegt, sondern INNEN per consteval um die System-ZELLWERTE der gebauten Zelle
/// VERVOLLSTAENDIGT (kSC, abi/system_cell_values.hpp): "code" -> "code.<token>". kFP rechnet ueber die
/// VERVOLLSTAENDIGTE Zeile, kSE parst sie, und das POD traegt sie. Damit diskriminiert der SHA512 ab
/// W10-C4 OS-Familie, ISA und SIMD-Zelle SELBST -- die B1-Kollision linux==macos ist mechanisch tot.
///
/// WARUM HIER UND NICHT IM EMITTER: der Tier-Emitter bleibt SYSTEM-BLIND (W4-B-Invariante). Die Zellwerte
/// kommen ueber das Compile-Define COMDARE_SYSTEM_CELL_VALUES herein, das die CEB-Bau-Naht je Zelle setzt
/// (perm_compile) -- exakt die sanktionierte Pre-Build-Define-Klasse (Muster COMDARE_OVERLAY_SOURCE_HASH /
/// COMDARE_GN_ALGO_SIG). Der emittierte Makro-CALL bleibt byte-identisch; die Round-Trip-Byte-Wache und der
/// golden-CRC-Anker sind unberuehrt.
///
/// OHNE DEFINE IST ALLES BYTE-IDENTISCH (Identitaet aus C1e): kSC.view() == system_lit, also derselbe kFP,
/// dasselbe kSE, dasselbe POD. KEIN POD-/Layout-Anfassen -- kSC.chars ist ein nullterminiertes char-Array
/// mit statischer Lebensdauer, genau wie das frueher hier stehende kS[].
///
/// R-3 (Format 4, 07.08.2026) -- DIE VIER SCHWANZ-GLIEDER REISEN AB HIER ALLE EXPLIZIT. Bis hierher rief
/// dieses Makro die 3-arg-Form; Toolchain/bvset/Overlay kamen als DEFAULT herein. Fuer das neue
/// Mess-Gates-Glied ist der Default-Weg VERBOTEN: sein Wert (abi/mess_gates_glied.hpp, kMessGatesTuGlied)
/// ist TU-abhaengig und hat interne Bindung -- als Default-Argument einer inline-Funktion waere er ein
/// stiller ODR-Verstoss (ausfuehrliche Herleitung bei anatomy_fingerprint_glieder). Der EINE Ort, an dem
/// genau eine Uebersetzungseinheit gemeint ist, ist der Expansionsort dieses Makros; also wird er hier
/// gereicht. Die drei anderen stehen der Symmetrie halber daneben und rechnen byte-identisch zum
/// Default-Weg -- kein Fingerprint bewegt sich dadurch, nur durch das neunte Glied selbst.
/// GOLDEN-NEUTRAL bleibt es: der EMITTIERTE Makro-CALL (2-/3-arg) ist unveraendert, der Fingerprint
/// materialisiert weiterhin erst in der Makro-EXPANSION -- die Round-Trip-Byte-Wachen und der
/// binary_id-CRC-Anker sehen davon nichts.
#define COMDARE_ANATOMY_VERSION_STAMP_M(measurement_lit, system_lit, organ_lit)                                        \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::abi::AnatomyVersionLines const*                     \
    comdare_anatomy_version_lines() noexcept {                                                                         \
        static constexpr char kM[] = measurement_lit;                                                                  \
        static constexpr auto kSC  = ::comdare::cache_engine::abi::complete_system_stamp_line_array<                   \
            ::comdare::cache_engine::abi::complete_system_stamp_line_size(                                             \
                system_lit, ::comdare::cache_engine::abi::kSystemCellValuesFromDefine)>(                               \
            system_lit, ::comdare::cache_engine::abi::kSystemCellValuesFromDefine);                                    \
        static constexpr char kO[] = organ_lit;                                                                        \
        static constexpr auto kFP  = ::comdare::cache_engine::abi::anatomy_fingerprint_hex(                            \
            ::comdare::cache_engine::abi::MessZeile{measurement_lit},                                                  \
            ::comdare::cache_engine::abi::SystemZeile{kSC.view()},                                                     \
            ::comdare::cache_engine::abi::OrganZeile{organ_lit},                                                       \
            ::comdare::cache_engine::abi::ToolchainGlied{::comdare::cache_engine::abi::kToolchainStampGlied},          \
            ::comdare::cache_engine::abi::BvsetGlied{::comdare::cache_engine::abi::kBuildVariantSetSignatureGlied},    \
            ::comdare::cache_engine::abi::OverlayHash{::comdare::cache_engine::abi::kOverlaySourceHash},               \
            ::comdare::cache_engine::abi::MessGatesGlied{::comdare::cache_engine::abi::kMessGatesTuGlied},             \
            ::comdare::cache_engine::abi::KompositMapGlied{::comdare::cache_engine::abi::kHybridKompositGlied});       \
        static constexpr auto kME =                                                                                    \
            ::comdare::cache_engine::abi::parse_stamp_entries<::comdare::cache_engine::abi::count_stamp_entries(kM)>(  \
                kM);                                                                                                   \
        static constexpr auto kSE =                                                                                    \
            ::comdare::cache_engine::abi::parse_stamp_entries<::comdare::cache_engine::abi::count_stamp_entries(       \
                kSC.view())>(kSC.chars);                                                                               \
        static constexpr auto kOE =                                                                                    \
            ::comdare::cache_engine::abi::parse_stamp_entries<::comdare::cache_engine::abi::count_stamp_entries(kO)>(  \
                kO);                                                                                                   \
        static constexpr ::comdare::cache_engine::abi::AnatomyVersionLines kL{                                         \
            .stamp_layout_version    = ::comdare::cache_engine::abi::kAnatomyVersionLinesLayout,                       \
            .reserved                = 0u,                                                                             \
            .measurement_line        = kM,                                                                             \
            .measurement_len         = sizeof(kM) - 1,                                                                 \
            .system_line             = kSC.chars,                                                                      \
            .system_len              = kSC.size(),                                                                     \
            .organ_line              = kO,                                                                             \
            .organ_len               = sizeof(kO) - 1,                                                                 \
            .sha512_line             = kFP.data(),                                                                     \
            .sha512_len              = kFP.size() - 1,                                                                 \
            .measurement_entries     = ::comdare::cache_engine::abi::stamp_entries_ptr(kME),                           \
            .measurement_entry_count = kME.size(),                                                                     \
            .system_entries          = ::comdare::cache_engine::abi::stamp_entries_ptr(kSE),                           \
            .system_entry_count      = kSE.size(),                                                                     \
            .organ_entries           = ::comdare::cache_engine::abi::stamp_entries_ptr(kOE),                           \
            .organ_entry_count       = kOE.size(),                                                                     \
            .komposit_line           = ::comdare::cache_engine::abi::kHybridKompositGlied.data(),                      \
            .komposit_len            = ::comdare::cache_engine::abi::kHybridKompositGlied.size()};                     \
        return &kL;                                                                                                    \
    }

/// COMDARE_ANATOMY_VERSION_STAMP(system_lit, organ_lit) -- W12-A2 2-arg-Kurzform: leitet an die 3-arg
/// _M-Vollform mit LEEREM Mess-Tooling-Stempel weiter (measurement_line -> "", measurement_len -> 0).
/// Der Emitter (adhoc_emitter.hpp) haengt diese Makro-Zeile NACH COMDARE_DEFINE_ANATOMY_MODULE_ADHOC an; bis die
/// Mess-Tooling-HAUPT-Auffaecherung (S4/P-MESSTOOL) den gewaehlten Tooling-Stempel durchreicht, bleibt das
/// Mess-Feld leer (ehrlich: kein Tooling einkompiliert), waehrend das POD-Layout bereits final ist.
///
/// S-6a: DIE ARGUMENT-FOLGE IST GEDREHT -- (system_lit, organ_lit) statt (organ_lit, system_lit), weil die
/// Kategorien-Ordnung MESS, SYSTEM, ORGAN lautet (KON21-03, #87 = Makro-/Argumentfolge) und die Mess-Zeile
/// in dieser Kurzform gerade die weggelassene ERSTE ist. Damit ist sie NICHT mehr rueckwaerts-kompatibel:
/// der emittierte Quelltext aendert sich, und das ist der DEKLARIERTE golden-Bruch dieser Scheibe
/// (KON5-04, Kosten-Ebene 1) -- die Byte-Identitaets-Wachen des Emitters und der golden-CRC-Anker
/// bewegen sich mit und werden im golden-Regen-Schnitt neu gesetzt, NICHT hier.
///
/// ACHTUNG, WARNUNG AN JEDEN, DER EINE EIGENE EMISSION SCHREIBT: beide Argumente sind Zeichenketten, ein
/// vertauschter Aufruf uebersetzt also. Die Sperre dagegen sitzt eine Ebene tiefer und nur fuer den
/// Fingerprint (die Traeger-Typen MessZeile/SystemZeile/OrganZeile); der POD selbst nimmt, was er kriegt.
/// Alle Emissions-Stellen im Haus sind mit diesem Commit gedreht (adhoc_emitter.hpp,
/// adhoc_emitter_shaped.hpp, sota_catalog.hpp) -- wer eine vierte baut, prueft die Reihenfolge am
/// gerenderten Stempel, nicht am Kompilieren.
#define COMDARE_ANATOMY_VERSION_STAMP(system_lit, organ_lit) COMDARE_ANATOMY_VERSION_STAMP_M("", system_lit, organ_lit)
