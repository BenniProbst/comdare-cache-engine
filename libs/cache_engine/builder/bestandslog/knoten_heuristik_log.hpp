#pragma once
// knoten_heuristik_log.hpp -- LB-1 (F9-Paketschnitt): das KNOTEN-LOG-System der Abnahmen 1-6
// (Owner-Abnahme 26.07.). Je Datei-System-Knoten des Lager-Baums eine "complete-heuristik.log".
//
// WAS SIE TRAEGT (ABNAHME-1/2):
//   * die ANZAHL Binaries bzw. Messwerte je AST (die binaere Grundlage: ist der Ast vollstaendig?),
//   * die Blaetter des Knotens, verankert auf dem v6-SHA512-Key -- DIESELBE EINE Wahrheit wie das
//     Bestandslog (bestandslog_index.hpp::derive_key_from_lines == abi::anatomy_fingerprint_hex).
//     Hier wird NICHTS nachgerechnet: der 128-hex kommt fertig herein und wird nur auf FORM geprueft.
//   * ein INLINE-UPDATE beim Lesen: wer den Knoten liest, traegt seine Beobachtung ein und der Ast
//     wird dadurch genauer, ohne dass ein zweiter Durchlauf noetig waere.
// Depth-first-Bau+Messung macht damit die FEHLENDEN Blaetter schnell ermittelbar: ein Ast, dessen
// Zaehlung die Erwartung trifft, wird gar nicht erst betreten.
//
// SPIN-LOCK-KOMMUNIKATION (Alleinschreiber je Knoten): WIEDERVERWENDET wird die bestehende
// bestandslog_lock-Mechanik (kLockTtlSeconds=1800 als Stale-Obergrenze, kSectionBudgetSeconds=30 als
// Frist-Wache, Zweit-Verify, RAII-Freigabe). Sie arbeitet auf der abstrakten BestandTransport-Naht --
// make_baum_transport() bindet sie an die BaumAblage des Writers. Es gibt deshalb KEINEN zweiten
// Lock-Mechanismus im Haus; wer die ttl aendert, aendert sie weiterhin genau an EINER Stelle.
// MULTITHREADING entsteht nicht durch feineres Locking, sondern durch BRANCHING auf der System-Achse
// via Meta-Meta-Achsen (ABNAHME-2): zwei Laeufe unterschiedlicher System-Zellen beruehren nie denselben
// Knoten, also kollidiert ihr Alleinschreiber-Lock nie.
//
// TRUNCATE-ZUSTANDSMASCHINE (A-1, vom Owner als BUG-QUELLE benannt -- deshalb eine EIGENE Test-Klasse):
//   Zustaende sind TYPEN (State-Pattern GoF), kein Flag-Gestruepp und kein std::variant:
//     KnotenLogOffen                  -- kann beobachten und schreiben, aber NICHT truncaten.
//     KnotenLogSchluss<BuildEnde>     -- Regelfall: Build-Ende, "Lager inventarisieren".
//     KnotenLogSchluss<GroesseUeberschritten> -- ad hoc, NUR wenn das Log > 4 KB ist.
//   Ein Truncate ausserhalb des Schluss-Zustands ist damit nicht "verboten", sondern NICHT AUSDRUECKBAR
//   (es gibt die Methode dort nicht) -- der Bruch ist compile-hart. Der Uebergang in den Schluss-Zustand
//   verlangt einen AlleinschreiberNachweis, den NUR der gelockte Zyklus mit_knoten_lock() ausstellt:
//   beide Wege (Build-Ende UND >4KB) sind also zwingend gelockt. Ein Nachweis fuer einen FREMDEN Knoten
//   wird zur Laufzeit klassifiziert abgewiesen (kein_alleinschreiber_lock).
//
// CRASH-SEMANTIK (Section 74 / A-2): eine halbfertige Zaehlung beschaedigt das Lager NICHT dauerhaft.
// Der Truncate laeuft ZWEISTUFIG: zuerst wird die CRASH-MARKE inventur_laeuft=1 geschrieben, erst
// danach weicht die Blatt-Liste der Zaehlung und abgeschlossen=1 loescht die Marke wieder. Ein Log,
// das die Marke beim Laden noch traegt, beweist einen Abriss MITTEN in der Inventur; der Ast wird dann
// NEU AUFGENOMMEN (inventarisiere_ast_neu). Eine Record-MUTATION -- die Zahlen "reparieren" -- ist
// ausdruecklich verboten, sie waere geraten. Die Marke ist bewusst NICHT dasselbe wie abgeschlossen=0:
// letzteres ist der voellig normale Zustand eines Astes, an dem gerade gebaut wird.
//
// SHA512-OVERLAY -- EHRLICH ABGEGRENZT, kein Bau-Punkt dieser Scheibe (L14-Muster): das Overlay-Glied
// des Fingerprints ist seit A13-M3/C3 STRUKTURELL vorhanden und traegt bis zur Owner-Definition nur
// seinen Separator (abi::kOverlaySourceHash == "", OF-M3-2 = Fallback B; die drei Festlegungen
// -- Verzeichnis-Schnitt, Sortier-Ordnung, Hash je Datei vs. Konkatenation -- existieren NIRGENDS).
// Diese Scheibe konsumiert den Fingerprint AS-IS. Das Voll-Soll der ABNAHME-3/4 erfuellt sich erst im
// Overlay-Fenster; das ist eine DEKLARIERTE, keine stille Luecke.
//
// CT/RT-GRENZE: Zustandsmaschine, Format-Konstanten (4-KB-Schwelle, Log-Name, Format-Version) und die
// Fehlerklassen sind CT. Zaehlerstaende, Locks, Zeit und I/O sind RT.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + bestandslog_lock/lager_baum_writer.

