#pragma once
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4:
// "Konformitaets-Orakel je Genus (Entscheide 1.1-1.4)") -- die vier Container-Gattungs-Orakel des
// V5-Konformitaets-Gates.
//
// WAS DIESE DATEI IST: das Gegenstueck zu conformance_gate.hpp (SearchAlgorithm-Gattung, Orakel std::map)
// fuer die vier Container-Genera. Der V5-Vertrag (pruef_dock.hpp:74-79) verlangt von JEDER
// measure()-Implementierung die Reihenfolge import -> KONFORMITAETS-GATE -> messen; bis heute konnte nur die
// SA-Gattung dieses Gate erfuellen, weil es fuer die vier Container-Genera kein Orakel gab.
//
// ORAKEL-ENTSCHEIDE (Bauplan 1.1-1.4, hier NICHT neu erfunden, sondern umgesetzt):
//   Set      -> std::set<std::uint64_t>                       (Bauplan 1.1: direkte Ordnungs-Analogie zum
//                                                              SA-std::map-Gate)
//   Sequence -> std::deque<std::uint64_t>                     (Bauplan 1.2)
//   Adapter  -> die benannten Orakel std::queue (FIFO) und    (Bauplan 1.3: "ein einzelnes Orakel wuerde die
//               std::stack (LIFO) -- PLUS std::priority_queue  Adapter-Richtungs-Semantik unpruefbar lassen")
//               (siehe ABWEICHUNG direkt unten)
//   View     -> std::span<std::uint64_t const>                (Bauplan 1.4: die einzige std-Klasse der
//                                                              Kandidatenliste mit ehrlicher View-Semantik)
//
// ABWEICHUNG VOM BAUPLAN (am Ist erzwungen, nicht geraten -- Bauplan 1.3 nennt ZWEI Adapter-Orakel, das Ist
// verlangt DREI): die inner_container-Achse traegt am HEAD drei reale Substrate (adapter_anatomy.hpp:52
// DequeInner, :69 VectorInner, :90 HeapInner). AdapterAnatomy::get() ist pop_front (adapter_anatomy.hpp:262).
// Fuer Deque/Vector ist front() das zuerst Abgelegte -> FIFO. HeapInner ist aber ein echter MAX-HEAP ueber
// std::push_heap/std::pop_heap, front() == Maximum, pop_front() == Extract-Max (adapter_anatomy.hpp:88-110)
// -> PRIORITY. Literal erhoben (eigene Probe 04.08., put(10),put(30),put(20)):
//     HeapInner front = 30   |   DequeInner front = 10   |   VectorInner front = 10
// Mit NUR zwei Orakeln waere HeapInner nach dem "letztes-heraus"-Kriterium faelschlich als LIFO erkannt
// worden (30 IST hier zufaellig das zuletzt einsortierte Maximum) und danach am std::stack-Orakel
// durchgefallen -- das Gate haette also jede Heap-Permutation von der Messung ausgeschlossen. Deshalb ein
// DRITTES benanntes Orakel std::priority_queue + eine Disziplin-Probe mit drei UNTERSCHEIDBAREN Werten
// (erstes != letztes != groesstes), die alle drei Faelle trennt statt zwei zu erraten.
// NEBENBEFUND (ehrlich, nicht kaschiert): am Ist gibt es KEINE LIFO-Permutation in der Registry -- 2 von 3
// Substraten sind FIFO, 1 ist PRIORITY. Das std::stack-Orakel bleibt trotzdem gebaut (Bauplan-Entscheid,
// und die Disziplin ist laut adapter_tier.hpp:58-59 API-Nutzung: eine kuenftige back/pop_back-Nutzung
// braucht es sofort); belegt wird es hier durch eine Test-Huelle, nicht durch eine Registry-Variante.
//
// AUFLAGE 7 (E24-DOSSIER:267) EINGEHALTEN: der conformance-BESTAND wird nur ERWEITERT, nie geaendert.
// conformance_gate.hpp ist byte-unberuehrt; von dort wird ausschliesslich der Ergebnis-Typ
// ConformanceResult WIEDERVERWENDET (ein zweiter, gattungs-eigener Ergebnis-Typ waere eine stille
// Vertrags-Spaltung -- die Quoten-Semantik "cases_total/cases_passed/first_fail" ist gattungs-unabhaengig).
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only. Die vier Gattungs-Antriebs-Sub-Interfaces
// (set_tier.hpp/sequence_tier.hpp/adapter_tier.hpp/view_tier.hpp) werden NUR AUFGERUFEN, nicht geaendert;
// abi_adapter.hpp, die Wire-PODs, abi/*_decl.hpp und die Stempel-/Fingerprint-Flaechen bleiben unberuehrt.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md Paragraf 8.8
// @doku docs/architecture/20260803-e24_container_gattungs_abi_dossier.md

#include "conformance_gate.hpp" // ConformanceResult (WIEDERVERWENDET, Bestand byte-unberuehrt)

#include <anatomy/adapter_tier.hpp>  // IAdapterTier
#include <anatomy/sequence_tier.hpp> // ISequenceTier
#include <anatomy/set_tier.hpp>      // ISetTier
#include <anatomy/view_tier.hpp>     // IViewTier

#include <array>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <limits>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <stack>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::pruef_dock {

