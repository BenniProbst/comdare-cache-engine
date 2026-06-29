#pragma once
// T3 path_compression — MATERIALISIERTER Patricia/Radix-Trie (User-Direktive 2026-06-04 §4.3).
//
// AUFTRAG (§4.3): „path_compression(Patricia) = materialisierter Patricia/Radix-Trie + echter Descent
// (statt synthetisch)." — analog §4.3 value_handle (echte Pool/Version/Chain-Slot-Struktur statt Roh-Puffer)
// und §4.3 filter (echter Train-then-Probe statt Konstante).
//
// KONTEXT: path_compression (T3) ist im HOT-Path AUTO-gekoppelt — compress() wird in tier_insert/tier_lookup
// gerufen (wie T1/T2). ABER in der M3-Matrix ist die Strategie `path_compression_none` GEPINNT (alle 320 Lebewesen)
// → dort ist compress() ein No-Op und KEIN Trie existiert (messneutral). Diese Datei macht die NICHT-none-Strategie
// `path_compression_patricia` REAL: sie traegt einen ECHTEN, persistenten, INKREMENTELL aufgebauten Patricia-Trie
// (Single-Bit-Split, Morrison 1968 / Knuth TAOCP Vol.3 §6.3 „Patricia"; in modernen In-Memory-Indizes HOT (Binna
// PVLDB 2018) und Wormhole (Wu EuroSys 2019, 10.1145/3302424.3303955)), gegen den insert_key(key) den Trie
// inkrementell baut und descend(key) einen ECHTEN bit-weisen Descent ausfuehrt (kein Roh-Puffer-Scan mehr).
//
// LEITPLANKEN (verbatim §4.3-Direktive 2026-06-04 §4.3):
//  (1) rein ADDITIV — `none` (M3-Pin) bleibt EXAKT No-Op + messneutral: fuer none-Strategien existiert KEINE
//      insert_key/descend/clear_trie-Methode (Compile-Zeit-Selektion → EmptyPatriciaTrie traegt sie NICHT) → der
//      abi_adapter-Build-Hook (`if constexpr (requires { pc_organ_.insert_key(key); })`) ruft NICHTS → none haelt
//      keinen Trie und wird nicht angefasst. EXAKT wie value_handle (EmptyRealSlot) / filter (None ohne insert_key).
//  (2) static-/Observer-Signaturen (path_descend_scan / compress / store_observe_path_compression) NICHT gebrochen —
//      diese Datei fuegt NUR Instanz-Methoden hinzu; die static path_descend_scan + die compress()-Mess-Mechanik
//      bleiben bit-identisch (der seg19-Timer + fill_observer_v3 unberuehrt).
//  (3) R1-Memento: der materialisierte Trie ist `std::vector`-basiert (copy-constructible + copy-assignable +
//      operator==) → ueber den ObservablePathCompression-Wrapper bit-exakt snapshot-/restore-faehig (saved_pc_ in
//      tier_save_all/tier_rollback_all, geleert in tier_clear) — analog saved_vh_/vh_organ_ + saved_flt_/flt_organ_.
//  (4) Zwei-Phasen-Warmup bleibt exakt (der Memento sichert/restauriert den Trie symmetrisch in beiden Pfaden).
//  (5) Lehrbuch-Pattern, zero-cost: die per-Strategie-Auswahl (Patricia-Trie vs leer) ist eine reine `if constexpr`-
//      Compile-Zeit-Selektion ueber das Vorhandensein von Strategy::key_split_bit (Strategy-Pattern,
//      [[no-runtime-switch]]). none-Strategien instanziieren `EmptyPatriciaTrie` (leer, 0-Footprint).
//  (6) keine Erfolgsmarke ohne literale Ausgabe (Test test_patricia_real.cpp).
//
// @topic path_compression @achse 02 @saeule 2 @task §4.3-PATRICIA-REAL @related axis_14_value_handle_real_slot (Vorlage)

