#pragma once
// abi/stempel_basis.hpp -- S-1 (Stempel-Strecke, KON38): der EINE CRTP-Vertrag der Stempel-Traeger.
//
// ORT (KON43-01(1)): abi/ ist das Stufe-0-Fundament -- die Stempel-Strecke ist ein Querschnitt und speist
// den Planer ZUERST. LAYER: diese Basis braucht nur abi + measurement (die Kante abi->measurement ist
// Bestand, anatomy_stamp_entries.hpp:97); abi->builder bleibt VERBOTEN (anatomy_version_stamp.hpp-Doktrin).
//
// WAS S-1 IST -- UND WAS NICHT:
//   * S-1 gibt den fuenf losen Stempel-Strukturen des Hauses einen GEMEINSAMEN Vertrag ("Erbin
//     verschaerft, nie aufweicht" als static_assert-Zwang, KEIN virtual) und BINDET den Bestand an,
//     statt eine sechste Fassung daneben zu stellen.
//   * S-1 aendert KEINEN Stempel-WERT (kein X.Y.Z-Bump, fingerprint_format unberuehrt): jede Erbin
//     DELEGIERT byte-identisch auf die Werte, die sie schon traegt.
//   * S-1 schreibt KEINE Zeilen-/Glied-REIHENFOLGE fest (S-6-Sperre: die Glieder-Reihenfolge ist im
//     Lager anders als ausserhalb -- Explore offen). Die gesamt_stempel-Pruefung unten ist deshalb
//     ORDNUNGSNEUTRAL (Teilstring-Enthaltensein), die Reihenfolge nennt ausschliesslich die Erbin.
//
// DIE SIEBEN INTERFACES (KON7-04) x VIER TRAEGER = 28 MATRIXZELLEN (dreiwertig, KON8-07):
//   version_xyz NUR Planer -- mess_zeile+system_zeile CEB/Tier/Hybrid -- organ_zeile Tier/Hybrid
//   (die CEB-Absenz IST der maschinelle Riegel "CEB traegt keine Organ-Achsen", KON7-07) --
//   fingerprint_sha+gesamt_stempel alle vier -- angeschlossene NUR Hybrid (RT-Hook; S-1 definiert nur
//   Concept/Signatur, der Datenbestand ist der KON47-02-Init-Cache, der Bau ist HY-A2).
//
// DER ZWANG (deferred-consteval): die Basis selbst ist LEER (0 virtual, keine Daten -- Hausform KON7-08).
// Die Vertragspruefung laeuft erst, wenn eine Erbin direkt unter ihrer Definition mit
//     static_assert(StempelVertrag<Erbin, Traeger>);
// schliesst. Eine unparsbare Zeile, eine fehlende Pflicht oder ein verbotenes Interface machen den
// Vertrag FALSCH (Substitutions-Ablehnung nach dem Muster stamp_line_is_parsable_impl,
// anatomy_stamp_entries.hpp:454-461) -- der static_assert der Erbin wird ROT, nicht die Basis.
//
// BAUSTEIN-EBENE (P1.4): die fuenf KON6-03-Bestands-Strukturen sind positional-init-Aggregate bzw.
// NTTP-Traeger -- eine leere Basisklasse fraesse den ERSTEN positionellen Initialisierer (die Pins
// anatomy_module_abi_v1_decl.hpp:191/:254/:261 und die Feld-Ordnungs-Wache toolchain_stamp_glied.hpp:
// 300-311 stuenden auf dem Spiel). Die Anbindung laeuft deshalb OHNE Basisklassen-Einbau ueber das
// Trait ist_stempel_baustein<T> mit Rollen-Tag; die Pins und Wachen bleiben byte-unberuehrt.

