#pragma once
// profile_facade/toolchain_stamp_naht.hpp -- NB/CX-4 (Neuanker-Nachbesserung 06.08.2026): die EINE Stelle,
// an der die LIVE-Werte der Preimage-Glieder [5] (Toolchain) und [6] (bvset) BESTIMMT und in die
// Compile-Define-Wertform gebracht werden. Spiegel und Praezedenz: system_cell_values_naht.hpp (W10-C4).
//
// WAS HIER GELOEST WIRD: C-2 hat den SLOT gebaut (abi/toolchain_stamp_glied.hpp rendert, abi/
// anatomy_fingerprint.hpp haelt die Positionen), aber beide Defines standen auf ihrem Default "" -- der
// produktive Fingerprint trug die zwei neuen Glieder also NICHT. Damit war die C1-/C6-Luecke formal noch
// offen: zwei Baue derselben Permutation mit anderer Toolchain bzw. anderer Enable-Menge hatten weiter
// denselben Digest. Ab hier tragen sie die Glieder LIVE.
//
// DIE HARTE BEDINGUNG DIESER NAHT -- KEINE DRIFT (Auflage aus dem C-2-Uebergabe-Punkt, verbatim): "die
// Toolchain-/bvset-Werte muessen GLEICHZEITIG (a) in den CEB-Laufzeit-Zwilling UND (b) als
// -DCOMDARE_TOOLCHAIN_STAMP_GLIED / -DCOMDARE_BUILD_VARIANT_SET_SIGNATURE an perm_compile gehen. Wird nur
// eine Seite verdrahtet, rechnet der consteval-Zwilling in der Tier-Binary ueber ANDERE Glieder als die
// CEB." Genau deshalb sind die beiden Komposition-Funktionen unten ARGUMENTLOS und REIN: derselbe Aufruf
// liefert im Bau-Kanal (perm_compile_flags -> Define) und im Laufzeit-Zwilling
// (make_lazy_adhoc_fingerprint_fn_from_env) byte-identische Strings. Eine Signatur mit per-Perm-Argumenten
// koennte diese Gleichheit nicht mehr strukturell garantieren, sondern nur behaupten.
//
// [HISTORIK, GESCHLOSSEN MIT dbdd2f9b8 ("T2-B", 2026-08-06) -- der folgende Absatz beschreibt den Stand
// BIS zu jenem Commit und bleibt als Begruendung der Bau-Reihenfolge stehen (Doku-Doktrin: nicht
// loeschen, nachfuehren). LEBENDER STAND: opt, ext UND gate stehen im Live-Glied. Sie kommen per
// Permutation ueber compose_toolchain_stamp_glied_for_perm (unten, :368-393) herein und gehen aus EINEM
// Aufruf gleichzeitig in den Bau-Kanal UND in den CEB-Laufzeit-Zwilling -- also genau in der Form, die
// der Absatz darunter als Bedingung nennt ("in EINEN Schnitt mit der Perm-Schleife"). Die
// Zweit-Ableitung, vor der er warnt, ist damit weiterhin nicht noetig und findet nicht statt: der Wert
// wird DURCHGEREICHT, nicht aus opt_flag/march_flag zurueckgerechnet. Task #61.]
//
// WAS BEWUSST (NOCH) NICHT IM LIVE-GLIED STEHT -- ehrlich benannt statt still weggelassen: die
// PER-PERM-Felder opt/ext/gate. Sie entstehen in der optxsimd-Schleife (profile_run_entry.hpp /
// experiment_run_entry.hpp) und erreichen diese Naht nur ueber die compile_for_perm-Signatur; der
// Laufzeit-Zwilling bekommt sie an derselben Stelle NICHT. Sie hier aus opt_flag/march_flag
// zurueckzurechnen waere eine ZWEIT-ABLEITUNG (genau das, was die Zellwert-Naht ausdruecklich verbietet:
// "KEINE Zweit-Ableitung, insbesondere kein Rueckschluss aus dem -march-Flag"), und sie nur auf der
// Bau-Seite zu setzen waere die oben zitierte Drift. Ihre Verdrahtung gehoert deshalb in EINEN Schnitt mit
// der Perm-Schleife (Buendel-Scheibe C-3). Die run-KONSTANTEN Felder (cxx, ceb, bt) sind ab hier live --
// sie schliessen den Compiler-/Contract-/Build-Typ-Teil der C1-Luecke.
//
// header-only, keine Bau-Abhaengigkeit ausser der Treiber-Signatur (die ist compile-time).

#include "build_type_stamp.hpp"      // (i) build_type_version_value(): DIESELBE Env-Entscheidung wie der Suffix
#include "system_version_suffix.hpp" // die EINE Suffix-Quelle + toolchain_stamp_parts_from_suffix_parts

#include <builder/driver_build_variant_signature.hpp> // kDriverBuildVariantSignature (CT-Zwilling, Glied [6])

// T2-B: das globale Umbrella-Gate der atomic128-Wahl. Es MUSS hier stehen und nicht bloss beim Aufrufer:
// die Entscheidung ist ein #if, und ein Header, der sie ohne den definierenden Header trifft, faellt in
// jeder TU anders aus, die ihn frueher inkludiert. Genau so entstuende ein Glied, das je Uebersetzungs-
// einheit etwas anderes behauptet -- die Drift-Klasse, gegen die diese Naht gebaut ist.
#include <axes/alloc/axis_06_allocator_flags.hpp> // COMDARE_AXIS_06_USE_SNMALLOC (globales Umbrella-Gate)

