#pragma once
// batch_planner.hpp -- G3 / #46b Lagerhaltung, Scheibe B5 (Ledger §62-B-NACHTRAG-2, B13/B15/B22).
//
// Der ASYNC Batch-Planer: EIN Producer-Thread konsolidiert ZU BEGINN ueber ALLE Binaries, erkennt
// jede fehlende Binary EINZELN (per-Binary-Miss gegen den Lager-Index), teilt die Fenster per
// PARITAET dieser Maschine zu (Gleichverteilung, B15/B16) und schiebt die betroffenen Slices in die
// Queue. Der Compile-Pool konsumiert ab der ersten Slice, waehrend der Planer weiterlaeuft (B22).
//
// ENTKOPPLUNG von B3: die Praesenz-Pruefung laeuft ueber ein PresencePredicate (index -> bool). Die
// Convenience-Bindung make_presence_predicate baut es aus einer Key-Menge (die vorhandenen
// key_sha512-Hex-Strings aus dem Bestandslog) + einer index->key-Abbildung -> keine Abhaengigkeit
// vom konkreten B3-Sha512Key-Typ.
//
// KORN: kGnBatchSlice=4096 spiegelt experiment_plan_director.hpp:439 (die Slice-Grenzen uebernimmt
// das Bestandslog 1:1 vom Planner-Takt). B13: Batch-Typ-Sequenz-Wache (planer_block->ceb->tier).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + slice_queue.hpp. Der
// Planer-Thread ist I/O-/Koordinations-Ebene (kein Hot-Path) -> std::function-Praedikat zulaessig.
//
// T2-A/F4 (2026-08-06, Owner-KERN Zaehler-Resume): dazu kommt die PLAN-ABLAGE -- <filesystem>/<fstream>
// stehen ab hier in der Include-Liste. Das bleibt innerhalb der Doktrin (reine stdlib) und innerhalb der
// Ebene (der Planer ist die I/O-/Koordinations-Ebene); ein Hot-Path wird nicht beruehrt.

#include "bestandslog_document.hpp" // BatchTyp
#include "slice_queue.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem> // T2-A/F4: die Plan-/Zaehler-Ablage (Koordinations-Ebene, kein Hot-Path)
#include <fstream>    // T2-A/F4: dito -- Plan schreiben, Zaehler lesen/schreiben
#include <functional>
#include <iterator> // T2-A/F4: istreambuf_iterator (Datei am Stueck lesen)
#include <optional> // T2-A/F4: fail-closed Rueckgaben der Plan-/Zaehler-Leser
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Korn-Konstante (spiegelt experiment_plan_director.hpp:439 kGnBatchSlice=4096).
inline constexpr std::uint64_t kGnBatchSlice = 4096;

// Praedikat: ist die Binary mit globalem Index i schon im Lager?
using PresencePredicate = std::function<bool(std::uint64_t index)>;

// Baut ein Praesenz-Praedikat aus einer Menge vorhandener key_sha512 + einer index->key-Abbildung
// (Membership gegen den Lager-Index; entkoppelt von B3s Sha512Key ueber die Hex-Strings).
[[nodiscard]] inline PresencePredicate make_presence_predicate(std::unordered_set<std::string>           present_keys,
                                                               std::function<std::string(std::uint64_t)> key_of) {
    return [present = std::move(present_keys), key_of = std::move(key_of)](std::uint64_t i) {
        return present.count(key_of(i)) != 0;
    };
}

// Paritaets-Zuteilung (Gleichverteilung): gehoert Fenster window_index dieser Maschine (rank von
// n_machines)? n_machines==0 -> alle (Ein-Maschinen-Fall).
[[nodiscard]] inline bool window_belongs_to(std::uint64_t window_index, unsigned rank, unsigned n_machines) noexcept {
    if (n_machines == 0) return true;
    return (window_index % n_machines) == rank;
}

// Einzeln-Miss-Erkennung in einem Fenster [begin, begin+count): die fehlenden globalen Indizes.
[[nodiscard]] inline std::vector<std::uint64_t> detect_missing_in_window(std::uint64_t begin, std::uint64_t count,
                                                                         PresencePredicate const& is_present) {
    std::vector<std::uint64_t> missing;
    for (std::uint64_t i = begin; i < begin + count; ++i)
        if (!is_present(i)) missing.push_back(i);
    return missing;
}

