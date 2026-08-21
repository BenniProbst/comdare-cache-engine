#pragma once
// measurement/kanal_id_schema.hpp -- I-2: das KANAL-ID-SCHEMA als CT-FESTSCHREIBUNG
// (#91-Vollzug W2-D, 2026-08-21; design91-v2 Abschnitt 4, I-2 = Identitaets-Klasse C: vor dem
// Trigger NUR festschreiben, der Kollektor-BAU bleibt nach dem Trigger [#53/#90-Strang].
// Schema-Quelle: DESIGN #90 Kanalwerk+Arena Abschnitt 3 [20260820-DESIGN-90-kanalwerk-arena.md],
// dort koordiniert festgeschrieben -- diese Datei ist die CT-Form derselben Festschreibung).
//
// WARUM IDENTITAET: Kanal-Tags gehen in persistierte Bestand-2/3-Records und in die xlsx-Legenden
// (design91-v2 I-2, Massstab (c)(f)) -- was die Kampagne ohne Tag misst, bleibt ohne Tag
// (Messdaten nie loeschen, nie nach-erheben).
//
// DIE VIER FESTSCHREIBUNGEN (DESIGN-90 Abschnitt 3.1-3.5):
//   (1) Kanal-ADRESSE = Registry-konforme Token-Kette entlang der Buendel-Hierarchie
//       Achse -> Genus -> Kategorie (KON110-02-Baum "Achse->Genus->w/PROFILER"); ein Kanal ist
//       IMMER ueber seinen Pfad im Baum adressiert, nie ueber einen freien Namen. Kuerzel werden
//       NIE flach gelesen (Stempel-Syntax-Doktrin, Grammatik-Wache je Position).
//   (2) REIHENFOLGE-KANON (Owner verbatim KON101-01 V-13): "Es wird nur die Reihenfolge
//       wallclock/macro/micro erlaubt, alles andere ist syntaktisch falsch." In JEDER
//       serialisierten Ebenen-Liste ist wallclock/macro/micro die EINZIG erlaubte Reihenfolge;
//       jede andere Permutation ist SYNTAKTISCH FALSCH = Wurf, KEINE stille Normalisierung.
//       TEILMENGEN sind erlaubt (B-19: Anordnungs-Freigabe je Stufe Subset {W,Ma,Mi}), die
//       Ordnung innerhalb der Teilmenge folgt dem Kanon. Durchsetzungs-Traeger an den drei
//       Serialisierungs-Stellen = Board-#99/B-5f (Di-25) -- DIESE Datei definiert die Regel und
//       stellt den Pruefer, sie baut die Validierungs-Stellen nicht.
//   (3) ALIAS STATT RENAME: compare/macro/micro == w/ma/mi lebt als Alias-Eintrag in der
//       Begriffs-Alias-Registry (naming/begriffs_alias_registry.hpp, M13) -- KEIN Rename
//       (I-7-Rangfolge). Die kanonischen SERIALISIERUNGS-Tokens sind die Registry-ids der
//       Mess-Tooling-Achse (mess_axes/measurement_tooling_registry.hpp: wallclock/macro/micro).
//   (4) TAG-/DESKRIPTOR-BINDUNG: im Hot-Path reist KEIN Kanal-String (R1); der Kanal reist als
//       deskriptor_ix (uint32) in die statische Deskriptor-Tabelle (mess_arena.hpp
//       MessCheckpointZeile). Der Enum-Wert MessEbene::Reserviert=3 wird NICHT stillschweigend
//       an die Hybrid-Ebene vergeben (LESEFALLE HY-0; eigener Entscheid im HY-C-/GO-3-Design
//       nach Trigger) -- bis dahin ist 3 reserviert und jede Verwendung ein Wurf.
//
// PIN-DOKTRIN: die Ebenen-Tokens stehen hier als BEWUSSTE Literal-Gegenprobe (wie der
// Namen-Anker in run_methodology_registry.hpp); die Kreuz-Wache gegen die Tooling-Registry und
// die MessEbene-Ordinale faehrt im Test (test_kanal_id_schema) -- der Querschnitt zieht dafuer
// KEINE neue Kante in mess_axes/ oder builder/ (Zielbild Design #29: kein Kopf des Querschnitts
// kennt Traeger-Inneres).
//
// SELBSTCHECK: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik, kein Stempel-/golden-
// Byte; header-only; ASCII-only; keine Beruehrung von axes/, topics/, heuristik/.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::measurement {

// -- (1) Die Kanal-Adress-Hierarchie (KON110-02) -------------------------------------------------
inline constexpr std::size_t kKanalHierarchieTiefe = 3;
/// Die drei Stufen der Kanal-Adresse, aussen nach innen (DESIGN-90 3.4: "Achse -> Genus ->
/// Kategorie"; die Kategorie-Stufe traegt die Mess-Ebene bzw. den PROFILER-Buendel-Fall).
inline constexpr std::array<std::string_view, kKanalHierarchieTiefe> kKanalHierarchie{"achse", "genus", "kategorie"};

// -- (2) Der Ebenen-Kanon (V-13/KON101) ----------------------------------------------------------
inline constexpr std::size_t kEbenenKanonZahl = 3;
/// Die EINZIG erlaubte serialisierte Reihenfolge der Mess-Ebenen (Owner V-13). Tokens == die
/// Registry-ids der Mess-Tooling-Achse (Kreuz-Wache im Test, Pin-Doktrin).
inline constexpr std::array<std::string_view, kEbenenKanonZahl> kEbenenKanon{"wallclock", "macro", "micro"};

