#pragma once
// planer_driven_build.hpp -- G3 / #46b Lagerhaltung, Scheibe I1b (Planer-getriebener provision-Bau).
//
// Producer-Consumer-Treiber des provision-Baus (opt-in, gegated wie I1). Ein ASYNC Producer slict die
// SELEKTIERTEN view-Indizes in Fenster der Groesse grain (Index-Reihenfolge ERHALTEN) und erkennt je
// Fenster JEDES fehlende Binary EINZELN (PresenceFn + detect_missing_in_window, LEDGER:3397). Der
// Consumer (Iterator) baut je Fenster NUR die Fehlenden (filter_window_for_build) und akkumuliert den
// builds-Vektor; dll_is_current bleibt als ZWEITE Verteidigungslinie der lokale Skip-Arbiter. RAII:
// der Producer-Thread joined im dtor (auch im Fehlerfall).
//
// Lager-TP1 (B) / G-A2: bis zu diesem Paket war die PresenceFn nur INFORMATIV (Reservierungs-Info,
// volle Fenster wurden gebaut, "Determinismus A"). Das SOLL (LEDGER:3397: "jedes fehlende Binary
// EINZELN erkannt, NICHT als Gesamt-Batch") macht sie zum BAU-FILTER: bekannte Bestands-Treffer
// werden VOR dem Bau-Versuch uebersprungen und als Skips ausgewiesen. OHNE Praedikat (Default) ist
// missing == Fenster -> der Bau bleibt byte-identisch zum Ist-Pfad (die Byte-Wachen bleiben gruen).
//
// Diese Naht ist eine FOKUS-Variante des B5-Producer-Consumer-Musters (SliceQueue): B5s BatchPlanner
// slict den GLOBALEN [0,total)-Indexraum mit Paritaet; I1b slict die SELEKTIERTE indices-Liste --
// beide erkennen Misses ueber DIESELBE Funktion (detect_missing_in_window, batch_planner.hpp).
//
// T2-A/F4 (2026-08-06, Owner-KERN Zaehler-Resume): der Producer STREAMT NICHT MEHR WAEHREND ER PLANT. Er
// konsolidiert den vollen Fenster-Plan, legt ihn (opt-in, PlanPersistenz) VOR dem Lauf ab und setzt beim
// Wiederanlauf hinter den Faechern auf, die der Bau-Zaehler bereits vollstaendig deckt. Ohne benannte
// Ablage aendert sich nur der ZEITPUNKT der Miss-Erkennung (alles vor dem ersten Push statt fensterweise)
// -- Reihenfolge, Fenster-Schnitt und gebaute Menge bleiben identisch.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib + ctsha512 (die EINE
// Hash-Primitive des Hauses -- s. slice_index_digest). pop()/push() thread-sicher.

#include "batch_planner.hpp" // detect_missing_in_window (die EINE per-Binary-Miss-Erkennung, B5)

#include <sha512/ctsha512.hpp> // T2-A/F4-NB: der Digest der Indexfolge im Plan-Stempel

#include <algorithm>
#include <atomic> // T2-A/F4: die zwei Berichts-Werte des Planers (Plan-Groesse, Resume-Front)
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem> // T2-A/F4: der Ablage-Pfad des Bau-Plans
#include <functional>
#include <iostream> // T2-A/F4: die Nie-stumm-Zeilen der Plan-Ablage (nur std::cerr, golden-/CSV-neutral)
#include <mutex>
#include <optional>
#include <span> // T2-A/F4-NB: die Byte-Sicht auf das Digest-Preimage
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// Praesenz je view-Index (Host-injiziert bzw. seit Lager-TP1(B) vom Iterator aus lager_contains +
// Fingerprint-Provider gebunden; Default absent -> "alles fehlt" -> baut alles = byte-identisch).
// Seit G-A2 ist sie der BAU-FILTER: present==true heisst "liegt als Bestand im Lager, nicht bauen";
// dll_is_current bleibt die zweite, lokale Verteidigungslinie fuer alles, was gebaut wird.
using PresenceFn = std::function<bool(std::size_t view_index)>;

