#pragma once
// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — LoudsSparseFilterStore: succinct LOUDS-Sparse-Substrat + Single-Scan-Builder.
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier) @paper P10 SuRF (Zhang et al. SIGMOD 2018)
//
// **Original-getreue Portierung** von surf_builder.hpp (buildSparse/insertKeyByte/insertKeyBytesToTrieUntilUnique)
// + louds_sparse.hpp (LoudsSparse-Ctor + lookupKey-Accessoren) aus ext/traversal/P10-SuRF (Apache-2.0).
// is_original=false ([[pseudocode-papers-fallback]]). SPARSE-ONLY (sparse_start_level=0, KEIN LoudsDense — reine
// Space-Opt, Inkrement 2; veraendert Membership/no-FN NICHT). node_count_dense_=child_count_dense_=0 fest verdrahtet.
//
// Keys = uint64 -> 8-Byte-BIG-ENDIAN (std::byteswap), damit numerische == lexikografische Trie-Ordnung (Range-Basis).
//
// A8-S5 Familie 02b (2026-08-04) -- SCHNITT-REGEL (Dossier 20260803-a8_f2 Abschn. 3.4: "Speicher NUR ueber das
// Allokator-Achsen-Interface"): DIESES Organ ist der dichteste Fremdgang der Familie gewesen -- 13 Treffer im
// Familien-grep, darunter der VERSCHACHTELTE Build-Container (labels_lv_/child_ind_lv_/louds_lv_/
// suffix_words_lv_), dessen BEIDE Ebenen am Default-Allokator lagen. Seit dem Schnitt haengt ALLES an der
// EINEN Strategie-Instanz dieses Organs: die aeusseren Level-Vektoren, jeder innere Level-Vektor, die
// flachen Puffer, die Build-Streu-Puffer UND die Puffer der eingebetteten Teil-Objekte (SurfRank/SurfSelect/
// SurfSuffixBits, denen allocator_ im Konstruktor hereingereicht wird). Owner-KERN 04.08. abend-11: die
// T6-Sicht umfasst ALLE Organ-Allokationen, die Strategien liegen HINTER dem einen Achsen-Interface.
//
// PAPERTREUE (die harte Zusage dieser Datei): der Schnitt wechselt ausschliesslich die Speicher-QUELLE.
// Kein Byte des LOUDS-Layouts, keine Wort-Arithmetik, keine Trie-Ordnung, kein Suffix-Bit und keine
// Vergleichs-Semantik aendert sich; Membership- und Range-Antworten sind bit-identisch (Vorher/Nachher-
// Kontrast auf identischen Keys, sechs Suffix-Konfigurationen). Die Zeilen-Anker in die Original-Header
// (surf_builder.hpp Z.185-197/Z.208-244, louds_sparse.hpp Z.161-162/Z.229-249, label_vector.hpp
// Z.107-150/Z.122-133/Z.193-216, config.hpp Z.24) sitzen unveraendert an den Rumpf-Zeilen, die sie
// beschreiben -- wo der Schnitt eine Zeile verschoben hat, ist der Anker MITGEWANDERT.

#include "surf_axis_allocator.hpp"
#include "surf_louds_bitvector.hpp"
#include "surf_suffix_bits.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::filter_axis::composable {

inline constexpr std::uint8_t  kSurfTerminator      = 255; // config.hpp Z.24
inline constexpr std::uint32_t kSurfRankBlockSize   = 512; // louds_sparse.hpp Z.161
inline constexpr std::uint32_t kSurfSelectSampleIvl = 64;  // louds_sparse.hpp Z.162

template <SurfSuffixType ST, unsigned HashLen, unsigned RealLen>
class LoudsSparseFilterStore {
public:
    using key_type    = std::uint64_t;
    using key_bytes_t = std::array<std::uint8_t, 8>;
    /// A8-S5-02b Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse -- nicht
    /// deklarativ, sondern real (JEDER Puffer traegt den StdAllocatorAdapter dieses allocator_, s. Member
    /// unten + filter_surf_allocator_statistics()).
    using allocator_type = filter_surf_allocator_t;