#include <cache_engine/abi/stempel_basis.hpp>                    // S-1: ist_stempel_baustein (Baustein-Anbindung)
#include <cache_engine/abi/toolchain_stamp_glied.hpp>            // Renderer + die CT-Compiler-Realversions-Erhebung
#include <system_axes/compiler_atomic_sub_axis.hpp> // T2-B: Cx16Option/-mcx16 (Single-Source)
#include <system_axes/compiler_system_axis.hpp>     // Dialekt-Ids + driver_default (Single-Source)

#include <array>
#include <cstdio> // T2-C: popen/pclose -- die RT-Realversions-Sonde startet den Treiber selbst
#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::profile_facade {

/// active_cxx_driver_tag() -- der TREIBER, mit dem die Tier-Binaries dieses Laufs uebersetzt werden.
/// EINE Quelle: der Env-Override, sonst der Default-Treiber der Compiler-System-Achse (INC-1h). Die Facade
/// (profile_run_facade cxx_compiler) zieht ab NB/CX-4 hier durch, statt die Entscheidung ein zweites Mal
/// zu buchstabieren -- sonst koennte der Treiber, ueber den das Glied urteilt, ein anderer sein als der,
/// der wirklich compiliert.
[[nodiscard]] inline std::string active_cxx_driver_tag() {
    if (char const* e = std::getenv("COMDARE_CXX"); e != nullptr && *e != '\0') return e;
    return std::string{::comdare::cache_engine::measurement::GccCompilerAxis::driver_default()};
}

/// Der DIALEKT des konfigurierten Treibers -- dieselbe eine Regel, nach der die Facade schon heute das
/// -fno-gnu-unique-Gate stellt (Treiber-Tag enthaelt "clang" => clang-Leg, sonst gcc-Leg). Die Ids kommen
/// aus der Achse, nie als Literal.
[[nodiscard]] inline std::string_view cxx_driver_dialect(std::string_view driver_tag) noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
    return driver_tag.find("clang") != std::string_view::npos ? cm::ClangCompilerAxis::compiler_id()
                                                              : cm::GccCompilerAxis::compiler_id();
}

/// Die VOLLE Versions-Kennung, die der Treiber-Tag SELBST nennt: der abschliessende Lauf aus Ziffern UND
/// Punkten ("g++-16" -> "16", "clang++-22" -> "22", "g++-16.2.0" -> "16.2.0").
///
/// STRENG UND FAIL-CLOSED: nur der Lauf am ENDE des Tags zaehlt. Ein Tag ohne Endziffern ("g++",
/// "/usr/bin/c++") nennt seine Version nicht -- dann wird auch nichts behauptet. Ein Rueckschluss aus einem
/// Pfad-Bestandteil (etwa "llvm-22" mitten im Pfad) waere geraten, nicht gelesen.
///
/// NB2-1 -- WARUM "VOLL" UND NICHT NUR DIE LETZTE ZIFFERNFOLGE: der Vorgaenger las ausschliesslich die
/// TRAILING DIGITS. Fuer "g++-16.1" ergab das "1" und wurde dann gegen den CEB-MAJOR verglichen -- ein Tag,
/// der seine Minor-Version nennt, wurde also mit einer Zahl abgeglichen, die gar nicht sein Major ist. Der
/// Punkt gehoert deshalb in den Lauf.
[[nodiscard]] inline std::string_view cxx_driver_version_kennung(std::string_view driver_tag) noexcept {
    if (driver_tag.empty()) return {};
    std::size_t const ende = driver_tag.size();
    if (driver_tag[ende - 1] < '0' || driver_tag[ende - 1] > '9') return {};
    std::size_t start = ende;
    while (start > 0) {
        char const c = driver_tag[start - 1];
        if ((c >= '0' && c <= '9') || c == '.') {
            --start;
            continue;
        }
        break;
    }
    return driver_tag.substr(start, ende - start);
}