// Slice-Korn (spiegelt planner::kGnBatchSlice, experiment_plan_director.hpp -- Bezeichner-Anker, keine
// Zeilennummer). LAG-P4: eine Abweichung von 4096 ist ab jetzt ein COMPILE-Fehler; die Wache steht in
// der Schicht, die alle drei Traeger sieht (experiment_plan_director.hpp, direkt bei
// planner::kGnBatchSlice). Diese Datei darf sie nicht selbst tragen -- der Bestandslog liegt UNTER dem
// Planer und inkludiert nie aufwaerts (progress_delta.hpp:5).
//
// WARUM DAS KORN HIER UEBERHAUPT NOCHMAL STEHT: es ist der Default des grain-Parameters von
// slice_view_indices und der Wert, den der Batch-Plan-Stempel ('|korn=') traegt. Der Mess-Lauf findet
// den Plan seines eigenen Bau-Laufs nur wieder, wenn beide Laeufe dieselbe Zahl stempeln.
inline constexpr std::size_t kBuildSliceGrain = 4096;

// Pure: slict die selektierten view-Indizes in Fenster der Groesse grain (Reihenfolge erhalten). Die
// Konkatenation der Fenster ergibt EXAKT die Eingabe zurueck -> Determinismus-Basis (aktiv == inaktiv).
[[nodiscard]] inline std::vector<std::vector<std::size_t>> slice_view_indices(std::vector<std::size_t> const& indices,
                                                                              std::size_t                     grain) {
    std::vector<std::vector<std::size_t>> out;
    if (grain == 0) grain = kBuildSliceGrain;
    for (std::size_t b = 0; b < indices.size(); b += grain) {
        std::size_t const e = std::min(indices.size(), b + grain);
        out.emplace_back(indices.begin() + static_cast<std::ptrdiff_t>(b),
                         indices.begin() + static_cast<std::ptrdiff_t>(e));
    }
    return out;
}

// TP1FK1-B1 (Codex-Befund CX-W2): das Reservierungs-Fenster als INTERVALL [begin, begin+count), das die
// (moeglicherweise luecken-behaftete) Index-MENGE des Slices VOLL ENTHAELT. Die Reservierung traegt auf
// dem Draht nur dieses Paar (slice_begin/slice_count, bestandslog_document.hpp); scope_covers_slice
// zaehlt darin die Selektions-Eintraege des uebernehmenden Laufs.
//
// begin = min(view_indices), count = die SPANNE max-min+1 -- NICHT die Kardinalitaet. Fuer die
// zusammenhaengenden Fenster, die dieses System schneidet (slice_view_indices ueber eine dichte
// Selektion), ist das EXAKT [front, front+size) und damit byte-identisch zur bisherigen Form. Bei einer
// LUECKENHAFTEN Menge trennen sich die beiden:
//   {0,2} als (front, size) = (0,2) beschreibt das Intervall {0,1} -- eine fremde Selektion {0,1}
//         bestand die Deckungspruefung und gab den Claim frei, obwohl Index 2 von NIEMANDEM gebaut
//         wird. Das war der VERLUSTBEHAFTETE Schreibweg (Arbeit bleibt liegen).
//   {0,2} als Spanne = (0,3) UMFASST die reale Menge: freigegeben wird erst, wenn die uebernehmende
//         Selektion das ganze Intervall {0,1,2} -- und damit jeden realen Index -- baut.
// Die Deckungs-Frage faellt so KONSERVATIV aus: nie faelschlich released, hoechstens einmal zu selten
// uebernommen (ein Claim aus luecken-behafteter Herkunft laeuft dann in seine pro-forma-Frist).
//
// GRENZE, ehrlich benannt: eine MENGEN-genaue Reservierung (die auch die Gegenrichtung -- der Lauf mit
// der identischen gappy Menge reapt sein eigenes Fenster -- aufloesen wuerde) braucht ein weiteres
// Draht-Feld und damit einen syntax_version-Bump des Bestandslog-Dokuments. Das ist ein Owner-Entscheid
// und hier bewusst NICHT genommen; die Spannen-Form kommt ohne jede Draht-Aenderung aus.
struct SliceWindowBounds {
    std::uint64_t begin = 0; // min(view_indices)
    std::uint64_t count = 0; // SPANNE max-min+1; == view_indices.size() genau bei Lueckenfreiheit
};

