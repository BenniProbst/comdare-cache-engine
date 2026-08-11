#pragma once
// bvset_teilmenge.hpp -- Task #59 (GLIED [6] ist richtungsblind): der RICHTUNGS-BEHAFTETE Vergleich zweier
// bvset-Mengen-Signaturen. Gegenstueck zu build_variant_set_signature.hpp, das die Signatur ERZEUGT.
//
// DER BEFUND, DER DIESEN HEADER NOETIG MACHT. Das Preimage-Glied [6] traegt die Enabled-MENGE der Build-Achsen
// dieses Treibers. Verglichen wurde sie bisher ausschliesslich als Teil des SHA-512 -- also auf GLEICHHEIT
// (build_orchestrator.hpp, "DER EINE VERGLEICH"). Gleichheit ist fuer eine MENGE aber die falsche Relation:
//   * Hardware-ERWEITERUNG (eine Achsen-Variante kommt hinzu): die alte Binary bleibt gueltig -- sie wurde mit
//     einer TEILMENGE der heutigen Faehigkeiten gebaut, und alles, was sie kann, kann der heutige Treiber auch.
//     Ein Neubau der GESAMTEN Flotte waere reine Verschwendung.
//   * Funktions-EINSCHRAENKUNG (eine Variante faellt weg): die alte Binary ist NICHT mehr gueltig -- sie
//     traegt Faehigkeiten, die der heutige Treiber nicht mehr fuehrt.
// Beide Faelle sehen unter Gleichheit identisch aus ("Signatur anders -> Neubau"). Genau diese Blindheit fuer
// die RICHTUNG ist der Defekt: der Vertrag ist additiv, die Implementierung war symmetrisch.
//
// WARUM DER KLARTEXT UND NICHT DER HASH: ein Hash kennt keine Teilmenge. Ueber zwei SHA-512 laesst sich nur
// gleich/ungleich entscheiden, nie "die eine Menge steckt in der anderen". Deshalb reist die Signatur seit
// dieser Scheibe zusaetzlich als KLARTEXT in Zeile 2 des `.fingerprint`-Sidecars (fingerprint_sidecar.hpp) --
// und deshalb steht der Vergleich hier und nicht im Digest.
//
// REGISTRY-FREI, NUR string_view -- gleiche Layering-Begruendung wie in driver_build_variant_signature.hpp:12-14:
// wer nur vergleichen will, soll die CMake-generierten Flags-Header nicht miterben. Dieser Header kennt die
// GRAMMATIK der Signatur, nicht ihre Herkunft. Die Grammatik ist am Emitter abgelesen
// (build_variant_set_signature.hpp::ct_put_variant_set_signature, Zeichenvorrat ;=[]{}):
//
//   bvset=<set-version>;bv=<pod-version>;page_type[<elem>...];simd_extension[<elem>...];general_hardware[<elem>...]
//   <elem> := {<name>;<feld>=<zahl>;...}
//
// FAIL-CLOSED IN JEDEM ZWEIFEL: was dieser Header nicht vollstaendig parsen kann, ist KEINE Teilmenge (false).
// Der Preis eines falschen false ist ein ueberfluessiger Neubau; der Preis eines falschen true ist eine
// Binary, die Faehigkeiten vortaeuscht, die sie nicht hat. Die Asymmetrie ist bewusst.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, nur stdlib.

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Die drei Achsen-Sektionen in der Reihenfolge, in der der Emitter sie schreibt. Die Reihenfolge ist Teil der
/// Grammatik (der Emitter sortiert nicht) -- eine andere Reihenfolge ist eine andere Signatur, kein Sonderfall.
inline constexpr std::array<std::string_view, 3> kBvsetAchsenNamen{"page_type", "simd_extension", "general_hardware"};