#include "concepts/axis_02_path_compression_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::path_compression {

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// EmptyPatriciaTrie — fuer `none` (und ByteWise, das sein eigenes echtes Byte-Prefix-Organ traegt). Traegt KEINE
// insert_key/descend-Methode → der Build-Hook im abi_adapter (requires-detektiert) greift nicht → `none` bleibt
// EXAKT No-Op + messneutral (Leitplanke 1). 0-Footprint. operator== = immer true (leere none-Struktur ist konstant).
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
struct EmptyPatriciaTrie {
    void                        clear() noexcept {}
    [[nodiscard]] std::size_t   node_count() const noexcept { return 0; }
    [[nodiscard]] std::size_t   key_count() const noexcept { return 0; }
    [[nodiscard]] std::uint64_t last_descent_depth() const noexcept { return 0; }
    [[nodiscard]] bool          operator==(EmptyPatriciaTrie const&) const noexcept = default;
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// PatriciaTrie — MATERIALISIERTER Patricia/Radix-Trie (Single-Bit-Split), 64-Bit-Schluessel, MSB-first.
//
//   Lehrbuch-Struktur (Morrison 1968 / Knuth TAOCP Vol.3 §6.3): jeder INNERE Knoten haelt das EINE signifikante
//   Bit (`crit_bit`, MSB-first 0..63), an dem sich zwei Teilbaeume trennen, + zwei Kind-Indizes (bit==0 → left,
//   bit==1 → right). Pfade ohne Verzweigung sind komprimiert (kein Knoten je Bit, nur je Branch-Punkt → die
//   Trie-Hoehe ist durch die ZAHL DER GESPEICHERTEN SCHLUESSEL beschraenkt, nicht durch 64). Blaetter halten den
//   vollstaendigen Schluessel; bei Descent wird am Blatt der volle Schluessel verglichen (Patricia-Standard).
//
//   Knoten sind in EINEM std::vector gepoolt (index-basierte Kinder, kNil = leer) → trivially copyable Slots,
//   bit-exakt vergleichbar (R1-Memento). Wurzel-Index = root_ (kNil bei leerem Trie).
//
//   insert_key(key)  — inkrementeller Aufbau (1 Schluessel je tier_insert):
//     leerer Trie → erstes Blatt = Wurzel. Sonst: descend bis Blatt → bestimme das HOECHSTWERTIGE differierende
//     Bit (crit) zwischen `key` und dem Blatt-Schluessel → fuege einen neuen inneren Knoten an genau der Stelle
//     ein, an der crit < crit_bit des naechsten inneren Knotens ist (kanonischer crit-bit-Trie-Insert).
//     Duplikate (key bereits vorhanden) → No-Op (idempotent, Set-Semantik).
//
//   descend(key)     — ECHTER bit-weiser Descent: ab Wurzel je innerem Knoten das crit_bit von `key` lesen →
//     left/right → bis Blatt; am Blatt voller Schlusselvergleich → (gefunden? + Descent-Tiefe). Die Tiefe ist
//     real, strukturell und schluessel-abhaengig (Anzahl durchquerter innerer Knoten), KEINE Konstante.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
struct PatriciaTrie {
    static constexpr std::uint32_t kNil = ~std::uint32_t{0};

    // EIN gepoolter Knoten-Typ (innerer Knoten ODER Blatt — Blatt gdw. crit_bit == kLeaf).
    static constexpr std::uint8_t kLeaf = 0xFFu; ///< Sentinel-crit_bit → dieser Knoten ist ein Blatt.
    struct Node {
        std::uint8_t  crit_bit = kLeaf; ///< MSB-first Bit-Position 0..63 (innerer Knoten); kLeaf → Blatt.
        std::uint32_t left     = kNil;  ///< Kind fuer crit_bit==0 (innerer Knoten); ungenutzt bei Blatt.
        std::uint32_t right    = kNil;  ///< Kind fuer crit_bit==1 (innerer Knoten); ungenutzt bei Blatt.
        std::uint64_t key      = 0;     ///< vollstaendiger Schluessel (Blatt); 0 bei innerem Knoten.

        [[nodiscard]] bool is_leaf() const noexcept { return crit_bit == kLeaf; }
        [[nodiscard]] bool operator==(Node const&) const noexcept = default;
    };

    std::vector<Node> nodes_{}; ///< Knoten-Pool (index-basiert) — innere Knoten + Blaetter.
    std::uint32_t     root_ = kNil;
    std::uint64_t     keys_ = 0; ///< Anzahl GESPEICHERTER (distinct) Schluessel = Anzahl Blaetter.
    // mutable: rein DIAGNOSTISCHER Nebeneffekt des const-Descent (kein persistenter Struktur-Zustand; aus operator==
    // ausgeklammert) — erlaubt descend() als const-Methode (analog ObservableValueHandle::deref_value const).
    mutable std::uint64_t last_depth_ = 0; ///< Tiefe (durchquerte innere Knoten) des letzten descend (Diagnose).

    // MSB-first Bit-Extraktion: bit an Position `b` (0 = hoechstwertig) — radix-trie-konventionell, identisch zur
    // Patricia-Strategie key_split_bit (axis_02_path_compression_patricia.hpp:44). Reine, branch-freie Op.
    [[nodiscard]] static constexpr std::uint8_t bit_at(std::uint64_t key, std::uint8_t b) noexcept {
        return static_cast<std::uint8_t>((key >> (63U - (b & 63U))) & 1U);
    }