namespace genus_gate_detail {

/// Der Zaehl-/Report-Apparat, den alle vier Gattungs-Gates teilen. Form BEWUSST identisch zu den lokalen
/// Lambdas in run_conformance_gate (conformance_gate.hpp:69-87): gleiche Quoten-Semantik, gleiche
/// Report-Zeilen ("    <name>: OK (n/m)" bzw. SKIP) -- ein zweites Report-Format waere ein stiller
/// Auswertungs-Bruch fuer jeden, der beide Gates nebeneinander liest.
class GateTally {
public:
    explicit GateTally(std::FILE* report) noexcept : report_{report} {}

    void check(bool ok) noexcept {
        ++r_.cases_total;
        if (ok) {
            ++r_.cases_passed;
        } else if (r_.first_fail == 0) {
            r_.first_fail = r_.cases_total;
        }
    }

    /// Markiert den Beginn eines Randfall-Blocks; der Rueckgabewert geht an block_end().
    [[nodiscard]] std::array<std::uint64_t, 2> block_begin() const noexcept {
        return {r_.cases_total, r_.cases_passed};
    }

    void block_end(char const* name, std::array<std::uint64_t, 2> const& before) noexcept {
        if (report_ == nullptr) return;
        auto const total_delta  = r_.cases_total - before[0];
        auto const passed_delta = r_.cases_passed - before[1];
        std::fprintf(report_, "    %s: %s (%llu/%llu)\n", name, (total_delta == passed_delta) ? "OK" : "FAIL",
                     static_cast<unsigned long long>(passed_delta), static_cast<unsigned long long>(total_delta));
    }

    void skip(char const* name, char const* why) noexcept {
        if (report_ == nullptr) return;
        std::fprintf(report_, "    %s: SKIP (%s)\n", name, why);
    }

    [[nodiscard]] ConformanceResult const& result() const noexcept { return r_; }

private:
    ConformanceResult r_{};
    std::FILE*        report_ = nullptr;
};

} // namespace genus_gate_detail

// ================================================================================================
// SET-Gattung -- Orakel std::set<std::uint64_t> (Bauplan 1.1)
// ================================================================================================

/// run_set_conformance_gate -- deterministische Randfall- + Zufallssequenz gegen ein std::set-Orakel.
/// `tier` wird VORHER geleert und am Ende geleert zurueckgelassen (saubere Ausgangslage fuer die Messung,
/// exakt wie beim SA-Gate). Je Op verglichen: Rueckgabe-Semantik (neu? / Mitglied? / existierte?) + Groesse.
/// noexcept: eine werfende Huelle verletzt den ABI-noexcept-Vertrag von ISetTier -> gilt als nicht-konform.
[[nodiscard]] inline ConformanceResult run_set_conformance_gate(anatomy::ISetTier& tier, std::uint64_t seed = 42,
                                                                std::uint64_t n_random = 2000, bool wide_keys = false,
                                                                std::FILE* report = nullptr) noexcept {
    genus_gate_detail::GateTally t{report};
    try {
        std::set<std::uint64_t> oracle;
        tier.tier_set_clear();

        // SRF1 leeres Tier: kein Mitglied, Kardinalitaet 0.
        auto const b1 = t.block_begin();
        t.check(tier.tier_set_contains(7) == oracle.contains(7));
        t.check(tier.tier_set_size() == oracle.size());
        t.block_end("SRF1 leer/contains/size", b1);

        // SRF2 insert NEU -> true (ein neuer Schluessel entstand); danach Mitglied; Kardinalitaet 1.
        auto const b2 = t.block_begin();
        t.check(tier.tier_set_insert(7) == oracle.insert(7).second);
        t.check(tier.tier_set_contains(7) == oracle.contains(7));
        t.check(tier.tier_set_size() == oracle.size());
        t.block_end("SRF2 insert(neu)/contains/size", b2);

        // SRF3 insert DUPLIKAT -> false (Mengen-Semantik: keine Duplikate); Kardinalitaet unveraendert.
        auto const b3 = t.block_begin();
        t.check(tier.tier_set_insert(7) == oracle.insert(7).second);
        t.check(tier.tier_set_size() == oracle.size());
        t.block_end("SRF3 insert(duplikat)", b3);

        // SRF4 erase EXISTIEREND -> true; danach kein Mitglied; Kardinalitaet dekrementiert; Menge leer.
        auto const b4 = t.block_begin();
        t.check(tier.tier_set_erase(7) == (oracle.erase(7) == 1u));
        t.check(tier.tier_set_contains(7) == oracle.contains(7));
        t.check(tier.tier_set_size() == oracle.size());
        t.check((tier.tier_set_size() == 0) == oracle.empty());
        t.block_end("SRF4 erase(hit)/contains/size", b4);

        // SRF5 erase NICHT-EXISTENT -> false; Kardinalitaet unveraendert.
        auto const b5 = t.block_begin();
        t.check(tier.tier_set_erase(999) == (oracle.erase(999) == 1u));
        t.check(tier.tier_set_size() == oracle.size());
        t.block_end("SRF5 erase(miss)", b5);

        // SRF6 Hit- und Miss-Faelle explizit gegen std::set::contains, inkl. erase-danach-Miss.
        auto const b6 = t.block_begin();
        t.check(tier.tier_set_insert(11) == oracle.insert(11).second);
        t.check(tier.tier_set_contains(11) == oracle.contains(11));
        t.check(tier.tier_set_contains(12) == oracle.contains(12));
        t.check(tier.tier_set_erase(11) == (oracle.erase(11) == 1u));
        t.check(tier.tier_set_contains(11) == oracle.contains(11));
        t.check(tier.tier_set_size() == oracle.size());
        t.block_end("SRF6 contains(hit/miss)/erase-danach", b6);

        // SRF7 Zufallssequenz ueber begrenzten Key-Raum (erzwingt Kollisionen/Duplikate/Erases),
        // je Schritt Rueckgabe-Semantik UND Kardinalitaet gegen das Orakel.
        auto const      b7 = t.block_begin();
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            std::uint64_t const key = rng() % 256;
            switch (rng() % 3) {
                case 0: { // insert
                    bool const tier_new = tier.tier_set_insert(key);
                    t.check(tier_new == oracle.insert(key).second);
                    break;
                }
                case 1: { // contains
                    t.check(tier.tier_set_contains(key) == oracle.contains(key));
                    break;
                }
                default: { // erase
                    bool const tier_existed = tier.tier_set_erase(key);
                    t.check(tier_existed == (oracle.erase(key) == 1u));
                    break;
                }
            }
            t.check(tier.tier_set_size() == oracle.size());
        }
        t.block_end("SRF7 random-core(insert/contains/erase/size)", b7);

        // SRF8 clear -> Kardinalitaet 0; beliebiges contains ist Miss.
        auto const b8 = t.block_begin();
        tier.tier_set_clear();
        oracle.clear();
        t.check(tier.tier_set_size() == oracle.size());
        t.check(tier.tier_set_contains(42) == oracle.contains(42));
        t.block_end("SRF8 clear/contains", b8);

        // SRF9 (opt-in) Rand des uint64-Key-Raums -- deckt Huellen auf, die Keys intern verkuerzen.
        if (wide_keys) {
            auto const                             b9 = t.block_begin();
            constexpr std::array<std::uint64_t, 4> keys{0u, 65'536u, (1ull << 32u),
                                                        std::numeric_limits<std::uint64_t>::max()};
            for (auto const key : keys) {
                t.check(tier.tier_set_insert(key) == oracle.insert(key).second);
                t.check(tier.tier_set_contains(key) == oracle.contains(key));
            }
            t.check(tier.tier_set_size() == oracle.size());
            t.block_end("SRF9 wide_keys opt-in", b9);
        } else {
            t.skip("SRF9 wide_keys opt-in", "wide_keys=false");
        }
        tier.tier_set_clear(); // saubere Ausgangslage fuer die anschliessende Messung
    } catch (...) {
        t.check(false); // werfende Huelle = nicht-konform
    }
    return t.result();
}