/// ct_realversion_deckt_treiber(tag) -- DIE EHRLICHKEITS-WACHE der Realversion.
///
/// WARUM SIE NOETIG IST (am Objekt gemessen, nicht theoretisch): abi::kDetectedCompilerRealVersion ist die
/// Version DERJENIGEN Uebersetzung, in der sie ausgewertet wird -- hier also die der CEB. Die Tier-Binaries
/// baut aber der Treiber aus active_cxx_driver_tag(). Auf dieser Maschine sind das VERSCHIEDENE Compiler
/// (CMAKE_CXX_COMPILER=/usr/bin/c++ gegen den Achsen-Default g++-16). Die CEB-Version als "die Version der
/// Tier-Uebersetzung" zu stempeln waere damit schlicht falsch.
///
/// -- NB2-1: DIE WACHE WAR FAIL-OPEN, JETZT IST SIE FAIL-CLOSED ----------------------------------------
///
/// DER BEFUND (Codex-Zweitreview [KRITISCH], verbatim): "Realversions-Deckung prueft nur Dialekt+Major des
/// Treiber-Tags und uebernimmt dann minor.patch der CEB-Toolchain -> g++-16=16.1 und g++-16=16.3 werden
/// beide als CEB-Version (z.B. 16.2.0) gestempelt". Der Grund ist strukturell: ein Tag, der NUR den Major
/// nennt, PINNT die Version nicht. Er ist mit jeder 16.x vertraeglich. Aus "beide sind 16" folgt eben
/// nicht "es ist derselbe Compiler" -- und nur dann duerfte die CEB-Version als Tier-Version gelten.
///
/// DIE NEUE DECKUNGS-BEDINGUNG, alle drei Teile GELESEN und keiner geraten:
///   (1) die CEB-Erhebung ist ueberhaupt bekannt (Dialekt + Realversion),
///   (2) der DIALEKT des Tags stimmt mit dem der CEB-Uebersetzung ueberein,
///   (3) die VOLLE Versions-Kennung des Tags ist BYTE-GLEICH der CEB-Realversion -- also alle drei Zahlen,
///       nicht bloss der Major. Erst damit sagt der Tag SELBST dieselbe Version, die die CEB gemessen hat;
///       die Behauptung "CEB-Compiler == Tier-Treiber" ruht dann auf zwei unabhaengigen Lesungen desselben
///       Werts statt auf einer Vermutung.
///
/// WAS DAS IN DER PRAXIS HEISST -- ehrlich benannt, nicht schoengeredet: uebliche Treiber-Tags ("g++-16")
/// nennen nur den Major und decken die Realversion damit NICHT mehr. Das Glied traegt dann keine
/// Versions-Behauptung. Das ist KEIN Informationsverlust gegenueber dem Vor-Stand, denn die dort
/// gestempelte Version war nicht gedeckt; und es ist auch kein Injektivitaets-Verlust, weil das cxx-Feld
/// seit NB2-1 (R1) IMMER den Treiber-Tag traegt -- g++-17 und g++-18 sind ab jetzt strukturell
/// unterscheidbar, gerade OHNE Versions-Behauptung. Die n/a-statt-NULL-Regel der Nachbar-Naht gilt
/// unveraendert: lieber eine schwaechere, wahre Aussage als eine starke, ungedeckte.
///
/// DER WEG ZU MEHR (deklariert, nicht geraten): eine wirklich per-Treiber erhobene Realversion braucht eine
/// EIGENE Erhebung an der bauenden Stufe (der Tier-Treiber stempelt seine eigene __GNUC__-Wahrheit). Das
/// ist ein Fingerprint-Ereignis mit eigenem Anker und gehoert nicht in dieses Nachbesserungs-Fenster.
[[nodiscard]] inline bool ct_realversion_deckt_treiber(std::string_view driver_tag) noexcept {
    namespace cea = ::comdare::cache_engine::abi;
    if (!cea::kDetectedCompilerIsKnown) return false;
    if (cxx_driver_dialect(driver_tag) != cea::kDetectedCompilerDialect) return false;
    std::string_view const tag_version = cxx_driver_version_kennung(driver_tag);
    if (tag_version.empty()) return false;
    return tag_version == cea::kDetectedCompilerRealVersion;
}

// -- T2-B: DIE PER-PERM-ACHSEN DES GLIEDS [5] ---------------------------------------------------------
//
// DER BEFUND, DEN DAS SCHLIESST (Codex-Zweitreview [CX-B1], KRITISCH, verbatim): "Live-Glied[5] nur
// cxx/ceb/bt -> O2/O3 derselben Zelle identischer Fingerprint = falscher Skip". Der Kopf dieses Headers
// hat den Zustand damals ehrlich benannt und die Heilung ausdruecklich in diese Scheibe verwiesen: die
// Felder opt/opt_flags/ext/gate entstehen in der optxsimd-Schleife (profile_run_entry.hpp /
// experiment_run_entry.hpp) und erreichen die Naht NUR ueber die compile_for_perm-Signatur.
//
// WARUM EIN TRAEGER UND KEINE VIER STRINGS: die Fabrik-Signatur traegt bereits opt_flag und march_flag als
// nackte Strings. Vier weitere gleichartige Strings daneben waeren genau die Vertausch-Falle, gegen die
// W10-C4 den benannten SystemCellValues-Traeger eingefuehrt hat (K-1-Muster). PermToolchainAchsen sammelt
// sie stattdessen zu EINEM benannten Argument mit sprechenden Feldnamen.
//
// KEINE ZWEIT-ABLEITUNG (bindende Regel der Nachbar-Naht, hier wortgleich): die Werte kommen als das,
// was die Schleife WIRKLICH permutiert -- opt_id/simd_id und die daraus AUFGELOESTEN Achsen-Flags. Nichts
// wird aus dem -march-Flag zurueckgerechnet, nichts geraten.
//
// EHRLICHE GRENZE, benannt statt verschwiegen -- DER DEBUG-FALL: bei Build-Typ Debug ersetzt die CompileFn
// die Optimierungs-Flags durch ex::debug_flags_for_toolchain() ("-O0 -g"), waehrend das Glied weiter die
// opt-ACHSE dieser Permutation nennt (opt=O3{-O3}). Zwei Debug-Permutationen O2 und O3 erzeugen damit
// byte-gleiche Binaries unter VERSCHIEDENEN Fingerprints. Das ist eine UEBER-Diskriminierung: sie kostet
// einen Neubau, sie erzeugt keinen falschen Skip -- also die fail-closed Richtung. Sie ist zudem sichtbar,
// weil +bt=Debug im selben Glied steht. Die Alternative (die effektiven Flags stempeln) waere eine
// Zweit-Ableitung aus dem CompileFn-Inneren und wuerde ausserdem Whitespace ins Glied tragen, den die
// Transport-Wache (T2-D) zu Recht verbietet.