[[nodiscard]] inline SliceWindowBounds slice_window_bounds(std::vector<std::size_t> const& view_indices) noexcept {
    if (view_indices.empty()) return SliceWindowBounds{0, 0}; // leeres Fenster: nichts zu beanspruchen
    auto const [mn, mx] = std::minmax_element(view_indices.begin(), view_indices.end());
    auto const lo       = static_cast<std::uint64_t>(*mn);
    auto const hi       = static_cast<std::uint64_t>(*mx);
    return SliceWindowBounds{lo, hi - lo + 1}; // hi >= lo -> Spanne >= 1, kein Unterlauf
}

// Ein Bau-Fenster-Plan: die (VOLLEN) view-Indizes des Fensters + die Miss-Zahl. Das Fenster bleibt
// VOLL im Plan (die Reservierung beansprucht das FENSTER, nicht nur seine Luecken); welche Indizes
// wirklich gebaut werden, entscheidet der Consumer ueber filter_window_for_build (G-A2).
struct BuildSlicePlan {
    std::vector<std::size_t> view_indices;
    std::size_t              missing_count = 0;

    friend bool operator==(BuildSlicePlan const&, BuildSlicePlan const&) = default;
};

// Ergebnis des BAU-FILTERS eines Fensters (G-A2): die zu bauenden (fehlenden) Indizes in
// Fenster-Reihenfolge + die Zahl der als Bestand uebersprungenen. Invariante:
// zu_bauen.size() + bestand_uebersprungen == Fenstergroesse.
struct BuildFilterErgebnis {
    std::vector<std::size_t> zu_bauen;
    std::size_t              bestand_uebersprungen = 0;

    friend bool operator==(BuildFilterErgebnis const&, BuildFilterErgebnis const&) = default;
};

// DER BAU-FILTER (G-A2, LEDGER:3397): jedes fehlende Binary des Fensters EINZELN erkannt -- ueber
// detect_missing_in_window (B5), positions-basiert auf die Fenster-Liste gehoben. OHNE Praedikat
// fehlt alles (byte-identisch: volles Fenster wird gebaut). Die Erkennungs-Funktion ist damit
// dieselbe, die der globale BatchPlanner nutzt -- EINE Miss-Wahrheit, zwei Slicing-Formen.
[[nodiscard]] inline BuildFilterErgebnis filter_window_for_build(std::vector<std::size_t> const& window,
                                                                 PresenceFn const&               present) {
    BuildFilterErgebnis erg;
    if (!present) {
        erg.zu_bauen = window; // kein Praedikat -> alles fehlt -> volles Fenster (Ist-Verhalten)
        return erg;
    }
    auto const fehlende_positionen =
        detect_missing_in_window(0, static_cast<std::uint64_t>(window.size()),
                                 [&](std::uint64_t pos) { return present(window[static_cast<std::size_t>(pos)]); });
    erg.zu_bauen.reserve(fehlende_positionen.size());
    for (auto const pos : fehlende_positionen) erg.zu_bauen.push_back(window[static_cast<std::size_t>(pos)]);
    erg.bestand_uebersprungen = window.size() - erg.zu_bauen.size();
    return erg;
}

// Thread-sichere FIFO-Queue der Plaene (Muster SliceQueue). Nicht kopier-/verschiebbar.
class SlicePlanQueue {
public:
    SlicePlanQueue()                                 = default;
    SlicePlanQueue(SlicePlanQueue const&)            = delete;
    SlicePlanQueue& operator=(SlicePlanQueue const&) = delete;

    void push(BuildSlicePlan p) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push_back(std::move(p));
        }
        cv_.notify_one();
    }

    // Blockiert bis ein Plan da ist ODER geschlossen + leer (dann nullopt).
    [[nodiscard]] std::optional<BuildSlicePlan> pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this] { return closed_ || !q_.empty(); });
        if (q_.empty()) return std::nullopt;
        BuildSlicePlan p = std::move(q_.front());
        q_.pop_front();
        return p;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    mutable std::mutex         mtx_;
    std::condition_variable    cv_;
    std::deque<BuildSlicePlan> q_;
    bool                       closed_ = false;
};