#include "bestandslog_document.hpp" // Genus (die zwei Bestands-Gattungen)
#include "bestandslog_lock.hpp"     // BestandTransport, with_document_lock, LockOwner, NowFn, ttl/Budget
#include "lager_baum_writer.hpp"    // BaumAblage + objekt_key/pfad_praefix_passt (LB-0-Grammatik)

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

/// Der Dateiname des Knoten-Logs. EINE Ableitung -- Schreiber und Sucher reden nie ueber zwei Namen.
inline constexpr std::string_view kKnotenLogName = "complete-heuristik.log";

/// Ad-hoc-Truncate-Schwelle (Owner-Abnahme: "ad hoc bei Log > 4 KB"). CT-Konstante, kein Tuning-Knopf.
inline constexpr std::size_t kKnotenLogTruncateSchwelleBytes = 4096;

/// Format-Version des Knoten-Logs. Ein HOEHERES Log wird abgelehnt (dieselbe Doktrin wie
/// document_syntax_supported: eine fremde Grammatik still zu lesen faelscht die Zaehlung).
inline constexpr int kKnotenLogFormatVersion = 1;

/// Kopf-Zeile des Formats (deterministisch, byte-stabil).
inline constexpr std::string_view kKnotenLogKopf = "complete-heuristik";

// ---------------------------------------------------------------------------
// Fehlerklassen (Pflicht). Jeder Ausgang hat einen Namen.
// ---------------------------------------------------------------------------
enum class KnotenLogFehler : int {
    ok = 0,
    knoten_leer,               // ein Log ohne Knoten-Pfad gehoert nirgendwohin
    blatt_key_form,            // Blatt-Key ist kein 128-stelliges Klein-Hex
    format_kaputt,             // Zeilen-Grammatik verletzt
    format_version_fremd,      // Log einer HOEHEREN Format-Version -> nie still lesen
    kein_alleinschreiber_lock, // Nachweis fehlt/gehoert einem anderen Knoten
    schwelle_nicht_erreicht,   // ad-hoc-Truncate ohne die 4-KB-Ueberschreitung
    ablage_lesen,              // Datei nicht lesbar
    ablage_schreiben,          // Datei nicht schreibbar
    unvollstaendig,            // abgeschlossen=0 -> Ast NEU inventarisieren (A-2), nie mutieren
};

[[nodiscard]] constexpr std::string_view to_string(KnotenLogFehler f) noexcept {
    switch (f) {
        case KnotenLogFehler::ok: return "ok";
        case KnotenLogFehler::knoten_leer: return "knoten_leer";
        case KnotenLogFehler::blatt_key_form: return "blatt_key_form";
        case KnotenLogFehler::format_kaputt: return "format_kaputt";
        case KnotenLogFehler::format_version_fremd: return "format_version_fremd";
        case KnotenLogFehler::kein_alleinschreiber_lock: return "kein_alleinschreiber_lock";
        case KnotenLogFehler::schwelle_nicht_erreicht: return "schwelle_nicht_erreicht";
        case KnotenLogFehler::ablage_lesen: return "ablage_lesen";
        case KnotenLogFehler::ablage_schreiben: return "ablage_schreiben";
        case KnotenLogFehler::unvollstaendig: return "unvollstaendig";
    }
    return "ok";
}

// ---------------------------------------------------------------------------
// Die PODs.
// ---------------------------------------------------------------------------

/// Ein Blatt des Knotens: der v6-SHA512-Key (die EINE Lager-Identitaet) + sein Genus + der Zeitpunkt.
/// Es gibt hier bewusst KEINEN Pfad: der Pfad ergibt sich aus dem Knoten und dem Key -- ein zweites
/// Feld waere eine zweite, driftfaehige Wahrheit.
struct KnotenBlatt {
    std::string key_sha512;
    Genus       genus = Genus::binary;
    std::string done_utc;

    friend bool operator==(KnotenBlatt const&, KnotenBlatt const&) = default;
};

