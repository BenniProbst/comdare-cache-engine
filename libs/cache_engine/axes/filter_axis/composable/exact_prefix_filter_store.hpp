#pragma once
// V41 Umstufung-A s4 (Task #43) — ExactPrefixFilterStore: exakte Filter-Correctness-Base (S1).
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Query-Logik — die EXAKTE Filter-Correctness-Base (S1, is_original=false): sortierte
// uint64-Keys (aus build_from_sorted_keys). Strukturell FP-Rate 0 (kein Suffix-Truncation), no-false-negative
// trivial — der GROUND-TRUTH-ANKER, gegen den S2 (echte succinct LOUDS-FST mit FP>0) per Kreuzbeleg
// `S2.contains(k) >= S1.contains(k)` verifiziert wird. Die contains/range-Logik lebt im ExactPrefixFilterOrgan.
//
// A8-S5 Familie 02b (2026-08-04) -- SCHNITT-REGEL (Dossier 20260803-a8_f2 Abschn. 3.4: "Speicher NUR ueber
// das Allokator-Achsen-Interface"): der Key-Puffer lief bis hierher ueber den Default-Allokator. Seit dem
// Schnitt haengt er an der EINEN Strategie-Instanz dieses Organs (surf_axis_allocator.hpp). Die
// GROUND-TRUTH-Eigenschaft ist davon unberuehrt: es wechselt allein die Speicher-QUELLE des sortierten
// Key-Vektors, nicht seine Ordnung, nicht seine Deduplizierung und nicht lower_bound.

#include "surf_axis_allocator.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::filter_axis::composable {

class ExactPrefixFilterStore {
public:
    using key_type = std::uint64_t;
    /// A8-S5-02b Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse -- nicht
    /// deklarativ, sondern real (keys_ traegt den StdAllocatorAdapter dieses allocator_, s. Member unten
    /// + filter_surf_allocator_statistics()).
    using allocator_type = filter_surf_allocator_t;

    // -- A8-S5-02b Lebensdauer-Vertrag der Achsen-Verdrahtung (Muster btree_node_pool_store.hpp:19/:84-105) --
    //   (a) allocator_ MUSS vor keys_ deklariert sein (Member-Reihenfolge unten),
    //   (b) keys_ wird IMMER mit allocator_.as_std_allocator<key_type>() konstruiert (der Adapter ist nicht
    //       default-konstruierbar -> es gibt kein stilles Zurueckfallen auf einen Default-Allokator),
    //   (c) die Kopie REBINDET auf das EIGENE allocator_ -- ein mitgeschleppter Fremd-Adapter waere ein
    //       dangling Zeiger, sobald eines der beiden Organe stirbt,
    //   (d) Move wird BEWUSST nicht deklariert: die benutzerdeklarierte Kopie unterdrueckt den impliziten
    //       Move, ein std::move degradiert zur (korrekt rebindenden) Kopie statt den Fremd-Adapter zu stehlen.
    // Die transiente Kopier-Allokation der Vollkopie ist kein Mess-Ereignis der Achse -> restore_statistics
    // setzt die Statistik auf den Quell-Stand zurueck (Memento-Symmetrie, btree_node_pool_store.hpp:91).
    ExactPrefixFilterStore() : keys_(allocator_.template as_std_allocator<key_type>()) {}
    ExactPrefixFilterStore(ExactPrefixFilterStore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    ExactPrefixFilterStore& operator=(ExactPrefixFilterStore const& o) {
        if (this != &o) {
            keys_ = o.keys_; // POCCA=false -> dieses Objekt behaelt seinen eigenen Adapter
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~ExactPrefixFilterStore() = default;

    /// Bulk-Aufbau aus AUFSTEIGEND sortierten Keys (SuRF baut single-scan aus sortierten Keys).
    /// Nicht-sortierte Eingabe wird defensiv sortiert.
    ///
    /// SONDERFALL [[allocation-failure-exception]] -- KAUSALITAET PRAEZISIERT (Posten 64, A8-S5-02b,
    /// Auflage 11): der Wurf kommt seit dem Schnitt NICHT mehr vom Default-Allokator, sondern vom
    /// StdAllocatorAdapter der Allokator-ACHSE. Die Achsen-Strategie meldet OOM per nullptr (Achsen-Semantik);
    /// der Adapter uebersetzt das an EINER Stelle in std::bad_alloc (axis_06_allocator_strategy_base.hpp,
    /// StdAllocatorAdapter::allocate). Die Aussage "darf werfen" bleibt damit WAHR -- nur der Traeger
    /// wechselt. failure_count der Strategie ist zum Wurf-Zeitpunkt BEREITS gezaehlt (die Strategie zaehlt
    /// vor dem `return nullptr`), der Wurf verdeckt also keinen Messwert. Fehlerklasse unveraendert der
    /// FK-5-Boden der Allokator-Achse (kOrganAxisErrorFloor).
    void build_from_sorted_keys(std::span<key_type const> sorted) {
        keys_.assign(sorted.begin(), sorted.end());
        if (!std::is_sorted(keys_.begin(), keys_.end())) std::sort(keys_.begin(), keys_.end());
        keys_.erase(std::unique(keys_.begin(), keys_.end()), keys_.end());
    }

    [[nodiscard]] std::size_t key_count() const noexcept { return keys_.size(); }
    [[nodiscard]] std::size_t bit_size() const noexcept { return keys_.size() * 64u; } // exakt: voller 64-Bit-Key
    [[nodiscard]] double      bits_per_key() const noexcept { return keys_.empty() ? 0.0 : 64.0; }

    /// Index des ersten Keys >= k (== size falls keiner). Fundament fuer contains/range.
    [[nodiscard]] std::size_t lower_bound(key_type k) const noexcept {
        return static_cast<std::size_t>(std::lower_bound(keys_.begin(), keys_.end(), k) - keys_.begin());
    }
    [[nodiscard]] key_type key_at(std::size_t i) const noexcept { return keys_[i]; }

    void clear() noexcept { keys_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-02b VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Organs. Damit ist die Form-B-Aussage der S5-Gate-Wache am
    /// Objekt pruefbar -- ein bloss deklarierter allocator_type ohne reale Verdrahtung bliebe hier auf 0
    /// stehen (Form-B-Grenze, tests/unit/s5_family_alloc_conformance.hpp:31). NICHT im T16-Mess-Pfad: die
    /// composition-getragene filter-Achse sind Bloom/Cuckoo/RangeSurf/Xor (heap-frei), NICHT dieses
    /// composable-Organ -- es entsteht also keine Doppelzaehlung (dieselbe bewusste Namens-Trennung wie
    /// 01d traversal_allocator_statistics / 02a trie_allocator_statistics).
    [[nodiscard]] typename allocator_type::snapshot_t filter_surf_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // A8-S5-02b: allocator_ VOR keys_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar).
    allocator_type       allocator_{};
    surf_vec_t<key_type> keys_; // aufsteigend sortiert, duplikatfrei -- ueber die Allokator-Achse
};

} // namespace comdare::cache_engine::filter_axis::composable