// B13 Batch-Typ-Sequenz-Wache: gueltige Bau-Ordnung planer_block(0) -> ceb(1) -> tier(2). Die
// Phasen-Raenge einer Sequenz muessen NICHT-fallend sein (kein Rueckfall in eine fruehere Phase).
[[nodiscard]] inline int type_phase_rank(BatchTyp t) noexcept {
    switch (t) {
        case BatchTyp::planer_block: return 0;
        case BatchTyp::ceb: return 1;
        case BatchTyp::tier: return 2;
    }
    return 0;
}
[[nodiscard]] inline bool is_valid_type_sequence(std::span<BatchTyp const> seq) noexcept {
    int prev = -1;
    for (BatchTyp t : seq) {
        int const r = type_phase_rank(t);
        if (r < prev) return false;
        prev = r;
    }
    return true;
}

// ===========================================================================
// T2-A/F4 -- DER PLAN IST EIN DOKUMENT, KEIN STROM (Owner-KERN Zaehler-Resume, Ledger abend-10:
// "Batch-Plan (Reihenfolge+Faecher) PERSISTENT VOR dem Lauf, Resume = Zaehler je Phase
// [kompiliert/separat gemessen] gegen den Plan").
//
// WARUM. Bis hierher ENTSTAND der Plan waehrend er verbraucht wurde: der Producer erkannte ein Fenster
// und schob es sofort weiter (Codex-Befund K1: "batch_planner.hpp:87 streamt sofort"). Damit gab es zu
// keinem Zeitpunkt ein Objekt, gegen das ein zweiter Lauf sich haette ausrichten koennen -- die einzige
// Ordnungs-Zahl war ein laufender Zaehler, der VOR Bau und Messung hochlief und Fehlversuche mitzaehlte.
// Ein Abbruch hinterliess deshalb keine beantwortbare Frage "wo stand ich": weder die Reihenfolge noch
// die Faecher waren nach dem Prozess-Ende noch irgendwo hinterlegt.
//
// WAS SICH AENDERT. Der Plan wird VOLLSTAENDIG konsolidiert, BEVOR das erste Fach die Queue erreicht,
// und (wo eine Ablage benannt ist) VOR dem Lauf geschrieben. Der Strom bleibt ein Strom -- der Consumer
// zieht weiter Fach fuer Fach und beginnt, bevor der Producer fertig ist (B22 bleibt gueltig; es ist die
// PUSH-Phase, die ueberlappt). Was faellt, ist allein die Vorstellung, der Plan sei erst am Ende bekannt.
//
// DIE FORM ist die des Resume-Stempels und nicht eine zweite: ein Kopf aus "|feld=wert"-Gliedern, dann
// DER Schwanz-Schluessel mit der Zahl der folgenden Zeilen. Dieser Schluessel wird HEREINGEREICHT
// (kLazyResumeRowsKey, cache_engine_builder_iterator.hpp) -- exakt wie MessFormatFakten::rows_key ihn
// dem Status-Leser hereinreicht. Ein zweites Literal "|rows=" entstuende sonst genau dort, wo die
// W5-Hebung es gerade beseitigt hat, und die Abhaengigkeits-Richtung (Iterator -> Bestandslog) verbietet
// den Zugriff von hier aus.
//
// FAIL-CLOSED: jede Abweichung -- fremder Stempel, andere Zeilenzahl, unparsbare Zahl, Zaehler groesser
// als der Plan -- ergibt "kein Resume-Anspruch" (nullopt) und damit einen ehrlichen Voll-Lauf. Dieselbe
// Doktrin wie lazy_try_resume_binary: es gibt keine stillen Teil-Uebernahmen.
// ===========================================================================

// Die Format-Marke der Plan-Ablage. v1 = Erst-Einfuehrung (T2-A/F4). Ein Bump invalidiert Alt-Plaene
// ueber den Praefix-Vergleich -- wie beim Resume-Stempel, aus demselben Grund.
inline constexpr char kBatchPlanFormat[] = "batchplan-v1";