// ================================================================================================
// SEQUENCE-Gattung -- Orakel std::deque<std::uint64_t> (Bauplan 1.2)
// ================================================================================================

/// run_sequence_conformance_gate -- Randfall- + Zufallssequenz gegen ein std::deque-Orakel.
///
/// BENANNTE HALBHEIT DER HEUTIGEN ABI-FLAECHE (deklariert statt weggeglaettet): der Bauplan waehlt
/// std::deque ausdruecklich wegen "Wachstum an beiden Enden" (1.2). ISequenceTier (sequence_tier.hpp:36-51)
/// traegt am Ist NUR push_back/at/size/clear -- eine push_front-Op existiert auf der ABI-Flaeche NICHT.
/// Das Gate treibt das Orakel deshalb ehrlich nur ueber seine HINTERE Haelfte; die vordere ist durch
/// ISequenceTier heute nicht erreichbar und wird auch nicht simuliert (das waere ein Orakel, das mehr
/// behauptet als es prueft). Waechst die ABI-Flaeche um push_front, gehoert der Block hier additiv dazu.
[[nodiscard]] inline ConformanceResult
run_sequence_conformance_gate(anatomy::ISequenceTier& tier, std::uint64_t seed = 42, std::uint64_t n_random = 2000,
                              std::uint64_t n_fill = 256, std::FILE* report = nullptr) noexcept {
    genus_gate_detail::GateTally t{report};
    try {
        std::deque<std::uint64_t> oracle;
        std::uint64_t             probe = 0;
        tier.tier_clear();

        // QRF1 leere Sequenz: at(0) ist Miss, Laenge 0. Der out-Zeiger darf bei Miss unangetastet bleiben.
        auto const b1 = t.block_begin();
        t.check(tier.tier_at(0, &probe) == false);
        t.check(tier.tier_size() == oracle.size());
        t.block_end("QRF1 leer/at(miss)/size", b1);

        // QRF2 push_back: Laenge waechst monoton, jedes Element ist an SEINEM Index lesbar (Reihenfolge-Beweis).
        auto const b2 = t.block_begin();
        for (std::uint64_t i = 0; i < n_fill; ++i) {
            tier.tier_push_back(i * 7u + 1u);
            oracle.push_back(i * 7u + 1u);
            t.check(tier.tier_size() == oracle.size());
        }
        for (std::uint64_t i = 0; i < n_fill; ++i) {
            std::uint64_t got = 0;
            t.check(tier.tier_at(i, &got) == true);
            t.check(got == oracle[static_cast<std::size_t>(i)]);
        }
        t.block_end("QRF2 push_back/at(index-treu)/size", b2);

        // QRF3 at OUT-OF-BOUNDS: index >= size ist Miss (an der Grenze UND weit darueber).
        auto const b3 = t.block_begin();
        t.check(tier.tier_at(oracle.size(), &probe) == false);
        t.check(tier.tier_at(oracle.size() + 1u, &probe) == false);
        t.check(tier.tier_at(std::numeric_limits<std::uint64_t>::max(), &probe) == false);
        t.block_end("QRF3 at(out-of-bounds)", b3);

        // QRF4 at mit NULL-out: darf nicht abstuerzen und muss dieselbe Hit/Miss-Antwort liefern.
        auto const b4 = t.block_begin();
        t.check(tier.tier_at(0, nullptr) == !oracle.empty());
        t.check(tier.tier_at(oracle.size(), nullptr) == false);
        t.block_end("QRF4 at(out=nullptr)", b4);

        // QRF5 Zufallssequenz ueber [0, 2*size): ~50% Hit/Miss, je Treffer der Wert gegen das Orakel.
        auto const      b5 = t.block_begin();
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            std::uint64_t const idx = oracle.empty() ? 0u : (rng() % (oracle.size() * 2u));
            std::uint64_t       got = 0;
            bool const          hit = tier.tier_at(idx, &got);
            bool const          exp = idx < oracle.size();
            t.check(hit == exp);
            if (exp) t.check(got == oracle[static_cast<std::size_t>(idx)]);
        }
        t.block_end("QRF5 random-at(hit/miss/value)", b5);

        // QRF6 push_back NACH Zufallslauf: die Sequenz haengt hinten an, alte Indizes bleiben stabil.
        auto const b6 = t.block_begin();
        tier.tier_push_back(0xFEEDu);
        oracle.push_back(0xFEEDu);
        t.check(tier.tier_size() == oracle.size());
        std::uint64_t tail = 0;
        t.check(tier.tier_at(oracle.size() - 1u, &tail) == true);
        t.check(tail == oracle.back());
        std::uint64_t head = 0;
        t.check(tier.tier_at(0, &head) == true);
        t.check(head == oracle.front());
        t.block_end("QRF6 append-Stabilitaet(front/back)", b6);

        // QRF7 clear -> Laenge 0; jeder Index ist Miss.
        auto const b7 = t.block_begin();
        tier.tier_clear();
        oracle.clear();
        t.check(tier.tier_size() == oracle.size());
        t.check(tier.tier_at(0, &probe) == false);
        t.block_end("QRF7 clear/at(miss)", b7);

        // QRF8 ehrlich SKIP statt Schein-Beweis: die vordere deque-Haelfte ist ueber ISequenceTier nicht
        // erreichbar (siehe Kopf-Kommentar). Kein check() -- eine uebersprungene Zusicherung darf die
        // Quote weder heben noch senken.
        t.skip("QRF8 push_front/pop_front", "ISequenceTier traegt am Ist nur push_back/at/size/clear");
    } catch (...) { t.check(false); }
    return t.result();
}