#include <cache_engine/abi/stempel_baustein_trait.hpp> // P1.4-Trait (ausgelagert 12.08.: Rolle/Trait/Tag/Concept)
#include <cache_engine/measurement/algo_semver.hpp>    // ce_owned_version_is_wellformed + gated CPU-Politik

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::abi {

namespace detail {
// VORWAERTSDEKLARATION statt #include <cache_engine/abi/anatomy_stamp_entries.hpp>: entries.hpp
// inkludiert die decl.hpp (AnatomyStampEntryV1), und die entries.hpp inkludiert DIESEN Header (S-1/P2-
// Anbindung der beiden ABI-PODs) -- ein entries-Include hier waere der Zyklus. Die Definition liefert
// anatomy_stamp_entries.hpp (der EINE Zeilen-Scanner); jede TU, die einen Traeger mit Mess-/System-/
// Organ-PFLICHT schliesst, zieht sie VOR dem static_assert -- sonst bricht die Konstantauswertung LAUT
// ("used before its definition"), nie still.
[[nodiscard]] consteval bool stamp_line_is_wellformed(std::string_view line);

/// Dependenz-Wrapper um den EINEN Scanner: die Zeilen-Proben unten nennen NUR diese (hier definierte)
/// Vorlage in ihren Default-Template-Argumenten. Die vorwaertsdeklarierte Scanner-Funktion wird damit
/// erst an einer INSTANZIIERUNG referenziert -- also nur in TUs, die einen Traeger mit Mess-/System-/
/// Organ-PFLICHT schliessen (und dort ist entries.hpp inkludiert). Ohne den Wrapper meldete clang an
/// jeder Planer-TU -Wundefined-inline auf die blosse Nennung im Default-Argument.
template <class Zeile>
[[nodiscard]] consteval bool zeile_durch_den_einen_scanner(Zeile const& zeile) {
    return stamp_line_is_wellformed(std::string_view{zeile});
}
} // namespace detail

// ================================================================================================
// P1.1 -- Traeger, Interfaces, dreiwertige Zulassungsmatrix
// ================================================================================================

/// Die vier Stempel-Traeger in Traeger-Stufen-Reihenfolge (Planer -> CEB -> Tier -> Hybrid).
enum class StempelTraeger : std::uint8_t { Planer = 0, Ceb = 1, Tier = 2, Hybrid = 3 };
inline constexpr std::size_t kStempelTraegerAnzahl = 4;

/// KON8-07: die Zulassung ist DREIWERTIG. ERLAUBT ist heute UNBELEGT (keine der 28 Zellen traegt es) --
/// die Dreiwertigkeit ist die KON8-07-Form, nicht eine Behauptung ueber den heutigen Bestand. Semantik:
/// PFLICHT = Interface muss existieren UND seine Wert-Pruefung bestehen; ERLAUBT = darf fehlen, MUSS
/// aber, wenn vorhanden, dieselbe Wert-Pruefung bestehen (Erbin verschaerft, nie aufweicht);
/// VERBOTEN = Absenz-Pflicht (!requires).
enum class StempelZulassung : std::uint8_t { Pflicht = 0, Erlaubt = 1, Verboten = 2 };

/// Die sieben KON7-04-Interfaces des Stempel-Vertrags.
enum class StempelInterface : std::uint8_t {
    VersionXyz     = 0, ///< statische Selbst-Version X.Y.Z[.flag]* (ce-Politik algo_semver.hpp)
    MessZeile      = 1, ///< Mess-Realm-Zeile (Grammatik: der EINE Scanner stamp_line_is_wellformed)
    SystemZeile    = 2, ///< System-Realm-Zeile (dito)
    OrganZeile     = 3, ///< Organ-Realm-Zeile (dito)
    FingerprintSha = 4, ///< SHA-512-Fingerprint, 128 hex (sha512_len-Doktrin decl.hpp:224-225)
    GesamtStempel  = 5, ///< das EINE Kompositum der Erbin (ordnungsneutral geprueft, S-6-Sperre)
    Angeschlossene = 6  ///< RT-Hook (NUR Hybrid): die angeschlossenen Stempel-Zeilen (KON47-02)
};
inline constexpr std::size_t kStempelInterfaceAnzahl = 7;

/// Die dreiwertige Zulassungsmatrix (KON8-07), Zeilen = Interfaces, Spalten = Traeger in
/// Stufen-Reihenfolge Planer/Ceb/Tier/Hybrid. Quelle der Belegung: die Marke KON7-04.
inline constexpr std::array<std::array<StempelZulassung, kStempelTraegerAnzahl>, kStempelInterfaceAnzahl>
    kStempelZulassungsMatrix{{
        // Planer                     Ceb                         Tier                        Hybrid
        {StempelZulassung::Pflicht, StempelZulassung::Verboten, StempelZulassung::Verboten,
         StempelZulassung::Verboten}, // version_xyz: NUR der Planer traegt eine Gesamt-Selbst-Version
        {StempelZulassung::Verboten, StempelZulassung::Pflicht, StempelZulassung::Pflicht,
         StempelZulassung::Pflicht}, // mess_zeile
        {StempelZulassung::Verboten, StempelZulassung::Pflicht, StempelZulassung::Pflicht,
         StempelZulassung::Pflicht}, // system_zeile
        {StempelZulassung::Verboten, StempelZulassung::Verboten, StempelZulassung::Pflicht,
         StempelZulassung::Pflicht}, // organ_zeile: CEB-VERBOT = KON7-07-Riegel "CEB traegt keine Organ-Achsen"
        {StempelZulassung::Pflicht, StempelZulassung::Pflicht, StempelZulassung::Pflicht,
         StempelZulassung::Pflicht}, // fingerprint_sha
        {StempelZulassung::Pflicht, StempelZulassung::Pflicht, StempelZulassung::Pflicht,
         StempelZulassung::Pflicht}, // gesamt_stempel
        {StempelZulassung::Verboten, StempelZulassung::Verboten, StempelZulassung::Verboten,
         StempelZulassung::Pflicht}, // angeschlossene: NUR Hybrid, als RT-Hook
    }};

/// Die EINE Ablese-Stelle der Matrix.
[[nodiscard]] constexpr StempelZulassung stempel_zulassung(StempelInterface i, StempelTraeger t) noexcept {
    return kStempelZulassungsMatrix[static_cast<std::size_t>(i)][static_cast<std::size_t>(t)];
}

// Dimensions-Pins: 7 Interfaces x 4 Traeger = 28 Zellen. Der VOLLE Zellen-Abgleich gegen die fremde
// Quelle (KON7-04-Liste) steht im Vertragstest (T-3); hier stehen die Dimensionen und die vier
// tragenden Eck-Aussagen, damit ein Matrix-Umbau schon in dieser Datei laut wird.
static_assert(kStempelZulassungsMatrix.size() == kStempelInterfaceAnzahl);
static_assert(kStempelZulassungsMatrix[0].size() == kStempelTraegerAnzahl);
static_assert(kStempelInterfaceAnzahl * kStempelTraegerAnzahl == 28, "KON7-04 x KON8-07: 28 Matrixzellen.");
static_assert(stempel_zulassung(StempelInterface::VersionXyz, StempelTraeger::Planer) == StempelZulassung::Pflicht,
              "KON7-04: version_xyz ist die Planer-Pflicht.");
static_assert(stempel_zulassung(StempelInterface::VersionXyz, StempelTraeger::Ceb) == StempelZulassung::Verboten,
              "KON7-04: version_xyz NUR Planer -- die CEB hat KEINE Gesamt-Version (ceb_version_stamp.hpp Kopf).");
static_assert(stempel_zulassung(StempelInterface::OrganZeile, StempelTraeger::Ceb) == StempelZulassung::Verboten,
              "KON7-07-Riegel: die CEB traegt keine Organ-Achsen -- diese Zelle ist der Riegel.");
static_assert(stempel_zulassung(StempelInterface::Angeschlossene, StempelTraeger::Hybrid) == StempelZulassung::Pflicht,
              "KON47-02: angeschlossene() ist der Hybrid-RT-Hook.");

// ================================================================================================
// P1.2 -- die leere CRTP-Basis
// ================================================================================================

/// StempelBasis<Erbin, Traeger> -- LEER: 0 virtual, keine Daten (Hausform KON7-08). Sie traegt nur die
/// Traeger-Kennung als benannte Konstante; der ZWANG ist der static_assert(StempelVertrag<...>), mit dem
/// jede Erbin direkt unter ihrer Definition schliesst (deferred-consteval, s. Kopf).
template <class Erbin, StempelTraeger Traeger>
struct StempelBasis {
    static constexpr StempelTraeger kTraeger = Traeger;
};

static_assert(std::is_empty_v<StempelBasis<int, StempelTraeger::Planer>>,
              "die Basis ist LEER -- eine Daten- oder vtable-tragende Basis waere ein ABI-Ereignis.");
static_assert(!std::is_polymorphic_v<StempelBasis<int, StempelTraeger::Planer>>,
              "0 virtual: der Vertrag ist Compile-Zeit (static_assert), kein Laufzeit-Dispatch.");

// ================================================================================================
// P6-Wert-Baustein -- ce-eigene VERSIONS-LITERALE
// ================================================================================================

/// ist_versions_literal_baustein -- die WERT-Form der Baustein-Anbindung: ein ce-eigenes
/// Versions-LITERAL (kein Typ, deshalb nicht ueber ist_stempel_baustein anbindbar) ist genau dann
/// Baustein der Stempel-Strecke, wenn es die EINE ce-Politik besteht: wohlgeformt nach
/// ce_owned_version_is_wellformed (ungated) und -- scharfgeschaltet wie ueberall -- mit CPU-Basis nach
/// ce_owned_version_satisfies_cpu_enforce. Konsument: pruef_dock_version.hpp (die fuenf Dock-Literale).
[[nodiscard]] consteval bool ist_versions_literal_baustein(std::string_view roh) noexcept {
    if (!::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(roh)) return false;
#if COMDARE_VERSION_HW_FLAG_ENFORCE
    if (!::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(roh)) return false;
#endif
    return true;
}

// ================================================================================================
// gesamt_stempel-Traegerform + der EINE consteval-Konkat-Helfer
// ================================================================================================

/// Die Traegerform des Erbin-Kompositums -- Muster CompletedSystemStampLine (system_cell_values.hpp:364):
/// nullterminiertes char-Array, N == Laenge + 1, damit {ptr,len}-Sichten und consteval-Parser es
/// unveraendert konsumieren koennen.
template <std::size_t N>
struct StempelKompositum {
    char chars[N]{};

    [[nodiscard]] constexpr std::string_view   view() const noexcept { return std::string_view{chars, N - 1}; }
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N - 1; }
};