// ---------------------------------------------------------------------------------------------------
// T2-A/F4 -- DIE PLAN-KONSOLIDIERUNG DES PRODUKTIVEN ZWILLINGS.
//
// plan_alle_faecher ist der PURE Plan: alle Fenster der Selektion, in Index-Reihenfolge, je mit der
// Miss-Zahl, die filter_window_for_build spaeter zum Bau freigibt (DIESELBE Erkennung -- EINE Wahrheit).
// Sie ist aus SlicePlanner::run herausgeloest, damit der Plan als GANZES existiert, bevor irgendetwas
// gebaut wird, und ohne Thread und Queue testbar ist.
// ---------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::vector<BuildSlicePlan> plan_alle_faecher(std::vector<std::size_t> const& indices,
                                                                   std::size_t grain, PresenceFn const& present) {
    std::vector<BuildSlicePlan> plan;
    for (auto& window : slice_view_indices(indices, grain)) {
        std::size_t const missing = filter_window_for_build(window, present).zu_bauen.size();
        plan.push_back(BuildSlicePlan{std::move(window), missing});
    }
    return plan;
}

// Die Dokument-Sicht des Bau-Plans: Fenster-Anfang + Fenster-Groesse + Miss-Zahl. begin ist der ERSTE
// Index des Fensters (nicht slice_window_bounds::begin -- das ist die Reservierungs-SPANNE einer evtl.
// luecken-behafteten Menge und beantwortet eine andere Frage). Der Plan haelt die Ordnung fest, in der
// die Fenster gebaut werden; die Reservierung haelt fest, was ein Fenster beansprucht.
[[nodiscard]] inline std::vector<PlanFach> slice_plan_faecher(std::vector<BuildSlicePlan> const& plan) {
    std::vector<PlanFach> aus;
    aus.reserve(plan.size());
    for (auto const& p : plan)
        aus.push_back(PlanFach{p.view_indices.empty() ? 0 : static_cast<std::uint64_t>(p.view_indices.front()),
                               static_cast<std::uint64_t>(p.view_indices.size()),
                               static_cast<std::uint64_t>(p.missing_count)});
    return aus;
}

// T2-A/F4-NB (Codex-Scope-F4, KRITISCH) -- DIE INDEXFOLGE SELBST GEHT IN DEN STEMPEL.
//
// DER BEFUND AM OBJEKT: bis hierher band der Stempel die Selektion ueber DREI Zahlen -- front(),
// size() und das Korn. Fuer eine DICHTE Selektion beschreiben die drei sie vollstaendig; fuer eine
// LUECKENBEHAFTETE tun sie es nicht. {0,1,2,3} und {0,5,7,9} teilen Anfang, Groesse und Korn -- und
// hatten damit denselben Stempel. Der Zaehler-Leser prueft zusaetzlich NUR die Fachzahl und die
// Atomsumme (parse_phasen_zaehler), und auch die stimmen bei gleicher Groesse und gleichem Korn
// ueberein. Ein Zaehlerstand, der fuer die EINE Selektion erarbeitet wurde, galt deshalb fuer die
// ANDERE: der Folgelauf uebersprang FUEHRENDE FAECHER, deren Binaries nie gebaut wurden. Das ist ein
// Loch in der Matrix, und zwar ein stilles -- der Plan-Resume behauptete Arbeit, die es nicht gab.
//
// DIE HEILUNG: ein Digest ueber die Indexfolge als eigenes Stempel-Glied. Er bindet REIHENFOLGE und
// INHALT, nicht nur die Ecken. Gebildet mit DERSELBEN Primitive, aus der auch der Fingerprint
// entsteht (sha512::sha512, ctsha512.hpp) -- diese Naht bringt keine zweite Hash-Wahrheit ins Haus.
//
// DAS PREIMAGE ist die Dezimalfolge der Indizes, je Index ein '\n' als Abschluss. Der Trenner liegt
// beweisbar ausserhalb des Zeichenvorrats der Glieder (Ziffern), die Abbildung Folge -> Preimage ist
// damit injektiv: es gibt keine zweite Folge, die dasselbe Preimage erzeugt (dieselbe Ueberlegung wie
// beim Delimiter des Anatomie-Preimages, A13-M3).
//
// EHRLICH BENANNT: ein Digest ist keine Identitaet, sondern ihre 512-Bit-Verdichtung -- zwei
// verschiedene Folgen KOENNTEN denselben tragen. Die Alternative waere, die Liste selbst in den
// Stempel zu legen; bei 2^17 Indizes ist das ein Megabyte, und der Stempel ist eine ZEILE. Gewaehlt
// ist der Digest, weil die Kollisionsfrage hier keine Angreifer-, sondern eine Zufallsfrage ist.
[[nodiscard]] inline std::string slice_index_digest(std::vector<std::size_t> const& indices) {
    std::string preimage;
    preimage.reserve(indices.size() * 8);
    for (std::size_t const i : indices) {
        preimage += std::to_string(i);
        preimage += '\n';
    }
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const hex = ::comdare::cache_engine::sha512::to_hex(digest);
    return std::string{hex.data(), hex.size()};
}