// ================================================================================================
// ADAPTER-Gattung -- std::queue (FIFO), std::stack (LIFO), std::priority_queue (PRIORITY)
// (Bauplan 1.3 + die am Ist erzwungene dritte Disziplin, siehe ABWEICHUNG im Kopf)
// ================================================================================================

/// Die Entnahme-Disziplin, die eine geladene Adapter-Permutation TATSAECHLICH zeigt.
enum class AdapterDiscipline : std::uint8_t {
    unknown  = 0, ///< keine der drei -- kein Orakel anwendbar (nicht-konform)
    fifo     = 1, ///< std::queue-Orakel        (DequeInner / VectorInner)
    lifo     = 2, ///< std::stack-Orakel        (back/pop_back-Nutzung)
    priority = 3  ///< std::priority_queue-Orakel (HeapInner, Extract-Max)
};

[[nodiscard]] inline std::string_view adapter_discipline_name(AdapterDiscipline d) noexcept {
    switch (d) {
        case AdapterDiscipline::fifo: return "fifo";
        case AdapterDiscipline::lifo: return "lifo";
        case AdapterDiscipline::priority: return "priority";
        case AdapterDiscipline::unknown: break;
    }
    return "unknown";
}

/// probe_adapter_discipline -- WELCHES Orakel gilt fuer DIESE Permutation?
///
/// WARUM EINE PROBE UND KEINE DEKLARATION (der Kern des Bauplan-Entscheids 1.3, "gewaehlt per
/// inner_container-Belegung der Permutation"): die Disziplin ist laut adapter_tier.hpp:58-59 und
/// adapter_abi_adapter.hpp:24-25 API-NUTZUNG (front vs. back) -- IAdapterTier traegt KEIN Feld, das sie
/// deklariert, und AdapterObserverSnapshotV1 zaehlt front_reads/back_reads erst NACH getriebenen Ops.
/// Die einzige nicht-geratene Quelle ist deshalb das Verhalten selbst.
///
/// DIE DREI SONDEN-WERTE SIND MIT ABSICHT SO GEWAEHLT (10, 30, 20): zuerst abgelegt == 10, zuletzt
/// abgelegt == 20, groesstes == 30 -- drei PAARWEISE VERSCHIEDENE Antworten. Genau daran trennen sich die
/// drei Disziplinen an EINER einzigen Entnahme. Waeren die Werte monoton (10, 20, 30), fielen LIFO und
/// PRIORITY zusammen und die Probe wuerde raten. Alles ausserhalb der drei -> unknown, und das Gate
/// faellt (statt sich ein Orakel auszusuchen, das gerade passt). Der Tier wird davor UND danach geleert.
[[nodiscard]] inline AdapterDiscipline probe_adapter_discipline(anatomy::IAdapterTier& tier) noexcept {
    constexpr std::uint64_t first_in = 10u; // zuerst abgelegt  -> FIFO-Antwort
    constexpr std::uint64_t largest  = 30u; // groesstes        -> PRIORITY-Antwort
    constexpr std::uint64_t last_in  = 20u; // zuletzt abgelegt -> LIFO-Antwort
    tier.tier_clear();
    tier.tier_put(first_in);
    tier.tier_put(largest);
    tier.tier_put(last_in);
    std::uint64_t out = 0;
    bool const    got = tier.tier_get(&out);
    tier.tier_clear();
    if (!got) return AdapterDiscipline::unknown;
    if (out == first_in) return AdapterDiscipline::fifo;
    if (out == last_in) return AdapterDiscipline::lifo;
    if (out == largest) return AdapterDiscipline::priority;
    return AdapterDiscipline::unknown;
}