/// Eine geparste Achsen-Sektion: der Name und die geklammerten Elemente in Emissions-Reihenfolge.
/// Die Elemente tragen ihre aeusseren geschweiften Klammern MIT -- so ist ein Element auch ausserhalb seiner
/// Sektion noch eindeutig als Element lesbar, und der Vergleich braucht keine zweite Normalisierung.
struct BvsetAchsenSektion {
    std::string_view              name{};
    std::vector<std::string_view> elemente{};
};

/// Die vollstaendig geparste Signatur. `kopf` ist alles VOR der ersten Achsen-Sektion, also
/// "bvset=<set-version>;bv=<pod-version>" -- beide Versions-Zahlen zusammen. Sie sind bewusst NICHT Teil der
/// Teilmengen-Rechnung, sondern eine Vorbedingung: ein Format- oder POD-Versions-Bump aendert die BEDEUTUNG der
/// Elemente, damit ist ein Element-fuer-Element-Vergleich ueber die Bump-Grenze hinweg sinnlos.
struct BvsetGeparst {
    std::string_view                    kopf{};
    std::array<BvsetAchsenSektion, 3>   achsen{};
};

namespace detail {

/// Liest EIN geklammertes Element ab Position `i` (dort muss '{' stehen) und gibt seine Sicht INKLUSIVE der
/// aeusseren Klammern zurueck; `i` steht danach hinter der schliessenden Klammer. nullopt = unparsebar.
///
/// TIEFEN-ZAEHLUNG statt Suche nach dem naechsten '}': die Namens-Wache des Emitters
/// (COMDARE_BVSET_NAME_WACHE) verbietet Strukturzeichen in Namen, verschachtelte Klammern koennen also heute
/// nicht entstehen. Der Zaehler kostet nichts und macht den Parser gegen eine kuenftige Grammatik-Erweiterung
/// robust, statt sich auf eine Wache in einer ANDEREN Datei zu verlassen.
[[nodiscard]] inline std::optional<std::string_view> bvset_lies_element(std::string_view s, std::size_t& i) {
    if (i >= s.size() || s[i] != '{') return std::nullopt;
    std::size_t const start = i;
    std::size_t       tiefe = 0;
    for (; i < s.size(); ++i) {
        if (s[i] == '{') ++tiefe;
        else if (s[i] == '}') {
            --tiefe;
            if (tiefe == 0) {
                ++i; // hinter die schliessende Klammer
                return s.substr(start, i - start);
            }
        }
    }
    return std::nullopt; // unbalanciert -> fail-closed
}

/// Sucht die Achsen-Marke ";<name>[" -- die VOLLE Marke, nicht bloss den Namen: ein Achsen-Name kaeme
/// sonst auch als Teilstring eines Element-Namens in Frage und wuerde den Kopf falsch abschneiden.
///
/// OHNE std::string gebaut (kein <string> in diesem Header, s. Kopfkommentar "nur string_view"). Die
/// naheliegende Formulierung waere ein zusammengesetzter Suchstring gewesen -- sie haette diesen Header
/// an <string> gebunden, fuer nichts als eine Konkatenation von drei bekannten Teilen.
[[nodiscard]] inline std::size_t bvset_finde_achsen_marke(std::string_view s, std::string_view name) noexcept {
    if (s.size() < name.size() + 2) return std::string_view::npos;
    for (std::size_t i = 0; i + name.size() + 2 <= s.size(); ++i) {
        if (s[i] != ';') continue;
        if (s.substr(i + 1, name.size()) != name) continue;
        if (s[i + 1 + name.size()] != '[') continue;
        return i;
    }
    return std::string_view::npos;
}

} // namespace detail