/// T2-B: der FERTIG GERENDERTE per-Perm-Wert des Glieds [5] als benannter Traeger mit EIGENEM Speicher.
///
/// WARUM OWNING UND NICHT WIE abi::SystemCellValues EINE SICHT: die Fabrik compile_for_perm haelt den Wert
/// bis zum Bau jeder einzelnen Binary dieser Permutation. Eine Sicht muesste sich auf eine Variable im
/// Schleifen-Rumpf stuetzen -- exakt die Lebensdauer-Falle, die T2-D an den abi-Traegern geschlossen hat
/// (dort compile-hart durch den geloeschten Rvalue-Konstruktor). Hier kostet Eigentum nichts: der Wert ist
/// ein reiner Laufzeit-String, es gibt keine consteval-Seite, die er verbauen koennte.
///
/// WARUM UEBERHAUPT EIN TYP UND KEIN NACKTER std::string: die Fabrik-Signatur traegt schon zwei
/// gleichartige Strings (opt_flag, march_flag). Ein dritter koennte an einem Alt-Aufruf still in den
/// falschen Slot rutschen -- dieselbe K-1-Begruendung wie bei SystemCellValues und OverlayHash.
struct PermToolchainGliedWert {
    std::string value;
};

/// T2-B: die per-Permutation aufgeloesten Toolchain-Achsen. LEER == die run-konstante Identitaet, also
/// exakt der Vor-T2-B-Wert -- der Einzel-Pfad und jeder Aufrufer ohne System-Achsen rechnet unveraendert.
struct PermToolchainAchsen {
    std::string_view opt{};               ///< opt_level-id dieser Permutation ("O3")
    std::string_view opt_flags{};         ///< die AUFGELOESTEN Flags dieser opt-Achse ("-O3")
    std::string_view simd{};              ///< simd-id dieser Permutation (no_extension => leer gereicht)
    std::string_view gate_contribution{}; ///< die Gate-Beitraege dieser Permutation
    std::string_view target_isa{};        ///< Ziel-ISA-Segment, falls der Pfad eines fuehrt
    std::string_view telemetry{};         ///< Telemetrie-Segment, falls der Pfad eines fuehrt
};