namespace genus_gate_detail {

/// Das Orakel-TRIO hinter EINER Aufruf-Flaeche: push/pop/size -- ueber std::queue (FIFO), std::stack
/// (LIFO) oder std::priority_queue (PRIORITY). Alle drei Bestand-Container werden real gehalten; die
/// Disziplin waehlt, WELCHER von ihnen getrieben wird und antwortet.
class AdapterOracle {
public:
    explicit AdapterOracle(AdapterDiscipline d) noexcept : discipline_{d} {}

    void push(std::uint64_t v) {
        switch (discipline_) {
            case AdapterDiscipline::lifo: lifo_.push(v); break;
            case AdapterDiscipline::priority: prio_.push(v); break;
            default: fifo_.push(v); break;
        }
    }

    /// Entnimmt gemaess Disziplin. false == leer (dann bleibt out unangetastet).
    [[nodiscard]] bool pop(std::uint64_t& out) {
        switch (discipline_) {
            case AdapterDiscipline::lifo:
                if (lifo_.empty()) return false;
                out = lifo_.top();
                lifo_.pop();
                return true;
            case AdapterDiscipline::priority:
                if (prio_.empty()) return false;
                out = prio_.top(); // Max-Heap-Top == Extract-Max (Default-Compare std::less)
                prio_.pop();
                return true;
            default:
                if (fifo_.empty()) return false;
                out = fifo_.front();
                fifo_.pop();
                return true;
        }
    }

    [[nodiscard]] std::uint64_t size() const noexcept {
        switch (discipline_) {
            case AdapterDiscipline::lifo: return static_cast<std::uint64_t>(lifo_.size());
            case AdapterDiscipline::priority: return static_cast<std::uint64_t>(prio_.size());
            default: return static_cast<std::uint64_t>(fifo_.size());
        }
    }

    void clear() {
        while (!lifo_.empty()) lifo_.pop();
        while (!fifo_.empty()) fifo_.pop();
        while (!prio_.empty()) prio_.pop();
    }

private:
    AdapterDiscipline                  discipline_ = AdapterDiscipline::fifo;
    std::queue<std::uint64_t>          fifo_{};
    std::stack<std::uint64_t>          lifo_{};
    std::priority_queue<std::uint64_t> prio_{};
};

} // namespace genus_gate_detail

