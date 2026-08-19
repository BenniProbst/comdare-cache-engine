// topics/axis.hpp -- gemeinsames Achsen-Dach (Bau-INC-1a, Q1-Ruling 2026-07-17).
//
// Layer Supertype (Fowler, PoEAA) als Marker-Concept + CRTP-Tag, kombiniert mit Separation of
// Hierarchies. Das Dach traegt Identitaet + Familien-Diskriminator + (seit V-01R) die KATEGORIEN-
// ZUORDNUNG; jede weitere Familien-Semantik bleibt in den Unter-Wurzeln (Blut-Direktive: Organ-
// vs. System-Achsen NIE mischen -- die Familien schneiden sich nicht, sie teilen nur dieses
// zustandslose Dach).
//
// M-21-NACHZUG (#15-Bruch, KON101-02 V-01R): der aeltere Kopf-Satz "Semantik-FREIES Dach ... NUR
// Identitaet + Familien-Diskriminator" ist im Zuordnungs-Teil SUPERSEDED. Owner verbatim (17.08.,
// KON101-01): "Das AxisKind Enum bezeichnet einfach nur compile time um Welche Mess-Achsen-
// kategorie es sich bei einer Achse handelt also Mess/System/Organ. Die Kategorie muss bezueglich
// des Vorhandenseins von Haupt-Achsen und Unter-Achsen erweitert werden, um eine Zuordnung einer
// Unter-Achse zu einer Haupt-Achse und die Zuordnung einer Haupt-Achse zu einer Kategorie zu
// gewaehrleisten. Daher definitiv mit drehen, sonst ergibt es keinen Sinn." Das Dach bleibt
// zustandslos und vtable-frei; NEU sind AxisCategory, axis_category() und axis_category_of<D>().

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::topics {

/// Die DREI Achsen-Kategorien (Trichotomie der Stufen-Zuordnung, Ledger par. 30): MESS-Achsen
/// leben am PLANER, SYSTEM-Achsen an der CEB, ORGAN-Achsen am Tier. Die Enumerator-ORDNUNG ist
/// die SOLL-Ordnung der drei Aussen-Ebenen MESS, SYSTEM, ORGAN (KON21-03, Owner 12.08.: "Ja
/// genau, meint auch #87 und #78") -- dieselbe Ordnung, die S-6a in Makro-Argumentfolge, POD-
/// Feldfolge und Preimage-Gliedern vollzogen hat. Konsumenten duerfen sich auf die Relation
/// mess < system < organ verlassen; die Wache unten haelt sie.
enum class AxisCategory : unsigned char {
    mess,   ///< MESS-Achsen (Planer-Ebene: Instrumente/Tooling-Traeger + Mess-Meta-Metas)
    system, ///< SYSTEM-Achsen (CEB-Ebene: Konfig-/Bau-Parameter + System-Meta-Metas)
    organ,  ///< ORGAN-Achsen (Tier-Ebene: permutieren die binary_id + Organ-Meta-Metas)
};

static_assert(AxisCategory::mess < AxisCategory::system && AxisCategory::system < AxisCategory::organ,
              "V-01R/KON21-03: die Kategorien-Ordnung ist MESS, SYSTEM, ORGAN. Sie ist der Kern des "
              "#15-Drehs und darf nie still umsortiert werden.");

/// Familien-Diskriminator des Achsen-Dachs -- bezeichnet compile-time die Mess-Achsen-KATEGORIE
/// (Mess/System/Organ, Owner-Wort KON101-01 V-01R) der jeweiligen Achsen-Familie.
///
/// V-01R-DREH (#15-Bruch, 19.08.2026): die Enumerator-Ordnung folgt der Kategorien-Ordnung
/// MESS, SYSTEM, ORGAN (blockweise; je Kategorie Haupt-Diskriminator vor Meta-Meta). Die
/// aelteren je-Enumerator-Zusagen "ANS ENDE angefuegt, damit die Zahlenwerte der bestehenden
/// Diskriminatoren unveraendert bleiben" sind damit SUPERSEDED -- der Owner ueberstimmt die
/// Lead-Empfehlung append-only ausdruecklich (KON101-02: "definitiv mit drehen"). WIRE- und
/// PREIMAGE-NEUTRAL bleibt der Dreh dennoch: die Wire-Form ist ein Token-STRING je Enumerator
/// (profile_facade/planner/experiment_dock_payload.hpp, axis_kind_token / parse_axis_kind),
/// kein Ordinal; kein Preimage-Glied hasht AxisKind-Zahlenwerte.
enum class AxisKind : unsigned char {
    // -- Kategorie MESS (Planer-Ebene) ------------------------------------------------------------
    system_measurement, ///< Mess-System-Achse ("Blut", host-seitig immer praesent, golden-neutral)
    /// MESS-Meta-Meta-Achse (O-8 Schritt 2; Ledger 70.1 / RF-1 VOLLES GO). Gegenstueck zu
    /// system_meta_meta auf der MESS-Seite: Mess-Achsen leben im PLANER und sind von den
    /// CEB-System-/Organ-Achsen VOELLIG getrennt -- je Realm ein eigener Meta-Meta-Diskriminator.
    /// Erster Traeger wird load_framework, das die System-Welt ersatzlos verlaesst (Ledger 69.1,
    /// Umzug in O-8 Schritt 4). Der historische Zusatz "ANS ENDE angefuegt, damit die Zahlenwerte
    /// der bestehenden Diskriminatoren unveraendert bleiben" ist per V-01R-Dreh SUPERSEDED (die
    /// Position folgt seither der Kategorien-Ordnung); "bis Schritt 4 gibt ihn KEINE Achse
    /// zurueck -- bis dahin verhaltens- und byte-neutral" gilt unveraendert.
    measurement_meta_meta,
    // -- Kategorie SYSTEM (CEB-Ebene) -------------------------------------------------------------
    system_config, ///< CEB-Konfig-System-Achse (Bau-/Steuer-Parameter, beruehrt NIE die binary_id)
    /// SYSTEM-Meta-Meta-Achse (Lane A A1 / Lane C C-1', Auftrag Abschnitt 1.4): eine VOLLE CT-Haupt-Achse,
    /// die unter dem external_utils-HUB haengt und selbst RT-Unter-Achsen tragen darf. NACHZUG R-G
    /// (Ledger 69.1, Owner-KERN 26.07.): der Hub traegt AUSSCHLIESSLICH System-Meta-Metas -- SIMD/AVX,
    /// externe Hardware, spaeter GPU/FPGA/NPU. Der frueher hier genannte Halbsatz "Mess-Framework;
    /// load_framework ist die ERSTE Meta-Meta" ist SUPERSEDED: load_framework ist eine Meta-Meta-
    /// HAUPT-Achse der MESS-Achsen (Planer-Stufe) und verlaesst die System-Welt ersatzlos.
    /// Rein ADDITIVER Diskriminator: solange keine Achse ihn zurueckgibt, ist er verhaltens- und
    /// byte-neutral. Er ist KEIN drittes festes Level -- die Rekursion bleibt offen (Layer-Modell D4:
    /// Unter-Achse == Voll-Achse). Der eigene Diskriminator fuer den Mess-Realm ist ENTSCHIEDEN
    /// (Ledger 70.1, RF-1 VOLLES GO): siehe measurement_meta_meta oben (Kategorie MESS).
    system_meta_meta,
    // -- Kategorie ORGAN (Tier-Ebene) -------------------------------------------------------------
    organ, ///< Organ-/Tier-Binary-Achse (permutiert die binary_id, 18 Slots; STRUKT-R ORG-18)
    /// ORGAN-Meta-Meta-Achse (A13-M2, Owner-Entscheid E2 vom 02.08.2026). Der Owner-Wortlaut kennt DREI
    /// Realms, zu denen eine Meta-Meta gehoeren kann: "Da eine Meta-Meta-Achse immer zu den Mess-Achsen,
    /// System-Achsen oder Organ-Achsen gehoert ...". Zwei davon hatten bisher einen eigenen Diskriminator;
    /// der dritte fehlte -- und mit ihm der MECHANISMUS, ueber den eine Organ-Meta-Meta ueberhaupt
    /// stempeln koennte. Dieser Enumerator schliesst die Luecke. Der historische Zusatz "ANS ENDE
    /// angefuegt (Zahlenwerte der bestehenden Diskriminatoren unveraendert)" ist per V-01R-Dreh
    /// SUPERSEDED (die Position folgt seither der Kategorien-Ordnung). HEUTE gibt ihn KEINE Achse
    /// zurueck und die Organ-Meta-Meta-Typliste ist LEER -- der Mechanismus ist gebaut, die
    /// Organ-Zeile bleibt byte-identisch (no-op). Genau so ist der OP-11-Rueckbau gemeint: nicht
    /// "Meta-Metas in die Organ-Zeile schuetten", sondern "die Organ-Zeile kann sie ab jetzt tragen".
    organ_meta_meta,
};

/// Anzahl der AxisKind-Enumeratoren. Schwanz-Anker unten haelt Zaehler und Enum gekoppelt; die
/// Enumeratoren sind lueckenlos (keine expliziten Werte), also deckt [0..kAxisKindCount) alle.
inline constexpr std::size_t kAxisKindCount = 6;

static_assert(static_cast<unsigned>(AxisKind::organ_meta_meta) == kAxisKindCount - 1u,
              "kAxisKindCount ist vom Enum abgerissen: wer AxisKind erweitert, zieht den Zaehler, die "
              "Kategorien-Zuordnung (axis_category) und die Ordnungs-Wache in EINEM Zug nach.");

/// V-01R: Zuordnung Diskriminator -> Kategorie als CT-Mechanik (total, kein Default-Ausfall).
/// Der Realm jedes Diskriminators ist unten einzeln gepinnt -- eine Fehlzuordnung beisst dort
/// namentlich, nicht erst in einer Summen-Wache. Beleg der Realm-Lage: die D1/D2-Fehlerklassen-
/// Bloecke (measurement/axis_error_traits.hpp) fuehren system_measurement als MESS-Realm,
/// system_config als Konfig-Realm; die Drei-Schichten-Lesart (Ledger par. 30/47) ordnet Organ dem
/// Tier, System der CEB und Mess dem Planer zu.
[[nodiscard]] constexpr AxisCategory axis_category(AxisKind k) noexcept {
    switch (k) {
        case AxisKind::system_measurement:
        case AxisKind::measurement_meta_meta: return AxisCategory::mess;
        case AxisKind::system_config:
        case AxisKind::system_meta_meta: return AxisCategory::system;
        case AxisKind::organ:
        case AxisKind::organ_meta_meta: return AxisCategory::organ;
    }
    // Alle Enumeratoren sind oben abschliessend behandelt (-Wswitch haelt die Fall-Liste
    // vollzaehlig, die Pins unten halten jede Zuordnung einzeln). Ein Wert ausserhalb des Enums
    // ist kein legitimer Zustand dieses Typs; im constant-expression-Kontext bricht das Erreichen
    // dieser Zeile den Build (std::unreachable ist dort ill-formed) -- CT-fail-loud.
    std::unreachable();
}

// -- V-01R-PINS: jede Diskriminator->Kategorie-Zuordnung einzeln, namentlich ---------------------
static_assert(axis_category(AxisKind::system_measurement) == AxisCategory::mess,
              "V-01R: system_measurement ist der MESS-Realm (axis_error_traits D2-Block; Wallclock/"
              "Observer/PMC melden ihn). Die aeltere Gegen-These 'system_measurement ist eine "
              "SYSTEM-Achse' war die K13-Fehl-Deckung und ist doppelt ueberholt (KON21-03 + V-01R).");
static_assert(axis_category(AxisKind::measurement_meta_meta) == AxisCategory::mess,
              "V-01R: measurement_meta_meta ist die Meta-Meta des MESS-Realms (RF-1, Ledger 70.1).");
static_assert(axis_category(AxisKind::system_config) == AxisCategory::system,
              "V-01R: system_config ist der SYSTEM-(Konfig-)Realm der CEB.");
static_assert(axis_category(AxisKind::system_meta_meta) == AxisCategory::system,
              "V-01R: system_meta_meta ist die Meta-Meta des SYSTEM-Realms (external_utils-Hub).");
static_assert(axis_category(AxisKind::organ) == AxisCategory::organ,
              "V-01R: organ ist der ORGAN-Realm (Tier-Binary, binary_id).");
static_assert(axis_category(AxisKind::organ_meta_meta) == AxisCategory::organ,
              "V-01R: organ_meta_meta ist die Meta-Meta des ORGAN-Realms (A13-M2/E2).");

namespace detail {
/// V-01R-ORDNUNGS-WACHE: die AxisKind-ORDINALE muessen der Kategorien-Ordnung MESS, SYSTEM, ORGAN
/// blockweise folgen (monoton nicht-fallend ueber [0..kAxisKindCount)). Diese Wache wurde VOR dem
/// Dreh eingebaut und hat den organ-first-Altstand literal rot gemacht (T-1-Beleg der A2.5-Fix-
/// Strecke 2, G1); seither haelt sie jede kuenftige Erweiterung in der Block-Ordnung.
[[nodiscard]] consteval bool axis_kind_ordinale_folgen_der_kategorien_ordnung() {
    for (unsigned i = 0; i + 1u < kAxisKindCount; ++i) {
        if (axis_category(static_cast<AxisKind>(i)) > axis_category(static_cast<AxisKind>(i + 1u))) return false;
    }
    return true;
}
} // namespace detail

static_assert(detail::axis_kind_ordinale_folgen_der_kategorien_ordnung(),
              "V-01R/KON101 (Owner 17.08.: 'definitiv mit drehen'): die AxisKind-Enumeratoren stehen "
              "NICHT in der Kategorien-Ordnung MESS, SYSTEM, ORGAN. Enum-Bloecke umsortieren -- nie "
              "die Kategorien-Zuordnung an eine falsche Ordnung anpassen.");

/// Das Dach: empty base, keine vtable, keine Familien-Semantik. Der Concept-Guard
/// (is_empty && !is_polymorphic) schlaegt beim Kompilieren fehl, falls je eine vtable
/// eingeschleppt wird (Anti-Runtime-Switch-Sicherung).
template <class Derived>
struct Axis {
protected:
    constexpr Axis() noexcept = default;
};

template <class D>
concept AxisConcept =
    std::derived_from<D, Axis<D>> && std::is_empty_v<Axis<D>> && (!std::is_polymorphic_v<Axis<D>>) && requires {
        { D::axis_kind() } -> std::convertible_to<AxisKind>;
    };

/// V-01R: eine Achse mit TYP-Eltern-Bezug (Unter-Achse). Der geforderte Alias heisst wie im
/// Bestand `parent_axis` (measurement/ceb_sub_axis.hpp, Paket A1: "using parent_axis = ParentAxis";
/// dort ist das Label ABGELEITET, nie wiederholt). Das Dach kennt die konkreten Wurzeln NICHT
/// (Layer-Schnitt) -- es fordert nur die Form und dockt damit exakt am Bestands-Alias an.
template <class D>
concept SubAxisBezugConcept = requires { typename D::parent_axis; };

/// V-01R CT-Zuordnung Unter-Achse -> Haupt-Achse -> Kategorie: die Kategorie einer Achse ist die
/// Kategorie der WURZEL ihrer Eltern-Kette (offene Rekursion; Layer-Modell D4: Unter-Achse ==
/// Voll-Achse, Tiefe emergent -- vgl. measurement::axis_depth). Traegt ein Ketten-Glied selbst
/// einen Diskriminator, MUSS er der Wurzel-Kategorie entsprechen (Konsistenz-Wache unten) -- eine
/// Unter-Achse kann ihre Kategorie nicht umlackieren. NP-01-AUFLAGE (KON106-02): diese Zuordnung
/// KONSULTIERT die Ordnungs-Single-Source kSystemAxisOrder und ersetzt sie nie; die Konsultations-
/// Wache lebt schichtgerecht im Test (tests/unit/test_axis_kind_kategorien_zuordnung.cpp), weil
/// das Dach abi/ nicht kennt. NP-02-VERMERK (KON106-02): ob Meta-Meta-Diskriminatoren in der
/// ZUORDNUNG als Haupt-Achsen oder dritte Ebene gefuehrt werden, ist dort gebucht und hier NICHT
/// vorentschieden -- diese Funktion ordnet KATEGORIEN zu, keine Haupt-/Unter-Rollen.
template <class D>
[[nodiscard]] constexpr AxisCategory axis_category_of() noexcept {
    if constexpr (SubAxisBezugConcept<D>) {
        constexpr AxisCategory wurzel = axis_category_of<typename D::parent_axis>();
        if constexpr (requires {
                          { D::axis_kind() } -> std::convertible_to<AxisKind>;
                      }) {
            static_assert(axis_category(D::axis_kind()) == wurzel,
                          "V-01R KONSISTENZ: der eigene Diskriminator dieser Unter-Achse widerspricht "
                          "der Kategorie ihrer Eltern-Ketten-Wurzel. Entweder haengt die Achse an der "
                          "falschen Haupt-Achse oder ihr axis_kind() ist falsch.");
        }
        return wurzel;
    } else {
        static_assert(
            requires {
                { D::axis_kind() } -> std::convertible_to<AxisKind>;
            }, "V-01R: Ketten-Wurzel ohne axis_kind() -- eine Haupt-Achse muss ihren Familien-"
               "Diskriminator tragen (AxisConcept), sonst ist keine Kategorie zuordenbar.");
        return axis_category(D::axis_kind());
    }
}

} // namespace comdare::cache_engine::topics