/// Strikte Ordnung ueber (key, genus) -- der byte-stabile Emit haengt daran.
[[nodiscard]] inline bool knoten_blatt_less(KnotenBlatt const& a, KnotenBlatt const& b) noexcept {
    if (a.key_sha512 != b.key_sha512) return a.key_sha512 < b.key_sha512;
    return static_cast<int>(a.genus) < static_cast<int>(b.genus);
}

struct KnotenLog {
    int         format_version = kKnotenLogFormatVersion;
    std::string knoten;                // Pfad des Knotens im Realm-Baum
    std::string stand_utc;             // Zeitpunkt der letzten Fortschreibung
    bool        abgeschlossen = false; // A-2: erst der VOLLZOGENE Truncate setzt das
    // A-2 / Section 74: die CRASH-MARKE. Sie wird VOR dem Truncate gesetzt und geschrieben und erst
    // mit dem abgeschlossenen Truncate wieder geloescht. Ein Log, das sie beim Laden noch traegt,
    // beweist einen Abriss MITTEN in der Inventur -- der Ast wird dann NEU aufgenommen, nie repariert.
    // Der Normalfall waehrend des Baus (abgeschlossen=0, Blaetter vorhanden, inventur_laeuft=0) ist
    // davon ausdruecklich NICHT betroffen: er ist kein Abriss, sondern Arbeit im Gang.
    bool          inventur_laeuft = false;
    std::uint64_t erwartet        = 0; // erwartete Blaetter des Astes (0 == unbekannt)
    std::uint64_t binaries        = 0; // gezaehlte Binary-Blaetter des Astes
    std::uint64_t messwerte       = 0; // gezaehlte Messwert-Blaetter des Astes

    // Die Blaetter dieses Knotens. Der TRUNCATE entfernt genau diese Liste (sie ist der wachsende
    // Teil) und laesst die ZAEHLUNG stehen -- das ist die "binaere Grundlage" der ABNAHME-1.
    std::vector<KnotenBlatt> blaetter;

    friend bool operator==(KnotenLog const&, KnotenLog const&) = default;

    /// Vollstaendig == abgeschlossen UND die Zaehlung trifft die Erwartung. Bei erwartet==0 ist die
    /// Frage nicht beantwortbar -> false (konservativ: lieber einmal zu viel absteigen).
    [[nodiscard]] bool ast_vollstaendig() const noexcept {
        return abgeschlossen && erwartet > 0 && (binaries + messwerte) >= erwartet;
    }

    /// Wie viele Blaetter dieser Ast noch schuldet (0 == keine bzw. unbekannt).
    [[nodiscard]] std::uint64_t rest_offen() const noexcept {
        std::uint64_t const ist = binaries + messwerte;
        return (erwartet > ist) ? (erwartet - ist) : 0;
    }
};

/// Der Pfad des Knoten-Logs unter einem Knoten (LB-0-Grammatik: derselbe Trenner wie ueberall).
[[nodiscard]] inline std::string knoten_log_pfad(std::string_view knoten) { return objekt_key(knoten, kKnotenLogName); }

// ---------------------------------------------------------------------------
// Serialisierung -- zeilenbasiert, deterministisch, byte-stabil:
//   complete-heuristik v1
//   knoten=<pfad>
//   stand_utc=<iso>
//   abgeschlossen=0|1
//   inventur_laeuft=0|1
//   erwartet=<n>
//   binaries=<n>
//   messwerte=<n>
//   blatt=<128hex>;genus=binary|measurement;done_utc=<iso>      (sortiert nach (key, genus))
// Roundtrip-Gate: emit(parse(emit(x))) == emit(x).
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string emit_knoten_log(KnotenLog const& l) {
    std::vector<KnotenBlatt> sortiert = l.blaetter;
    std::sort(sortiert.begin(), sortiert.end(), knoten_blatt_less);

    std::string out;
    out += kKnotenLogKopf;
    out += " v";
    out += std::to_string(l.format_version);
    out += "\nknoten=";
    out += l.knoten;
    out += "\nstand_utc=";
    out += l.stand_utc;
    out += "\nabgeschlossen=";
    out += (l.abgeschlossen ? "1" : "0");
    out += "\ninventur_laeuft=";
    out += (l.inventur_laeuft ? "1" : "0");
    out += "\nerwartet=";
    out += std::to_string(l.erwartet);
    out += "\nbinaries=";
    out += std::to_string(l.binaries);
    out += "\nmesswerte=";
    out += std::to_string(l.messwerte);
    out += "\n";
    for (auto const& b : sortiert) {
        out += "blatt=";
        out += b.key_sha512;
        out += ";genus=";
        out += to_string(b.genus);
        out += ";done_utc=";
        out += b.done_utc;
        out += "\n";
    }
    return out;
}