/// run_adapter_conformance_gate -- Randfall- + Zufallssequenz gegen das per Probe gewaehlte Orakel
/// (std::queue ODER std::stack). `out_discipline` (optional) nimmt die erkannte Disziplin auf, damit der
/// Aufrufer sie protokollieren kann, ohne die Probe zu wiederholen.
[[nodiscard]] inline ConformanceResult
run_adapter_conformance_gate(anatomy::IAdapterTier& tier, std::uint64_t seed = 42, std::uint64_t n_random = 2000,
                             AdapterDiscipline* out_discipline = nullptr, std::FILE* report = nullptr) noexcept {
    genus_gate_detail::GateTally t{report};
    AdapterDiscipline            discipline = AdapterDiscipline::unknown;
    try {
        // ARF1 Disziplin-Probe: sie ist selbst eine Zusicherung -- eine Huelle, die weder FIFO noch LIFO
        // zeigt, hat KEIN Orakel und ist damit nicht-konform (das Gate endet hier, ohne ein Orakel zu raten).
        auto const b1 = t.block_begin();
        discipline    = probe_adapter_discipline(tier);
        t.check(discipline != AdapterDiscipline::unknown);
        t.block_end("ARF1 Disziplin-Probe(FIFO|LIFO|PRIORITY)", b1);
        if (out_discipline != nullptr) *out_discipline = discipline;
        if (discipline == AdapterDiscipline::unknown) {
            t.skip("ARF2-ARF6", "keine erkannte Entnahme-Disziplin -> kein Orakel anwendbar");
            tier.tier_clear();
            return t.result();
        }

        genus_gate_detail::AdapterOracle oracle{discipline};
        std::uint64_t                    probe = 0;
        tier.tier_clear();
        oracle.clear();

        // ARF2 leerer Adapter: get ist Miss, Belegung 0.
        auto const b2 = t.block_begin();
        t.check(tier.tier_get(&probe) == false);
        t.check(tier.tier_size() == oracle.size());
        t.block_end("ARF2 leer/get(miss)/size", b2);

        // ARF3 put/get in Disziplin-Reihenfolge: 5 hinein, 5 heraus -- jeder Wert an SEINER Position.
        auto const b3 = t.block_begin();
        for (std::uint64_t i = 0; i < 5; ++i) {
            tier.tier_put(i * 13u + 5u);
            oracle.push(i * 13u + 5u);
            t.check(tier.tier_size() == oracle.size());
        }
        for (std::uint64_t i = 0; i < 5; ++i) {
            std::uint64_t got = 0;
            std::uint64_t exp = 0;
            bool const    a   = tier.tier_get(&got);
            bool const    b   = oracle.pop(exp);
            t.check(a == b);
            if (a && b) t.check(got == exp);
            t.check(tier.tier_size() == oracle.size());
        }
        t.check(tier.tier_get(&probe) == false); // leergelaufen
        t.block_end("ARF3 put/get(Disziplin-treu)/size", b3);

        // ARF4 get mit NULL-out: darf nicht abstuerzen, konsumiert aber weiterhin ein Element.
        auto const b4 = t.block_begin();
        tier.tier_put(77u);
        oracle.push(77u);
        t.check(tier.tier_get(nullptr) == true);
        t.check(oracle.pop(probe) == true);
        t.check(tier.tier_size() == oracle.size());
        t.check(tier.tier_get(nullptr) == false);
        t.block_end("ARF4 get(out=nullptr)", b4);

        // ARF5 Zufalls-Mischlauf put/get: erzwingt Leerlauf-Faelle mitten in der Sequenz.
        auto const      b5 = t.block_begin();
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            if (rng() % 2 == 0) {
                std::uint64_t const v = rng() % 100'000u;
                tier.tier_put(v);
                oracle.push(v);
            } else {
                std::uint64_t got = 0;
                std::uint64_t exp = 0;
                bool const    a   = tier.tier_get(&got);
                bool const    b   = oracle.pop(exp);
                t.check(a == b);
                if (a && b) t.check(got == exp);
            }
            t.check(tier.tier_size() == oracle.size());
        }
        t.block_end("ARF5 random-mix(put/get/size)", b5);

        // ARF6 clear -> Belegung 0; get ist Miss.
        auto const b6 = t.block_begin();
        tier.tier_clear();
        oracle.clear();
        t.check(tier.tier_size() == oracle.size());
        t.check(tier.tier_get(&probe) == false);
        t.block_end("ARF6 clear/get(miss)", b6);
    } catch (...) { t.check(false); }
    return t.result();
}

// ================================================================================================
// VIEW-Gattung -- Orakel std::span<std::uint64_t const> (Bauplan 1.4)
// ================================================================================================

/// Die per Probe ermittelte INDEX-ABBILDUNG einer View: Index -> Zelle im gebundenen Fenster.
/// cell[i] == kViewCellMiss bedeutet: read(i) war ein Miss.
inline constexpr std::size_t kViewCellMiss = static_cast<std::size_t>(-1);

/// ZWEITE ABWEICHUNG VOM BAUPLAN, AM BAU ERZWUNGEN (Bauplan 1.4 unterstellt eine IDENTISCHE Sicht auf den
/// Puffer -- das Ist hat eine Layout-ACHSE):
/// ViewAnatomy::read (view_anatomy.hpp:89-98) legt VOR dem Zugriff die axis_layout-Policy auf den Index:
///   off = axis_layout_policy_.index_of(index);  if (data_ == nullptr || off >= size_) -> Miss
/// Die Registry traegt drei reale Layouts (view_policies.hpp): LayoutRight/LayoutLeft mit index_of(i)==i,
/// aber LayoutStrided<Stride> mit index_of(i)==i*Stride (view_policies.hpp:37) -- "liest jede Stride-te
/// Zelle ... ein read(i) trifft physisch eine andere Speicherzelle" (view_policies.hpp:32-34).
/// Ein Orakel, das read(i) == span[i] fordert, wuerde also JEDE gestridete Permutation fuer defekt
/// erklaeren und von der Messung ausschliessen -- obwohl die Layout-Achse genau das tun SOLL. Literal
/// erhoben (erster Bau dieses Gates, 04.08.): 4 von 6 View-Permutationen bestanden, die 2 mit
/// LayoutStrided<2> fielen bei "first_fail 7" -- das ist read(1), wo der Stride zum ersten Mal wirkt.
///
/// DIE AUFLOESUNG (dasselbe Muster wie bei der Adapter-Disziplin: PROBE, dann STRIKT daran festhalten):
/// std::span bleibt das Orakel, aber als FENSTER, nicht als Identitaet. Das Gate ermittelt die Abbildung
/// aus dem Verhalten (der Puffer traegt paarweise verschiedene, invertierbare Werte) und haelt die View
/// danach an den Fenster-Eigenschaften fest, die JEDES Layout erfuellen muss:
///   (a) ENTHALTENSEIN  -- jeder gelieferte Wert liegt IM span (eine non-owning View erfindet nichts und
///                         liest nie ausserhalb ihres Fensters),
///   (b) INJEKTIVITAET  -- verschiedene Indizes treffen verschiedene Zellen (eine View bildet ab, sie
///                         verteilt nicht mehrfach auf dieselbe Zelle),
///   (c) FENSTER-FORM   -- die Menge der lesbaren Indizes ist ein PRAEFIX von [0, size): ein Fenster mit
///                         Loechern waere kein Fenster,
///   (d) REINHEIT       -- derselbe Index liefert wiederholt dieselbe Antwort (die View haelt keinen
///                         Zustand ausser der Bindung),
///   (e) EXTENT-TREUE   -- ein re-bind auf ein kuerzeres Fenster schneidet die lesbaren Indizes GENAU an
///                         der neuen Grenze ab, mit UNVERAENDERTER Abbildung.
/// Bei Identitaets-Layouts faellt daraus read(i) == span[i] von selbst heraus; gestridete Layouts werden
/// gemessen statt verworfen. Wer die Layout-Achse spaeter erweitert, muss (a)-(e) erfuellen -- das ist
/// der Vertrag, nicht eine Liste heutiger Policies.