// T2-A/F4-NB2 (Codex-Voll-Scope, Befund 3) -- DIE BAU-IDENTITAET GEHOERT IN DEN STEMPEL.
//
// DER LEITSATZ, der diese Naht traegt: DER ZAEHLER-RESUME DARF NIE MEHR BEHAUPTEN, ALS DER FINGERPRINT
// DECKT. Bis hierher war er die zweite, davon UNABHAENGIGE Resume-Autoritaet: er entfernt ganze FAECHER
// aus dem Strom (SlicePlanner::run, Schritt (4)), also VOR dem Bau -- und damit vor dll_is_current, dem
// EINEN Vergleich, der die Bau-Identitaet prueft. Ein Zaehler, der unter der Bau-Identitaet A erarbeitet
// wurde, galt fuer einen Lauf mit der Bau-Identitaet B weiter, weil der Stempel von B nichts wusste:
// Selektion, Groesse, Korn und Indexfolge sind identisch, wenn sich nur die TOOLCHAIN aendert. Der
// Folgelauf uebersprang dann fuehrende Faecher, deren .so aus einem anderen Bau-Stand stammen -- die
// Fingerprint-Pruefung wurde nicht ueberstimmt, sie wurde NIE GEFRAGT.
//
// DIE HEILUNG ist die exakte und nicht die naheliegende: gebunden wird nicht "eine Bau-Versions-Zeichenkette",
// sondern GENAU DIE MENGE DER ERWARTETEN FINGERPRINTS, die dll_is_current je Index vergleichen wuerde --
// als Digest ueber die '\n'-abgeschlossene Folge in PLAN-ORDNUNG. Damit gilt die Zusage buchstaeblich:
// aendert sich irgendetwas, was einen einzigen dieser Fingerprints bewegt (Toolchain-Realversion,
// opt/ext/gate/atomic128, bvset, Zellwerte, Organ-Achsen-Versionen -- oder die Abbildung Index -> Anatomie
// selbst), traegt der Stempel einen anderen Digest und der Alt-Zaehler wird fail-closed abgelehnt.
//
// KOSTEN, ehrlich benannt: ein voller Durchlauf der FingerprintFn ueber die Selektion, EINMAL vor dem Bau.
// Das ist dieselbe Groessenordnung, die der Plan ohnehin zweimal zahlt (der Miss-Scan des Planers und der
// Bau-Filter des Consumers rufen die PresenceFn je Index, und die loest ueber denselben Provider auf).
// Der Stempel selbst waechst um EIN Glied fester Laenge -- die Doktrin "der Stempel ist eine ZEILE" haelt.
//
// OHNE ANKER: liefert der Aufrufer keine Identitaets-Funktion, traegt das Glied das Wort `ohne-anker`.
// Das ist keine Bequemlichkeit, sondern die zweite Haelfte des Leitsatzes: ohne Fingerprint-Provider
// deckt dll_is_current NICHTS (es ist fail-closed und skippt nie), also darf ein Zaehler aus einem Lauf
// MIT Ankern hier nicht gelten -- und umgekehrt. Die beiden Welten tragen verschiedene Stempel und
// koennen sich nicht gegenseitig zertifizieren.
//
// SPEICHER, ehrlich beziffert: das Preimage ist EIN zusammenhaengender Puffer von rund 129 Byte je Index
// (die sha512-Primitive dieses Hauses nimmt eine span, sie hat keine inkrementelle Form -- dieselbe Lage
// wie bei slice_index_digest daneben). Fuer ein Pass-Fenster von 2^17 Binaries sind das ~17 MB, EINMAL,
// waehrend der Plan gebildet wird -- neben dem results-Vektor O(K) desselben Fensters keine neue
// Groessenordnung, aber auch keine Null; deshalb steht die Zahl hier und nicht nur im Kopf des Autors.
// Die Klammer dagegen ist die Aufrufer-Seite: ohne Plan-Ablage wird diese Funktion nie mit einer
// Identitaets-Funktion gerufen (plan_identitaet_of, cache_engine_builder_iterator.hpp).
using PlanIdentitaetFn = std::function<std::string(std::size_t view_index)>;

