#pragma once
// abi/meta_meta_stamp_suffix.hpp -- A13-M2 (Owner-Entscheid E2 + Antwort Q1 vom 02.08.2026): der KLAMMER-ANHANG
// der Meta-Meta-Achsen am ENDE einer Realm-Zeile.
//
// OWNER-WORTLAUT (E2, verbatim): "Die Meta-Meta-Achsen und deren Stempel-Eintraege sind wie alle Hauptachsen
// PFLICHT, das gilt fuer ALLE Hauptachsen. Der stempel muss sich dynamisch per Metaprogrammierung den
// Anforderungen anpassen. Da eine Meta-Meta-Achse immer zu den Mess-Achsen, System-Achsen oder Organ-Achsen
// gehoert, wird sie auch einfach dynamisch ans Ende der Kette in den bestehenden Zeilen angehaengt."
// OWNER-WORTLAUT (Q1, verbatim): "Q1 - Wie empfohlen nach Klammern (derzeit auch so geplant, bitte nachlesen)."
//
// DARAUS DIE ZWEI ZUSAGEN DIESES HEADERS:
//   (1) ANS ENDE: der Anhang wird IMMER an das Ende der bereits gerenderten Realm-Zeile gehaengt, nie als
//       eigene Zeile und nie zwischen die Haupt-Achsen. Es gibt daher KEINE Meta-Meta-Zeile.
//   (2) IN KLAMMERN: die EBENE steckt in der Klammer-TIEFE, nicht im Namen (Owner Q-A; die Grammatik selbst
//       steht im Kopf von abi/anatomy_stamp_entries.hpp, der consteval-Parser setzt sie durch). Ein Glied,
//       das selbst Hub ist, behaelt seine eigene Klammer -- die Rekursion ist offen (Layer-Modell D4/Q-D).
//
// "DYNAMISCH PER METAPROGRAMMIERUNG" (Owner-Wortlaut) heisst hier woertlich: der Glied-Satz kommt aus der
// TYPLISTE des jeweiligen Hubs (MetaMetaMembers) und wird per Fold-Expression (for_each_meta_meta) gefaltet --
// kein Deskriptor-Array, kein std::variant, keine vtable, kein Runtime-Switch, KEINE zweite Glied-Liste. Wer
// ein Glied einhaengt, aendert die Zeile; wer nichts einhaengt, aendert kein Byte. BOOST-FREI (dieselbe
// Hermetik-Auflage wie hardware_meta_meta_axis.hpp: der Hub haengt an Generator-Targets ohne Boost-Link).
//
// ZWEI SPEISEWEGE, EIN RENDERER (bewusst, nicht aus Bequemlichkeit):
//   * TYP-getrieben (meta_meta_stamp_suffix<Hub>()) -- fuer Realms, deren Meta-Metas ihre Identitaet KOMPLETT
//     im Typ tragen (System-Realm: ExternalUtilsHub -> SimdExternalUtilsFamily::axis_code_version).
//   * ENTRY-getrieben (meta_meta_stamp_suffix_from(entries)) -- fuer Realms, deren Meta-Meta-Wahl aus einer
//     REGISTRY kommt und damit nicht im Typ steht (Mess-Realm: die gewaehlte load_framework-id + ihre Version
//     aus kMeasurementFrameworkRegistry). Beide schreiben die Klammern ueber dieselbe eine Stelle
//     (detail::wrap_meta_meta_group) -- die Grammatik existiert nur einmal.
//
// header-only, C++23. Der Renderer ist Laufzeit (std::string, wie build_axis_version_stamp_line); die
// GLIED-AUSWAHL ist reine Compile-Zeit.

#include <cache_engine/measurement/algo_semver.hpp>             // algo_semver_string (X.Y.Z-Voll-Form)
#include <cache_engine/measurement/axis_version_stamp.hpp>      // AxisVersionEntry + build_axis_version_stamp_line
#include <cache_engine/measurement/hardware_meta_meta_axis.hpp> // MetaMetaMembers/for_each_meta_meta/members_of

#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Die beiden Klammer-Zeichen der Grammatik -- die EINZIGE Wahrheit ihrer Schreibweise. Sie sind
/// C-literal-sicher (kein Escaping im emittierten Modul-Quelltext noetig).
inline constexpr char kMetaMetaGroupOpen  = '[';
inline constexpr char kMetaMetaGroupClose = ']';

/// Der Algorithmus-MARKER einer Meta-Meta im Stempel. "code" == die statische Code-Identitaet der Achse --
/// exakt derselbe Marker wie in der System-Zeile (system_stamp_line), damit die Zeile EINE Sprache spricht.
inline constexpr std::string_view kMetaMetaAlgorithmMarker = "code";

