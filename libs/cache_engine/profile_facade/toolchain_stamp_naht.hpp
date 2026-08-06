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

#include <cache_engine/abi/toolchain_stamp_glied.hpp>        // Renderer + die CT-Compiler-Realversions-Erhebung
#include <cache_engine/measurement/compiler_system_axis.hpp> // Dialekt-Ids + driver_default (Single-Source)

#include <cstdlib>
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

/// compose_live_toolchain_stamp_glied() -- der LIVE-Wert des Preimage-Glieds [5]. ARGUMENTLOS und REIN
/// (s. Kopf): Bau-Kanal und Laufzeit-Zwilling rufen dieselbe Funktion und bekommen denselben String.
///
/// Die Felder kommen aus der EINEN Suffix-Quelle (SystemVersionSuffixParts + der Konverter
/// toolchain_stamp_parts_from_suffix_parts), damit Glied und build_version-Suffix nicht getrennt driften
/// koennen -- die Ordnungs-Deckung beider ist zusaetzlich per static_assert bewiesen.
///
/// NB2-1: das cxx-Feld traegt ab hier IMMER den Tier-Treiber-Tag und NUR BEI BEWIESENER DECKUNG die
/// CEB-Realversion (Regel R1/R2 in abi/toolchain_stamp_glied.hpp, Deckungs-Wache
/// ct_realversion_deckt_treiber oben). Damit ergeben verschiedene Treiber-Tags strukturell verschiedene
/// Glieder -- der Fail-open-Fall "g++-17 und g++-18 kollabieren auf cxx=gcc" kann nicht mehr entstehen.
[[nodiscard]] inline std::string compose_live_toolchain_stamp_glied() {
    std::string const driver_tag = active_cxx_driver_tag();
    std::string const ceb        = ceb_contract_version_text();
    std::string const bt         = ::comdare::cache_engine::thesis_lazy::build_type_version_value();

    SystemVersionSuffixParts p{};
    p.cxx        = driver_tag; // NB2-1 (R1): der Tier-Treiber-Tag ist der Identitaets-Diskriminator
    p.ceb        = ceb; // Perm-Pfad-Wert, G-C2 "heilt Fall C" -- der Contract-Bump wirkt ab hier im Preimage
    p.build_type = bt;  // (i): "Debug" nur im Debug-Bau, sonst leer => kein Segment
    // opt/simd/target/tel/gate bleiben LEER -- per-Perm bzw. profil-abhaengig, s. Kopf (Scheibe C-3).

    std::string_view const dialekt = cxx_driver_dialect(driver_tag);
    std::string_view const realver =
        ct_realversion_deckt_treiber(driver_tag) ? ::comdare::cache_engine::abi::kDetectedCompilerRealVersion
                                                 : std::string_view{};
    return ::comdare::cache_engine::abi::render_toolchain_stamp_glied(
        toolchain_stamp_parts_from_suffix_parts(p, dialekt, realver));
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