namespace detail {

[[nodiscard]] inline std::uint64_t knoten_log_u64(std::string_view s) noexcept {
    std::uint64_t v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

/// Eine Zeile in (key, wert) zerlegen; nullopt, wenn das '=' fehlt.
[[nodiscard]] inline std::optional<std::pair<std::string_view, std::string_view>>
knoten_log_kv(std::string_view zeile) noexcept {
    auto const eq = zeile.find('=');
    if (eq == std::string_view::npos) return std::nullopt;
    return std::pair<std::string_view, std::string_view>{zeile.substr(0, eq), zeile.substr(eq + 1)};
}

} // namespace detail

/// Parsen. nullopt bei kaputter Grammatik, fremdem Genus oder FALSCHEM Blatt-Key-Format -- nie stille
/// Fehlfaerbung (ein still verschluckter Blatt-Eintrag verfaelschte die Zaehlung des ganzen Astes).
/// Eine HOEHERE Format-Version ist ebenfalls nullopt (der Aufrufer unterscheidet ueber
/// knoten_log_format_supported, welche der beiden Ursachen vorlag).
[[nodiscard]] inline std::optional<KnotenLog> parse_knoten_log(std::string_view text) {
    KnotenLog   l;
    bool        kopf_gesehen = false;
    std::size_t i            = 0;
    while (i <= text.size()) {
        auto const nl    = text.find('\n', i);
        auto const zeile = text.substr(i, (nl == std::string_view::npos ? text.size() : nl) - i);
        i                = (nl == std::string_view::npos) ? text.size() + 1 : nl + 1;
        if (zeile.empty()) continue;
        if (!kopf_gesehen) {
            if (!zeile.starts_with(kKnotenLogKopf)) return std::nullopt;
            auto const rest = zeile.substr(kKnotenLogKopf.size());
            if (rest.size() < 3 || rest[0] != ' ' || rest[1] != 'v') return std::nullopt;
            l.format_version = static_cast<int>(detail::knoten_log_u64(rest.substr(2)));
            if (l.format_version <= 0) return std::nullopt;
            kopf_gesehen = true;
            continue;
        }
        auto const kv = detail::knoten_log_kv(zeile);
        if (!kv) return std::nullopt;
        auto const [key, wert] = *kv;
        if (key == "knoten") {
            l.knoten = std::string{wert};
        } else if (key == "stand_utc") {
            l.stand_utc = std::string{wert};
        } else if (key == "abgeschlossen") {
            if (wert != "0" && wert != "1") return std::nullopt;
            l.abgeschlossen = (wert == "1");
        } else if (key == "inventur_laeuft") {
            if (wert != "0" && wert != "1") return std::nullopt;
            l.inventur_laeuft = (wert == "1");
        } else if (key == "erwartet") {
            l.erwartet = detail::knoten_log_u64(wert);
        } else if (key == "binaries") {
            l.binaries = detail::knoten_log_u64(wert);
        } else if (key == "messwerte") {
            l.messwerte = detail::knoten_log_u64(wert);
        } else if (key == "blatt") {
            // "<128hex>;genus=<g>;done_utc=<iso>"
            KnotenBlatt      b;
            std::string_view rest = wert;
            auto const       s1   = rest.find(';');
            if (s1 == std::string_view::npos) return std::nullopt;
            b.key_sha512 = std::string{rest.substr(0, s1)};
            if (!ist_fingerprint_hex(b.key_sha512)) return std::nullopt;
            rest             = rest.substr(s1 + 1);
            auto const s2    = rest.find(';');
            auto const gteil = rest.substr(0, s2);
            if (!gteil.starts_with("genus=")) return std::nullopt;
            auto const g = genus_from_string(gteil.substr(6));
            if (!g) return std::nullopt;
            b.genus = *g;
            if (s2 != std::string_view::npos) {
                auto const dteil = rest.substr(s2 + 1);
                if (!dteil.starts_with("done_utc=")) return std::nullopt;
                b.done_utc = std::string{dteil.substr(9)};
            }
            l.blaetter.push_back(std::move(b));
        } else {
            return std::nullopt; // unbekannter Schluessel -> Grammatik-Bruch, nicht ignorieren
        }
    }
    if (!kopf_gesehen) return std::nullopt;
    return l;
}

/// Format-Vertraeglichkeit (Doktrin document_syntax_supported): eine HOEHERE Version ist nicht sicher
/// lesbar; nach unten wird treu gelesen.
[[nodiscard]] constexpr bool knoten_log_format_supported(KnotenLog const& l) noexcept {
    return l.format_version >= 1 && l.format_version <= kKnotenLogFormatVersion;
}

// ---------------------------------------------------------------------------
// INLINE-UPDATE (ABNAHME-2): eine Beobachtung eintragen. IDEMPOTENT -- dasselbe (key, genus) zweimal
// zaehlt einmal. Das ist die Bedingung dafuer, dass ein Leser den Knoten beim Lesen fortschreiben
// darf, ohne die Zaehlung zu verfaelschen.
// ---------------------------------------------------------------------------
struct BeobachtungsErgebnis {
    KnotenLogFehler fehler = KnotenLogFehler::ok;
    bool            neu    = false; // false == war schon verzeichnet (idempotent)