// EIN FACH des Plans: das Fenster [begin, begin+count) im Index-Raum des Planers + die Zahl der darin
// beim Planen als FEHLEND erkannten Atome. `offen` ist bewusst nur eine Zahl: das Dokument haelt die
// ORDNUNG und die FAECHER fest (Owner-Wortlaut), nicht die Miss-Liste -- die ist Lauf-Zustand und wird
// beim naechsten Lauf ohnehin frisch erhoben (das Lager kann sich zwischen zwei Laeufen bewegt haben).
struct PlanFach {
    std::uint64_t begin = 0;
    std::uint64_t count = 0; ///< Zahl der Plan-Atome (Binaries) dieses Fachs -- die Zaehl-Einheit
    std::uint64_t offen = 0; ///< davon beim Planen fehlend (Momentaufnahme, kein Anspruch)

    friend bool operator==(PlanFach const&, PlanFach const&) = default;
};

// DIE ZAEHLER JE PHASE. Beide zaehlen ATOME (Binaries), nicht Faecher -- ein Fach ist die Resume-KOERNUNG,
// das Atom ist die Zaehl-EINHEIT; zwei Groessen, die man nicht vermischen darf.
//   kompiliert : Atome, deren Bau in einem frueheren Lauf VOLLSTAENDIG und FEHLERFREI abgeschlossen ist.
//   gemessen   : Atome, deren Messung abgeschlossen ist. Sie wird SEPARAT hochgezaehlt, weil Bau und
//                Messung in dieser Maschinerie getrennte Laeufe sind (Owner: "kompiliert/separat
//                gemessen"); ein Bau-Lauf beruehrt `gemessen` nie und umgekehrt.
// BEIDE werden erst NACH dem Vollzug fortgeschrieben und NIE fuer einen Fehlversuch -- das ist der Kern
// des Befunds K1 ("IDs vor Bau/Messung gezaehlt inkl. Fehlversuche"). Ein Zaehler, der vor der Arbeit
// hochlaeuft, behauptet Arbeit, die es nicht gibt.
struct PhasenZaehler {
    std::uint64_t kompiliert = 0;
    std::uint64_t gemessen   = 0;

    friend bool operator==(PhasenZaehler const&, PhasenZaehler const&) = default;
};

// Summe der Plan-Atome (die Bezugsgroesse beider Zaehler).
[[nodiscard]] inline std::uint64_t plan_atome(std::vector<PlanFach> const& faecher) noexcept {
    std::uint64_t n = 0;
    for (auto const& f : faecher) n += f.count;
    return n;
}

// DER RESUME-PUNKT: wie viele FUEHRENDE Faecher deckt der Zaehler vollstaendig ab? Ein nur TEILWEISE
// abgedecktes Fach wird bewusst NICHT uebersprungen -- der Plan ist ein Praefix-Resume, und ein halbes
// Fach zur Haelfte zu ueberspringen hiesse raten, welche Haelfte. Der Nachbau eines schon gebauten Atoms
// ist billig (dll_is_current/das .fingerprint-Sidecar fangen ihn als Skip ab); eine uebersprungene, aber
// nie gebaute Binary waere ein Loch in der Matrix.
[[nodiscard]] inline std::size_t plan_resume_faecher(std::vector<PlanFach> const& faecher,
                                                     std::uint64_t                zaehler) noexcept {
    std::uint64_t kumuliert = 0;
    std::size_t   n         = 0;
    for (auto const& f : faecher) {
        if (kumuliert + f.count > zaehler) break;
        kumuliert += f.count;
        ++n;
    }
    return n;
}

// ---------------------------------------------------------------------------
// Die Grammatik: rendern und lesen. Der rows_key kommt von aussen (s. Kopf-Abschnitt).
// ---------------------------------------------------------------------------

[[nodiscard]] inline std::string render_batch_plan(std::string const& stamp, std::vector<PlanFach> const& faecher,
                                                   std::string const& rows_key) {
    std::string aus = stamp + rows_key + std::to_string(faecher.size()) + "\n";
    for (auto const& f : faecher) {
        aus += std::to_string(f.begin);
        aus += ';';
        aus += std::to_string(f.count);
        aus += ';';
        aus += std::to_string(f.offen);
        aus += '\n';
    }
    return aus;
}

// Der Zaehler-Stand steht in DERSELBEN Ordnung: Kopf-Glieder, dann der Schwanz-Schluessel mit der Zahl
// der Faecher, GEGEN die gezaehlt wurde. Genau diese Zahl bindet die Zaehler an den Plan -- ein Plan
// anderer Groesse macht einen Alt-Zaehler wertlos, und das faellt beim Lesen auf statt im Betrieb.
[[nodiscard]] inline std::string render_phasen_zaehler(std::string const& stamp, PhasenZaehler const& z,
                                                       std::size_t fach_zahl, std::string const& rows_key) {
    return stamp + "|kompiliert=" + std::to_string(z.kompiliert) + "|gemessen=" + std::to_string(z.gemessen) +
           rows_key + std::to_string(fach_zahl) + "\n";
}