inline constexpr char kPlanOhneAnker[] = "ohne-anker";

[[nodiscard]] inline std::string plan_bau_digest(std::vector<std::size_t> const& indices,
                                                 PlanIdentitaetFn const&         identitaet) {
    if (!identitaet) return std::string{kPlanOhneAnker};
    std::string preimage;
    preimage.reserve(indices.size() * 129);
    for (std::size_t const i : indices) {
        preimage += identitaet(i);
        preimage += '\n'; // derselbe Trenner wie im Indexfolge-Preimage: ausserhalb des Hex-Alphabets
    }
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const hex = ::comdare::cache_engine::sha512::to_hex(digest);
    return std::string{hex.data(), hex.size()};
}

// Der Stempel des Bau-Plans: GENAU die Groessen, die den Plan bestimmen. Die PresenceFn steht bewusst
// NICHT darin -- sie ist Lager-ZUSTAND, kein Plan-Parameter; ein zwischenzeitlich gefuelltes Lager darf
// einen Zaehler nicht entwerten, es aendert nur die Miss-Zahlen innerhalb derselben Faecher.
//
// T2-A/F4-NB: |idx= bindet die Indexfolge EINDEUTIG (s. slice_index_digest). start/indizes bleiben
// stehen -- nicht als Wache (das ist jetzt |idx=), sondern als LESBARKEIT: wer die Ablage von Hand
// ansieht, erkennt Fenster und Groesse, ohne einen Digest aufloesen zu koennen.
//
// T2-A/F4-NB2: |bau= bindet die BAU-IDENTITAET (s. plan_bau_digest). |idx= sagt "genau diese Folge",
// |bau= sagt "genau dieser Bau-Stand" -- erst zusammen decken sie, was der Zaehler behauptet.
[[nodiscard]] inline std::string slice_plan_stamp(std::vector<std::size_t> const& indices, std::size_t grain,
                                                  PlanIdentitaetFn const& identitaet = {}) {
    return std::string{kBatchPlanFormat} +
           "|art=bau-slice|start=" + std::to_string(indices.empty() ? std::size_t{0} : indices.front()) +
           "|indizes=" + std::to_string(indices.size()) +
           "|korn=" + std::to_string(grain == 0 ? kBuildSliceGrain : grain) + "|idx=" + slice_index_digest(indices) +
           "|bau=" + plan_bau_digest(indices, identitaet);
}

// DIE PLAN-ABLAGE-NAHT (opt-in, Muster der uebrigen bestandslog-Injektionen): leerer Pfad => inert =>
// der Bau bleibt byte-/verhaltensidentisch zum Ist. rows_key wird vom Iterator hereingereicht
// (kLazyResumeRowsKey) -- die Abhaengigkeits-Richtung erlaubt hier keinen Zugriff darauf, und ein
// zweites Literal waere genau die Format-Drift, gegen die die W5-Hebung gebaut wurde.
struct PlanPersistenz {
    std::filesystem::path plan_datei; ///< leer => keine Ablage, kein Resume
    std::string           stamp;      ///< slice_plan_stamp(...) dieses Laufs
    std::string           rows_key;   ///< == ex::kLazyResumeRowsKey

    [[nodiscard]] bool aktiv() const noexcept { return !plan_datei.empty() && !stamp.empty() && !rows_key.empty(); }
    /// Der Zaehler-Stand liegt NEBEN dem Plan (abgeleiteter Name, eine Quelle): <plan>.zaehler
    [[nodiscard]] std::filesystem::path zaehler_datei() const {
        return std::filesystem::path{plan_datei.string() + ".zaehler"};
    }
};