/// stempel_kompositum<&teile_fn>() -- fuegt die Teile in der GENANNTEN Reihenfolge zusammen. Die
/// REIHENFOLGE nennt die ERBIN (ihre teile_fn), NIE die Basis: S-1 schreibt keine Glied-Ordnung fest
/// (S-6-Sperre). teile_fn ist eine constexpr-Funktion, die ein std::array<std::string_view, K> liefert.
template <auto TeileFn>
[[nodiscard]] consteval auto stempel_kompositum() {
    constexpr auto        kTeile  = TeileFn();
    constexpr std::size_t kLaenge = [] {
        std::size_t n = 0;
        for (std::string_view const t : TeileFn()) n += t.size();
        return n;
    }();
    StempelKompositum<kLaenge + 1> out{};
    std::size_t                    p = 0;
    for (std::string_view const t : kTeile)
        for (char const c : t) out.chars[p++] = c;
    return out;
}

// ================================================================================================
// P1.3 -- der Vertrag: "Erbin verschaerft, nie aufweicht" als deferred-consteval-Assert-Kette
// ================================================================================================

/// Der angeschlossene()-Vertrag (NUR Hybrid): ein RT-Hook, der die angeschlossenen Stempel-Zeilen als
/// Span liefert. S-1 definiert AUSSCHLIESSLICH diese Signatur; der Datenbestand ist der
/// KON47-02-Init-Cache am Dock, der Bau ist HY-A2 (WARTE-Liste W-D).
/// EXAKTER Span-Rueckgabetyp (same_as, nicht convertible_to): convertible_to liesse auch ein per Wert
/// zurueckgegebenes temporaeres std::array zu, dessen implizit erzeugter Span nach dem Vollausdruck
/// haengt (Zweitlens-Befund 12.08.). API-INVARIANTE dazu: der Backing-Storage des Spans ist statisch
/// langlebig (der Init-Cache am Dock), nie ein Temporaer der Aufruf-Stelle.
template <class E>
concept AngeschlosseneVertrag = requires(E const& e) {
    { e.angeschlossene() } noexcept -> std::same_as<std::span<std::string_view const>>;
};

