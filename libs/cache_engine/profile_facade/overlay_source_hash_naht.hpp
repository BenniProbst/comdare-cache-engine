#pragma once
// profile_facade/overlay_source_hash_naht.hpp -- E-E (07.08.2026): die EINE Stelle, an der der
// Overlay-Quell-Hash (Preimage-Glied [7]) in die Wertform eines Compile-Defines gebracht und an die
// Tier-Uebersetzung gehaengt wird. Spiegel und Praezedenz: toolchain_stamp_naht.hpp (NB/CX-4) und
// system_cell_values_naht.hpp (W10-C4).
//
// -- DIE HARTE BEDINGUNG DIESER NAHT: KEINE DRIFT ------------------------------------------------------
// Der Fingerprint hat ZWEI Rechen-Stellen, und sie muessen ueber DIESELBEN Glieder rechnen:
//   (a) der consteval-Zwilling IN der Tier-Binary (anatomy_module_abi_v1.hpp, COMDARE_ANATOMY_VERSION_
//       STAMP_M) -- er sieht das Glied ueber das Compile-Define, das DIESE Naht anhaengt;
//   (b) der Laufzeit-Zwilling in der CEB (lazy_adhoc_source_gen.hpp, lazy_adhoc_fingerprint_for) -- er
//       liest abi::kOverlaySourceHash, also die in die CEB einkompilierte Konstante.
// Waere nur eine Seite verdrahtet, rechnete der consteval-Zwilling ueber ANDERE Glieder als die CEB, und
// die Binary wuerde nie wieder gefunden (dll_is_current vergleicht genau diese Zahl).
//
// WIE DIE GLEICHHEIT MECHANISCH WIRD, statt versprochen zu sein: die Funktion unten ist ARGUMENTLOS und
// REIN -- sie zieht ihren Wert aus derselben Konstante abi::kOverlaySourceHash, die auch der
// Laufzeit-Zwilling liest. Es gibt keinen Parameter, ueber den ein Aufrufer eine zweite Wahrheit
// hereinreichen koennte. Dieselbe Bauart wie compose_live_toolchain_stamp_glied().
//
// -- WARUM DIE CEB IHREN EIGENEN WERT WEITERREICHT UND NICHT DIE TIER-SEITE IHN SELBST BILDET ----------
// Weil die CEB die Instanz ist, die die Binary spaeter WIEDERERKENNEN muss. Sie haengt genau den Wert an,
// gegen den sie vergleicht. Der generierte Header (cmake/overlay_source_hash.cmake) traegt sein
// #define unter #ifndef -- ein per -D gereichter Wert GEWINNT also. Das ist die richtige Rangfolge: wo
// CEB und lokaler Quellbaum auseinanderliefen, ist die Erwartung der CEB die massgebliche Identitaet,
// nicht der zufaellige Stand des Include-Pfades der Tier-Uebersetzung.
//
// -- WARUM DER WERT RUN-KONSTANT IST (und deshalb in perm_stamp_glied_defines gehoert) -----------------
// Anders als Glied [5] (Toolchain, seit T2-B PER PERMUTATION) haengt der Overlay-Hash an gar nichts
// Permutations-Spezifischem: er ist der Quelltext-Stand DIESER CEB, also fuer den ganzen Lauf eine
// Konstante. Er reist deshalb im run-konstanten Kanal, genau wie das bvset-Glied [6]. Zwei konkurrierende
// -D-Argumente (run-konstant + per-Perm) koennen so gar nicht erst entstehen -- die Falle, die bei
// Glied [5] eigens eine Parameter-Weiche gekostet hat.
//
// ASCII-only (Umlaute als ae/oe/ue/ss). Header-only, keine Bau-Abhaengigkeit.

#include <cache_engine/abi/anatomy_fingerprint.hpp> // kOverlaySourceHash + die Format-Wache

#include <stdexcept>
#include <string>
#include <string_view>

