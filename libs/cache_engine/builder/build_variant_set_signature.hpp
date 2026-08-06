#pragma once
// G2 Folge-B (Lager-Gate, Stempel-Paket A7-B) -- die MENGEN-Signatur ueber die drei Enabled-Typlisten der Build-Achsen
// page_type(01) / simd_extension(09b) / general_hardware(12). Sie ist das Gegenstueck zu A7s Einzel-POD-Signatur
// (build_variant_sidecar.hpp::compose_variant_signature) und beantwortet eine ANDERE Frage:
//
//   A7  (compose_variant_signature): "welche EINE Zelle/ISA traegt diese Binary?"  -- Eingabe = 1 POD.
//   A7-B (variant_set_signature):    "welche Achsen-Varianten hat der TREIBER ueberhaupt einkompiliert?"
//                                    -- Eingabe = die CMake-enabled Registry-Listen.
//
// WARUM MENGE UND NICHT EINZEL-POD: ein Dev-/Voll-Build enabled ALLE Registry-Varianten gleichzeitig -- eine einzelne
// POD-Signatur waere dort nicht wohldefiniert (welche der 6 page_types?). Es existiert zudem KEINE Name->Typ-
// Rueckabbildung und KEINE Runtime-Zell-Achse fuer PT/HW, also waere jede Runtime-String->Typ-Ableitung die verbotene
// Zweitwahrheit. Die Mengen-Signatur spiegelt stattdessen die DRIVER-CONFIG-IDENTITAET: per-Maschine restringierte
// Builds (andere COMDARE_AXIS_*_ENABLE_*-Flags) ergeben eine andere Signatur => .variant-Sidecar-Mismatch => Neubau.
// Das ist das Cross-Maschinen-Gate. Voll-Enable auf beiden Maschinen => identische Signatur => inert-korrekt (kein
// Fehlalarm). Die per-Zelle-ISA-Deckung leistet NICHT diese Signatur, sondern der Lager-Schluessel (Zell-Koordinaten).
//
// §66-N3-REIN: CT-Eingabe (Typlisten) -> CT-Ausgabe (consteval-String). Kein std::variant, kein visit, keine
// Runtime->CT-Richtung. Je Achse SEPARAT GEKLAMMERT (N3 Punkt 4) -- die Achsen werden nie zu einem Feld fusioniert.
// Deterministisch: mp_for_each laeuft die Registry-Listen in Deklarations-Reihenfolge ab, es wird nicht sortiert.
//
// EINE FELDQUELLE: die je-Element-Properties kommen aus anatomy::page_axis_fields/simd_axis_fields/hw_axis_fields --
// exakt denselben Lesern, aus denen build_variant_definition<PT,SE,HW>() seinen POD zusammensetzt. Die Feldauswahl je
// Achse ist die A7-Auswahl (page_kind / simd_width_bits+simd_avx512+simd_avx10_version / hw_cache_line+hw_numa_capable);
// page_is_branch/page_is_leaf bleiben wie in A7 draussen (aus page_kind ableitbar -> keine Redundanz).
//
// REGISTRY-FREI: dieser Header kennt die konkreten Achsen-Registries NICHT (nur die Listen als Template-Parameter).
// Die Treiber-Konstante ueber die realen EnabledPageTypes/EnabledExtensions/EnabledPlatforms steht in
// driver_build_variant_signature.hpp -- nur die zieht die CMake-generierten Flags-Header nach sich.

#include "anatomy/build_variant_definition.hpp" // *_axis_fields (EINE Feldquelle) + kBuildVariantDefinitionVersion

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

// Format-Version der MENGEN-Signatur, unabhaengig von der POD-Version (anatomy::kBuildVariantDefinitionVersion, die
// separat mitgestempelt wird). Bumpen, sobald sich Trennzeichen, Feldauswahl oder Achsen-Reihenfolge aendern --
// ein Bump ist per Konstruktion ein Signatur-Bruch und erzwingt damit Neubau statt stiller Fehl-Vergleiche.
inline constexpr std::uint64_t kBuildVariantSetSignatureVersion = 1;