/// Position eines Ebenen-Tokens im Kanon; kEbenenKanonZahl fuer unbekannte Tokens.
[[nodiscard]] constexpr std::size_t ebenen_kanon_position(std::string_view token) noexcept {
    for (std::size_t i = 0; i < kEbenenKanonZahl; ++i)
        if (kEbenenKanon[i] == token) return i;
    return kEbenenKanonZahl;
}

/// DER Kanon-Pruefer (Definitions-Seite von #99/B-5f): eine serialisierte Ebenen-Liste ist genau
/// dann gueltig, wenn sie eine (auch leere) TEILFOLGE des Kanons ist -- jedes Token bekannt,
/// Positionen STRENG aufsteigend (keine Duplikate, keine Permutation). Alles andere ist
/// syntaktisch falsch = Wurf beim Aufrufer, KEINE stille Normalisierung.
[[nodiscard]] constexpr bool ist_kanon_reihenfolge(std::span<std::string_view const> ebenen) noexcept {
    std::size_t letzte = 0;
    bool        erste  = true;
    for (std::string_view token : ebenen) {
        std::size_t const pos = ebenen_kanon_position(token);
        if (pos == kEbenenKanonZahl) return false; // unbekanntes Token
        if (!erste && pos <= letzte) return false; // Permutation oder Duplikat
        letzte = pos;
        erste  = false;
    }
    return true;
}

// -- (4) Tag-/Deskriptor-Bindung -----------------------------------------------------------------
/// Der Kanal reist im Hot-Path als Index in die statische Deskriptor-Tabelle (R1: kein Text im
/// heissen Pfad). Typgleichheit mit MessCheckpointZeile::deskriptor_ix haelt der Test.
using KanalDeskriptorIx = std::uint32_t;

/// Die gebauten Mess-Ebenen-Ordinale (mess_arena.hpp MessEbene, Kreuz-Wache im Test): 0/1/2 IST
/// bereits w-vor-ma-vor-mi -- der Kanon bindet die SERIALISIERTE Reihenfolge, die Ordinale
/// muessen nicht gedreht werden (DESIGN-90 3.2, Objekt-Beleg).
inline constexpr std::uint8_t kMessEbeneOrdinalWallclock = 0;
inline constexpr std::uint8_t kMessEbeneOrdinalMacro     = 1;
inline constexpr std::uint8_t kMessEbeneOrdinalMicro     = 2;
/// Reserviert=3: NICHT stillschweigend die Hybrid-Ebene (LESEFALLE HY-0). Ob die Hybrid-Macro-
/// Schicht als vierter Wert eintritt, ist ein EIGENER, offener Entscheid (HY-C-/GO-3-Design,
/// nach Trigger). Bis dahin ist jede Verwendung von 3 ein Wurf beim Aufrufer.
inline constexpr std::uint8_t kMessEbeneOrdinalReserviert = 3;

namespace detail {
[[nodiscard]] consteval bool kanal_id_schema_ist_konsistent() {
    for (std::size_t i = 0; i < kKanalHierarchieTiefe; ++i)
        if (kKanalHierarchie[i].empty()) return false;
    for (std::size_t i = 0; i < kEbenenKanonZahl; ++i) {
        if (kEbenenKanon[i].empty()) return false;
        if (ebenen_kanon_position(kEbenenKanon[i]) != i) return false; // Position == Index
    }
    return true;
}
} // namespace detail

static_assert(detail::kanal_id_schema_ist_konsistent(),
              "Kanal-ID-Schema: Hierarchie-Tokens nie leer, Kanon-Position == Index.");
// Namen-Anker (Pin-Doktrin): Drift des V-13-Kanons bricht hier compile-time.
static_assert(kEbenenKanon[0] == std::string_view{"wallclock"} && kEbenenKanon[1] == std::string_view{"macro"} &&
                  kEbenenKanon[2] == std::string_view{"micro"},
              "V-13/KON101: die EINZIG erlaubte Reihenfolge ist wallclock/macro/micro.");
// Der Pruefer selbst, compile-time bewiesen: Kanon und Teilmengen GUELTIG, Permutation/Duplikat/
// Unbekanntes UNGUELTIG (CT-Gegeneingang).
static_assert(
    [] {
        constexpr std::array<std::string_view, 3> voll{"wallclock", "macro", "micro"};
        constexpr std::array<std::string_view, 2> teil{"wallclock", "micro"};
        constexpr std::array<std::string_view, 2> permutiert{"macro", "wallclock"};
        constexpr std::array<std::string_view, 2> doppelt{"macro", "macro"};
        constexpr std::array<std::string_view, 1> fremd{"profiler"};
        return ist_kanon_reihenfolge(voll) && ist_kanon_reihenfolge(teil) &&
               ist_kanon_reihenfolge(std::span<std::string_view const>{}) && !ist_kanon_reihenfolge(permutiert) &&
               !ist_kanon_reihenfolge(doppelt) && !ist_kanon_reihenfolge(fremd);
    }(),
    "ist_kanon_reihenfolge: Teilfolgen des Kanons gueltig; Permutation, Duplikat, Unbekanntes = Wurf.");

} // namespace comdare::cache_engine::measurement