    // Hoechstwertige differierende Bit-Position zweier Schluessel (MSB-first 0..63). Verlangt a != b.
    [[nodiscard]] static constexpr std::uint8_t highest_diff_bit(std::uint64_t a, std::uint64_t b) noexcept {
        std::uint64_t const x   = a ^ b; // x != 0 (Aufrufer garantiert a != b)
        std::uint8_t        pos = 0;
        for (std::uint8_t i = 0; i < 64U; ++i) { // MSB-first: erstes gesetztes Bit von oben
            if (((x >> (63U - i)) & 1U) != 0U) {
                pos = i;
                break;
            }
        }
        return pos;
    }

    // Folge die crit-bit-Kette ab Wurzel bis zu einem Blatt; liefert den Blatt-Index (Trie ist nicht leer).
    [[nodiscard]] std::uint32_t walk_to_leaf_(std::uint64_t key) const noexcept {
        std::uint32_t cur = root_;
        while (cur != kNil && !nodes_[cur].is_leaf()) {
            cur = (bit_at(key, nodes_[cur].crit_bit) == 0U) ? nodes_[cur].left : nodes_[cur].right;
        }
        return cur;
    }

    /// Build (Setup, NICHT gemessen): einen Schluessel inkrementell in den Patricia-Trie einordnen. Set-Semantik
    /// (Duplikat → No-Op). Kanonischer crit-bit-Trie-Insert: das hoechstwertige differierende Bit gegen das durch
    /// den Descent gefundene „naechste" Blatt bestimmt den neuen inneren Knoten; er wird an der Position eingehaengt,
    /// an der seine crit_bit oberhalb der naechsten inneren crit_bit liegt (MSB-first, kleinere Position = weiter oben).
    void insert_key(std::uint64_t key) noexcept {
        if (root_ == kNil) { // erster Schluessel → Blatt = Wurzel
            nodes_.push_back(Node{kLeaf, kNil, kNil, key});
            root_ = static_cast<std::uint32_t>(nodes_.size() - 1);
            ++keys_;
            return;
        }
        std::uint32_t const leaf = walk_to_leaf_(key);
        std::uint64_t const lk   = nodes_[leaf].key;
        if (lk == key) return; // Duplikat → No-Op (idempotent, Set)

        std::uint8_t const crit = highest_diff_bit(key, lk); // hoechstwertiges differierendes Bit
        std::uint8_t const dir  = bit_at(key, crit);         // Seite des NEUEN Schluessels (1 → rechts)

        // Einhaengepunkt suchen: ab Wurzel folgen, solange der naechste innere Knoten ein crit_bit > crit hat
        // (MSB-first: groessere Position = weiter unten → der neue Branch liegt OBERHALB). `parent_link` = Adresse
        // des Index-Felds, in das der neue innere Knoten gehaengt wird.
        std::uint32_t* parent_link = &root_;
        std::uint32_t  cur         = root_;
        while (cur != kNil && !nodes_[cur].is_leaf() && nodes_[cur].crit_bit < crit) {
            parent_link = (bit_at(key, nodes_[cur].crit_bit) == 0U) ? &nodes_[cur].left : &nodes_[cur].right;
            cur         = *parent_link;
        }

        // KRITISCH (Realloc-Safety, Fuzz-aufgedeckt): `sub` aus `parent_link` lesen, BEVOR irgendein push_back nodes_
        // realloziert. Bei Nicht-Wurzel-Einhaengung zeigt parent_link IN nodes_ (Z.150 &nodes_[cur].left/right) und
        // wuerde nach einer Reallocation baumeln → *parent_link = UB/OOB (SIGSEGV bei /O2, N gross). Lese-Pfad zuerst.
        std::uint32_t const sub      = *parent_link; // bestehender Teilbaum unter dem Einhaengepunkt (VOR push_back)
        std::uint32_t const new_leaf = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back(
            Node{kLeaf, kNil, kNil, key}); // das neue Blatt (kann nodes_ reallozieren → parent_link ab hier ungueltig)
        Node inner{};
        inner.crit_bit = crit;
        inner.left     = (dir == 0U) ? new_leaf : sub; // bit==0 → links
        inner.right    = (dir == 0U) ? sub : new_leaf; // bit==1 → rechts
        nodes_.push_back(inner);
        // ACHTUNG: parent_link kann durch das push_back invalidiert sein (Reallocation von nodes_) — daher den
        // Einhaengepunkt NICHT ueber den alten Zeiger schreiben, sondern erneut auffinden (robust gegen Realloc).
        std::uint32_t const inner_idx = static_cast<std::uint32_t>(nodes_.size() - 1);
        relink_(key, crit, sub, inner_idx);
        ++keys_;
    }