/// NB2-3: die Zeichen, die in der Mengen-Signatur STRUKTUR sind -- Segment-Trenner, Zuweisung, Element- und
/// Achsen-Klammern. '@' kommt in der heutigen Form nicht vor, ist aber im Nachbar-Glied [5] Struktur und wird
/// deshalb gleich mitgesperrt: ein Name, der ihn traegt, waere dort sofort ein Problem.
inline constexpr std::string_view kVariantSetSignatureStrukturZeichen = ";={}[]@";

/// NB2-3: ein Element-NAME ist wohlgeformt, wenn er nicht leer ist und weder Struktur- noch Steuerzeichen traegt.
/// LEER waere ebenfalls unzulaessig -- `{;page_kind=1}` liesse die Grenze zwischen Name und erstem Feld offen.
[[nodiscard]] constexpr bool variant_set_signature_name_ist_wohlgeformt(std::string_view n) noexcept {
    if (n.empty()) return false;
    for (char const c : n) {
        if (c == '\n' || c == '\r') return false;
        if (kVariantSetSignatureStrukturZeichen.find(c) != std::string_view::npos) return false;
    }
    return true;
}

// -- NB-3/T2-D: DIE PAARWEISE NAMENS-EINDEUTIGKEIT ---------------------------------------------------------------
//
// DER BEFUND (Codex-Zweitreview [MITTEL] "bvset paarweise Namens-Eindeutigkeit CT"): NB2-3 hat geprueft, dass ein
// Name den Zeichenvorrat einhaelt -- also dass er die Signatur nicht ZERLEGT. Ungeprueft blieb die andere Haelfte
// derselben Zusage: dass zwei VERSCHIEDENE Wrapper-Typen nicht denselben Namen tragen. Der Name ist genau deshalb
// im Element, weil die Properties allein nicht diskriminieren (der Kommentar oben sagt es: "unterscheidet Wrapper,
// deren Properties zufaellig gleich sind"). Tragen zwei Typen denselben Namen UND dieselben Feldwerte, dann rendert
// die Enable-Menge {A} byte-identisch zu {B} -- zwei verschieden gebaute Treiber, eine Signatur, seit Format 3 also
// derselbe Fingerprint und ein falscher Skip. Der Name als Diskriminator war damit eine Annahme, keine Zusage.
//
// WO DIE WACHE BEISST -- und wo ehrlicherweise nicht: sie laeuft ueber die Liste, die WIRKLICH emittiert wird
// (dieselbe Doktrin wie COMDARE_BVSET_NAME_WACHE: nur was in eine Signatur wandert, muss etwas beweisen). Damit
// faellt jedes Namens-Duplikat auf, dessen beide Traeger gleichzeitig enabled sind. Ein Duplikat, dessen Traeger
// sich per CMake-Flags gegenseitig AUSSCHLIESSEN, sieht diese Ebene per Konstruktion nicht -- sie bekommt immer
// nur eine der beiden Listen zu sehen. Genau diese Restluecke schliesst driver_build_variant_signature.hpp, indem
// es die Wache zusaetzlich ueber die VOLLEN Registry-Listen (All*) stellt: dort sind beide Traeger gleichzeitig
// sichtbar, unabhaengig davon, was gerade enabled ist. Zwei Ebenen, weil dieser Header registry-frei bleiben muss.
//
// LAUFZEIT-KOSTEN: keine. Der Vergleich ist O(n^2) ueber hoechstens eine Handvoll Registry-Eintraege und laeuft
// ausschliesslich im konstanten Ausdruck.

/// Sind die name()-Werte aller Listen-Elemente paarweise verschieden? Verglichen wird der TEXT, nicht der Zeiger:
/// zwei Wrapper duerfen sehr wohl auf dasselbe Literal zeigen -- dann sind sie eben nicht unterscheidbar, und
/// genau das soll auffallen.
template <class L>
[[nodiscard]] constexpr bool variant_set_signature_namen_paarweise_eindeutig() noexcept {
    constexpr std::size_t                          n = mp::mp_size<L>::value;
    std::array<std::string_view, (n == 0 ? 1 : n)> namen{};
    std::size_t                                    i = 0;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, L>>(
        [&namen, &i](auto id) { namen[i++] = std::string_view{decltype(id)::type::name()}; });
    for (std::size_t a = 0; a + 1 < n; ++a) {
        for (std::size_t b = a + 1; b < n; ++b) {
            if (namen[a] == namen[b]) return false;
        }
    }
    return true;
}