namespace detail {
// std::stoull ohne Wurf-Pfad: eine Zahl, die nicht VOLLSTAENDIG aus Ziffern besteht, ist keine.
[[nodiscard]] inline std::optional<std::uint64_t> nur_ziffern_zu_u64(std::string_view s) noexcept {
    if (s.empty() || s.size() > 20) return std::nullopt;
    std::uint64_t w = 0;
    for (char const c : s) {
        if (c < '0' || c > '9') return std::nullopt;
        std::uint64_t const ziffer = static_cast<std::uint64_t>(c - '0');
        if (w > (~std::uint64_t{0} - ziffer) / 10) return std::nullopt; // Ueberlauf ist keine Zahl
        w = w * 10 + ziffer;
    }
    return w;
}
} // namespace detail

// Liest die Faecher aus dem Dokument-Text. Erwarteter Stempel + rows_key muessen exakt passen.
[[nodiscard]] inline std::optional<std::vector<PlanFach>>
parse_batch_plan(std::string const& text, std::string const& stamp, std::string const& rows_key) {
    std::size_t const nl = text.find('\n');
    if (nl == std::string::npos) return std::nullopt;
    std::string kopf = text.substr(0, nl);
    while (!kopf.empty() && kopf.back() == '\r') kopf.pop_back();
    if (kopf.size() <= stamp.size() + rows_key.size()) return std::nullopt;
    if (kopf.compare(0, stamp.size(), stamp) != 0) return std::nullopt;                  // fremder Plan
    if (kopf.compare(stamp.size(), rows_key.size(), rows_key) != 0) return std::nullopt; // fremdes Format
    auto const soll = detail::nur_ziffern_zu_u64(std::string_view{kopf}.substr(stamp.size() + rows_key.size()));
    if (!soll.has_value()) return std::nullopt;

    std::vector<PlanFach> faecher;
    std::size_t           pos = nl + 1;
    while (pos < text.size()) {
        std::size_t const ende  = text.find('\n', pos);
        std::string       zeile = text.substr(pos, ende == std::string::npos ? std::string::npos : ende - pos);
        pos                     = (ende == std::string::npos) ? text.size() : ende + 1;
        while (!zeile.empty() && zeile.back() == '\r') zeile.pop_back();
        if (zeile.empty()) continue;
        std::size_t const t1 = zeile.find(';');
        if (t1 == std::string::npos) return std::nullopt;
        std::size_t const t2 = zeile.find(';', t1 + 1);
        if (t2 == std::string::npos) return std::nullopt;
        auto const b = detail::nur_ziffern_zu_u64(std::string_view{zeile}.substr(0, t1));
        auto const c = detail::nur_ziffern_zu_u64(std::string_view{zeile}.substr(t1 + 1, t2 - t1 - 1));
        auto const o = detail::nur_ziffern_zu_u64(std::string_view{zeile}.substr(t2 + 1));
        if (!b.has_value() || !c.has_value() || !o.has_value()) return std::nullopt;
        if (*o > *c) return std::nullopt; // mehr offen als gross: das Dokument widerspricht sich
        faecher.push_back(PlanFach{*b, *c, *o});
    }
    if (faecher.size() != static_cast<std::size_t>(*soll)) return std::nullopt; // abgeschnitten/angehaengt
    return faecher;
}