// -- S-1 (P3): Baustein-Anbindung der beiden Naht-Glieder UNTER ihren Definitionen -- OHNE
//    Basisklassen-Einbau (beide sind positional-init-Aggregate; eine leere Basis fraesse den ersten
//    Initialisierer). Die Spezialisierung gehoert in den abi-Namensraum, deshalb das kurze Fenster.
} // namespace comdare::cache_engine::profile_facade
namespace comdare::cache_engine::abi {
template <>
struct ist_stempel_baustein<profile_facade::PermToolchainGliedWert>
    : StempelBausteinTag<StempelBausteinRolle::NahtGlied> {};
template <>
struct ist_stempel_baustein<profile_facade::PermToolchainAchsen> : StempelBausteinTag<StempelBausteinRolle::NahtGlied> {
};
static_assert(StempelBaustein<profile_facade::PermToolchainGliedWert> &&
              StempelBaustein<profile_facade::PermToolchainAchsen>);
} // namespace comdare::cache_engine::abi
namespace comdare::cache_engine::profile_facade {

/// T2-B: die atomic128-Wahl DIESES Baus -- die EINE Quelle fuer Glied und Compile-Flag zugleich.
///
/// WARUM HIER UND NICHT (nur) IN DER FACADE: perm_compiler_isa_cflags() traf dieselbe Entscheidung ueber
/// dasselbe #if. Zwei Orte, eine Entscheidung -- also die Konstellation, aus der jede Divergenz dieses
/// Fensters entstanden ist. Ab hier liest die Facade diese Funktion, statt die Bedingung ein zweites Mal
/// zu buchstabieren; damit kann das Glied gar keine andere atomic128-Wahl behaupten als die gebaute.
///
/// Die Achse ist RUN-KONSTANT (CT-Gate: snmalloc-Organ + x86_64), deshalb steht sie in beiden Wegen --
/// im run-konstanten Live-Glied wie in jeder Permutation.
struct Atomic128Wahl {
    std::string_view id{};
    std::string_view flags{};
};

[[nodiscard]] inline Atomic128Wahl active_atomic128_wahl() noexcept {
    namespace cm = ::comdare::cache_engine::measurement;
#if defined(COMDARE_AXIS_06_USE_SNMALLOC) && COMDARE_AXIS_06_USE_SNMALLOC && defined(COMDARE_ARCH_X86_64)
    return Atomic128Wahl{cm::Cx16Option::atomic128_id(), cm::Cx16Option::gcc_flag()};
#else
    return Atomic128Wahl{cm::DefaultCompilerAtomicOption::atomic128_id(), cm::DefaultCompilerAtomicOption::gcc_flag()};
#endif
}

// -- T2-C: DIE RT-REALVERSIONS-SONDE AM TIER-TREIBER SELBST -------------------------------------------
//
// DER BEFUND, DEN DAS SCHLIESST (Codex-Zweitreview [K], KRITISCH, Ledger vormittag-1 verbatim):
// "Tier-Treiber-REALVERSION wird nie gemessen (g++-16-Binary 16.1->16.3 = identischer Stempel =
// falscher Skip)". NB2-1 hat das Problem korrekt DIAGNOSTIZIERT und fail-closed entschaerft -- ohne
// bewiesene Deckung behauptete das Glied gar keine Version mehr. Das war ehrlich, aber es blieb ein
// Verzicht: zwei Baue mit g++-16 = 16.0.1 und g++-16 = 16.3.0 tragen weiterhin DASSELBE Glied, weil der
// Tag identisch ist. Der Skip zwischen ihnen ist damit weiter falsch, nur nicht mehr auf einer LUEGE
// gegruendet, sondern auf einer LUECKE.
//
// DIE HEILUNG, die NB2-1 selbst als "der Weg zu mehr" deklariert hat: die Version wird am TIER-TREIBER
// ERHOBEN, statt von der CEB geerbt. Das ist die Laufzeit-Factory-Doktrin des Owners, wortgleich
// angewandt (Hardware-Werte nie statisch -- Laufzeit-Erhebung je ISAxOS): der Treiber sagt seine Version
// selbst, wir lesen sie, wir raten sie nicht.
//
// EINMAL JE TAG, GECACHT: die Sonde startet einen Prozess. In einem Lauf mit hunderten Permutationen
// waere ein Aufruf je Permutation absurd -- und er koennte zwischen zwei Permutationen sogar
// VERSCHIEDENE Antworten geben (ein Toolchain-Wechsel mitten im Lauf). Der Cache je Tag macht die
// Erhebung damit nicht nur billig, sondern auch KONSISTENT: ein Lauf hat genau eine Wahrheit je Tag.
//
// FAIL-CLOSED, DREISTUFIG -- und die dritte Stufe ist die eigentliche Zusage:
//   (1) Der Tag ist nicht sondierbar (s. treiber_tag_ist_sondierbar) -> Version UNBEKANNT.
//   (2) Die Sonde laeuft nicht oder antwortet unbrauchbar               -> Version UNBEKANNT.
//   (3) UNBEKANNT heisst NICHT SKIP-FAEHIG. Nicht "nimm halt den Tag" -- die Identitaet der Binary ist
//       dann schlicht nicht vollstaendig bestimmt, und eine unbestimmte Identitaet darf keinen Skip
//       tragen. Mechanisch faellt dafuer der Fingerprint-Provider weg (dieselbe Mechanik wie beim
//       `na`-Zellwert, W10-C4): kein Provider -> kein .fingerprint -> dll_is_current gibt bei leerer
//       Erwartung IMMER false zurueck (build_orchestrator.hpp:305, Punkt (1) der dortigen Regel) ->
//       ehrlicher Neubau statt geratener Wiederverwendung.
//
// KEINE PHANTOM-VERSIONEN (Owner-KERN frueh-12, verbatim: "wir arbeiten mit gcc 15.3 und die neueste
// Version (die ueberhaupt existieren kann) ist gcc 16"): die Sonde erfindet nichts. Sie liefert genau
// das, was der Treiber ausgibt -- auf dieser Flotte g++ -> 15.3.0 und g++-16 -> 16.0.1.

namespace detail {

/// T2-C: EIN Kommando starten und seine ERSTE Ausgabezeile lesen. Bewusst winzig: die Sonde stellt genau
/// eine Frage, deren Antwort ein einziges Token ist. Ein allgemeiner Prozess-Runner waere hier mehr
/// Angriffsflaeche als Nutzen.
[[nodiscard]] inline std::optional<std::string> erste_ausgabe_zeile(std::string const& cmd) {
#if defined(_WIN32)
    std::FILE* p = ::_popen(cmd.c_str(), "r");
#else
    std::FILE* p = ::popen(cmd.c_str(), "r");
#endif
    if (p == nullptr) return std::nullopt;
    std::array<char, 256> puffer{};
    char const*           gelesen = std::fgets(puffer.data(), static_cast<int>(puffer.size()), p);
#if defined(_WIN32)
    int const rc = ::_pclose(p);
#else
    int const rc = ::pclose(p);
#endif
    if (gelesen == nullptr || rc != 0) return std::nullopt; // (2) keine Antwort oder Fehler-Exit
    std::string zeile{puffer.data()};
    while (!zeile.empty() && (zeile.back() == '\n' || zeile.back() == '\r')) zeile.pop_back();
    if (zeile.empty()) return std::nullopt;
    return zeile;
}

} // namespace detail

/// T2-C: DARF dieser Tag ueberhaupt an eine Shell? Der Tag kommt aus COMDARE_CXX, also von aussen.
///
/// WARUM DIESE WACHE EIGENSTAENDIG DASTEHT: die Glied-Wachen fragen "ist der Text im Stempel eindeutig
/// zerlegbar und transportfaehig" -- sie sind DENYLISTEN und lassen z.B. '$', '&', '|', '(' passieren,
/// weil die im PREIMAGE voellig harmlos sind. In einer Kommandozeile sind sie es nicht. Deshalb wird hier
/// zusaetzlich die ALLOWLIST des Preimage-Zeichenvorrats verlangt (abi::injizierter_glied_wert_ist_
/// wohlgeformt): uebrig bleiben Buchstaben, Ziffern und '. , + - _ /', also genau die Form, die ein
/// Compiler-Treiber-Tag oder -Pfad real hat. Alles andere wird NICHT sondiert -- Version unbekannt,
/// fail-closed, statt einer Zeichenkette aus der Umgebung eine Shell zu oeffnen.
[[nodiscard]] inline bool treiber_tag_ist_sondierbar(std::string_view tag) noexcept {
    if (tag.empty()) return false;
    if (!::comdare::cache_engine::abi::toolchain_treiber_tag_ist_wohlgeformt(tag)) return false;
    return ::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(tag);
}

/// T2-C: eine Sonden-ANTWORT ist brauchbar, wenn sie wie eine Version aussieht -- Ziffern und Punkte,
/// beginnend mit einer Ziffer. Ein Treiber, der auf `-dumpfullversion` etwas anderes sagt (Fehlertext,
/// Banner, leere Zeile), hat die Frage nicht beantwortet; dann wird auch nichts behauptet.
[[nodiscard]] inline bool sonden_antwort_ist_version(std::string_view v) noexcept {
    if (v.empty() || v.front() < '0' || v.front() > '9') return false;
    for (char const c : v) {
        if (!((c >= '0' && c <= '9') || c == '.')) return false;
    }
    // Die Klebepunkt-Regel des cxx-Feldes gilt weiter: keine '-'/':'/Struktur-Zeichen (hier per
    // Konstruktion erfuellt, aber die Wache steht da, wo die Zusage gebraucht wird).
    return ::comdare::cache_engine::abi::toolchain_realversion_ist_wohlgeformt(v);
}

/// T2-C: die REALVERSION DIESES Tier-Treibers, am Treiber selbst erhoben. std::nullopt == UNBEKANNT.
/// Einmal je Tag; das Ergebnis (auch das negative) wird gecacht, damit ein fehlender Treiber nicht je
/// Permutation erneut einen Prozessstart kostet.
[[nodiscard]] inline std::optional<std::string> tier_realversion_von(std::string const& driver_tag) {
    static std::mutex                                        schloss;
    static std::map<std::string, std::optional<std::string>> cache;
    std::lock_guard<std::mutex> const                        sperre{schloss};
    if (auto const it = cache.find(driver_tag); it != cache.end()) return it->second;

    std::optional<std::string> ergebnis = std::nullopt;
    if (treiber_tag_ist_sondierbar(driver_tag)) {
        namespace cm = ::comdare::cache_engine::measurement;
        // Die Frage je Dialekt: gcc kennt -dumpfullversion (volle X.Y.Z), clang antwortet auf
        // -dumpversion mit derselben Form. Beide geben EIN Token -- deshalb reicht die erste Zeile.
        std::string_view const frage = cxx_driver_dialect(driver_tag) == cm::ClangCompilerAxis::compiler_id()
                                           ? "-dumpversion"
                                           : "-dumpfullversion";
        std::string const      cmd   = driver_tag + " " + std::string{frage} + " 2>/dev/null";
        if (auto const zeile = detail::erste_ausgabe_zeile(cmd); zeile.has_value()) {
            if (sonden_antwort_ist_version(*zeile)) ergebnis = *zeile;
        }
    }
    cache.emplace(driver_tag, ergebnis);
    return ergebnis;
}

/// T2-C: die Realversion des AKTIVEN Tier-Treibers (leer == UNBEKANNT).
[[nodiscard]] inline std::string active_tier_realversion() {
    auto const v = tier_realversion_von(active_cxx_driver_tag());
    return v.has_value() ? *v : std::string{};
}

/// T2-C: DIE SKIP-FAEHIGKEITS-FRAGE. Ist sie mit NEIN beantwortet, darf keine Binary dieses Laufs ueber
/// den Fingerprint uebersprungen werden -- die Aufrufer loeschen dafuer ihren Fingerprint-Provider
/// (Mechanik und Praezedenz: der `na`-Zellwert der W10-C4-Naht).
[[nodiscard]] inline bool tier_realversion_ist_bekannt() { return !active_tier_realversion().empty(); }

/// compose_toolchain_stamp_glied_for_perm(achsen) -- DER EINE Renderer-Weg des Glieds [5].
///
/// Die Felder kommen aus der EINEN Suffix-Quelle (SystemVersionSuffixParts + der Konverter
/// toolchain_stamp_parts_from_suffix_parts), damit Glied und build_version-Suffix nicht getrennt driften
/// koennen -- die Ordnungs-Deckung beider ist zusaetzlich per static_assert bewiesen.
///
/// NB2-1: das cxx-Feld traegt IMMER den Tier-Treiber-Tag und NUR BEI BEWIESENER DECKUNG die
/// CEB-Realversion (Regel R1/R2 in abi/toolchain_stamp_glied.hpp, Deckungs-Wache
/// ct_realversion_deckt_treiber oben). Damit ergeben verschiedene Treiber-Tags strukturell verschiedene
/// Glieder -- der Fail-open-Fall "g++-17 und g++-18 kollabieren auf cxx=gcc" kann nicht mehr entstehen.
///
/// T2-B: DIE ZUSAGE DER REINHEIT BLEIBT. Auch diese Funktion ist REIN -- gleiche Achsen rein, gleicher
/// String raus. Genau das traegt die Drift-Freiheit: der Bau-Kanal (perm_compile_flags -> Define) und der
/// Laufzeit-Zwilling (make_lazy_adhoc_fingerprint_fn_from_env) bekommen denselben PermToolchainAchsen-Wert
/// aus derselben Schleifen-Iteration und rufen dieselbe Funktion. Eine per-Perm-Signatur allein waere
/// keine Garantie -- die Garantie ist, dass BEIDE Seiten aus EINEM Aufruf gespeist werden (der Aufrufer
/// bildet den String genau einmal und reicht ihn zweimal weiter).
[[nodiscard]] inline std::string compose_toolchain_stamp_glied_for_perm(PermToolchainAchsen const& achsen) {
    std::string const driver_tag = active_cxx_driver_tag();
    std::string const ceb        = ceb_contract_version_text();
    std::string const bt         = ::comdare::cache_engine::thesis_lazy::build_type_version_value();

    SystemVersionSuffixParts p{};
    p.cxx        = driver_tag; // NB2-1 (R1): der Tier-Treiber-Tag ist der Identitaets-Diskriminator
    p.ceb        = ceb;        // Perm-Pfad-Wert, G-C2 "heilt Fall C" -- der Contract-Bump wirkt ab hier im Preimage
    p.build_type = bt;         // (i): "Debug" nur im Debug-Bau, sonst leer => kein Segment
    // T2-B: die per-Perm-Achsen. LEER (Default) => kein Segment => der run-konstante Vor-T2-B-Wert.
    p.opt               = achsen.opt;
    p.simd              = achsen.simd;
    p.target_isa        = achsen.target_isa;
    p.telemetry         = achsen.telemetry;
    p.gate_contribution = achsen.gate_contribution;

    std::string_view const dialekt = cxx_driver_dialect(driver_tag);
    // T2-C: die Version kommt AB HIER vom Tier-Treiber selbst (RT-Sonde, einmal je Tag gecacht), nicht
    // mehr geerbt von der CEB-Uebersetzung. Die alte Deckungs-Wache ct_realversion_deckt_treiber bleibt
    // als Diagnose-Werkzeug stehen (sie beantwortet eine ANDERE Frage: "ist die CEB mit demselben
    // Compiler gebaut wie die Tier-Binaries?"), aber sie entscheidet nicht mehr ueber den Stempel --
    // eine geerbte Version war immer eine Aussage ueber den falschen Compiler.
    std::string const   realversion = active_tier_realversion(); // leer == unbekannt (fail-closed)
    Atomic128Wahl const a128        = active_atomic128_wahl();
    return ::comdare::cache_engine::abi::render_toolchain_stamp_glied(
        toolchain_stamp_parts_from_suffix_parts(p, dialekt, realversion, achsen.opt_flags, a128.id, a128.flags));
}

/// compose_live_toolchain_stamp_glied() -- der RUN-KONSTANTE Wert des Glieds [5] (keine System-Achsen).
/// Er ist ab T2-B kein eigener Weg mehr, sondern der Sonderfall "leere Perm-Achsen" des EINEN Renderers
/// darueber. Damit kann der Einzel-Pfad gar nicht anders rendern als der Perm-Pfad.
[[nodiscard]] inline std::string compose_live_toolchain_stamp_glied() {
    return compose_toolchain_stamp_glied_for_perm(PermToolchainAchsen{});
}

/// live_build_variant_set_signature_glied() -- der LIVE-Wert des Preimage-Glieds [6]. Reine CT->RT-
/// Materialisierung der Treiber-Signatur (erlaubte Richtung); es wird nichts abgeleitet oder geraten.
/// Derselbe Wert, den das `.variant`-Sidecar als Provenienz traegt -- ab Format 3 ist er zusaetzlich
/// IDENTITAET (F7-(b): damit wird COMDARE_VARIANT_GATE funktional obsolet).
[[nodiscard]] inline std::string live_build_variant_set_signature_glied() {
    return std::string{::comdare::cache_engine::builder::experiment::kDriverBuildVariantSignature};
}

// -- NB-3/T2-D: DIE TRANSPORT-NAHT SPRICHT DIESELBE SPRACHE WIE DIE WACHE ------------------------------
//
// DER BEFUND (Codex-Zweitreview [MITTEL] rsp-Zeichenvorrat, hier zu Ende gedacht): die beiden
// Define-Argumente behaupteten in ihrem Kommentar, die Wertformen enthielten "weder Whitespace noch
// Backslash", weil "die Injektivitaets-Wachen in abi/ das garantieren". Fuer den TRAEGER-Weg stimmte das
// (abi::anatomy_glied_zeichen_erlaubt ist eine ALLOWLIST ohne Whitespace) -- nur laeuft der Bau-Kanal gar
// nicht ueber einen Traeger: perm_stamp_glied_defines() reicht den frisch GERENDERTEN String direkt
// hierher. Auf genau diesem Weg war die zugesagte Eigenschaft also unbewiesen; ein Wert mit Whitespace
// waere in der @rsp-Datei in zwei Optionen zerfallen, und die Tier-Uebersetzung haette ein anderes
// (oder gar kein) Stempel-Define bekommen als der Laufzeit-Zwilling.
//
// DIE AUFLOESUNG: die Behauptung wird zur PRUEFUNG, und zwar mit DEMSELBEN Praedikat, das der Traeger
// benutzt (abi::injizierter_glied_wert_ist_wohlgeformt). Damit kann Transport-Erwartung und Wachen-Zusage
// nicht mehr auseinanderlaufen -- es gibt nur noch eine Sprache und einen Ort, an dem sie definiert ist.
// Die Feld-Ebene bleibt zusaetzlich schaerfer gegatet (abi/toolchain_stamp_glied.hpp,
// kToolchainGliedTransportZeichen) -- das ist kein Widerspruch, sondern eine Verengung: sie nennt das
// verletzende FELD beim Namen, bevor ueberhaupt ein Glied entsteht.

/// Die gemeinsame Transport-Wache beider Define-Argumente. FAIL-LOUD mit benannter Fehlerklasse: ein
/// stilles Weglassen des Defines waere das Schlimmste -- die Tier-Binary bekaeme dann einen ANDEREN
/// Fingerprint einkompiliert als den, den die CEB fuer sie erwartet, und wuerde nie wieder gefunden.
inline void require_define_arg_transportfaehig(std::string_view define_name, std::string_view werte) {
    if (::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(werte)) return;
    throw std::invalid_argument(
        std::string{"fehlerklasse=stempel_transport: die Wertform fuer "} + std::string{define_name} +
        " verletzt die Injektivitaets-/Transport-Wache der injizierten Preimage-Glieder. Sie reist als EIN "
        "Argument durch eine gcc-Response-Datei (@rsp); Whitespace, Backslash oder Anfuehrungszeichen "
        "wuerden sie dort in mehrere Optionen zerlegen, und die Tier-Uebersetzung bekaeme einen anderen "
        "Stempel als der CEB-Laufzeit-Zwilling. Den WERT korrigieren, nicht die Wache. Wert: '" +
        std::string{werte} + "'");
}

/// Das Compiler-Argument, das die Wertform als C-STRING-LITERAL in die Tier-Uebersetzung traegt.
/// Escaping-Begruendung wortgleich zur Zellwert-Naht: die Argumente reisen ueber eine gcc-Response-Datei
/// (@rsp, build_orchestrator make_gpp_compile_fn), dort trennt Whitespace die Optionen und ein Backslash
/// schuetzt das naechste Zeichen. Dass beide Wertformen weder Whitespace noch Backslash noch
/// Anfuehrungszeichen tragen, ist ab NB-3/T2-D GEPRUEFT statt angenommen (s. Abschnitt oben) -- deshalb
/// reicht das Escapen der beiden Anfuehrungszeichen, die das Makro selbst braucht.
/// LEERE Wertform => LEERES Argument => kein Define => Identitaet.
[[nodiscard]] inline std::string toolchain_stamp_glied_define_arg(std::string_view werte) {
    if (werte.empty()) return {};
    require_define_arg_transportfaehig("COMDARE_TOOLCHAIN_STAMP_GLIED", werte);
    std::string out = "-DCOMDARE_TOOLCHAIN_STAMP_GLIED=\\\"";
    out += werte;
    out += "\\\"";
    return out;
}

/// Spiegel fuer das bvset-Glied [6].
[[nodiscard]] inline std::string build_variant_set_signature_define_arg(std::string_view werte) {
    if (werte.empty()) return {};
    require_define_arg_transportfaehig("COMDARE_BUILD_VARIANT_SET_SIGNATURE", werte);
    std::string out = "-DCOMDARE_BUILD_VARIANT_SET_SIGNATURE=\\\"";
    out += werte;
    out += "\\\"";
    return out;
}

// -- WACHEN --------------------------------------------------------------------------------------------
// Die bvset-Signatur MUSS die Format-Wache der injizierten Glieder bestehen, bevor sie in ein Define geht:
// sonst braeche der Tier-Bau erst tief im consteval-Fingerprint (oder, schlimmer, das Preimage waere
// mehrdeutig). Der Toolchain-Wert wird vom Renderer selbst fail-loud gegatet (NB/CX-2).
static_assert(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(
                  ::comdare::cache_engine::builder::experiment::kDriverBuildVariantSignature),
              "NB/CX-4: die bvset-Signatur dieses Treibers verletzt die Injektivitaets-Format-Wache des "
              "Preimage-Glieds [6] -- sie darf so nicht als Compile-Define in eine Tier-Uebersetzung.");
// Kein Whitespace und kein Backslash -- sonst zerfiele das Argument in der @rsp-Datei in mehrere Optionen.
static_assert(
    [] {
        for (char const c : ::comdare::cache_engine::builder::experiment::kDriverBuildVariantSignature)
            if (c == ' ' || c == '\t' || c == '\\' || c == '"') return false;
        return true;
    }(),
    "NB/CX-4: die bvset-Signatur enthaelt Whitespace, Backslash oder Anfuehrungszeichen -- die @rsp-Naht "
    "wuerde sie in mehrere Optionen zerlegen.");

} // namespace comdare::cache_engine::profile_facade