namespace vertrag_detail {

inline constexpr std::size_t kFingerprintShaHexLaenge = 128; // sha512_len-Doktrin (decl.hpp:224-225)

// -- Existenz-Formen (PFLICHT verlangt die STATISCHE, noexcept-Form; string_view-konvertibel) ----------
template <class E>
inline constexpr bool hat_version_xyz = requires {
    { E::version_xyz() } noexcept -> std::convertible_to<std::string_view>;
};
template <class E>
inline constexpr bool hat_mess_zeile = requires {
    { E::mess_zeile() } noexcept -> std::convertible_to<std::string_view>;
};
template <class E>
inline constexpr bool hat_system_zeile = requires {
    { E::system_zeile() } noexcept -> std::convertible_to<std::string_view>;
};
template <class E>
inline constexpr bool hat_organ_zeile = requires {
    { E::organ_zeile() } noexcept -> std::convertible_to<std::string_view>;
};
template <class E>
inline constexpr bool hat_fingerprint_sha = requires {
    { E::fingerprint_sha() } noexcept -> std::convertible_to<std::string_view>;
};
template <class E>
inline constexpr bool hat_gesamt_stempel = requires {
    { E::gesamt_stempel() } noexcept -> std::convertible_to<std::string_view>;
};

// -- Anwesenheits-Formen fuer VERBOT/ERLAUBT: die Instanz-Form deckt statische WIE nicht-statische
//    Mitglieder (ein VERBOT ist eine Absenz-Pflicht in JEDER Form, KON8-07). ---------------------------
template <class E>
inline constexpr bool vorhanden_version_xyz = requires(E const& e) { e.version_xyz(); };
template <class E>
inline constexpr bool vorhanden_mess_zeile = requires(E const& e) { e.mess_zeile(); };
template <class E>
inline constexpr bool vorhanden_system_zeile = requires(E const& e) { e.system_zeile(); };
template <class E>
inline constexpr bool vorhanden_organ_zeile = requires(E const& e) { e.organ_zeile(); };
template <class E>
inline constexpr bool vorhanden_fingerprint_sha = requires(E const& e) { e.fingerprint_sha(); };
template <class E>
inline constexpr bool vorhanden_gesamt_stempel = requires(E const& e) { e.gesamt_stempel(); };
template <class E>
inline constexpr bool vorhanden_angeschlossene = requires(E const& e) { e.angeschlossene(); };

// -- deferred-consteval-Proben (Muster stamp_line_is_parsable_impl, anatomy_stamp_entries.hpp:454-461):
//    schlaegt die Auswertung im Default-Template-Argument fehl (Wurf des Scanners, nicht-konstanter
//    Ausdruck, fehlendes Mitglied), ist das eine Substitutions-Ablehnung und die '...'-Ueberladung
//    gewinnt -- der Vertrag wird FALSCH, die Uebersetzung der Basis bricht nie. ------------------------

// M/S/O-Zeilen: durch den EINEN Scanner (keine zweite Grammatik-Wahrheit; via Dependenz-Wrapper, s.o.).
template <class E, bool B = detail::zeile_durch_den_einen_scanner(E::mess_zeile())>
consteval bool mess_zeile_scanner_probe(int) {
    return B;
}
template <class E>
consteval bool mess_zeile_scanner_probe(...) {
    return false;
}
template <class E, bool B = detail::zeile_durch_den_einen_scanner(E::system_zeile())>
consteval bool system_zeile_scanner_probe(int) {
    return B;
}
template <class E>
consteval bool system_zeile_scanner_probe(...) {
    return false;
}
template <class E, bool B = detail::zeile_durch_den_einen_scanner(E::organ_zeile())>
consteval bool organ_zeile_scanner_probe(int) {
    return B;
}
template <class E>
consteval bool organ_zeile_scanner_probe(...) {
    return false;
}

// Leerwert-Erkennung (nur nach bestandener Scanner-Probe belastbar).
template <class E, bool B = std::string_view{E::mess_zeile()}.empty()>
consteval bool mess_zeile_leer_probe(int) {
    return B;
}
template <class E>
consteval bool mess_zeile_leer_probe(...) {
    return true;
}
template <class E, bool B = std::string_view{E::system_zeile()}.empty()>
consteval bool system_zeile_leer_probe(int) {
    return B;
}
template <class E>
consteval bool system_zeile_leer_probe(...) {
    return true;
}
template <class E, bool B = std::string_view{E::organ_zeile()}.empty()>
consteval bool organ_zeile_leer_probe(int) {
    return B;
}
template <class E>
consteval bool organ_zeile_leer_probe(...) {
    return true;
}

// Leerwert NUR mit kXxxBewusstLeer + NICHT-LEEREM Grund-Literal: stille Leere ist unmoeglich.
template <class E, bool B = (static_cast<bool>(E::kMessZeileBewusstLeer) &&
                             !std::string_view{E::kMessZeileBewusstLeerGrund}.empty())>
consteval bool mess_zeile_bewusst_leer_probe(int) {
    return B;
}
template <class E>
consteval bool mess_zeile_bewusst_leer_probe(...) {
    return false;
}
template <class E, bool B = (static_cast<bool>(E::kSystemZeileBewusstLeer) &&
                             !std::string_view{E::kSystemZeileBewusstLeerGrund}.empty())>
consteval bool system_zeile_bewusst_leer_probe(int) {
    return B;
}
template <class E>
consteval bool system_zeile_bewusst_leer_probe(...) {
    return false;
}
template <class E, bool B = (static_cast<bool>(E::kOrganZeileBewusstLeer) &&
                             !std::string_view{E::kOrganZeileBewusstLeerGrund}.empty())>
consteval bool organ_zeile_bewusst_leer_probe(int) {
    return B;
}
template <class E>
consteval bool organ_zeile_bewusst_leer_probe(...) {
    return false;
}

// version_xyz: die EINE ce-Politik (algo_semver.hpp) -- wohlgeformt schliesst leer aus.
template <class E, bool B = ist_versions_literal_baustein(std::string_view{E::version_xyz()})>
consteval bool version_xyz_politik_probe(int) {
    return B;
}
template <class E>
consteval bool version_xyz_politik_probe(...) {
    return false;
}

// fingerprint_sha: 128 KLEINBUCHSTABEN-Hex-Zeichen ODER bewusst-leer mit Grund. Die Laenge allein
// genuegt nicht -- 128x'g' oder NUL-Bytes erfuellten sonst den behaupteten SHA-512-Vertrag
// (Zweitlens-Befund 12.08.). Kanonische Schreibweise der kFP-Ausgabe ist lowercase-hex
// (anatomy_fingerprint_hex; decl.hpp:224-225 sha512_len-Doktrin).
[[nodiscard]] consteval bool ist_sha512_hex_128(std::string_view s) noexcept {
    if (s.size() != kFingerprintShaHexLaenge) return false;
    for (char const c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
template <class E, bool B = ist_sha512_hex_128(std::string_view{E::fingerprint_sha()})>
consteval bool fingerprint_sha_laenge_probe(int) {
    return B;
}
template <class E>
consteval bool fingerprint_sha_laenge_probe(...) {
    return false;
}
template <class E, bool B = std::string_view{E::fingerprint_sha()}.empty()>
consteval bool fingerprint_sha_leer_probe(int) {
    return B;
}
template <class E>
consteval bool fingerprint_sha_leer_probe(...) {
    return false;
}
template <class E, bool B = (static_cast<bool>(E::kFingerprintShaBewusstLeer) &&
                             !std::string_view{E::kFingerprintShaBewusstLeerGrund}.empty())>
consteval bool fingerprint_sha_bewusst_leer_probe(int) {
    return B;
}
template <class E>
consteval bool fingerprint_sha_bewusst_leer_probe(...) {
    return false;
}

// gesamt_stempel: konstant auswertbar und nie leer (das Kompositum IST die Identitaet der Erbin).
template <class E, bool B = !std::string_view{E::gesamt_stempel()}.empty()>
consteval bool gesamt_stempel_nicht_leer_probe(int) {
    return B;
}
template <class E>
consteval bool gesamt_stempel_nicht_leer_probe(...) {
    return false;
}

/// ORDNUNGSNEUTRAL (S-6-Sperre): NUR Teilstring-Enthaltensein, KEINE Positions- oder Reihenfolge-Aussage.
/// Nur unter bestandener gesamt_stempel_nicht_leer_probe rufen (dann ist E::gesamt_stempel() konstant
/// auswertbar).
template <class E>
[[nodiscard]] consteval bool gesamt_enthaelt(std::string_view teil) {
    return teil.empty() || std::string_view{E::gesamt_stempel()}.find(teil) != std::string_view::npos;
}

// -- die Pflicht-Pruefung je Interface (existiert + constexpr-auswertbar + noexcept + Wert-Regel) ------
template <class E>
[[nodiscard]] consteval bool version_xyz_pflicht_erfuellt() {
    if constexpr (!hat_version_xyz<E>)
        return false;
    else
        return version_xyz_politik_probe<E>(0);
}
template <class E>
[[nodiscard]] consteval bool mess_zeile_pflicht_erfuellt() {
    if constexpr (!hat_mess_zeile<E>)
        return false;
    else {
        if (!mess_zeile_scanner_probe<E>(0)) return false;
        if (mess_zeile_leer_probe<E>(0) && !mess_zeile_bewusst_leer_probe<E>(0)) return false;
        return true;
    }
}
template <class E>
[[nodiscard]] consteval bool system_zeile_pflicht_erfuellt() {
    if constexpr (!hat_system_zeile<E>)
        return false;
    else {
        if (!system_zeile_scanner_probe<E>(0)) return false;
        if (system_zeile_leer_probe<E>(0) && !system_zeile_bewusst_leer_probe<E>(0)) return false;
        return true;
    }
}
template <class E>
[[nodiscard]] consteval bool organ_zeile_pflicht_erfuellt() {
    if constexpr (!hat_organ_zeile<E>)
        return false;
    else {
        if (!organ_zeile_scanner_probe<E>(0)) return false;
        if (organ_zeile_leer_probe<E>(0) && !organ_zeile_bewusst_leer_probe<E>(0)) return false;
        return true;
    }
}
template <class E>
[[nodiscard]] consteval bool fingerprint_sha_pflicht_erfuellt() {
    if constexpr (!hat_fingerprint_sha<E>)
        return false;
    else {
        if (fingerprint_sha_laenge_probe<E>(0)) return true;
        return fingerprint_sha_leer_probe<E>(0) && fingerprint_sha_bewusst_leer_probe<E>(0);
    }
}
template <class E, StempelTraeger T>
[[nodiscard]] consteval bool gesamt_stempel_pflicht_erfuellt() {
    if constexpr (!hat_gesamt_stempel<E>)
        return false;
    else {
        if (!gesamt_stempel_nicht_leer_probe<E>(0)) return false;
        // "enthaelt jede eigene nicht-leere Zeile + den sha als TEILSTRING" -- je Interface nur dann
        // INSTANZIIERT, wenn es der Traeger nach der Matrix pflichtig traegt UND die Erbin das Mitglied
        // wirklich hat (hat_*-Gate): fehlt ein Pflicht-Mitglied, faellt der Vertrag bereits an dessen
        // eigener Zellen-Pruefung -- ein direkter E::xxx()-Zugriff HIER waere dann kein 'false', sondern
        // ein harter Uebersetzungsfehler am T-4-Gegeneingang (genau das darf die Basis nie).
        bool ok = true;
        if constexpr (stempel_zulassung(StempelInterface::VersionXyz, T) == StempelZulassung::Pflicht &&
                      hat_version_xyz<E>)
            ok = ok && gesamt_enthaelt<E>(std::string_view{E::version_xyz()});
        if constexpr (stempel_zulassung(StempelInterface::MessZeile, T) == StempelZulassung::Pflicht &&
                      hat_mess_zeile<E>)
            ok = ok && gesamt_enthaelt<E>(std::string_view{E::mess_zeile()});
        if constexpr (stempel_zulassung(StempelInterface::SystemZeile, T) == StempelZulassung::Pflicht &&
                      hat_system_zeile<E>)
            ok = ok && gesamt_enthaelt<E>(std::string_view{E::system_zeile()});
        if constexpr (stempel_zulassung(StempelInterface::OrganZeile, T) == StempelZulassung::Pflicht &&
                      hat_organ_zeile<E>)
            ok = ok && gesamt_enthaelt<E>(std::string_view{E::organ_zeile()});
        if constexpr (stempel_zulassung(StempelInterface::FingerprintSha, T) == StempelZulassung::Pflicht &&
                      hat_fingerprint_sha<E>)
            ok = ok && gesamt_enthaelt<E>(std::string_view{E::fingerprint_sha()});
        return ok;
    }
}
template <class E>
[[nodiscard]] consteval bool angeschlossene_pflicht_erfuellt() {
    return AngeschlosseneVertrag<E>;
}

/// Die Zellen-Pruefung: EIN Interface eines Traegers nach seiner Matrixzelle.
template <class E, StempelTraeger T, StempelInterface I>
[[nodiscard]] consteval bool interface_vertrag() {
    constexpr StempelZulassung kZelle = stempel_zulassung(I, T);
    if constexpr (I == StempelInterface::VersionXyz) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_version_xyz<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_version_xyz<E> || version_xyz_pflicht_erfuellt<E>();
        else
            return version_xyz_pflicht_erfuellt<E>();
    } else if constexpr (I == StempelInterface::MessZeile) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_mess_zeile<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_mess_zeile<E> || mess_zeile_pflicht_erfuellt<E>();
        else
            return mess_zeile_pflicht_erfuellt<E>();
    } else if constexpr (I == StempelInterface::SystemZeile) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_system_zeile<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_system_zeile<E> || system_zeile_pflicht_erfuellt<E>();
        else
            return system_zeile_pflicht_erfuellt<E>();
    } else if constexpr (I == StempelInterface::OrganZeile) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_organ_zeile<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_organ_zeile<E> || organ_zeile_pflicht_erfuellt<E>();
        else
            return organ_zeile_pflicht_erfuellt<E>();
    } else if constexpr (I == StempelInterface::FingerprintSha) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_fingerprint_sha<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_fingerprint_sha<E> || fingerprint_sha_pflicht_erfuellt<E>();
        else
            return fingerprint_sha_pflicht_erfuellt<E>();
    } else if constexpr (I == StempelInterface::GesamtStempel) {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_gesamt_stempel<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_gesamt_stempel<E> || gesamt_stempel_pflicht_erfuellt<E, T>();
        else
            return gesamt_stempel_pflicht_erfuellt<E, T>();
    } else {
        if constexpr (kZelle == StempelZulassung::Verboten)
            return !vorhanden_angeschlossene<E>;
        else if constexpr (kZelle == StempelZulassung::Erlaubt)
            return !vorhanden_angeschlossene<E> || angeschlossene_pflicht_erfuellt<E>();
        else
            return angeschlossene_pflicht_erfuellt<E>();
    }
}