/// Parst eine bvset-Signatur vollstaendig. nullopt = die Eingabe ist keine wohlgeformte Signatur.
///
/// VOLLSTAENDIG heisst hier woertlich: nach der dritten Achsen-Klammer darf KEIN Byte mehr folgen. Ein
/// toleranter Parser, der einen Rest verschluckt, wuerde genau die Faelschung durchlassen, gegen die dieser
/// Vergleich gebaut ist -- angehaengter Text ist ein Unterschied, der niemand still verschwinden darf.
[[nodiscard]] inline std::optional<BvsetGeparst> bvset_parse(std::string_view s) {
    BvsetGeparst g{};
    if (!s.starts_with("bvset=")) return std::nullopt;

    // Der Kopf endet, wo die erste Achsen-Sektion beginnt.
    std::size_t const kopf_ende = detail::bvset_finde_achsen_marke(s, kBvsetAchsenNamen[0]);
    if (kopf_ende == std::string_view::npos) return std::nullopt;
    g.kopf = s.substr(0, kopf_ende);

    std::size_t i = kopf_ende;
    for (std::size_t a = 0; a < kBvsetAchsenNamen.size(); ++a) {
        if (i >= s.size() || s[i] != ';') return std::nullopt;
        ++i;
        auto const name = kBvsetAchsenNamen[a];
        if (s.substr(i).size() < name.size() || s.substr(i, name.size()) != name) return std::nullopt;
        i += name.size();
        if (i >= s.size() || s[i] != '[') return std::nullopt;
        ++i;
        g.achsen[a].name = name;
        while (i < s.size() && s[i] != ']') {
            auto const e = detail::bvset_lies_element(s, i);
            if (!e) return std::nullopt;
            g.achsen[a].elemente.push_back(*e);
        }
        if (i >= s.size() || s[i] != ']') return std::nullopt;
        ++i;
    }
    if (i != s.size()) return std::nullopt; // Rest hinter der letzten Achse -> fail-closed
    return g;
}

/// Ist `recorded` eine TEILMENGE von `current`? Das ist die Frage, die ueber den Skip entscheidet:
/// "wurde die vorhandene Binary mit hoechstens den Faehigkeiten gebaut, die der heutige Treiber fuehrt?"
///
/// DIE DREI BEDINGUNGEN, ALLE NOETIG:
///   (1) beide Seiten sind vollstaendig parsebar          -- sonst fail-closed
///   (2) der Kopf ist BYTE-GLEICH                         -- gleiches Signatur-Format UND gleiche POD-Version;
///                                                           ueber einen Bump hinweg bedeuten gleiche Bytes
///                                                           nicht mehr dasselbe, ein Element-Vergleich waere
///                                                           dann eine Behauptung ohne Grundlage
///   (3) JE ACHSE: jedes Element von recorded steht BYTE-GENAU auch in current
///
/// AUSDRUECKLICH NICHT SYMMETRISCH -- das ist der ganze Zweck. bvset_ist_teilmenge(A, A+X) ist true
/// (Erweiterung: die alte Binary bleibt gueltig), bvset_ist_teilmenge(A+X, A) ist false (Einschraenkung:
/// die alte Binary traegt etwas, das es nicht mehr gibt). Wer hier versehentlich Gleichheit einsetzt, macht
/// beide Faelle wieder ununterscheidbar.
///
/// BYTE-GENAU je Element und nicht nur ueber den Namen: zwei Varianten mit gleichem Namen, aber anderen Feldern
/// (etwa eine geaenderte cache_line) sind VERSCHIEDENE Faehigkeiten. Ein Namens-Vergleich wuerde die
/// Feld-Aenderung still schlucken und die Binary faelschlich als gueltig durchwinken.
[[nodiscard]] inline bool bvset_ist_teilmenge(std::string_view recorded, std::string_view current) {
    auto const r = bvset_parse(recorded);
    if (!r) return false;
    auto const c = bvset_parse(current);
    if (!c) return false;
    if (r->kopf != c->kopf) return false;
    for (std::size_t a = 0; a < kBvsetAchsenNamen.size(); ++a) {
        if (r->achsen[a].name != c->achsen[a].name) return false;
        for (auto const& e : r->achsen[a].elemente) {
            bool gefunden = false;
            for (auto const& k : c->achsen[a].elemente) {
                if (e == k) {
                    gefunden = true;
                    break;
                }
            }
            if (!gefunden) return false;
        }
    }
    return true;
}

} // namespace comdare::cache_engine::builder::experiment