// Async Producer: KONSOLIDIERT ZUERST den vollen Plan (plan_alle_faecher), legt ihn -- wo eine Ablage
// benannt ist -- VOR dem ersten Push auf die Platte, ueberspringt die vom Bau-Zaehler bereits
// vollstaendig gedeckten FUEHRENDEN Faecher und schiebt den Rest IN REIHENFOLGE in die Queue; schliesst
// am Ende. RAII: der Thread joined im dtor (auch im Fehlerfall).
//
// WARUM DER ZAEHLER `kompiliert` UND NICHT `gemessen` DIE RESUME-FRONT DIESES PLANERS IST: dieser Pfad
// ist per planer_driven_active an provision_only gebunden -- er ist der BAU-Lauf. Seine eigene Front ist
// die Bau-Front. `gemessen` fuehrt der SEPARATE Mess-Lauf fort (Owner: "kompiliert/separat gemessen");
// er liest denselben Plan, aber sein feinkoerniger Resume-Arbiter ist und bleibt die per-Binary
// result.csv+stamp-Naht (T2-A/K2) -- der Plan-Zaehler beantwortet dort die GROBE Frage des Planers und
// ist nicht die zweite Autoritaet.
//
// T2-A/F4-NB2 (Befund 2): `gemessen` ist seit dem batchplan-v3-Bump ebenfalls ein PRAEFIX (s. PhasenZaehler,
// batch_planner.hpp). Das aendert NICHTS an der Zustaendigkeit -- dieser Planer liest weiterhin allein
// `kompiliert` als seinen Resume-Punkt und reicht `gemessen` unveraendert durch. Dass die Zahl jetzt eine
// Front IST, macht sie nicht zur Front DIESES Weges: Bau-Arbeit auf Grundlage einer Mess-Zahl zu
// ueberspringen bliebe die Vermischung zweier Phasen, die der Owner-KERN ausdruecklich trennt.
class SlicePlanner {
public:
    SlicePlanner(SlicePlanQueue& q, std::vector<std::size_t> indices, std::size_t grain, PresenceFn present,
                 PlanPersistenz persistenz = {})
        : q_{q}, indices_{std::move(indices)}, grain_{grain == 0 ? kBuildSliceGrain : grain},
          present_{std::move(present)}, persistenz_{std::move(persistenz)} {
        worker_ = std::thread([this] { run(); });
    }

    SlicePlanner(SlicePlanner const&)            = delete;
    SlicePlanner& operator=(SlicePlanner const&) = delete;

    ~SlicePlanner() { join(); }

    void join() {
        if (worker_.joinable()) worker_.join();
    }