    /// A8-S5-02b -- die BEIDEN Ebenen des verschachtelten Build-Containers, oeffentlich fuer die
    /// Familien-Konformitaets-Wache (Muster 02a LayoutAwareChunkedStore::chunk_index_allocator_type):
    /// die Wache prueft compile-hart, dass BEIDE `::allocator_type` der Achsen-Adapter sind. Ein Scrub, der
    /// nur die aeussere Ebene hebt (der klassische Fehler -- uses-allocator-Konstruktion greift ohne
    /// scoped_allocator_adaptor NICHT), faellt genau hier durch.
    using level_element_vector_type      = surf_vec_t<std::uint8_t>;              // INNERE Ebene: ein Level
    using level_index_vector_type        = surf_vec_t<level_element_vector_type>; // AEUSSERE Ebene: der Index
    using word_level_element_vector_type = surf_word_vec_t;                       // INNERE Ebene der Wort-Level
    using word_level_index_vector_type   = surf_word_vec_per_level_t;             // AEUSSERE Ebene der Wort-Level

private:
    /// Der innere Label-Vektor EINER Trie-Ebene -- die zweite Ebene des verschachtelten Build-Containers.
    using label_level_vec_t = level_element_vector_type;

public:
    // -- A8-S5-02b Lebensdauer-Vertrag der Achsen-Verdrahtung (Muster btree_node_pool_store.hpp:19/:84-105,
    //    Verschachtelung nach axis_04_node_type_chunked_store.hpp) ---------------------------------------
    // Der StdAllocatorAdapter haelt einen Zeiger auf allocator_ (Wert-Adapter). Daraus folgt:
    //   (a) allocator_ MUSS vor allen Puffern deklariert sein (Member-Reihenfolge unten),
    //   (b) JEDER Puffer -- die aeusseren Level-Vektoren, JEDER innere Level-Vektor, die flachen Puffer und
    //       die Puffer der eingebetteten Teil-Objekte -- wird mit dem Adapter DIESES allocator_ konstruiert;
    //       der Adapter traegt keinen Default-Ctor, ein stilles Zurueckfallen auf einen Default-Allokator
    //       ist damit compile-hart ausgeschlossen (das frueher moegliche `labels_lv_.emplace_back()` waere
    //       heute nicht mehr wohlgeformt),
    //   (c) die Kopie REBINDET auf das EIGENE allocator_ -- und zwar auf BEIDEN Ebenen EINZELN: die
    //       allokator-erweiterte Kopie des AEUSSEREN Vektors rebindet nur DEN; die inneren Vektoren wuerden
    //       ueber allocator_traits::construct ihren Quell-Allokator mitbringen (uses-allocator-Konstruktion
    //       greift ohne scoped_allocator_adaptor NICHT). Deshalb wird jeder innere Level-Vektor EINZELN
    //       allokator-erweitert kopiert -- sonst zeigte der Adapter der Kopie auf die QUELLE.
    //   (d) Move wird BEWUSST nicht deklariert: die benutzerdeklarierte Kopie unterdrueckt den impliziten
    //       Move, ein std::move degradiert damit zur (korrekt rebindenden) Kopie statt den Fremd-Adapter zu
    //       stehlen.
    // Die transiente Kopier-Allokation der Vollkopie ist kein Mess-Ereignis der Achse -> restore_statistics
    // setzt die Statistik auf den Quell-Stand zurueck (Memento-Symmetrie, btree_node_pool_store.hpp:91).
    LoudsSparseFilterStore()
        : labels_lv_(allocator_.template as_std_allocator<label_level_vec_t>()),
          child_ind_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          louds_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          suffix_words_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          node_counts_(allocator_.template as_std_allocator<std::uint32_t>()),
          is_last_term_(allocator_.template as_std_allocator<std::uint8_t>()),
          labels_flat_(allocator_.template as_std_allocator<std::uint8_t>()), child_indicator_(allocator_),
          louds_bits_(allocator_), suffix_(allocator_) {}