    [[nodiscard]] bool ok() const noexcept { return fehler == KnotenLogFehler::ok; }
};

[[nodiscard]] inline BeobachtungsErgebnis beobachte_blatt(KnotenLog& l, KnotenBlatt const& b) {
    BeobachtungsErgebnis e;
    if (!ist_fingerprint_hex(b.key_sha512)) {
        e.fehler = KnotenLogFehler::blatt_key_form;
        return e;
    }
    for (auto const& vorhanden : l.blaetter)
        if (vorhanden.key_sha512 == b.key_sha512 && vorhanden.genus == b.genus) return e; // schon da
    l.blaetter.push_back(b);
    if (b.genus == Genus::binary)
        ++l.binaries;
    else
        ++l.messwerte;
    e.neu = true;
    return e;
}

/// Die FEHLENDEN Blaetter eines Knotens (ABNAHME-2, depth-first): welche der erwarteten Keys sind
/// noch nicht verzeichnet? Deterministische Reihenfolge = die der Erwartungs-Liste.
/// GRENZE, ehrlich: nach einem Truncate ist die Blatt-Liste fort (das ist ihr Zweck) -- dann
/// beantwortet nur noch rest_offen() die Frage "wie viele", und die Antwort "welche" kommt aus einer
/// Ast-NEU-Inventarisierung. Genau dafuer gibt es inventarisiere_ast_neu().
[[nodiscard]] inline std::vector<KnotenBlatt> fehlende_blaetter(KnotenLog const&             l,
                                                                std::span<KnotenBlatt const> erwartet) {
    std::vector<KnotenBlatt> fehlt;
    for (auto const& e : erwartet) {
        bool gefunden = false;
        for (auto const& b : l.blaetter)
            if (b.key_sha512 == e.key_sha512 && b.genus == e.genus) {
                gefunden = true;
                break;
            }
        if (!gefunden) fehlt.push_back(e);
    }
    return fehlt;
}

// ---------------------------------------------------------------------------
// A-2 / Section 74 -- AST-NEU-INVENTARISIERUNG statt Record-Mutation.
//
// Ein Log mit abgeschlossen=0 ist eine ABGERISSENE Zaehlung. Es waere ein Leichtes, die Zahlen
// "nachzubessern" -- und genau das ist verboten: die Differenz zwischen halber Zaehlung und Wahrheit
// ist nicht rekonstruierbar, jede Korrektur waere geraten. Der einzige Heilweg ist, den Ast neu
// aufzunehmen: die REAL vorgefundenen Blaetter ersetzen die Zaehlung vollstaendig.
//
// Diese Funktion ist PUR (der Aufrufer enumeriert den Ast): dieselbe Eingabe ergibt dasselbe Log,
// unabhaengig davon, was vorher im Knoten stand. Genau das macht den Crash-Fall testbar.
// ---------------------------------------------------------------------------
[[nodiscard]] inline KnotenLog inventarisiere_ast_neu(std::string knoten, std::span<KnotenBlatt const> ast_ist,
                                                      std::uint64_t erwartet, std::string stand_utc) {
    KnotenLog neu;
    neu.knoten          = std::move(knoten);
    neu.stand_utc       = std::move(stand_utc);
    neu.erwartet        = erwartet;
    neu.abgeschlossen   = false;
    neu.inventur_laeuft = false; // die Neuaufnahme LOESCHT die Crash-Marke -- sie repariert keine Zahl
    for (auto const& b : ast_ist) (void)beobachte_blatt(neu, b);
    return neu;
}

// ---------------------------------------------------------------------------
// ALLEINSCHREIBER-NACHWEIS -- das Token, das der gelockte Zyklus ausstellt.
//
// Es ist NICHT frei konstruierbar: nur mit_knoten_lock() kann eines herstellen. Damit ist "Truncate
// nur mit Lock" keine Bitte an den Aufrufer, sondern eine Eigenschaft des Typsystems. Kopieren ist
// verboten (ein weitergereichtes Token ueberlebte sonst die Lock-Sektion).
// ---------------------------------------------------------------------------
class AlleinschreiberNachweis {
public:
    AlleinschreiberNachweis(AlleinschreiberNachweis const&)            = delete;
    AlleinschreiberNachweis& operator=(AlleinschreiberNachweis const&) = delete;
    AlleinschreiberNachweis(AlleinschreiberNachweis&&)                 = delete;
    AlleinschreiberNachweis& operator=(AlleinschreiberNachweis&&)      = delete;

    [[nodiscard]] std::string const& knoten() const noexcept { return knoten_; }
    [[nodiscard]] std::string const& owner_uuid() const noexcept { return owner_uuid_; }