    // Haenge `inner_idx` an genau die Stelle, an der zuvor `sub` hing (re-walk, realloc-sicher).
    void relink_(std::uint64_t key, std::uint8_t crit, std::uint32_t sub, std::uint32_t inner_idx) noexcept {
        if (root_ == sub) {
            root_ = inner_idx;
            return;
        }
        std::uint32_t cur = root_;
        while (cur != kNil && !nodes_[cur].is_leaf() && nodes_[cur].crit_bit < crit) {
            std::uint32_t& child = (bit_at(key, nodes_[cur].crit_bit) == 0U) ? nodes_[cur].left : nodes_[cur].right;
            if (child == sub) {
                child = inner_idx;
                return;
            }
            cur = child;
        }
    }

    /// ECHTER bit-weiser Descent: ab Wurzel je innerem Knoten das crit_bit von `key` lesen → left/right → bis Blatt;
    /// am Blatt voller Schluesselvergleich. Liefert true gdw. `key` real gespeichert ist; `last_depth_` = Anzahl
    /// durchquerter innerer Knoten (reale, strukturelle, schluessel-abhaengige Tiefe — KEINE Konstante).
    [[nodiscard]] bool descend(std::uint64_t key) const noexcept {
        last_depth_       = 0;
        std::uint32_t cur = root_;
        while (cur != kNil && !nodes_[cur].is_leaf()) {
            ++last_depth_;
            cur = (bit_at(key, nodes_[cur].crit_bit) == 0U) ? nodes_[cur].left : nodes_[cur].right;
        }
        return cur != kNil && nodes_[cur].key == key;
    }

    void clear() noexcept {
        nodes_.clear();
        root_       = kNil;
        keys_       = 0;
        last_depth_ = 0;
    }

    [[nodiscard]] std::size_t   node_count() const noexcept { return nodes_.size(); } ///< innere + Blatt-Knoten
    [[nodiscard]] std::size_t   key_count() const noexcept { return static_cast<std::size_t>(keys_); }
    [[nodiscard]] std::uint64_t last_descent_depth() const noexcept { return last_depth_; }

    [[nodiscard]] bool operator==(PatriciaTrie const& o) const noexcept {
        // Memento-Vertrag: NUR die persistente Struktur (Knoten + Wurzel + Schlusselzahl) vergleichen, NICHT die
        // diagnostische last_depth_ (transienter Descent-Nebeneffekt, kein Struktur-Zustand) — exakt wie
        // ObservableValueHandle::operator== nur real_slot_ (nicht die Stats) vergleicht.
        if (root_ != o.root_ || keys_ != o.keys_ || nodes_.size() != o.nodes_.size()) return false;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
            if (!(nodes_[i] == o.nodes_[i])) return false;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// real_trie_for<Strategy> — Compile-Zeit-Selektion der realen Trie-Struktur je Strategie (Strategy-Pattern,
// [[no-runtime-switch]], zero-cost). Patricia (traegt static key_split_bit) → PatriciaTrie (echter Descent). Sonst
// (none / ByteWise) → EmptyPatriciaTrie (kein Build-Hook, none bleibt EXAKT No-Op; ByteWise traegt sein eigenes
// echtes Byte-Prefix-Organ, ByteWiseKeyPrefix). Die Detektion nutzt das Patricia-Diskriminator-Primitiv
// key_split_bit (das genau die Single-Bit-Split-Strategie auszeichnet), NICHT name()-Stringvergleich.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
template <class Strategy>
concept HasKeySplitBit = requires(std::uint64_t k, unsigned d) { Strategy::key_split_bit(k, d); };

template <class Strategy>
struct real_trie_selector {
    using type = std::conditional_t<HasKeySplitBit<Strategy>, PatriciaTrie, EmptyPatriciaTrie>;
};

template <class Strategy>
using real_trie_t = typename real_trie_selector<Strategy>::type;

// Das reale Trie-Backing ist fuer JEDE Strategie kopierbar + vergleichbar (R1-Memento, Leitplanke 3).
static_assert(std::is_copy_constructible_v<EmptyPatriciaTrie> && std::is_copy_assignable_v<EmptyPatriciaTrie>);
static_assert(std::is_copy_constructible_v<PatriciaTrie> && std::is_copy_assignable_v<PatriciaTrie>);

} // namespace comdare::cache_engine::path_compression