/// probe_view_index_map -- ermittelt die Abbildung Index -> Zelle aus dem Verhalten. `window` muss
/// PAARWEISE VERSCHIEDENE Werte tragen (sonst ist die Zelle aus dem Wert nicht rekonstruierbar);
/// die Rekonstruktion laeuft ueber die uebergebene Invertierung.
/// Rueckgabe: cell[i] je Index in [0, window.size()), kViewCellMiss bei Miss oder nicht-invertierbarem Wert.
[[nodiscard]] inline std::vector<std::size_t> probe_view_index_map(anatomy::IViewTier&            tier,
                                                                   std::span<std::uint64_t const> window,
                                                                   std::uint64_t base, std::uint64_t step) {
    std::vector<std::size_t> cell(window.size(), kViewCellMiss);
    for (std::size_t i = 0; i < window.size(); ++i) {
        std::uint64_t got = 0;
        if (!tier.tier_read(static_cast<std::uint64_t>(i), &got)) continue;
        if (got < base) continue;
        std::uint64_t const delta = got - base;
        if (step == 0 || (delta % step) != 0) continue;
        std::size_t const c = static_cast<std::size_t>(delta / step);
        if (c < window.size()) cell[i] = c;
    }
    return cell;
}

/// run_view_conformance_gate -- Randfall- + Zufallssequenz gegen ein std::span-Orakel (non-owning),
/// layout-agnostisch nach (a)-(e) oben.
///
/// Der Puffer lebt WAEHREND des gesamten Gates hier im Rahmen (die View referenziert ihn nur) -- exakt die
/// Lebensdauer-Disziplin, die view_dock.hpp:37-39 fuer den Mess-Pfad beschreibt. Nach dem Gate wird die
/// View auf (nullptr, 0) zurueckgebunden, damit sie KEINEN Zeiger auf den ablaufenden Puffer behaelt.
[[nodiscard]] inline ConformanceResult run_view_conformance_gate(anatomy::IViewTier& tier, std::uint64_t seed = 42,
                                                                 std::uint64_t n_random = 2000,
                                                                 std::uint64_t n_bind   = 256,
                                                                 std::FILE*    report   = nullptr) noexcept {
    genus_gate_detail::GateTally t{report};
    try {
        constexpr std::uint64_t kBase = 3u;  // Puffer-Werte: base + i*step -- paarweise verschieden und
        constexpr std::uint64_t kStep = 11u; // aus dem Wert eindeutig auf die Zelle zurueckrechenbar.
        std::uint64_t           probe = 0;

        // VRF1 UNGEBUNDENE View: Fensterlaenge 0, jeder read ist Miss.
        auto const b1 = t.block_begin();
        tier.tier_bind(nullptr, 0);
        std::span<std::uint64_t const> window{};
        t.check(tier.tier_size() == window.size());
        t.check(tier.tier_read(0, &probe) == false);
        t.block_end("VRF1 ungebunden/read(miss)/size", b1);

        std::vector<std::uint64_t> buffer(static_cast<std::size_t>(n_bind));
        for (std::uint64_t i = 0; i < n_bind; ++i) buffer[static_cast<std::size_t>(i)] = kBase + i * kStep;

        // VRF2 bind: die Fensterlaenge folgt der Bindung (das ist die span-Aussage ueber die Groesse).
        auto const b2 = t.block_begin();
        tier.tier_bind(buffer.data(), buffer.size());
        window = std::span<std::uint64_t const>{buffer};
        t.check(tier.tier_size() == window.size());
        t.block_end("VRF2 bind/size == span.size()", b2);

        // VRF3 (a) ENTHALTENSEIN + Abbildungs-Probe: jeder gelieferte Wert liegt IM span, an einer Zelle,
        // die sich aus ihm eindeutig rekonstruieren laesst.
        auto const  b3   = t.block_begin();
        auto const  map  = probe_view_index_map(tier, window, kBase, kStep);
        std::size_t hits = 0;
        for (std::size_t i = 0; i < window.size(); ++i) {
            std::uint64_t got = 0;
            if (!tier.tier_read(static_cast<std::uint64_t>(i), &got)) {
                t.check(map[i] == kViewCellMiss); // Miss bleibt Miss
                continue;
            }
            ++hits;
            t.check(map[i] != kViewCellMiss); // der Wert war invertierbar UND im Fenster
            if (map[i] == kViewCellMiss) continue;
            t.check(got == window[map[i]]); // und er ist WIRKLICH der Fenster-Inhalt dieser Zelle
        }
        t.check(hits > 0); // eine View, die aus einem nicht-leeren Fenster nichts liefert, ist keine View
        t.block_end("VRF3 (a) Enthaltensein im span + Abbildungs-Probe", b3);

        // VRF4 (b) INJEKTIVITAET: keine zwei Indizes auf dieselbe Zelle.
        auto const            b4 = t.block_begin();
        std::set<std::size_t> seen_cells;
        bool                  injective = true;
        for (std::size_t i = 0; i < map.size(); ++i) {
            if (map[i] == kViewCellMiss) continue;
            if (!seen_cells.insert(map[i]).second) injective = false;
        }
        t.check(injective);
        t.check(seen_cells.size() == hits);
        t.block_end("VRF4 (b) Injektivitaet der Abbildung", b4);

        // VRF5 (c) FENSTER-FORM: die lesbaren Indizes sind ein PRAEFIX von [0, size) -- ein Fenster mit
        // Loechern waere kein Fenster.
        auto const b5     = t.block_begin();
        bool       prefix = true;
        bool       gap    = false;
        for (std::size_t i = 0; i < map.size(); ++i) {
            if (map[i] == kViewCellMiss) {
                gap = true;
            } else if (gap) {
                prefix = false; // Treffer NACH einem Miss = Loch
            }
        }
        t.check(prefix);
        t.block_end("VRF5 (c) lesbare Indizes bilden ein Praefix", b5);

        // VRF6 (d) REINHEIT: derselbe Index liefert wiederholt exakt dieselbe Antwort.
        auto const b6 = t.block_begin();
        for (std::size_t i = 0; i < map.size(); ++i) {
            std::uint64_t a  = 0;
            std::uint64_t b  = 0;
            bool const    ha = tier.tier_read(static_cast<std::uint64_t>(i), &a);
            bool const    hb = tier.tier_read(static_cast<std::uint64_t>(i), &b);
            t.check(ha == hb);
            if (ha && hb) t.check(a == b);
        }
        t.block_end("VRF6 (d) Reinheit (wiederholtes read identisch)", b6);

        // VRF7 read AM UND HINTER DEM FENSTER-ENDE: index >= Fensterlaenge ist Miss (der Extent-Vertrag,
        // den view_dock.hpp:37-39 fuer den Mess-Pfad bereits so beschreibt).
        auto const b7 = t.block_begin();
        t.check(tier.tier_read(window.size(), &probe) == false);
        t.check(tier.tier_read(window.size() + 1u, &probe) == false);
        t.check(tier.tier_read(std::numeric_limits<std::uint64_t>::max(), &probe) == false);
        t.block_end("VRF7 read(hinter dem Fenster)", b7);

        // VRF8 read mit NULL-out: kein Absturz, gleiche Hit/Miss-Antwort wie mit out.
        auto const    b8       = t.block_begin();
        std::uint64_t with_out = 0;
        t.check(tier.tier_read(0, nullptr) == tier.tier_read(0, &with_out));
        t.check(tier.tier_read(window.size(), nullptr) == false);
        t.block_end("VRF8 read(out=nullptr)", b8);

        // VRF9 Zufallssequenz ueber [0, 2*size): jede Antwort muss GENAU der geprobten Abbildung folgen.
        auto const      b9 = t.block_begin();
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            std::uint64_t const idx     = window.empty() ? 0u : (rng() % (window.size() * 2u));
            std::uint64_t       got     = 0;
            bool const          hit     = tier.tier_read(idx, &got);
            bool const          exp_hit = (idx < map.size()) && (map[static_cast<std::size_t>(idx)] != kViewCellMiss);
            t.check(hit == exp_hit);
            if (exp_hit) t.check(got == window[map[static_cast<std::size_t>(idx)]]);
        }
        t.block_end("VRF9 random-read gegen die geprobte Abbildung", b9);

        // VRF10 (e) EXTENT-TREUE: re-bind auf die HALBE Fensterlaenge. Die Laenge folgt sofort, und die
        // lesbaren Indizes sind GENAU die, deren Zelle noch im kuerzeren Fenster liegt -- bei
        // unveraenderter Abbildung. Genau hier trennt sich eine ehrliche View (extent_policy) von einer,
        // die nur den Zeiger merkt: der Speicher hinter der neuen Grenze ist noch gueltig, gehoert aber
        // nicht mehr zum Fenster.
        auto const b10  = t.block_begin();
        auto const half = buffer.size() / 2u;
        tier.tier_bind(buffer.data(), half);
        window = std::span<std::uint64_t const>{buffer.data(), half};
        t.check(tier.tier_size() == window.size());
        for (std::size_t i = 0; i < map.size(); ++i) {
            std::uint64_t got     = 0;
            bool const    hit     = tier.tier_read(static_cast<std::uint64_t>(i), &got);
            bool const    exp_hit = (map[i] != kViewCellMiss) && (map[i] < half);
            t.check(hit == exp_hit);
            if (exp_hit) t.check(got == window[map[i]]);
        }
        t.block_end("VRF10 (e) Extent-Treue nach re-bind(kuerzer)", b10);

        // VRF11 zurueck auf ungebunden: Fensterlaenge 0, jeder read Miss -- und KEIN Zeiger auf den
        // Puffer, der gleich ablaeuft.
        auto const b11 = t.block_begin();
        tier.tier_bind(nullptr, 0);
        window = std::span<std::uint64_t const>{};
        t.check(tier.tier_size() == window.size());
        t.check(tier.tier_read(0, &probe) == false);
        t.block_end("VRF11 unbind/read(miss)", b11);
    } catch (...) { t.check(false); }
    return t.result();
}

} // namespace comdare::cache_engine::builder::pruef_dock