    LoudsSparseFilterStore(LoudsSparseFilterStore const& o)
        : allocator_(o.allocator_), labels_lv_(allocator_.template as_std_allocator<label_level_vec_t>()),
          child_ind_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          louds_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          suffix_words_lv_(allocator_.template as_std_allocator<surf_word_vec_t>()),
          node_counts_(o.node_counts_, allocator_.template as_std_allocator<std::uint32_t>()),
          is_last_term_(o.is_last_term_, allocator_.template as_std_allocator<std::uint8_t>()),
          labels_flat_(o.labels_flat_, allocator_.template as_std_allocator<std::uint8_t>()),
          child_indicator_(o.child_indicator_, allocator_), louds_bits_(o.louds_bits_, allocator_),
          suffix_(o.suffix_, allocator_), key_count_(o.key_count_) {
        copy_levels_from(o);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    LoudsSparseFilterStore& operator=(LoudsSparseFilterStore const& o) {
        if (this != &o) {
            // Die VERSCHACHTELTEN Ebenen einzeln (s. (c) oben); die flachen Puffer und die Teil-Objekte
            // duerfen gewoehnlich zuweisen: POCCA ist am Adapter false, das Ziel behaelt seinen eigenen.
            labels_lv_.clear();
            child_ind_lv_.clear();
            louds_lv_.clear();
            suffix_words_lv_.clear();
            copy_levels_from(o);
            node_counts_     = o.node_counts_;
            is_last_term_    = o.is_last_term_;
            labels_flat_     = o.labels_flat_;
            child_indicator_ = o.child_indicator_;
            louds_bits_      = o.louds_bits_;
            suffix_          = o.suffix_;
            key_count_       = o.key_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~LoudsSparseFilterStore() = default;

    [[nodiscard]] static key_bytes_t to_bytes(std::uint64_t k) noexcept {
        std::uint64_t const be = std::byteswap(k); // config.hpp uint64ToString
        key_bytes_t         b{};
        for (unsigned i = 0; i < 8; ++i) b[i] = static_cast<std::uint8_t>((be >> (i * 8)) & 0xFFU);
        return b;
    }

    /// Single-Scan-Build aus SORTIERTEN uint64-Keys (defensiv kopiert+sortiert+dedupliziert wie S1).
    ///
    /// SONDERFALL [[allocation-failure-exception]] -- KAUSALITAET PRAEZISIERT (Posten 64, A8-S5-02b,
    /// Auflage 11): der Build-Pfad kann werfen, und der Wurf kommt seit dem Schnitt NICHT mehr vom
    /// Default-Allokator, sondern vom StdAllocatorAdapter der Allokator-ACHSE. Die Achsen-Strategie meldet
    /// OOM per nullptr; der Adapter uebersetzt das an EINER Stelle in std::bad_alloc
    /// (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate) -- der Wurf bleibt, nur der
    /// Traeger wechselt. failure_count der Strategie ist zum Wurf-Zeitpunkt BEREITS gezaehlt. Fehlerklasse
    /// unveraendert der FK-5-Boden der Allokator-Achse (kOrganAxisErrorFloor).
    ///
    /// A8-S5-02b SCOPE (bewusst, nicht vergessen): auch die beiden TRANSIENTEN Build-Puffer ks/kb laufen
    /// ueber die Achse. Das ist die Abweichung zur 02a-Scheibe, die ihre transienten Umbau-/Beobachtungs-
    /// Puffer BEGRUENDET am Default-Allokator liess -- dort waere jeder observe-Aufruf ein zusaetzliches
    /// Zaehler-Ereignis im T6-Pfad gewesen (Doppelzaehlungs-Charakter). Hier gilt das Gegenteil: dieses
    /// Organ liegt NICHT im Mess-Pfad der filter-Achse (das sind Bloom/Cuckoo/RangeSurf/Xor, heap-frei),
    /// die Puffer gehoeren zum Bulk-Load desselben Organs, ihre Strategie-Instanz liegt ohnehin zur Hand --
    /// und nur so wird der Familien-grep ehrlich 0 statt "0 bis auf zwei".
    void build_from_sorted_keys(std::span<std::uint64_t const> keys) {
        clear();
        surf_vec_t<std::uint64_t> ks(keys.begin(), keys.end(), allocator_.template as_std_allocator<std::uint64_t>());
        std::sort(ks.begin(), ks.end());
        ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
        key_count_ = ks.size();
        if (key_count_ == 0) return;

        surf_vec_t<key_bytes_t> kb(ks.size(), allocator_.template as_std_allocator<key_bytes_t>());
        for (std::size_t i = 0; i < ks.size(); ++i) kb[i] = to_bytes(ks[i]);

        build_sparse(kb);
        finalize();
    }

    // ── Query-Accessoren (sparse-only: node_count_dense_=child_count_dense_=0) ──
    [[nodiscard]] std::size_t key_count() const noexcept { return key_count_; }
    [[nodiscard]] bool        empty() const noexcept { return key_count_ == 0; }

    [[nodiscard]] std::uint8_t  label_at(std::uint32_t pos) const noexcept { return labels_flat_[pos]; }
    [[nodiscard]] bool          child_bit(std::uint32_t pos) const noexcept { return child_indicator_.read_bit(pos); }
    [[nodiscard]] std::uint32_t child_node_num(std::uint32_t pos) const noexcept {
        return child_indicator_.rank(pos);
    } // +child_count_dense_(0)
    [[nodiscard]] std::uint32_t first_label_pos(std::uint32_t node) const noexcept {
        return louds_bits_.select(node + 1);
    }
    [[nodiscard]] std::uint32_t suffix_pos(std::uint32_t pos) const noexcept {
        return pos - child_indicator_.rank(pos);
    }
    [[nodiscard]] std::uint32_t node_size(std::uint32_t pos) const noexcept {
        return louds_bits_.distance_to_next_set_bit(pos);
    }
    [[nodiscard]] std::uint32_t num_label_bits() const noexcept { return louds_bits_.num_bits(); }

    [[nodiscard]] bool suffix_check_equality(std::uint32_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        return suffix_.check_equality(idx, kb, level);
    }
    [[nodiscard]] int suffix_compare(std::uint32_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        return suffix_.compare(idx, kb, level);
    }

    // labels_->search (label_vector.hpp Z.107-150): Terminator-Skip + linear(<3)/binary(>=3, OHNE SSE).
    [[nodiscard]] bool label_search(std::uint8_t target, std::uint32_t& pos, std::uint32_t search_len) const noexcept {
        if (search_len > 1 && labels_flat_[pos] == kSurfTerminator) {
            ++pos;
            --search_len;
        }
        if (search_len < 3) {
            for (std::uint32_t i = 0; i < search_len; ++i)
                if (labels_flat_[pos + i] == target) {
                    pos += i;
                    return true;
                }
            return false;
        }
        std::uint32_t l = pos, r = pos + search_len;
        while (l < r) {
            std::uint32_t const m = (l + r) >> 1;
            if (target < labels_flat_[m])
                r = m;
            else if (target == labels_flat_[m]) {
                pos = m;
                return true;
            } else
                l = m + 1;
        }
        return false;
    }

    // labels_->searchGreaterThan (label_vector.hpp Z.122-133/193-216) — fuer Range moveToLeftInNextSubtrie.
    [[nodiscard]] bool label_search_greater_than(std::uint8_t target, std::uint32_t& pos,
                                                 std::uint32_t search_len) const noexcept {
        if (search_len > 1 && labels_flat_[pos] == kSurfTerminator) {
            ++pos;
            --search_len;
        }
        std::uint32_t const base = pos;
        if (search_len < 3) {
            for (std::uint32_t i = 0; i < search_len; ++i)
                if (labels_flat_[pos + i] > target) {
                    pos += i;
                    return true;
                }
            return false;
        }
        std::uint32_t l = base, r = base + search_len;
        while (l < r) {
            std::uint32_t const m = (l + r) >> 1;
            if (target < labels_flat_[m])
                r = m;
            else if (target == labels_flat_[m]) {
                if (m < base + search_len - 1) {
                    pos = m + 1;
                    return true;
                }
                return false;
            } else
                l = m + 1;
        }
        if (l < base + search_len) {
            pos = l;
            return true;
        }
        return false;
    }

    // ── bit_size / Tuning-Observer ──
    [[nodiscard]] std::size_t bit_size() const noexcept {
        return labels_flat_.size() * 8 // Labels (1 Byte/Item)
               + child_indicator_.bit_size_bits() + child_indicator_.lut_bits() + louds_bits_.bit_size_bits() +
               louds_bits_.lut_bits() + suffix_.bit_size();
    }
    [[nodiscard]] double bits_per_key() const noexcept {
        return key_count_ ? static_cast<double>(bit_size()) / static_cast<double>(key_count_) : 0.0;
    }

    void clear() noexcept {
        labels_lv_.clear();
        child_ind_lv_.clear();
        louds_lv_.clear();
        suffix_words_lv_.clear();
        node_counts_.clear();
        is_last_term_.clear();
        labels_flat_.clear();
        // A8-S5-02b: frueher `child_indicator_ = SurfRank{}` -- ein default-konstruiertes Temporaer-Objekt
        // gibt es nicht mehr (es haette keine Strategie-Instanz). Das eigene clear() setzt exakt dieselben
        // Felder zurueck (Bits, LUT, Block-/Sample-Parameter) und BEHAELT den Achsen-Adapter.
        child_indicator_.clear();
        louds_bits_.clear();
        suffix_.clear();
        key_count_ = 0;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-02b VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Organs -- sie deckt ALLE Puffer ab, auch die der
    /// eingebetteten Teil-Objekte (Owner-KERN 04.08. abend-11: EIN Achsen-Interface je Organ). Damit ist
    /// die Form-B-Aussage der S5-Gate-Wache am Objekt pruefbar; ein bloss deklarierter allocator_type ohne
    /// reale Verdrahtung bliebe hier auf 0 stehen (Form-B-Grenze, tests/unit/s5_family_alloc_conformance.hpp:31).
    /// NICHT im T16-Mess-Pfad (dort liegen Bloom/Cuckoo/RangeSurf/Xor) -> keine Doppelzaehlung.
    [[nodiscard]] typename allocator_type::snapshot_t filter_surf_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    /// A8-S5-02b: die VERSCHACHTELTEN Level-Vektoren einzeln allokator-erweitert kopieren (s. Vertrag (c)).
    /// Ein blosses `labels_lv_ = o.labels_lv_` liesse die inneren Vektoren ueber allocator_traits::construct
    /// mit dem Adapter der QUELLE entstehen -- die Kopie zeigte dann auf fremden Speicher.
    void copy_levels_from(LoudsSparseFilterStore const& o) {
        labels_lv_.reserve(o.labels_lv_.size());
        for (auto const& lv : o.labels_lv_)
            labels_lv_.emplace_back(lv, allocator_.template as_std_allocator<std::uint8_t>());
        child_ind_lv_.reserve(o.child_ind_lv_.size());
        for (auto const& lv : o.child_ind_lv_)
            child_ind_lv_.emplace_back(lv, allocator_.template as_std_allocator<std::uint64_t>());
        louds_lv_.reserve(o.louds_lv_.size());
        for (auto const& lv : o.louds_lv_)
            louds_lv_.emplace_back(lv, allocator_.template as_std_allocator<std::uint64_t>());
        suffix_words_lv_.reserve(o.suffix_words_lv_.size());
        for (auto const& lv : o.suffix_words_lv_)
            suffix_words_lv_.emplace_back(lv, allocator_.template as_std_allocator<std::uint64_t>());
    }

    // ── Build-Hilfen (port surf_builder.hpp) ──
    [[nodiscard]] std::uint32_t tree_height() const noexcept { return static_cast<std::uint32_t>(labels_lv_.size()); }
    [[nodiscard]] std::uint32_t num_items(std::uint32_t level) const noexcept {
        return static_cast<std::uint32_t>(labels_lv_[level].size());
    }

    // A8-S5-02b: Signaturen auf den achsen-gebundenen Wort-Vektor gezogen; Bit-Arithmetik unveraendert
    // (MSB-first, surf_builder.hpp Z.32-44).
    static void set_bit(surf_word_vec_t& bits, std::uint32_t pos) noexcept {
        bits[pos / 64] |= (kSurfMsbMask >> (pos % 64));
    }
    static bool read_bit(surf_word_vec_t const& bits, std::uint32_t pos) noexcept {
        return (bits[pos / 64] & (kSurfMsbMask >> (pos % 64))) != 0;
    }

    void add_level() {
        // A8-S5-02b: jeder NEUE innere Level-Vektor wird MIT dem Adapter dieses allocator_ angelegt -- ein
        // default-konstruierter innerer Vektor waere hier nicht einmal mehr wohlgeformt (der Adapter traegt
        // keinen Default-Ctor); genau das ist die gewuenschte compile-harte Absicherung.
        labels_lv_.emplace_back(allocator_.template as_std_allocator<std::uint8_t>());
        child_ind_lv_.emplace_back(allocator_.template as_std_allocator<std::uint64_t>());
        louds_lv_.emplace_back(allocator_.template as_std_allocator<std::uint64_t>());
        suffix_words_lv_.emplace_back(allocator_.template as_std_allocator<std::uint64_t>());
        node_counts_.push_back(0);
        is_last_term_.push_back(0);
        child_ind_lv_.back().push_back(0);
        louds_lv_.back().push_back(0);
    }

    [[nodiscard]] bool is_char_common_prefix(std::uint8_t c, std::uint32_t level) const noexcept {
        return (level < tree_height()) && (is_last_term_[level] == 0) && (c == labels_lv_[level].back());
    }
    [[nodiscard]] bool is_level_empty(std::uint32_t level) const noexcept {
        return (level >= tree_height()) || labels_lv_[level].empty();
    }
    void move_to_next_item_slot(std::uint32_t level) {
        if (num_items(level) % 64 == 0) {
            child_ind_lv_[level].push_back(0);
            louds_lv_[level].push_back(0);
        }
    }
    void insert_key_byte(std::uint8_t c, std::uint32_t level, bool is_start_of_node, bool is_term) {
        if (level >= tree_height()) add_level();
        if (level > 0) set_bit(child_ind_lv_[level - 1], num_items(level - 1) - 1);
        labels_lv_[level].push_back(c);
        if (is_start_of_node) {
            set_bit(louds_lv_[level], num_items(level) - 1);
            ++node_counts_[level];
        }
        is_last_term_[level] = is_term ? 1 : 0;
        move_to_next_item_slot(level);
    }

    [[nodiscard]] std::uint32_t skip_common_prefix(key_bytes_t const& key) {
        std::uint32_t level = 0;
        while (level < 8 && is_char_common_prefix(key[level], level)) {
            set_bit(child_ind_lv_[level], num_items(level) - 1);
            ++level;
        }
        return level;
    }
    // next_len==0 => kein Nachfolger (letzter Key). insertKeyBytesToTrieUntilUnique (surf_builder.hpp Z.208-244).
    [[nodiscard]] std::uint32_t insert_until_unique(key_bytes_t const& key, key_bytes_t const& next, unsigned next_len,
                                                    std::uint32_t start_level) {
        std::uint32_t level            = start_level;
        bool          is_start_of_node = false;
        if (is_level_empty(level)) is_start_of_node = true;
        insert_key_byte(key[level], level, is_start_of_node, false);
        ++level;
        // level > next_len  ODER  Praefix [0,level) unterscheidet sich von next
        bool prefix_same = (level <= next_len);
        if (prefix_same)
            for (unsigned i = 0; i < level; ++i)
                if (key[i] != next[i]) {
                    prefix_same = false;
                    break;
                }
        if (level > next_len || !prefix_same) return level;

        is_start_of_node = true;
        while (level < 8 && level < next_len && key[level] == next[level]) {
            insert_key_byte(key[level], level, is_start_of_node, false);
            ++level;
        }
        if (level < 8) {
            insert_key_byte(key[level], level, is_start_of_node, false);
        } else {
            insert_key_byte(kSurfTerminator, level, is_start_of_node, true);
        }
        ++level;
        return level;
    }
    void insert_suffix(key_bytes_t const& key, std::uint32_t level) {
        if (level >= tree_height()) add_level();
        std::uint64_t const w = SurfSuffixBits<ST, HashLen, RealLen>::construct_suffix(key, level);
        suffix_words_lv_[level - 1].push_back(w);
    }

    // ANKER MITGEFUEHRT (A8-S5-02b): der Parameter-Typ wechselt auf den achsen-gebundenen Vektor, der
    // Rumpf ist Zeile fuer Zeile der des Originals -> der Z-Anker bleibt gueltig und nachziehbar.
    void build_sparse(surf_vec_t<key_bytes_t> const& keys) { // surf_builder.hpp Z.185-197
        for (std::size_t i = 0; i < keys.size(); ++i) {
            std::uint32_t     level  = skip_common_prefix(keys[i]);
            std::size_t const curpos = i;
            while ((i + 1 < keys.size()) && keys[curpos] == keys[i + 1]) ++i; // (bei dedup nie wahr)
            if (i < keys.size() - 1)
                level = insert_until_unique(keys[curpos], keys[i + 1], 8, level);
            else
                level = insert_until_unique(keys[curpos], key_bytes_t{}, 0, level);
            insert_suffix(keys[curpos], level);
        }
    }

    void finalize() {
        std::uint32_t const height = tree_height();
        surf_u32_vec_t      nbpl(height, allocator_.template as_std_allocator<std::uint32_t>());
        for (std::uint32_t L = 0; L < height; ++L) nbpl[L] = num_items(L);

        // Labels level-major flach legen.
        for (std::uint32_t L = 0; L < height; ++L)
            labels_flat_.insert(labels_flat_.end(), labels_lv_[L].begin(), labels_lv_[L].end());

        // A8-S5-02b: IN PLACE bauen statt ein Temporaer-Objekt zuzuweisen (frueher
        // `child_indicator_ = SurfRank(kSurfRankBlockSize, child_ind_lv_, nbpl)`). Der Rumpf ist identisch
        // (SurfRank::build ruft denselben Original-Konstruktor-Code), aber es entsteht kein zweiter Puffer,
        // der ueber die Achse alloziert und sofort wieder kopiert wuerde -- die Achsen-Statistik dieses
        // Organs bleibt damit die WAHRE Zahl seiner Allokationen.
        child_indicator_.build(kSurfRankBlockSize, child_ind_lv_, nbpl);
        louds_bits_.build(kSurfSelectSampleIvl, louds_lv_, nbpl);

        // Suffixe level-major anhaengen (passend zu getSuffixPos = pos - rank(pos)).
        if constexpr (ST != SurfSuffixType::kNone) {
            for (std::uint32_t L = 0; L < height; ++L)
                for (std::uint64_t w : suffix_words_lv_[L]) suffix_.append_word(w);
        }
    }

    // A8-S5-02b: allocator_ VOR allen Puffern (Lebensdauer des Zeigers im StdAllocatorAdapter, s.
    // Ctor-Kommentar). EINE Instanz fuer das GANZE Organ -- auch die eingebetteten Teil-Objekte
    // (child_indicator_/louds_bits_/suffix_) bekommen genau diese Instanz hereingereicht.
    allocator_type allocator_{};

    // per-Level Build-Vektoren -- VERSCHACHTELT, BEIDE Ebenen ueber die Achse
    surf_vec_t<label_level_vec_t> labels_lv_;
    surf_word_vec_per_level_t     child_ind_lv_;
    surf_word_vec_per_level_t     louds_lv_;
    surf_word_vec_per_level_t     suffix_words_lv_;
    surf_u32_vec_t                node_counts_;
    surf_vec_t<std::uint8_t>      is_last_term_;

    // finalisierte succinct Strukturen
    surf_vec_t<std::uint8_t>             labels_flat_;
    SurfRank                             child_indicator_;
    SurfSelect                           louds_bits_;
    SurfSuffixBits<ST, HashLen, RealLen> suffix_;
    std::size_t                          key_count_ = 0;
};

} // namespace comdare::cache_engine::filter_axis::composable