// Liest den Zaehler-Stand. `faecher` ist der GERADE geplante Stand -- der Zaehler gilt nur gegen ihn.
[[nodiscard]] inline std::optional<PhasenZaehler> parse_phasen_zaehler(std::string const&           text,
                                                                       std::string const&           stamp,
                                                                       std::vector<PlanFach> const& faecher,
                                                                       std::string const&           rows_key) {
    std::size_t const nl    = text.find('\n');
    std::string       zeile = text.substr(0, nl == std::string::npos ? text.size() : nl);
    while (!zeile.empty() && zeile.back() == '\r') zeile.pop_back();
    if (zeile.compare(0, stamp.size(), stamp) != 0) return std::nullopt;
    std::string const kompiliert_schluessel = "|kompiliert=";
    std::string const gemessen_schluessel   = "|gemessen=";
    std::size_t const p_k                   = zeile.find(kompiliert_schluessel, stamp.size());
    if (p_k != stamp.size()) return std::nullopt; // Feld-Ordnung ist Teil des Formats
    std::size_t const p_g = zeile.find(gemessen_schluessel, p_k + kompiliert_schluessel.size());
    if (p_g == std::string::npos) return std::nullopt;
    std::size_t const p_r = zeile.find(rows_key, p_g + gemessen_schluessel.size());
    if (p_r == std::string::npos) return std::nullopt;

    auto const k = detail::nur_ziffern_zu_u64(
        std::string_view{zeile}.substr(p_k + kompiliert_schluessel.size(), p_g - (p_k + kompiliert_schluessel.size())));
    auto const g = detail::nur_ziffern_zu_u64(
        std::string_view{zeile}.substr(p_g + gemessen_schluessel.size(), p_r - (p_g + gemessen_schluessel.size())));
    auto const fach_zahl = detail::nur_ziffern_zu_u64(std::string_view{zeile}.substr(p_r + rows_key.size()));
    if (!k.has_value() || !g.has_value() || !fach_zahl.has_value()) return std::nullopt;
    if (*fach_zahl != faecher.size()) return std::nullopt; // gegen einen ANDEREN Plan gezaehlt
    std::uint64_t const atome = plan_atome(faecher);
    if (*k > atome || *g > atome) return std::nullopt; // mehr getan als geplant: unglaubwuerdig
    if (*g > *k) return std::nullopt;                  // gemessen ohne gebaut gibt es nicht
    return PhasenZaehler{*k, *g};
}

// ---------------------------------------------------------------------------
// Die Ablage. ATOMAR (schreiben nach .tmp, dann rename): ein abgebrochener Schreibvorgang darf keinen
// halben Plan hinterlassen, den der Folgelauf fuer einen ganzen haelt. rename ist auf einem Dateisystem
// die einzige Operation, die diese Zusage gibt.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool schreibe_atomar(std::filesystem::path const& ziel, std::string const& inhalt) {
    std::error_code ec;
    if (ziel.has_parent_path()) std::filesystem::create_directories(ziel.parent_path(), ec);
    std::filesystem::path const tmp = std::filesystem::path{ziel.string() + ".tmp"};
    {
        std::ofstream os{tmp, std::ios::trunc | std::ios::binary};
        if (!os) return false;
        os << inhalt;
        if (!os) return false;
    }
    std::filesystem::rename(tmp, ziel, ec);
    if (ec) {
        std::error_code weg;
        std::filesystem::remove(tmp, weg);
        return false;
    }
    return true;
}

[[nodiscard]] inline std::optional<std::string> lies_datei(std::filesystem::path const& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) return std::nullopt;
    std::ifstream is{p, std::ios::binary};
    if (!is) return std::nullopt;
    return std::string{std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] inline bool write_batch_plan(std::filesystem::path const& p, std::string const& stamp,
                                           std::vector<PlanFach> const& faecher, std::string const& rows_key) {
    return schreibe_atomar(p, render_batch_plan(stamp, faecher, rows_key));
}

[[nodiscard]] inline std::optional<std::vector<PlanFach>>
read_batch_plan(std::filesystem::path const& p, std::string const& stamp, std::string const& rows_key) {
    auto const text = lies_datei(p);
    if (!text.has_value()) return std::nullopt;
    return parse_batch_plan(*text, stamp, rows_key);
}

[[nodiscard]] inline bool write_phasen_zaehler(std::filesystem::path const& p, std::string const& stamp,
                                               PhasenZaehler const& z, std::size_t fach_zahl,
                                               std::string const& rows_key) {
    return schreibe_atomar(p, render_phasen_zaehler(stamp, z, fach_zahl, rows_key));
}

[[nodiscard]] inline std::optional<PhasenZaehler> read_phasen_zaehler(std::filesystem::path const& p,
                                                                      std::string const&           stamp,
                                                                      std::vector<PlanFach> const& faecher,
                                                                      std::string const&           rows_key) {
    auto const text = lies_datei(p);
    if (!text.has_value()) return std::nullopt;
    return parse_phasen_zaehler(*text, stamp, faecher, rows_key);
}