    /// Deckt dieses Token den genannten Knoten? Ein Nachweis fuer einen FREMDEN Knoten ist kein
    /// Nachweis -- sonst genuegte irgendein gehaltener Lock, um irgendwo zu truncaten.
    [[nodiscard]] bool deckt(std::string_view knoten) const noexcept { return knoten_ == knoten; }

private:
    AlleinschreiberNachweis(std::string knoten, std::string owner_uuid)
        : knoten_{std::move(knoten)}, owner_uuid_{std::move(owner_uuid)} {}

    friend struct detail_nachweis_fabrik;

    std::string knoten_;
    std::string owner_uuid_;
};

/// Die einzige Fabrik. Sie ist ein detail-Typ und wird ausschliesslich von mit_knoten_lock benutzt.
struct detail_nachweis_fabrik {
    [[nodiscard]] static AlleinschreiberNachweis stelle_aus(std::string knoten, std::string owner_uuid) {
        return AlleinschreiberNachweis{std::move(knoten), std::move(owner_uuid)};
    }
};

// ---------------------------------------------------------------------------
// Die BaumAblage als BestandTransport -- so laeuft der Knoten-Lock auf DERSELBEN Mechanik wie der
// Bestandslog-Lock (kein zweiter Lock im Haus). remove faellt auf datei_entfernen; fehlt das Verb,
// meldet der Transport ehrlich false und der Lock-Zyklus bricht sichtbar statt still.
// ---------------------------------------------------------------------------
[[nodiscard]] inline BestandTransport make_baum_transport(BaumAblage const& ablage) {
    BestandTransport t;
    t.fetch = [&ablage](std::string const& key) -> std::optional<std::string> {
        if (!ablage.datei_lesen) return std::nullopt;
        return ablage.datei_lesen(key);
    };
    t.store = [&ablage](std::string const& key, std::string const& inhalt) -> bool {
        if (!ablage.datei_schreiben) return false;
        return ablage.datei_schreiben(key, inhalt);
    };
    t.remove = [&ablage](std::string const& key) -> bool {
        if (!ablage.datei_entfernen) return false;
        return ablage.datei_entfernen(key);
    };
    t.stat = [&ablage](std::string const& key) -> std::optional<ObjectStat> {
        if (!ablage.datei_lesen) return std::nullopt;
        auto const inhalt = ablage.datei_lesen(key);
        if (!inhalt) return std::nullopt;
        return ObjectStat{.size = static_cast<std::uint64_t>(inhalt->size()), .mtime_epoch_s = 0};
    };
    return t;
}

// ---------------------------------------------------------------------------
// mit_knoten_lock -- EIN gelockter Alleinschreiber-Zyklus auf dem Knoten-Log.
//
// Reine Wiederverwendung: der Lock-Schluessel ist lock_key_for(knoten_log_pfad(knoten)), also das
// bekannte <doc>.lock-Nebenobjekt; die Frist-Wache, der Zweit-Verify und die RAII-Freigabe kommen
// unveraendert aus bestandslog_lock.hpp. fn bekommt den Nachweis und liefert bool == gelungen.
// ---------------------------------------------------------------------------
template <class Fn>
[[nodiscard]] inline LockOutcome
mit_knoten_lock(BaumAblage const& ablage, std::string const& knoten, LockOwner const& me, NowFn const& now_fn, Fn&& fn,
                int ttl_s = kLockTtlSeconds, int section_budget_s = kSectionBudgetSeconds) {
    static_assert(std::is_invocable_r_v<bool, Fn&, AlleinschreiberNachweis const&>,
                  "fn(nachweis) muss bool liefern: true == Arbeit gelungen");
    BestandTransport const t       = make_baum_transport(ablage);
    std::string const      doc_key = knoten_log_pfad(knoten);
    Fn&                    work    = fn;
    return with_document_lock(
        t, doc_key, me, ttl_s, now_fn,
        [&]() {
            AlleinschreiberNachweis const nachweis = detail_nachweis_fabrik::stelle_aus(knoten, me.owner_uuid);
            return work(nachweis);
        },
        section_budget_s);
}

// ---------------------------------------------------------------------------
// DIE ZUSTANDSMASCHINE (State-Pattern GoF, Typ-Zustaende).
// ---------------------------------------------------------------------------

/// Die zwei GRUENDE, aus denen der Schluss-Zustand betreten werden darf. Sie sind TYPEN, damit die
/// Zustandsmaschine sie unterscheidet, ohne ein Flag mitzufuehren -- und damit ein dritter Grund nicht
/// versehentlich entsteht, sondern als neuer Typ bewusst eingefuehrt werden muss.
struct BuildEnde {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "build_ende"; }
};
struct GroesseUeberschritten {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "groesse_ueber_4kb"; }
};

template <typename G>
concept SchlussGrund = requires {
    { G::name() } -> std::same_as<std::string_view>;
};

static_assert(SchlussGrund<BuildEnde>);
static_assert(SchlussGrund<GroesseUeberschritten>);

