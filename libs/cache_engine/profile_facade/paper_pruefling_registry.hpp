#pragma once
// paper_pruefling_registry -- P-H/#89 (Ledger-#44/PV-4; KON110-05 R-1/R-2, KON112-08/-09,
// 2026-08-21): die UEBERSETZUNGS-Registry Paper -> Pruefling als COMPILE-TIME-Deklaration.
//
// R-1 (Owner 17.08., PV-4): "die 33 Paper werden in PRUEFLINGE uebersetzt -- mangels eigener
// Achsen ABSTRAKT oder mit vollen Achsen als VOLLER Pruefling". Diese Registry ist die EINE
// foermliche Traeger-Liste dieser Uebersetzung: je Paper (paper_ref P01..P33) GENAU EIN
// Pruefling (profil_id == die id des sota/<id>.profile.xml-Bestands) mit seinem Typ
// (full/abstract, AP-6/#240-Vokabular des Parsers: leeres Attribut == full).
//
// R-2 (Owner 17.08., PV-4; KON112-09): BEGRIFFS-KONFORMITAET STATT UEBERSETZER -- "Registry-
// Deklaration MEHRERER Begriffe, die compile-time als DASSELBE aufgefasst/umbenannt werden
// (Alias-Deklaration, dieselben Ziele). Muessen wir UEBERSETZEN, ist das eine REGRESSION =
// COMPILE-TIME-FEHLER." Deshalb traegt dieser Header AUSDRUECKLICH KEINEN Uebersetzer: es gibt
// keine Laufzeit-Transformations-Tabelle und keine Quer-Abbildung zwischen NICHT deklarierten
// Begriffen. Es gibt NUR (a) die deklarierte Gleichheit (same_begriff) und (b) die kanonische
// UMBENENNUNG innerhalb einer deklarierten Gruppe (begriff_kanonisch); ein unbekannter Begriff
// bleibt er selbst. Code-ADAPTER fuer Original-Paper-Interfaces bleiben die erlaubte AUSNAHME
// (leben bei den Kompositionen, nicht hier). XML traegt WAS/WO/WANN, NIE das WIE.
//
// ALIAS-ERSTBELEGUNG (KON112-09-Reihung, kanonisch = lebendes Registry-/Enum-Vokabular):
//   (1) node4 == SPARSE_NODE4_ART            (Ledger-#44/F1-Vokabular-Naht, KON107-02)
//   (2) w/ma/mi == compare/macro/micro       (KON112-01c, erster Anwendungsfall)
//   (3) Verbund1..3 == Stufe1..3-Altnamen    (B-2/V-11R-Uebergang)
//
// ABGRENZUNG M13 (Begriffs-Alias-Registry-GENERALISIERUNG, Slot W2-D, ANDERER Strang): hier
// lebt die PAPER-DOMAENEN-Anwendung des R-2-Mechanismus; die generische M13-Registry kann sie
// absorbieren (I-7: Alias-VOR-Rename ist im Design festgeschrieben, kein Byte-Ereignis noetig).
//
// CT-ERROR-PFLICHT (KON112-08 (e)): die Wachen unten sind static_asserts -- eine Registry-
// Verletzung (Dublette, Luecke im P01..P33-Raum, unbekannter Typ, L6-Anker verletzt) bricht die
// Kompilation MIT Klartext, nicht erst einen Test. Leichtgewichtig: KEINE Achsen-/Registry-
// Includes (die Stempel-Seite lebt in pruefling_stempel_farben.hpp).
//
// C++23, header-only, ASCII-only.

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::thesis_lazy {

// -- Ein Eintrag der Paper->Pruefling-Uebersetzung (Single-Source-Spiegel des XML-Bestands
//    libs/cache_engine/algorithm_profiles/sota/<profil_id>.profile.xml; der Gleichlauf beider
//    Richtungen ist ctest-bewacht: test_ph89_paper_prueflinge, Nenner FREMD aus dem Bestand). --
struct PaperPrueflingEintrag {
    std::string_view paper_ref;     // "P01".."P33" -- zugleich der <template ref>-Namensraum (D2)
    std::string_view profil_id;     // sota/<profil_id>.profile.xml == base_tier-/lebewesen-id
    std::string_view pruefling_typ; // "full" | "abstract" (leeres XML-Attribut == full, Parser-Vertrag)
};

// -- DIE 33 PAPER ALS PRUEFLINGE (R-1 (a): foermlich, abstrakt/voll). Reihenfolge P01..P33. --
inline constexpr std::array<PaperPrueflingEintrag, 33> kPaperPrueflingRegistry = {{
    {"P01", "art", "full"},
    {"P02", "hot", "full"},
    {"P03", "masstree", "full"},
    {"P04", "coco_trie", "full"},
    {"P05", "start", "full"},
    {"P06", "b2tree", "full"},
    {"P07", "wormhole", "full"},
    {"P08", "olc", "abstract"},
    {"P09", "louds", "abstract"},
    {"P10", "surf", "full"},
    {"P11", "css_tree", "full"},
    {"P12", "csb_tree", "full"},
    {"P13", "hankins", "full"},
    {"P14", "samuel", "full"},
    {"P15", "graefe", "full"},
    {"P16", "bender_treelayout", "full"},
    {"P17", "bender_cacheoblivious", "full"},
    {"P18", "saikkonen_multilevel", "full"},
    {"P19", "saikkonen_layoutinvariant", "full"},
    {"P20", "btreesareback", "full"},
    {"P21", "chen_prefetch", "full"},
    {"P22", "chen_fractal", "full"},
    {"P23", "khan_adaptive", "full"},
    {"P24", "naderan_tahan", "full"},
    {"P25", "mahling", "full"},
    {"P26", "zhang_fgcs", "full"},
    {"P27", "zhang_asplos", "full"},
    {"P28", "kuehn", "full"},
    {"P29", "rcu", "full"},
    {"P30", "hazard_pointers", "full"},
    {"P31", "ungethuem", "full"},
    {"P32", "to_stride", "full"},
    {"P33", "vampir", "abstract"},
}};

// -- Lookups (constexpr-faehig; Laufzeit-Nutzer: validate/Stempel/Planer). --
[[nodiscard]] constexpr PaperPrueflingEintrag const* find_paper(std::string_view paper_ref) noexcept {
    for (auto const& e : kPaperPrueflingRegistry)
        if (e.paper_ref == paper_ref) return &e;
    return nullptr;
}

[[nodiscard]] constexpr PaperPrueflingEintrag const* paper_by_profil_id(std::string_view profil_id) noexcept {
    for (auto const& e : kPaperPrueflingRegistry)
        if (e.profil_id == profil_id) return &e;
    return nullptr;
}

/// paper_template_known -- traegt der <template ref>-Namensraum (== paper_ref, Design-Entscheid
/// D2 dieses Strangs) den Wert? Konsument: validate_experiment_profile Pruefung (14) -- mit
/// dieser Registry faellt der alte tolerant-Fallback (XSD-Kommentar "Registries fuehren die
/// Paper-Templates NOCH NICHT" == Altstand U-8-(3)); ein unbekannter nicht-leerer ref ist ein
/// HARTER Fehler (R-4/KON112-10, fail-loud).
[[nodiscard]] constexpr bool paper_template_known(std::string_view ref) noexcept { return find_paper(ref) != nullptr; }

// -- CT-WACHEN (KON112-08 (e)): Verletzung == Compile-Fehler. --
namespace paper_registry_wachen {

[[nodiscard]] consteval bool paper_refs_eindeutig_und_p01_bis_p33() {
    // Eindeutigkeit + Vollstaendigkeit des P-Raums: je n in 1..33 genau ein "P<nn>".
    for (std::size_t n = 1; n <= kPaperPrueflingRegistry.size(); ++n) {
        char const  z1      = static_cast<char>('0' + (n / 10));
        char const  z2      = static_cast<char>('0' + (n % 10));
        std::size_t treffer = 0;
        for (auto const& e : kPaperPrueflingRegistry)
            if (e.paper_ref.size() == 3 && e.paper_ref[0] == 'P' && e.paper_ref[1] == z1 && e.paper_ref[2] == z2)
                ++treffer;
        if (treffer != 1) return false;
    }
    return true;
}

[[nodiscard]] consteval bool profil_ids_eindeutig() {
    for (std::size_t i = 0; i < kPaperPrueflingRegistry.size(); ++i)
        for (std::size_t j = i + 1; j < kPaperPrueflingRegistry.size(); ++j)
            if (kPaperPrueflingRegistry[i].profil_id == kPaperPrueflingRegistry[j].profil_id) return false;
    return true;
}

[[nodiscard]] consteval bool typen_bekannt_und_l6_anker() {
    std::size_t abstrakt = 0;
    for (auto const& e : kPaperPrueflingRegistry) {
        if (e.pruefling_typ != "full" && e.pruefling_typ != "abstract") return false;
        if (e.pruefling_typ == "abstract") ++abstrakt;
    }
    // L6-Anker (KON110-05 R-3 / AP-6): GENAU die 3 Marker-Prueflinge P08/P09/P33 sind abstrakt.
    if (abstrakt != 3) return false;
    auto const* p08 = find_paper("P08");
    auto const* p09 = find_paper("P09");
    auto const* p33 = find_paper("P33");
    return p08 != nullptr && p08->pruefling_typ == "abstract" && p09 != nullptr && p09->pruefling_typ == "abstract" &&
           p33 != nullptr && p33->pruefling_typ == "abstract";
}

static_assert(kPaperPrueflingRegistry.size() == 33, "P-H/R-1: die Uebersetzung traegt GENAU 33 Paper (KON110-05).");
static_assert(paper_refs_eindeutig_und_p01_bis_p33(),
              "P-H/R-1: paper_ref-Raum muss exakt P01..P33 sein (eindeutig, lueckenlos).");
static_assert(profil_ids_eindeutig(), "P-H/R-1: profil_id-Dubletten sind eine Registry-Regression.");
static_assert(typen_bekannt_und_l6_anker(), "P-H/R-3+L6: pruefling_typ nur full|abstract; GENAU P08/P09/P33 abstrakt.");

} // namespace paper_registry_wachen

// --
// R-2 -- BEGRIFFS-ALIAS (Mechanismus-Erstbelegung der Paper-Domaene, KON112-09). Eine Zeile
// deklariert ZWEI Schreibweisen DESSELBEN Dings; `kanonisch` traegt die M13-KANON-RICHTUNG
// (naming/begriffs_alias_registry.hpp: SPARSE_NODE4_ART/wallclock/macro/micro), `alias` die
// Kuerzel-/Alt-Schreibweise. Gleiches Kanon-Ziel darf MEHRFACH auftreten (wallclock <- w UND
// compare): 8 Zeilen / 7 Aequivalenzklassen. KEIN Uebersetzer (s. Kopf).
// FIX FUND-1 (Audit ph89 r1, H-11/P-23): Richtung an M13 angeglichen; die CT-Kreuz-Wache im
// Test bindet beide Registries bis zur getragenen M13-Absorption (Staffel 3) aneinander.
// --
struct BegriffsAliasGruppe {
    std::string_view kanonisch; // M13-Kanon (Baustein-Tag / Serialisierungs-Token / Verbund-Name)
    std::string_view alias;     // als DASSELBE deklarierte Kuerzel-/Alt-Schreibweise
};

inline constexpr std::array<BegriffsAliasGruppe, 8> kBegriffsAliasRegistry = {{
    // (1) Ledger-#44/F1-Vokabular-Naht: Baustein-Tag (M13-Kanon) <- Registry-Wrapper-name().
    {"SPARSE_NODE4_ART", "node4"},
    // (2) KON112-01c: Mess-Ebenen-Kanons wallclock/macro/micro <- Kuerzel w/ma/mi; die
    //     Ebene-0-Dualitaet traegt ZWEI Aliasse auf dasselbe Kanon-Ziel (wallclock <- w, compare).
    {"wallclock", "w"},
    {"wallclock", "compare"},
    {"macro", "ma"},
    {"micro", "mi"},
    // (3) B-2/V-11R: Verbund-Namen tragen; Stufe*-Altnamen sind DASSELBE Ding (kein Re-Parse).
    {"Verbund1_CeOnly", "Stufe1_CeOnly"},
    {"Verbund2_Replace", "Stufe2_PrueflingReplace"},
    {"Verbund3_Union", "Stufe3_FullJoin"},
}};

/// begriff_kanonisch -- die kanonische UMBENENNUNG innerhalb einer deklarierten Gruppe
/// (compile-time "als dasselbe umbenannt", Owner-R-2-Wortlaut; Richtung = M13-Kanon, ein Kanon
/// ist sein eigener Fixpunkt). Unbekannte Begriffe bleiben sie selbst (Identitaet --
/// ausdruecklich KEINE Transformation, kein Fallback-Raten).
[[nodiscard]] constexpr std::string_view begriff_kanonisch(std::string_view begriff) noexcept {
    for (auto const& g : kBegriffsAliasRegistry)
        if (begriff == g.kanonisch || begriff == g.alias) return g.kanonisch;
    return begriff;
}

/// same_begriff -- sind a und b DASSELBE deklarierte Ding (oder identisch)? Projektion auf das
/// KANON-Ziel: zwei Aliasse DESSELBEN Kanons sind dasselbe (w == compare via wallclock). NICHT
/// deklarierte Paare sind NICHT dasselbe -- die Registry uebersetzt nicht quer (R-2: Uebersetzen
/// = Regression); Unbekanntes projiziert auf sich selbst (Identitaet bleibt wahr).
[[nodiscard]] constexpr bool same_begriff(std::string_view a, std::string_view b) noexcept {
    return begriff_kanonisch(a) == begriff_kanonisch(b);
}

namespace begriffs_alias_wachen {
[[nodiscard]] consteval bool begriffe_disjunkt() {
    // M13-deckungsgleiche Eindeutigkeit (FIX FUND-1): gleiches KANON-Ziel zweier Zeilen ist
    // ERLAUBT (dieselbe Sache, z.B. wallclock <- w UND compare); ein ALIAS darf nur EINMAL
    // vorkommen (== M13s (fach,alias)-Wache, hier fachlos global) und NIE zugleich Kanon-Ziel
    // sein -- sonst waere die kanonische Projektion mehrdeutig bzw. kein Fixpunkt (der Fall
    // j == i deckt den Selbst-Alias `kanonisch == alias` mit ab).
    for (std::size_t i = 0; i < kBegriffsAliasRegistry.size(); ++i) {
        auto const& a = kBegriffsAliasRegistry[i];
        for (std::size_t j = 0; j < kBegriffsAliasRegistry.size(); ++j) {
            auto const& b = kBegriffsAliasRegistry[j];
            if (i != j && a.alias == b.alias) return false; // Alias-Dublette == mehrdeutige Zeile
            if (a.alias == b.kanonisch) return false;       // Alias zugleich Kanon-Ziel (inkl. Selbst-Alias)
        }
    }
    return true;
}
static_assert(begriffe_disjunkt(), "R-2: Aliasse eindeutig und nie zugleich Kanon-Ziel (gleiches Kanon-Ziel mehrerer "
                                   "Zeilen ist erlaubt -- dieselbe Sache).");
} // namespace begriffs_alias_wachen

// --
// I-5 -- STEMPEL-"FARBEN"-DEKLARATION (KON112-08 (d), Mechanik ENTSCHIEDEN: Achsen-Tokens im
// Stempel, KEINE Merge-Zeile). Je ABSTRAKTEM Pruefling seine Marker-Achsen in NEUER Achsen-
// Sprache (kCompositionAxisNames-Slot + Registry-Wrapper-name()). Die Wert-Realitaet (Wrapper
// existiert, traegt algo_version) erzwingt die Versions-Tabellen-Aufloesung in
// pruefling_stempel_farben.hpp (Sentinel-Verbot, ctest-bewacht) -- die CT-Pflicht (e) fuer
// die Slot-Namen prueft ein static_assert DORT gegen kCompositionAxisNames (Single-Source;
// dieser Header bleibt bewusst frei von Builder-Includes).
// VOLLE Prueflinge: ihre "Farben" SIND die volle Komposition (binary_id-Pfad); deren
// Organ-Zeile haengt am dokumentierten SOTA-METADATEN-BLOCKER (sota_catalog.hpp, K-3-REST)
// und wird dort geloest -- hier entsteht KEINE zweite Zeilen-Ableitung (O-8-Lehre).
// --
struct PaperFarbToken {
    std::string_view paper_ref; // Registry-Eintrag (muss existieren, CT-Wache unten)
    std::string_view achse;     // kCompositionAxisNames-Slot (CT-Wache in pruefling_stempel_farben.hpp)
    std::string_view wert;      // Registry-Wrapper-name() der Marker-Achse
};

inline constexpr std::array<PaperFarbToken, 3> kPaperFarbTokens = {{
    {"P08", "concurrency", "olc_optimistic"},                // axis_08_concurrency_olc.hpp
    {"P09", "memory_layout", "memory_layout_packed_bitmap"}, // axis_05_memory_layout_packed_bitmap.hpp
    {"P33", "allocator", "vampir_nfp"},                      // axis_06_allocator_vampir_nfp.hpp
}};

namespace farb_token_wachen {
[[nodiscard]] consteval bool farb_tokens_nur_fuer_registrierte_abstrakte() {
    for (auto const& t : kPaperFarbTokens) {
        auto const* e = find_paper(t.paper_ref);
        if (e == nullptr) return false;                   // Phantom-Paper
        if (e->pruefling_typ != "abstract") return false; // volle Prueflinge: Farben == Komposition
        if (t.achse.empty() || t.wert.empty()) return false;
    }
    return true;
}
static_assert(farb_tokens_nur_fuer_registrierte_abstrakte(),
              "I-5: Farb-Tokens nur fuer registrierte ABSTRAKTE Prueflinge (P08/P09/P33).");
} // namespace farb_token_wachen

} // namespace comdare::cache_engine::thesis_lazy