    /// Die Zahl der Faecher des GESAMTEN Plans (inkl. der uebersprungenen). Beide Werte werden gesetzt,
    /// BEVOR das erste Fach in die Queue geht -- ein Konsument, der bereits ein Fach gezogen hat (oder die
    /// geschlossene Queue gesehen hat), liest sie deshalb fertig. atomar, damit diese Zusage nicht an einer
    /// Speicher-Ordnungs-Ueberlegung haengt, sondern am Typ.
    [[nodiscard]] std::size_t plan_faecher_zahl() const noexcept { return plan_faecher_.load(); }
    /// Die Atome der uebersprungenen FUEHRENDEN Faecher == der Bau-Zaehlerstand, auf dem dieser Lauf aufsetzt.
    [[nodiscard]] std::uint64_t resume_atome() const noexcept { return resume_atome_.load(); }
    /// Die vorgefundene MESS-FRONT. Der Bau-Lauf schreibt sie unveraendert zurueck, statt sie auf 0 zu setzen:
    /// er baut ausschliesslich HINTER seiner eigenen Bau-Front, ruehrt also keine bereits gemessene Binary an.
    ///
    /// NAMENS-HISTORIE (zwei Wellen, zwei Bedeutungen -- der Name folgte beide Male der Sache):
    ///   * `resume_gemessen()` (T2-A/F4) las sich wie eine zweite RESUME-FRONT dieses Planers. Das war sie nie.
    ///   * `vorgefundene_mess_bilanz()` (T2-A/F4-NB) pinnte den damaligen Ist: eine BILANZ, weil der Mess-Weg
    ///     die Zahl bei jeder irgendwo erfolgreichen Zelle hochzaehlte (Codex-Voll-Scope Befund 2).
    ///   * `vorgefundene_mess_front()` (T2-A/F4-NB2) benennt den geheilten Ist: der Mess-Weg schreibt ab jetzt
    ///     die Zahl der FUEHRENDEN zertifizierten Zellen -- dieselbe Praefix-Semantik wie beim Bau-Zaehler.
    ///
    /// UNVERAENDERT GILT: der Resume-Punkt DIESES Planers ist und bleibt ALLEIN die Bau-Front (`kompiliert`,
    /// s. den Klassen-Kopf). Wer die Mess-Front hier als Bau-Praefix konsumiert, ueberspringt BAU-Arbeit auf
    /// Grundlage einer MESS-Zahl -- zwei Phasen, die der Owner-KERN ausdruecklich trennt ("kompiliert/separat
    /// gemessen"). Dass die Zahl jetzt eine Front IST, macht sie nicht zur Front DIESES Weges.
    [[nodiscard]] std::uint64_t vorgefundene_mess_front() const noexcept { return mess_front_.load(); }

private:
    void run() {
        // (1) ERST DER PLAN -- vollstaendig, bevor ein Fach die Queue erreicht.
        auto plan = plan_alle_faecher(indices_, grain_, present_);

        // (2) DANN DIE ABLAGE + DER RESUME-PUNKT (nur mit benannter Ablage; sonst byte-identisch zum Ist).
        std::size_t ab = 0;
        if (persistenz_.aktiv()) {
            auto const faecher = slice_plan_faecher(plan);
            // Der Zaehler-Stand wird gelesen, BEVOR der Plan geschrieben wird: ein Alt-Zaehler gilt gegen
            // den GERADE geplanten Stand (Faecher-Zahl + Atom-Summe), und die Pruefung dafuer steckt in
            // read_phasen_zaehler. Nach dem Schreiben waere die Reihenfolge dieselbe -- die Trennung haelt
            // aber sichtbar, dass der Zaehler den Plan NICHT mitbringt, sondern sich an ihm ausweist.
            if (auto const z = read_phasen_zaehler(persistenz_.zaehler_datei(), persistenz_.stamp, faecher,
                                                   persistenz_.rows_key)) {
                ab = plan_resume_faecher(faecher, z->kompiliert);
                mess_front_.store(z->gemessen);
            }
            if (!write_batch_plan(persistenz_.plan_datei, persistenz_.stamp, faecher, persistenz_.rows_key))
                // Nie-stumm: eine nicht geschriebene Plan-Ablage ist KEIN Bau-Fehler (der Bau laeuft mit
                // dem Plan im Speicher weiter), aber sie ist auch keine Nebensache -- der naechste Lauf
                // haette sonst still keinen Resume-Anspruch und niemand wuesste warum.
                std::cerr << "[bestandslog] warn: Batch-Plan nicht persistiert (" << persistenz_.plan_datei.string()
                          << ") -- der Lauf baut weiter, ein Folgelauf hat aber keinen Plan-Resume-Anspruch\n"
                          << std::flush;
            if (ab > 0)
                std::cerr << "[bestandslog] plan-resume: " << ab << " von " << faecher.size()
                          << " Faechern bereits kompiliert (Zaehler gegen den Plan) -- uebersprungen\n"
                          << std::flush;
        }

        // (3) DIE BERICHTS-WERTE VOR DEM ERSTEN PUSH -- s. plan_faecher_zahl()/resume_atome().
        plan_faecher_.store(plan.size());
        std::uint64_t atome = 0;
        for (std::size_t i = 0; i < ab; ++i) atome += static_cast<std::uint64_t>(plan[i].view_indices.size());
        resume_atome_.store(atome);

        // (4) DANN DER STROM.
        for (std::size_t i = ab; i < plan.size(); ++i) q_.push(std::move(plan[i]));
        q_.close();
    }

    SlicePlanQueue&            q_;
    std::vector<std::size_t>   indices_;
    std::size_t                grain_;
    PresenceFn                 present_;
    PlanPersistenz             persistenz_;
    std::atomic<std::size_t>   plan_faecher_{0};
    std::atomic<std::uint64_t> resume_atome_{0};
    std::atomic<std::uint64_t> mess_front_{0};
    std::thread                worker_;
};

} // namespace comdare::cache_engine::builder::bestandslog