/// Ergebnis eines Truncate.
struct TruncateErgebnis {
    KnotenLogFehler fehler        = KnotenLogFehler::ok;
    std::size_t     vorher_bytes  = 0;
    std::size_t     nachher_bytes = 0;
    std::size_t     blaetter_fort = 0; // wie viele Blatt-Zeilen die Zaehlung ersetzt hat
    std::uint64_t   binaries      = 0; // die ERHALTENE Zaehlung (die binaere Grundlage)
    std::uint64_t   messwerte     = 0;

    [[nodiscard]] bool ok() const noexcept { return fehler == KnotenLogFehler::ok; }
};

template <SchlussGrund G>
class KnotenLogSchluss;

// ---------------------------------------------------------------------------
// KnotenLogOffen -- der Arbeits-Zustand. Beobachten und Schreiben ja; truncaten NEIN (die Methode
// existiert hier nicht -- der Bruch ist compile-hart, nicht Laufzeit-Disziplin).
// ---------------------------------------------------------------------------
class KnotenLogOffen {
public:
    explicit KnotenLogOffen(KnotenLog log) : log_{std::move(log)} {}

    [[nodiscard]] KnotenLog const& log() const noexcept { return log_; }

    /// ABNAHME-2: Inline-Update beim Lesen. Braucht KEIN Lock -- es aendert nur den RAM-Stand; erst
    /// das SCHREIBEN laeuft durch den Alleinschreiber-Zyklus.
    [[nodiscard]] BeobachtungsErgebnis beobachte(KnotenBlatt const& b) { return beobachte_blatt(log_, b); }

    /// Die aktuelle Groesse des serialisierten Logs (die Basis der 4-KB-Frage).
    [[nodiscard]] std::size_t bytes() const { return emit_knoten_log(log_).size(); }

    [[nodiscard]] bool schwelle_erreicht() const { return bytes() > kKnotenLogTruncateSchwelleBytes; }

    /// Den (noch nicht abgeschlossenen) Stand schreiben. Erlaubt und normal -- was hier NICHT
    /// passiert, ist ein Truncate.
    [[nodiscard]] KnotenLogFehler schreibe(BaumAblage const& ablage, AlleinschreiberNachweis const& nachweis) {
        if (!nachweis.deckt(log_.knoten)) return KnotenLogFehler::kein_alleinschreiber_lock;
        if (!ablage.datei_schreiben) return KnotenLogFehler::ablage_schreiben;
        if (!ablage.datei_schreiben(knoten_log_pfad(log_.knoten), emit_knoten_log(log_)))
            return KnotenLogFehler::ablage_schreiben;
        return KnotenLogFehler::ok;
    }

    /// UEBERGANG: Build-Ende ("Lager inventarisieren"). Immer zulaessig -- aber nur mit Nachweis.
    [[nodiscard]] std::optional<KnotenLogSchluss<BuildEnde>> schluss_build_ende(AlleinschreiberNachweis const& n) &&;

    /// UEBERGANG: ad hoc. NUR wenn das Log die 4-KB-Schwelle wirklich ueberschritten hat -- sonst
    /// nullopt (der Aufrufer meldet schwelle_nicht_erreicht). Die Bedingung wird HIER geprueft, nicht
    /// beim Truncate: ein Zustand, den man gar nicht erst betreten darf, ist sicherer als eine
    /// Methode, die man nicht rufen darf.
    [[nodiscard]] std::optional<KnotenLogSchluss<GroesseUeberschritten>>
    schluss_ad_hoc(AlleinschreiberNachweis const& n) &&;

private:
    KnotenLog log_;
};

// ---------------------------------------------------------------------------
// KnotenLogSchluss<Grund> -- der EINZIGE Zustand, in dem truncaten ausdrueckbar ist.
// ---------------------------------------------------------------------------
template <SchlussGrund G>
class KnotenLogSchluss {
public:
    [[nodiscard]] static constexpr std::string_view grund() noexcept { return G::name(); }

    [[nodiscard]] KnotenLog const& log() const noexcept { return log_; }