namespace detail {

/// Die EINE Stelle, an der Klammern geschrieben werden. Leerer Inhalt -> leerer Anhang (kein "[]": ein
/// leerer Hub darf die Zeile nicht um zwei Zeichen verschieben -- das waere ein Fingerprint-Ereignis ohne
/// Informationsgewinn; genau daran haengt die Golden-Neutralitaet des heute leeren Organ-Realms).
[[nodiscard]] inline std::string wrap_meta_meta_group(std::string const& inner) {
    if (inner.empty()) return {};
    std::string out;
    out.reserve(inner.size() + 2);
    out += kMetaMetaGroupOpen;
    out += inner;
    out += kMetaMetaGroupClose;
    return out;
}

/// Der STEMPEL-NAME einer Meta-Meta. Bewusst sub_axis_label() und NICHT axis_label(): axis_label() ist das
/// Label der HAUPT-Achse, unter der die Familie haengt ("external_utils") und ist damit fuer alle Glieder
/// desselben Hubs gleich -- als Stempel-Name waere es mehrdeutig. sub_axis_label() ist die EIGENE Kennung des
/// Glieds ("simd"), per Konvention == family_id (external_utils_family_axis.hpp: "die Familie SPANNT ihre
/// gleichnamige Unter-Achse"). Die HUB-Zugehoerigkeit steht bereits in der Klammerung, nicht im Namen --
/// genau das ist der Owner-Q1-Unterschied zur verworfenen Punkt-Pfad-Form.
template <class M>
[[nodiscard]] constexpr std::string_view meta_meta_stamp_name() noexcept {
    return M::sub_axis_label();
}

template <class Members>
[[nodiscard]] inline std::string render_meta_meta_members(Members);

/// EIN Glied: "<name>=code@X.Y.Z" plus -- falls es SELBST Hub ist -- seine eigene Klammer eine Ebene tiefer.
///
/// A13-M3/C2 (Befund Z-07, nachhaltige Klassen-Schliessung): HIER, am RENDERER-ENGPASS, laufen die beiden
/// Versions-Wachen -- nicht nur opt-in am Definitionsort der jeweiligen Meta-Meta. Der Unterschied ist der
/// ganze Punkt: eine Wache am Definitionsort muss jemand HINSCHREIBEN (heute steht sie an genau EINER Achse,
/// external_utils_family_axis.hpp; die vier test-lokalen Meta-Metas haben sie nicht), eine Wache am Engpass
/// KANN NIEMAND VERGESSEN -- jedes Glied, das je in eine Stempel-Zeile gerendert wird, kommt durch diese
/// Funktion. Eine neue Meta-Meta ohne Wachen-Zwilling kann damit nicht mehr still am Engpass vorbeistempeln.
/// Die Wachen selbst bleiben die B12-Single-Source (Z-03, hardware_meta_meta_axis.hpp) -- hier steht KEINE
/// zweite Politik, nur ihr Anwendungsort. Byte-neutral: der gesamte heutige Bestand besteht beide.
template <class M>
[[nodiscard]] inline std::string render_meta_meta_entry() {
    using ::comdare::cache_engine::measurement::algo_semver_string;
    static_assert(::comdare::cache_engine::measurement::meta_meta_version_wohlgeformt<M>(),
                  "A13-M3/C2 (Z-07): diese Meta-Meta wird gestempelt, ihre axis_code_version ist aber nicht "
                  "wohlgeformt -- sie muss dreistellig parsbar sein, darf nicht auf dem Sentinel stehen, kein "
                  "'e' (Pruefling-Markierung, Owner-E2) und kein falsches Hardware-Flag tragen (Owner-Q3).");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
    static_assert(::comdare::cache_engine::measurement::meta_meta_version_cpu_pflicht<M>(),
                  "A13-M3/C2 (Z-07): diese Meta-Meta wird gestempelt, ihre axis_code_version traegt aber kein "
                  "CPU-Hardware-Flag -- im CPU-only-Scope MUSS sie auf 'c' enden (Owner-Q3); "
                  "COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
#endif
    std::string out{meta_meta_stamp_name<M>()};
    out += '=';
    out += kMetaMetaAlgorithmMarker;
    out += '@';
    out += algo_semver_string(M::axis_code_version); // X.Y.Z-Voll-Form (SEPARATE Welt zur .algos-Sig)
    // OFFENE REKURSION (D4/Q-D): kein Level-Zaehler, kein Maximum -- ein Glied, das `meta_metas` exponiert,
    // bekommt seine eigene Klammer, und zwar ohne dass hier eine Ebene benannt waere.
    // Die eigene Klammer ist ein ';'-GESCHWISTER-Segment hinter dem Glied, nicht direkt angeklebt: nur so
    // ist die Grammatik auf JEDER Ebene dieselbe (segment := entry | group) und die Zeile aus dem
    // Entry-Array samt Ebenen VERLUSTFREI rekonstruierbar (Beweis: test_m_w12 A4-Round-Trip).
    if (std::string const sub =
            render_meta_meta_members(::comdare::cache_engine::measurement::meta_meta_members_of_t<M>{});
        !sub.empty()) {
        out += ';';
        out += sub;
    }
    return out;
}

/// Die Glied-Liste eines Hubs als GRUPPE: "[g1;g2;...]" in Typlisten-Reihenfolge. Fold-Expression ueber
/// for_each_meta_meta -- keine Schleife im erzeugten Code, keine zweite Reihenfolge.
template <class... Ms>
[[nodiscard]] inline std::string
render_meta_meta_members_impl(::comdare::cache_engine::measurement::MetaMetaMembers<Ms...>) {
    std::string inner;
    ::comdare::cache_engine::measurement::for_each_meta_meta(
        ::comdare::cache_engine::measurement::MetaMetaMembers<Ms...>{}, [&inner](auto tag) {
            using M = typename decltype(tag)::type;
            if (!inner.empty()) inner += ';';
            inner += render_meta_meta_entry<M>();
        });
    return wrap_meta_meta_group(inner);
}

template <class Members>
[[nodiscard]] inline std::string render_meta_meta_members(Members m) {
    return render_meta_meta_members_impl(m);
}

} // namespace detail

/// meta_meta_stamp_suffix_from_members<Members>() -- der Klammer-Anhang aus einer bereits BENANNTEN
/// Glied-Liste. Fuer Realms, die ihre Glieder als eigene Single-Source fuehren statt an einem Hub-TYP
/// (Organ-Realm: abi::OrganMetaMetas, heute leer). Leere Liste -> leerer Anhang (byte-neutral, no-op).
template <class Members>
[[nodiscard]] inline std::string meta_meta_stamp_suffix_from_members() {
    return detail::render_meta_meta_members(Members{});
}

/// meta_meta_stamp_suffix<Hub>() -- der TYP-getriebene Klammer-Anhang eines Hubs (System-Realm). Leerer Hub
/// -> leerer Anhang (byte-neutral). Die Glied-Reihenfolge ist die der Hub-Typliste (Single-Source), NIE eine
/// hier nachgebaute Ordnung.
template <class Hub>
[[nodiscard]] inline std::string meta_meta_stamp_suffix() {
    return meta_meta_stamp_suffix_from_members<::comdare::cache_engine::measurement::meta_meta_members_of_t<Hub>>();
}

/// meta_meta_stamp_suffix_from(entries) -- der ENTRY-getriebene Klammer-Anhang (Mess-Realm: die Meta-Meta-WAHL
/// steht in einer Registry, nicht im Typ). Dieselbe Klammer, dieselbe X.Y.Z-Voll-Form, derselbe Renderer fuer
/// den Inhalt (build_axis_version_stamp_line) -- es entsteht KEINE zweite Zeilen-Grammatik. Leere Menge ->
/// leerer Anhang.
[[nodiscard]] inline std::string
meta_meta_stamp_suffix_from(std::span<::comdare::cache_engine::measurement::AxisVersionEntry const> entries) {
    return detail::wrap_meta_meta_group(::comdare::cache_engine::measurement::build_axis_version_stamp_line(entries));
}

/// append_meta_meta_suffix(line, suffix) -- haengt den Anhang ANS ENDE der Realm-Zeile (Owner-E2 "ans Ende der
/// Kette"). Zwei Leer-Regeln, beide bewusst:
///   * leerer Anhang -> die Zeile bleibt BYTE-IDENTISCH (heute der Organ-Realm: Mechanismus gebaut, no-op);
///   * leere Zeile -> bleibt leer. Eine sonst leere Mess-Zeile heisst "kein Mess-Tooling einkompiliert"; ein
///     einsames Rahmen-Segment machte daraus ein "etwas gemessen" (die Makro-Materialisierung verlaesst sich
///     auf measurement_line == "" mit measurement_len == 0).
///
/// A13-M3/C5 (Befund Z-11, K-6-Sweep) -- AUSLEGUNG, NICHT OWNER-WORTLAUT. Die zweite Leer-Regel verwirft den
/// Meta-Meta-Anhang STILL, wenn die Realm-Zeile leer ist. Owner-E2 erklaert Meta-Meta-Stempeleintraege zur
/// PFLICHT "wie alle Hauptachsen"; dass diese Pflicht bei einer LEEREN Realm-Zeile entfaellt, ist NIRGENDS
/// owner-dokumentiert -- es ist eine Implementierungs-Entscheidung, und sie steht hier als solche benannt,
/// damit der naechste Leser sie nicht fuer eine Owner-Regel haelt.
/// AM HEUTIGEN IST KONSEQUENZLOS, literal geprueft: der einzige reale Leer-Fall ist die Mess-Zeile ohne
/// einkompiliertes Tooling, und dort entsteht der load_framework-Anhang im ZWEITEN Ableitungsweg
/// (builder/ceb_version_stamp.hpp) konsistent ebenfalls nicht -- beide Wege verhalten sich gleich, es gibt
/// keine Drift.
/// SOBALD je eine Meta-Meta an einer real LEEREN Realm-Zeile haengen soll (z.B. eine Mess-Meta-Meta ohne
/// Tooling-Wahl), ist das eine OWNER-FRAGE und KEIN stiller Default.
inline void append_meta_meta_suffix(std::string& line, std::string_view suffix) {
    if (line.empty() || suffix.empty()) return;
    line += ';';
    line += suffix;
}

} // namespace comdare::cache_engine::abi