/// Die Paar-Wache je Achsen-Liste. ACHSE ist ein String-Literal, damit die Fehlerzeile die Achse beim Namen nennt --
/// "irgendwo im bvset stehen zwei gleiche Namen" waere im 3-Achsen-Fall eine Suche statt eines Befundes.
#define COMDARE_BVSET_PAAR_WACHE(L, ACHSE)                                                                             \
    static_assert(                                                                                                     \
        ::comdare::cache_engine::builder::experiment::variant_set_signature_namen_paarweise_eindeutig<L>(),            \
        "NB-3/T2-D: zwei Registry-Wrapper der Achse " ACHSE " tragen denselben name(). Der Name ist der "              \
        "Diskriminator, der Wrapper mit zufaellig gleichen Properties unterscheidet -- bei gleichem Namen UND "        \
        "gleichen Feldern rendern zwei VERSCHIEDENE Enable-Mengen dieselbe bvset-Signatur, also seit Format 3 "        \
        "denselben Fingerprint (falscher Skip). Den NAMEN eindeutig machen, nicht die Wache.")

namespace detail {

// -- Zwei-Pass-Senken: erst zaehlen (Laenge), dann fuellen (exakt dimensioniertes std::array). Beide Paesse laufen
//    ueber DENSELBEN Emitter -> die Laenge kann nicht von der Ausgabe abweichen (kein Puffer-Ueberlauf moeglich).
struct CtCountSink {
    std::size_t    n = 0;
    constexpr void put(char) noexcept { ++n; }
};

struct CtFillSink {
    char*          p = nullptr;
    std::size_t    n = 0;
    constexpr void put(char c) noexcept { p[n++] = c; }
};

template <class Sink>
constexpr void ct_put_text(Sink& s, std::string_view t) noexcept {
    for (char const c : t) s.put(c);
}

// Dezimal-Ausgabe ohne <charconv>/<string> (beide sind im consteval-Pfad unbrauchbar bzw. allokierend). 20 Ziffern
// decken den vollen uint64-Bereich (max. 18446744073709551615 = 20 Stellen).
template <class Sink>
constexpr void ct_put_uint(Sink& s, std::uint64_t v) noexcept {
    char        buf[20]{};
    std::size_t n = 0;
    do {
        buf[n++] = static_cast<char>('0' + static_cast<char>(v % 10u));
        v /= 10u;
    } while (v != 0u);
    while (n != 0u) s.put(buf[--n]);
}

// -- Je-Element-Form: `{<typ-name>;<achsen-felder>}`. Der Typ-Name (static constexpr name() jedes Registry-Wrappers)
//    macht die Signatur lesbar UND unterscheidet Wrapper, deren Properties zufaellig gleich sind (z.B. zwei 256-bit-
//    Erweiterungen ohne AVX-512) -- ohne ihn waere das Gate an genau dieser Stelle blind.
//
// -- NB2-3: DIE NAMENS-WACHE (Codex-Zweitreview [MITTEL], am Code bestaetigt) -------------------------------------
//
// DER BEFUND: der Typ-Name wurde UNESCAPED zwischen die Strukturzeichen gesetzt, und die AEUSSERE Wache des
// Preimage-Glieds (abi::anatomy_glied_zeichen_erlaubt) erlaubt genau diese Zeichen. Ein Wrapper mit
// name() == "a;page_kind=1}{b" rendert deshalb byte-identisch zu ZWEI Wrappern (a,1) und (b,2) -- zwei
// verschiedene Enable-Mengen, eine Signatur, also derselbe Fingerprint und ein falscher Skip. Seit Format 3 ist
// diese Signatur IDENTITAET (Glied [6]), nicht mehr blosse Provenienz; die Luecke ist damit dieselbe Klasse wie
// die NB/CX-2-Kollisionen des Toolchain-Glieds.
//
// WARUM ZEICHENVORRAT STATT ESCAPING (wortgleiche Begruendung wie in abi/toolchain_stamp_glied.hpp): Escaping
// braucht einen Unescaper, den niemand hat -- die Signatur wird nirgends zerlegt, sondern nur verglichen. Der
// Zeichenvorrat ist die schaerfere und billigere Zusage. Er ist hier zusaetzlich COMPILE-HART durchsetzbar, weil
// der Name eine Compile-Zeit-Konstante des Wrapper-Typs ist: ein Verstoss bricht den Bau mit Namen, statt eine
// Kollision in die Identitaet zu schreiben. Die Bestands-Namen der drei Registries sind sauber (reine
// Bezeichner) -> die Wache ist DIGEST-NEUTRAL und beisst nur bei kuenftigen Fehlgriffen.

/// Die Wache je Wrapper-Typ. Sie steht als static_assert IM Emitter, damit sie genau die Typen trifft, die
/// wirklich in eine Signatur wandern -- eine Registry-Liste, die niemand emittiert, muss auch nichts beweisen.
#define COMDARE_BVSET_NAME_WACHE(W)                                                                                    \
    static_assert(::comdare::cache_engine::builder::experiment::variant_set_signature_name_ist_wohlgeformt(W::name()), \
                  "NB2-3: der Registry-Wrapper-Name traegt ein STRUKTUR-Zeichen (';', '=', '{', '}', '[', ']', "       \
                  "'@') oder einen Zeilenumbruch. Er wird UNESCAPED in die bvset-Mengen-Signatur gesetzt -- zwei "     \
                  "verschiedene Enable-Mengen koennten damit dieselbe Signatur ergeben, also denselben "               \
                  "Fingerprint (falscher Skip). Den NAMEN korrigieren, nicht die Wache.")

template <class Sink, class PT>
constexpr void ct_put_page_element(Sink& s) noexcept {
    COMDARE_BVSET_NAME_WACHE(PT);
    anatomy::PageAxisFields const f = anatomy::page_axis_fields<PT>();
    s.put('{');
    ct_put_text(s, PT::name());
    ct_put_text(s, ";page_kind=");
    ct_put_uint(s, f.page_kind);
    s.put('}');
}

template <class Sink, class SE>
constexpr void ct_put_simd_element(Sink& s) noexcept {
    COMDARE_BVSET_NAME_WACHE(SE);
    anatomy::SimdAxisFields const f = anatomy::simd_axis_fields<SE>();
    s.put('{');
    ct_put_text(s, SE::name());
    ct_put_text(s, ";simd_width_bits=");
    ct_put_uint(s, f.simd_width_bits);
    ct_put_text(s, ";simd_avx512=");
    ct_put_uint(s, f.simd_avx512);
    ct_put_text(s, ";simd_avx10_version=");
    ct_put_uint(s, f.simd_avx10_version);
    s.put('}');
}

template <class Sink, class HW>
constexpr void ct_put_hw_element(Sink& s) noexcept {
    COMDARE_BVSET_NAME_WACHE(HW);
    anatomy::HwAxisFields const f = anatomy::hw_axis_fields<HW>();
    s.put('{');
    ct_put_text(s, HW::name());
    ct_put_text(s, ";hw_cache_line=");
    ct_put_uint(s, f.hw_cache_line);
    ct_put_text(s, ";hw_numa_capable=");
    ct_put_uint(s, f.hw_numa_capable);
    s.put('}');
}

// -- Je-Achse-Gruppe: `<achsen-name>[<elem><elem>...]`. Die Klammer ist die Achsen-Grenze (§66-N3 Punkt 4): eine
//    leere Achse bleibt als `name[]` sichtbar statt spurlos zu verschwinden. mp_transform<mp_identity,L> ist die
//    Mp11-Standardform, damit mp_for_each auch ueber NICHT default-konstruierbare Wrapper-Typen laufen kann
//    (es instanziiert mp_identity<T>, nie T selbst). Reihenfolge = Listen-Reihenfolge, es wird nicht sortiert.

template <class Sink, class PageList>
constexpr void ct_put_page_group(Sink& s) noexcept {
    COMDARE_BVSET_PAAR_WACHE(PageList, "page_type");
    ct_put_text(s, "page_type[");
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, PageList>>(
        [&s](auto id) { ct_put_page_element<Sink, typename decltype(id)::type>(s); });
    s.put(']');
}

template <class Sink, class SimdList>
constexpr void ct_put_simd_group(Sink& s) noexcept {
    COMDARE_BVSET_PAAR_WACHE(SimdList, "simd_extension");
    ct_put_text(s, "simd_extension[");
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, SimdList>>(
        [&s](auto id) { ct_put_simd_element<Sink, typename decltype(id)::type>(s); });
    s.put(']');
}

template <class Sink, class HwList>
constexpr void ct_put_hw_group(Sink& s) noexcept {
    COMDARE_BVSET_PAAR_WACHE(HwList, "general_hardware");
    ct_put_text(s, "general_hardware[");
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, HwList>>(
        [&s](auto id) { ct_put_hw_element<Sink, typename decltype(id)::type>(s); });
    s.put(']');
}

// Der EINE Emitter, ueber den beide Paesse laufen. Kopf `bvset=<format>;bv=<pod-version>` -- der fuehrende `bvset=`
// macht die Mengen-Form fuer parse_variant_signature (A7) formal unlesbar, also koennen die beiden Sidecar-Formen
// nicht verwechselt werden. Ein POD-Versions-Bump bricht auch diese Signatur (gewollt: Neubau).
template <class PageList, class SimdList, class HwList, class Sink>
constexpr void ct_put_variant_set_signature(Sink& s) noexcept {
    ct_put_text(s, "bvset=");
    ct_put_uint(s, kBuildVariantSetSignatureVersion);
    ct_put_text(s, ";bv=");
    ct_put_uint(s, static_cast<std::uint64_t>(anatomy::kBuildVariantDefinitionVersion));
    s.put(';');
    ct_put_page_group<Sink, PageList>(s);
    s.put(';');
    ct_put_simd_group<Sink, SimdList>(s);
    s.put(';');
    ct_put_hw_group<Sink, HwList>(s);
}

template <class PageList, class SimdList, class HwList>
[[nodiscard]] consteval std::size_t variant_set_signature_length() noexcept {
    CtCountSink s{};
    ct_put_variant_set_signature<PageList, SimdList, HwList>(s);
    return s.n;
}

template <class PageList, class SimdList, class HwList, std::size_t N>
[[nodiscard]] consteval std::array<char, N> variant_set_signature_chars() noexcept {
    std::array<char, N> out{};
    CtFillSink          s{out.data(), 0};
    ct_put_variant_set_signature<PageList, SimdList, HwList>(s);
    return out;
}

} // namespace detail

/// Der Zeichen-Speicher der Mengen-Signatur -- exakt dimensioniert (Zaehl-Pass), also kein toter Puffer im Binary.
template <class PageList, class SimdList, class HwList>
inline constexpr auto kVariantSetSignatureChars =
    detail::variant_set_signature_chars<PageList, SimdList, HwList,
                                        detail::variant_set_signature_length<PageList, SimdList, HwList>()>();

/// variant_set_signature<PageList,SimdList,HwList>() -- die fertige, compile-time bekannte Mengen-Signatur.
/// Rein deklarativ ueber Typlisten: dieselben drei Listen ergeben immer denselben String, andere Listen (oder eine
/// andere Reihenfolge derselben Typen) einen anderen. Genau das ist das Gate.
template <class PageList, class SimdList, class HwList>
[[nodiscard]] constexpr std::string_view variant_set_signature() noexcept {
    return std::string_view{kVariantSetSignatureChars<PageList, SimdList, HwList>.data(),
                            kVariantSetSignatureChars<PageList, SimdList, HwList>.size()};
}

} // namespace comdare::cache_engine::builder::experiment