/// Die volle Assert-Kette eines Traegers. KURZSCHLUSS-Ordnung: die Wert-tragenden Interfaces zuerst,
/// gesamt_stempel ZULETZT -- seine Teilstring-Pruefung wertet die anderen Getter nur aus, wenn deren
/// eigene Pruefung schon bestanden ist (kein harter Fehler an einer nicht-konstanten Erbin).
template <class E, StempelTraeger T>
[[nodiscard]] consteval bool stempel_vertrag_erfuellt() {
    bool ok = true;
    ok      = ok && interface_vertrag<E, T, StempelInterface::VersionXyz>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::MessZeile>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::SystemZeile>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::OrganZeile>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::FingerprintSha>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::Angeschlossene>();
    ok      = ok && interface_vertrag<E, T, StempelInterface::GesamtStempel>();
    return ok;
}

} // namespace vertrag_detail

/// StempelVertrag<Erbin, Traeger> -- DER Zwang (P1.2): jede Erbin schliesst direkt unter ihrer
/// Definition mit static_assert(StempelVertrag<Erbin, Traeger>). Verlangt die CRTP-Anbindung an
/// StempelBasis (mit dem RICHTIGEN Traeger) und die volle Assert-Kette der Matrixzeile.
template <class Erbin, StempelTraeger Traeger>
concept StempelVertrag = std::is_class_v<Erbin> && std::derived_from<Erbin, StempelBasis<Erbin, Traeger>> &&
                         vertrag_detail::stempel_vertrag_erfuellt<Erbin, Traeger>();

// ================================================================================================
// P1.4 -- Baustein-Ebene: Anbindung des Bestands OHNE Basisklassen-Einbau
// ================================================================================================
// AUSGELAGERT 12.08.2026 nach stempel_baustein_trait.hpp (leichter Trait-Header, oben inkludiert):
// Rolle + Primaertrait + Tag + Concept stehen dort, damit die ABI-POD-Spezialisierungen direkt beim
// Eigentuemer (anatomy_module_abi_v1_decl.hpp) veroeffentlicht werden koennen -- die Include-Order-
// Falle "Trait vor entries.hpp instanziiert => still false/Keine bzw. specialization after
// instantiation" (Zweitlens-Befund 12.08.) ist damit strukturell geschlossen.

} // namespace comdare::cache_engine::abi