namespace comdare::cache_engine::profile_facade {

/// live_overlay_source_hash_glied() -- der Wert des Preimage-Glieds [7] DIESES Laufs.
///
/// ARGUMENTLOS UND REIN (s. Kopf): derselbe Aufruf liefert im Bau-Kanal und im Laufzeit-Zwilling
/// byte-identische Strings, weil beide dieselbe einkompilierte Konstante lesen. LEER heisst hier
/// ehrlich "kein Codegen gelaufen" -- und dann traegt auch die Tier-Uebersetzung kein Define, rechnet
/// also byte-identisch zum Vor-E-E-Stand. Es gibt keinen Zustand, in dem eine Seite belegt und die
/// andere leer waere.
[[nodiscard]] inline std::string_view live_overlay_source_hash_glied() noexcept {
    return ::comdare::cache_engine::abi::kOverlaySourceHash;
}

/// Die Transport-Wache. Wortgleiche Begruendung wie bei den Nachbar-Nahten: der Wert reist als EIN
/// Argument durch eine gcc-Response-Datei (@rsp, build_orchestrator make_gpp_compile_fn); dort trennt
/// Whitespace die Optionen und ein Backslash schuetzt das naechste Zeichen. Ein zerfallendes Argument
/// hiesse: die Tier-Uebersetzung bekaeme einen ANDEREN Stempel als der CEB-Laufzeit-Zwilling.
///
/// FAIL-LOUD statt stiller Auslassung -- ein weggelassenes Define waere hier das Schlimmste: die Binary
/// bekaeme einen anderen Fingerprint einkompiliert als den, den die CEB fuer sie erwartet, und wuerde nie
/// wieder gefunden. Der Wert ist ein 128-hex und besteht die Wache damit immer; die Pruefung steht
/// trotzdem da, weil sie den Tag abdeckt, an dem jemand die Wertform aendert.
inline void require_overlay_hash_transportfaehig(std::string_view werte) {
    if (::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(werte)) return;
    throw std::invalid_argument(
        std::string{"fehlerklasse=stempel_transport: die Wertform fuer COMDARE_OVERLAY_SOURCE_HASH verletzt die "
                    "Injektivitaets-/Transport-Wache der injizierten Preimage-Glieder. Den WERT korrigieren, "
                    "nicht die Wache. Wert: '"} +
        std::string{werte} + "'");
}

/// Das Compiler-Argument, das die Wertform als C-STRING-LITERAL in die Tier-Uebersetzung traegt.
/// Escaping-Begruendung wortgleich zu den Nachbar-Nahten: die beiden Anfuehrungszeichen, die das Makro
/// selbst braucht, werden geschuetzt; Whitespace/Backslash koennen im Wert nicht vorkommen (Wache oben).
/// LEERE Wertform => LEERES Argument => kein Define => Identitaet.
[[nodiscard]] inline std::string overlay_source_hash_define_arg(std::string_view werte) {
    if (werte.empty()) return {};
    require_overlay_hash_transportfaehig(werte);
    std::string out = "-DCOMDARE_OVERLAY_SOURCE_HASH=\\\"";
    out += werte;
    out += "\\\"";
    return out;
}

// -- WACHEN --------------------------------------------------------------------------------------------
// Der einkompilierte Wert MUSS die Format-Wache der injizierten Glieder bestehen, BEVOR er in ein Define
// geht -- sonst braeche der Tier-Bau erst tief im consteval-Fingerprint (oder, schlimmer, das Preimage
// waere mehrdeutig). Die abi-Seite prueft dasselbe Praedikat bereits; hier steht es an der TRANSPORT-Naht,
// also dort, wo der Wert das Haus verlaesst.
static_assert(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(
                  ::comdare::cache_engine::abi::kOverlaySourceHash),
              "E-E: der Overlay-Quell-Hash verletzt die Injektivitaets-Format-Wache des Preimage-Glieds [7] -- "
              "er darf so nicht als Compile-Define in eine Tier-Uebersetzung.");
// Kein Whitespace und kein Backslash -- sonst zerfiele das Argument in der @rsp-Datei in mehrere Optionen.
static_assert(
    [] {
        for (char const c : ::comdare::cache_engine::abi::kOverlaySourceHash)
            if (c == ' ' || c == '\t' || c == '\\' || c == '"') return false;
        return true;
    }(),
    "E-E: der Overlay-Quell-Hash traegt Whitespace, Backslash oder ein Anfuehrungszeichen -- er kann so nicht "
    "als EIN Argument durch die gcc-Response-Datei reisen.");
// Ein belegtes Glied ist GENAU ein SHA-512-Hex. Die Laenge steht hier und nicht nur im Budget-Beleg des
// abi-Headers, weil hier die Stelle ist, an der ein falsch gebauter Codegen-Wert zuerst auffallen wuerde.
static_assert(::comdare::cache_engine::abi::kOverlaySourceHash.empty() ||
                  ::comdare::cache_engine::abi::kOverlaySourceHash.size() == 128,
              "E-E: das Overlay-Glied ist entweder LEER (kein Codegen) oder GENAU ein 128-stelliger "
              "SHA-512-Hex. Jede andere Laenge ist ein Codegen-Fehler, kein zulaessiger Wert.");

} // namespace comdare::cache_engine::profile_facade