    /// Truncate + Inventur. Die BLATT-LISTE weicht der ZAEHLUNG (die binaere Grundlage bleibt); als
    /// LETZTER Schritt wird abgeschlossen=1 geschrieben -- bricht der Vorgang vorher ab, findet der
    /// naechste Lauf abgeschlossen=0 und inventarisiert den Ast NEU (A-2), statt Zahlen zu erfinden.
    [[nodiscard]] TruncateErgebnis truncate_und_inventarisiere(BaumAblage const&              ablage,
                                                               AlleinschreiberNachweis const& nachweis,
                                                               std::string                    stand_utc) {
        TruncateErgebnis e;
        if (!nachweis.deckt(log_.knoten)) {
            e.fehler = KnotenLogFehler::kein_alleinschreiber_lock;
            return e;
        }
        if (log_.knoten.empty()) {
            e.fehler = KnotenLogFehler::knoten_leer;
            return e;
        }
        if (!ablage.datei_schreiben) {
            e.fehler = KnotenLogFehler::ablage_schreiben;
            return e;
        }
        e.vorher_bytes  = emit_knoten_log(log_).size();
        e.blaetter_fort = log_.blaetter.size();
        // SCHRITT 1 -- die CRASH-MARKE setzen und SCHREIBEN, bevor irgendetwas verloren geht. Bricht
        // der Lauf jetzt ab, findet der naechste inventur_laeuft=1 und nimmt den Ast neu auf (A-2).
        log_.inventur_laeuft = true;
        if (!ablage.datei_schreiben(knoten_log_pfad(log_.knoten), emit_knoten_log(log_))) {
            log_.inventur_laeuft = false;
            e.fehler             = KnotenLogFehler::ablage_schreiben;
            return e;
        }
        // SCHRITT 2 -- die Blatt-Liste weicht der Zaehlung; abgeschlossen=1 und das Loeschen der Marke
        // sind die LETZTEN Schritte.
        log_.blaetter.clear();
        log_.stand_utc         = std::move(stand_utc);
        log_.abgeschlossen     = true;
        log_.inventur_laeuft   = false;
        std::string const text = emit_knoten_log(log_);
        if (!ablage.datei_schreiben(knoten_log_pfad(log_.knoten), text)) {
            e.fehler = KnotenLogFehler::ablage_schreiben;
            return e;
        }
        e.nachher_bytes = text.size();
        e.binaries      = log_.binaries;
        e.messwerte     = log_.messwerte;
        return e;
    }

private:
    friend class KnotenLogOffen;
    explicit KnotenLogSchluss(KnotenLog log) : log_{std::move(log)} {}

    KnotenLog log_;
};

inline std::optional<KnotenLogSchluss<BuildEnde>>
KnotenLogOffen::schluss_build_ende(AlleinschreiberNachweis const& n) && {
    if (!n.deckt(log_.knoten)) return std::nullopt;
    return KnotenLogSchluss<BuildEnde>{std::move(log_)};
}

inline std::optional<KnotenLogSchluss<GroesseUeberschritten>>
KnotenLogOffen::schluss_ad_hoc(AlleinschreiberNachweis const& n) && {
    if (!n.deckt(log_.knoten)) return std::nullopt;
    if (!schwelle_erreicht()) return std::nullopt; // 4 KB sind eine Bedingung, keine Empfehlung
    return KnotenLogSchluss<GroesseUeberschritten>{std::move(log_)};
}

// ---------------------------------------------------------------------------
// Laden eines Knoten-Logs (der Lese-Weg, auf dem das Inline-Update aufsetzt).
//
// Fehlt die Datei, entsteht ein FRISCHES Log fuer den Knoten (Erst-Lauf, kein Fehler). Ist sie da,
// aber unlesbar/fremd/abgerissen, meldet diese Funktion die Klasse -- und der Aufrufer nimmt den
// Ast neu auf. Sie REPARIERT nichts.
// ---------------------------------------------------------------------------
struct KnotenLogLadung {
    KnotenLog       log;
    KnotenLogFehler fehler = KnotenLogFehler::ok;
    bool            frisch = false; // true == es gab noch kein Log (Erst-Lauf)

    [[nodiscard]] bool ok() const noexcept { return fehler == KnotenLogFehler::ok; }
};

[[nodiscard]] inline KnotenLogLadung lade_knoten_log(BaumAblage const& ablage, std::string const& knoten) {
    KnotenLogLadung erg;
    erg.log.knoten = knoten;
    if (knoten.empty()) {
        erg.fehler = KnotenLogFehler::knoten_leer;
        return erg;
    }
    if (!ablage.datei_lesen) {
        erg.fehler = KnotenLogFehler::ablage_lesen;
        return erg;
    }
    auto const roh = ablage.datei_lesen(knoten_log_pfad(knoten));
    if (!roh) {
        erg.frisch = true;
        return erg;
    }
    auto geparst = parse_knoten_log(*roh);
    if (!geparst) {
        erg.fehler = KnotenLogFehler::format_kaputt;
        return erg;
    }
    if (!knoten_log_format_supported(*geparst)) {
        erg.fehler = KnotenLogFehler::format_version_fremd;
        return erg;
    }
    erg.log = std::move(*geparst);
    if (erg.log.knoten.empty()) erg.log.knoten = knoten;
    // A-2: ein ABGERISSENER Stand wird GEMELDET, nicht geheilt. Erkennungsmerkmal ist die
    // Crash-Marke, NICHT das blosse abgeschlossen=0 -- letzteres ist der voellig normale Zustand
    // eines Astes, an dem gerade gebaut wird. Die Zahlen bleiben unangetastet im Ergebnis stehen,
    // damit der Aufrufer sieht, WAS er verwirft.
    if (erg.log.inventur_laeuft) erg.fehler = KnotenLogFehler::unvollstaendig;
    return erg;
}

} // namespace comdare::cache_engine::builder::bestandslog