// ---------------------------------------------------------------------------
// Die PURE Konsolidierung des B5-Plans: alle eigenen Fenster, in Reihenfolge, mit ihren Misses. Sie ist
// aus BatchPlanner::run herausgeloest, damit "planen" und "streamen" zwei benennbare Schritte sind --
// und damit der Plan ohne Thread und ohne Queue testbar ist.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::vector<BatchSlice> plan_batch_slices(std::uint64_t total, BatchTyp typ, unsigned rank,
                                                               unsigned n_machines, PresencePredicate const& is_present,
                                                               std::uint64_t grain = kGnBatchSlice) {
    std::vector<BatchSlice> plan;
    if (grain == 0) grain = kGnBatchSlice;
    std::uint64_t const num_windows = (total + grain - 1) / grain;
    for (std::uint64_t w = 0; w < num_windows; ++w) {
        if (!window_belongs_to(w, rank, n_machines)) continue;
        std::uint64_t const begin   = w * grain;
        std::uint64_t const count   = std::min(grain, total - begin);
        auto                missing = detect_missing_in_window(begin, count, is_present);
        if (!missing.empty()) plan.push_back(BatchSlice{begin, count, typ, std::move(missing)});
    }
    return plan;
}

// Die Plan-SICHT eines B5-Slice-Plans (Dokument-Form): Fenster + Miss-Zahl, ohne die Miss-Liste.
[[nodiscard]] inline std::vector<PlanFach> plan_faecher_sicht(std::vector<BatchSlice> const& plan) {
    std::vector<PlanFach> aus;
    aus.reserve(plan.size());
    for (auto const& s : plan) aus.push_back(PlanFach{s.begin, s.count, static_cast<std::uint64_t>(s.missing.size())});
    return aus;
}

// ---------------------------------------------------------------------------
// BatchPlanner -- EIN Producer-Thread. Er KONSOLIDIERT ZUERST den vollen Plan (plan_batch_slices) und
// schiebt ihn danach Fach fuer Fach in die Queue; am Ende schliesst er sie. Der Consumer konsumiert ab
// dem ersten Fach, waehrend der Producer noch schiebt (B22 Start-vor-Fertig -- das gilt fuer die
// PUSH-Phase; die Planungs-Phase ist ein reiner Praedikat-Durchlauf ohne Bau und ohne Messung). Der
// dtor joined.
//
// T2-A/F4: das "erst planen, dann streamen" ist die Verhaltens-Aenderung dieses Pakets. Sie kostet einen
// vollen Praedikat-Durchlauf vor dem ersten Push und liefert dafuer einen Plan, der als GANZES existiert
// -- die Voraussetzung dafuer, dass ein Zaehler ueberhaupt "gegen den Plan" stehen kann.
// ---------------------------------------------------------------------------
class BatchPlanner {
public:
    BatchPlanner(SliceQueue& q, std::uint64_t total_count, BatchTyp typ, unsigned rank, unsigned n_machines,
                 PresencePredicate is_present, std::uint64_t slice_grain = kGnBatchSlice)
        : q_{q}, total_{total_count}, typ_{typ}, rank_{rank}, n_machines_{n_machines},
          is_present_{std::move(is_present)}, grain_{slice_grain == 0 ? kGnBatchSlice : slice_grain} {
        worker_ = std::thread([this] { run(); });
    }

    BatchPlanner(BatchPlanner const&)            = delete;
    BatchPlanner& operator=(BatchPlanner const&) = delete;

    ~BatchPlanner() { join(); }

    void join() {
        if (worker_.joinable()) worker_.join();
    }

private:
    void run() {
        // T2-A/F4: ERST DER PLAN. Die Fenster-Ordnung und die Faecher stehen vollstaendig fest, bevor
        // das erste Fach die Queue erreicht -- der Strom bildet den Plan ab, statt ihn zu ERSETZEN.
        auto plan = plan_batch_slices(total_, typ_, rank_, n_machines_, is_present_, grain_);
        for (auto& s : plan) q_.push(std::move(s));
        q_.close();
    }

    SliceQueue&       q_;
    std::uint64_t     total_;
    BatchTyp          typ_;
    unsigned          rank_;
    unsigned          n_machines_;
    PresencePredicate is_present_;
    std::uint64_t     grain_;
    std::thread       worker_;
};

} // namespace comdare::cache_engine::builder::bestandslog
