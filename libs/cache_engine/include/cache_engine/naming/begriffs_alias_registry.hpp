#pragma once
// naming/begriffs_alias_registry.hpp -- M13: die BEGRIFFS-ALIAS-REGISTRY (R-2), W2-D-SKELETT
// (#91-Vollzug, 2026-08-21; design91-v2 Abschnitt 3/M13 + Abschnitt 4, I-7 = Identitaets-Klasse C).
//
// I-7 -- HIERMIT IN KRAFT (design91-v2, Klasse C, Festschreib-Traeger = dieses Skelett):
// RANGFOLGE ALIAS VOR RENAME. Kuenftige Begriffs-Kollisionen werden per CT-Alias-Eintrag in
// DIESER Registry deklariert statt umbenannt -- ein Rename ist die AUSNAHME und braucht ein
// Owner-Wort (wie V-11R); owner-gesetzte Renames werden vollzogen wie entschieden, und die
// Registry traegt den Uebergang als Eintrag der Art "uebergang" (alter Begriff bleibt lesbar).
// Das Namens-SCHEMA ("was ein Ding IST") bleibt separater Owner-Entscheid (KON112-09) -- die
// Vorlage faellt erst an der ersten Neu-Benennung, nicht hier.
//
// DOKTRIN (Skizze M13 + KON110-05 R-2): zwei Namen fuer dieselbe Sache sind compile-time
// DASSELBE; UEBERSETZEN ist eine Regression und bricht compile-time (kanon_of ist consteval --
// ein unbekannter Begriff ist ein CT-Fehler, ein Laufzeit-Uebersetzer hat hier keinen Platz).
// ADAPTER ist die Paper-AUSNAHME (Skizze M13): Paper-Terminologie darf am Adapter-Rand uebersetzt
// werden, nirgends sonst.
//
// ERST-EINTRAEGE (design91-v2 M13, in der dortigen Reihenfolge):
//   1. node4            -- KON112-09 Kandidat (1): der sprachliche Begriff "node4" zum
//                          technischen Baustein-Tag "SPARSE_NODE4_ART"
//                          (include/cache_engine/abi/baustein_variants.hpp NodeSparseNode4Art).
//   2. w/ma/mi          -- KON110-02/KON112-01(c): die Genus-Interface-Kuerzel w/ma/mi zu den
//                          kanonischen Serialisierungs-Tokens wallclock/macro/micro
//                          (mess_axes/measurement_tooling_registry.hpp); dazu die am Objekt
//                          gebaute Namens-Dualitaet der Ebene 0: MessEbene::Compare (mess_arena/
//                          konfiguration) == wallclock (Registry/Stempel). KEIN Rename --
//                          genau der Alias-Fall, den DESIGN-90 3.3 festschreibt. Das work_mode-
//                          Token "compare" (run_methodology_registry) ist ein ANDERES Fachgebiet;
//                          die fach-Spalte trennt die Welten.
//   3. Verbund-Uebergang -- V-11R (Owner: "Vorschlag angenommen. Genau so."), im #15-Umfeld
//                          VOLLZOGEN (design91-v2 S4): Stufe1_CeOnly -> Verbund1_CeOnly
//                          (Phasen-Ebene) und fulljoin -> union (Achsen-Token-Ebene;
//                          merge_plan.hpp wirft fuer "fulljoin" bereits mit V-11R-Hinweis).
//
// FUELLUNG ueber die Erst-Eintraege hinaus = M13-FUELLUNG, nach Trigger CT-only nachziehbar
// (design91-v2 Abschnitt 5). Die Kreuz-Wachen gegen die Objekt-Quellen (Tooling-Registry,
// Baustein-Tag, Merge-Modi) fahren im Test (test_begriffs_alias_registry) -- der Querschnitt
// zieht hier KEINE neuen Kanten (Pin-Doktrin, bewusste Literal-Gegenproben).
//
// SELBSTCHECK: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik, kein Stempel-/golden-
// Byte; header-only; ASCII-only; keine Beruehrung von axes/, topics/, heuristik/.

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::naming {

/// Art eines Eintrags: "alias" = zwei lebende Namen, eine Sache (Rangfolge I-7);
/// "uebergang" = owner-gesetzter Rename vollzogen, der alte Name bleibt als Historie lesbar.
inline constexpr std::string_view kArtAlias     = "alias";
inline constexpr std::string_view kArtUebergang = "uebergang";

/// EIN Begriffs-Paar: der KANON (die kanonische/neue Form) und sein ALIAS (Kuerzel, Zweitname
/// oder Alt-Form), gebunden an ein Fachgebiet (gleiche Worte in fremden Fachgebieten kollidieren
/// NICHT -- z.B. mess_ebene "compare" vs work_mode "compare").
// Default-Initialisierer nach dem Muster des heuristik-Bestands: jedes Feld bestimmt.
struct BegriffsAlias {
    std::string_view kanon  = {}; ///< kanonische Form (Serialisierung/Neu-Name)
    std::string_view alias  = {}; ///< Kuerzel/Zweitname (alias) bzw. Alt-Name (uebergang)
    std::string_view fach   = {}; ///< Fachgebiet (Namensraum des Begriffs)
    std::string_view art    = {}; ///< kArtAlias oder kArtUebergang
    std::string_view quelle = {}; ///< Owner-/KON-Quelle des Eintrags
};

// Single-Source: ein 8. Erst-Eintrag bricht hier compile-time (statt still 7 zu bleiben).
inline constexpr std::size_t kBegriffsAliasCount = 7;

/// DIE Registry (Erst-Eintraege des W2-D-Skeletts, Reihenfolge = design91-v2 M13).
inline constexpr std::array<BegriffsAlias, kBegriffsAliasCount> kBegriffsAliasRegistry{{
    // 1. node4 (KON112-09 Kandidat (1))
    {"SPARSE_NODE4_ART", "node4", "organ_baustein", kArtAlias, "KON112-09(1)"},
    // 2. w/ma/mi + die gebaute compare-Dualitaet der Ebene 0 (KON110-02, KON112-01(c), DESIGN-90 3.3)
    {"wallclock", "w", "mess_ebene", kArtAlias, "KON110-02"},
    {"wallclock", "compare", "mess_ebene", kArtAlias, "KON112-01(c)"},
    {"macro", "ma", "mess_ebene", kArtAlias, "KON110-02/KON112-01(c)"},
    {"micro", "mi", "mess_ebene", kArtAlias, "KON110-02/KON112-01(c)"},
    // 3. Verbund-Uebergang (V-11R, vollzogen im #15-Umfeld; Registry traegt den Uebergang)
    {"Verbund1_CeOnly", "Stufe1_CeOnly", "verbund_phase", kArtUebergang, "V-11R"},
    {"union", "fulljoin", "verbund_achsen_token", kArtUebergang, "V-11R"},
}};

/// constexpr-Suche (RT-tauglich, z.B. fuer Fehlermeldungs-Texte): nullptr fuer unbekannte
/// Begriffe -- der Aufrufer entscheidet LAUT, nie still.
[[nodiscard]] constexpr BegriffsAlias const* begriffs_alias_zeile(std::string_view fach,
                                                                  std::string_view token) noexcept {
    for (std::size_t i = 0; i < kBegriffsAliasCount; ++i) {
        BegriffsAlias const& z = kBegriffsAliasRegistry[i];
        if (z.fach == fach && (z.kanon == token || z.alias == token)) return &z;
    }
    return nullptr;
}

/// DER Uebersetzungs-Punkt -- und zwar der EINZIGE, und nur compile-time (R-2: Uebersetzen zur
/// Laufzeit ist eine Regression). Liefert die kanonische Form eines Begriffs; ein UNBEKANNTER
/// Begriff bricht compile-time (consteval + throw), er faellt NIE still durch.
[[nodiscard]] consteval std::string_view kanon_of(std::string_view fach, std::string_view token) {
    BegriffsAlias const* z = begriffs_alias_zeile(fach, token);
    if (z == nullptr) throw "begriffs_alias_registry: unbekannter Begriff (fach/token) -- CT-Fehler per R-2";
    return z->kanon;
}

namespace detail {
[[nodiscard]] consteval bool begriffs_alias_registry_ist_konsistent() {
    for (std::size_t i = 0; i < kBegriffsAliasCount; ++i) {
        BegriffsAlias const& z = kBegriffsAliasRegistry[i];
        if (z.kanon.empty() || z.alias.empty() || z.fach.empty() || z.quelle.empty()) return false;
        if (z.kanon == z.alias) return false; // ein Alias auf sich selbst ist keiner
        if (z.art != kArtAlias && z.art != kArtUebergang) return false;
        for (std::size_t j = 0; j < i; ++j) { // (fach, alias) eindeutig -- kein Doppel-Eintrag
            BegriffsAlias const& fr = kBegriffsAliasRegistry[j];
            if (fr.fach == z.fach && fr.alias == z.alias) return false;
        }
    }
    return true;
}
} // namespace detail

static_assert(kBegriffsAliasRegistry.size() == kBegriffsAliasCount,
              "kBegriffsAliasRegistry: Array-Groesse == kBegriffsAliasCount (Anzahl-Anker).");
static_assert(detail::begriffs_alias_registry_ist_konsistent(),
              "kBegriffsAliasRegistry: Felder nie leer, kanon != alias, Art gueltig, (fach, alias) eindeutig.");
// Namen-Anker (Pin-Doktrin): die drei Erst-Eintrags-Gruppen, je ein bewusstes Literal.
static_assert(kanon_of("organ_baustein", "node4") == std::string_view{"SPARSE_NODE4_ART"} &&
                  kanon_of("mess_ebene", "ma") == std::string_view{"macro"} &&
                  kanon_of("mess_ebene", "compare") == std::string_view{"wallclock"} &&
                  kanon_of("verbund_achsen_token", "fulljoin") == std::string_view{"union"} &&
                  kanon_of("verbund_phase", "Stufe1_CeOnly") == std::string_view{"Verbund1_CeOnly"},
              "Erst-Eintraege: node4->SPARSE_NODE4_ART, ma->macro, compare->wallclock (Ebene-0-"
              "Dualitaet), fulljoin->union und Stufe1_CeOnly->Verbund1_CeOnly (V-11R-Uebergang).");
// Ein KANON uebersetzt auf sich selbst (Idempotenz -- kein Ping-Pong zwischen den Namensreihen).
static_assert(kanon_of("mess_ebene", "macro") == std::string_view{"macro"},
              "kanon_of: die kanonische Form ist ihr eigener Fixpunkt.");
// V-13-Vertraeglichkeit: die mess_ebene-Kanons stehen in der Registry in Kanon-Reihenfolge
// (wallclock vor macro vor micro; Zeilen 2/4/5 -- Zeile 3 ist die zweite wallclock-Zeile).
static_assert(kBegriffsAliasRegistry[1].kanon == std::string_view{"wallclock"} &&
                  kBegriffsAliasRegistry[3].kanon == std::string_view{"macro"} &&
                  kBegriffsAliasRegistry[4].kanon == std::string_view{"micro"},
              "mess_ebene-Eintraege folgen der V-13-Kanon-Reihenfolge (Lesbarkeits-Invariante).");

} // namespace comdare::cache_engine::naming
