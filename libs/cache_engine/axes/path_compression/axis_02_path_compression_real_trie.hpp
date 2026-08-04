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
//  (3) R1-Memento: der materialisierte Trie ist vektor-basiert (copy-constructible + copy-assignable +
//      operator==) → ueber den ObservablePathCompression-Wrapper bit-exakt snapshot-/restore-faehig (saved_pc_ in
//      tier_save_all/tier_rollback_all, geleert in tier_clear) — analog saved_vh_/vh_organ_ + saved_flt_/flt_organ_.
//      A8-S5-02a (2026-08-04): der Knoten-Pool laeuft seit dem Schnitt ueber das Allokator-ACHSEN-Interface
//      (path_compression_trie_allocator_t + StdAllocatorAdapter). Die Leitplanke wird dadurch SCHAERFER, nicht
//      schwaecher: die Kopie rebindet auf ihr EIGENES allocator_ und ist damit ein vollstaendig eigenstaendiges
//      Backing statt eines Adapters, der auf den Memento-Partner zeigt (das waere ein dangling Adapter, sobald
//      einer der beiden stirbt). Muster: btree_node_pool_store.hpp:19/:84-105.
//  (4) Zwei-Phasen-Warmup bleibt exakt (der Memento sichert/restauriert den Trie symmetrisch in beiden Pfaden).
//  (5) Lehrbuch-Pattern, zero-cost: die per-Strategie-Auswahl (Patricia-Trie vs leer) ist eine reine `if constexpr`-
//      Compile-Zeit-Selektion ueber das Vorhandensein von Strategy::key_split_bit (Strategy-Pattern,
//      [[no-runtime-switch]]). none-Strategien instanziieren `EmptyPatriciaTrie` (leer, 0-Footprint).
//  (6) keine Erfolgsmarke ohne literale Ausgabe (Test test_patricia_real.cpp).
//
// @topic path_compression @achse 02 @saeule 2 @task §4.3-PATRICIA-REAL @related axis_14_value_handle_real_slot (Vorlage)

#include "concepts/axis_02_path_compression_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp> // A8-S5-02a: Versorger-Achse des Knoten-Pools (s.u.)
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::path_compression {

/// A8-S5 Familie 02a (Dossier 20260803-a8_f2 Abschn. 3.4, Schnitt-Regel): der Knoten-Pool des
/// materialisierten Patricia-Tries bezieht seinen Speicher NICHT mehr ueber den Default-Allokator,
/// sondern ueber das Allokator-ACHSEN-Interface (axis_06). DIESE Zeile ist die EINE Stelle, die den
/// Versorger benennt -- der Trie selbst und der Form-B-Ausweis der S5-Gate-Wache lesen sie, damit kein
/// zweiter Name driften kann.
///
/// WARUM ExgenAllocator: derselbe Achsen-Default, den die bereits konformen Pool-Stores des Repos fuehren
/// (btree_node_pool_store.hpp:56, tree_node_pool_store.hpp, surf_fst_map_pool_store.hpp) und den die
/// S5-Scheiben 03/01d fuer ihre Nicht-Template-Organe gewaehlt haben. Das Organ ist single-threaded
/// getrieben (insert_key haengt am tier_insert-Build-Hook) -- die Sub-Achse AA4 des Exgen
/// (Single-Threaded Specialized) passt. Bei abgeschaltetem Vendor-Flag faellt die Strategie intern auf
/// portable_aligned_alloc zurueck: derselbe libc-Heap wie vorher, aber ueber das Achsen-Interface.
///
/// WARUM HIER KEIN Kompositions-Allokator (Abgrenzung zum HERZ-Fall der node-Achse): PatriciaTrie ist
/// KEIN Template ueber A -- er wird ueber real_trie_t<Strategy> aus der path_compression-STRATEGIE
/// selektiert, die den Allokator der Komposition nicht kennt. Der benannte Achsen-Default ist deshalb hier
/// der richtige Schnitt; die Kompositions-Durchbindung ist der Gegenstand des 01c-Design-Vorlaufs
/// (LEDGER-Nachtrag 04.08. abend-11, T6 = Option B strikt).
///
/// ABHAENGIGKEITSRICHTUNG (Dossier 3.3): path_compression -> alloc. Der Allokator ist die UNTERSTE
/// Versorger-Achse; axes/alloc/ zieht keinen path_compression-Header -> kein Zyklus.
using path_compression_trie_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

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
//   Knoten sind in EINEM Vektor gepoolt (index-basierte Kinder, kNil = leer) -> trivially copyable Slots,
//   bit-exakt vergleichbar (R1-Memento). Wurzel-Index = root_ (kNil bei leerem Trie). Der Pool-Speicher kommt
//   seit A8-S5-02a REAL ueber die Allokator-Achse (path_compression_trie_allocator_t, s.o.).
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

    /// A8-S5-02a Form-B-Ausweis: der Speicher dieses Organs laeuft ueber die Allokator-Achse -- nicht
    /// deklarativ, sondern real (nodes_ traegt den StdAllocatorAdapter dieses allocator_, s. Member unten
    /// + trie_allocator_statistics()).
    using allocator_type = path_compression_trie_allocator_t;
    using node_alloc     = typename allocator_type::template StdAllocatorAdapter<Node>;

    // -- A8-S5-02a Lebensdauer-Vertrag der Achsen-Verdrahtung (Muster btree_node_pool_store.hpp:19/:84-105) --
    // Der StdAllocatorAdapter haelt einen Zeiger auf allocator_. Daraus folgt:
    //   (a) allocator_ MUSS vor nodes_ deklariert sein (Member-Reihenfolge unten),
    //   (b) nodes_ wird IMMER mit allocator_.as_std_allocator<Node>() konstruiert (der Adapter ist nicht
    //       default-konstruierbar -> es gibt kein stilles Zurueckfallen auf einen Default-Allokator),
    //   (c) die R1-MEMENTO-Kopie REBINDET auf das EIGENE allocator_ -- genau das verlangt Leitplanke 3:
    //       saved_pc_.emplace(pc_organ_) (abi_adapter.hpp:1951) und pc_organ_ = *saved_pc_ muessen ZWEI
    //       eigenstaendige Tries erzeugen; ein mitgeschleppter Fremd-Adapter waere ein dangling Zeiger,
    //       sobald einer der beiden stirbt,
    //   (d) Move wird BEWUSST nicht deklariert: die benutzerdeklarierte Kopie unterdrueckt den impliziten
    //       Move, ein std::move degradiert zur (korrekt rebindenden) Kopie statt den Fremd-Adapter zu stehlen.
    // Die transiente Kopier-Allokation der Vollkopie ist kein Mess-Ereignis der Achse -> restore_statistics
    // setzt die Statistik auf den Quell-Stand zurueck (Memento-Symmetrie, btree_node_pool_store.hpp:91).
    PatriciaTrie() : nodes_(allocator_.template as_std_allocator<Node>()) {}
    PatriciaTrie(PatriciaTrie const& o)
        : allocator_(o.allocator_), nodes_(o.nodes_, allocator_.template as_std_allocator<Node>()), root_(o.root_),
          keys_(o.keys_), last_depth_(o.last_depth_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    PatriciaTrie& operator=(PatriciaTrie const& o) {
        if (this != &o) {
            nodes_      = o.nodes_; // Allokator propagiert NICHT (POCCA=false) -> dieses Objekt behaelt seinen Adapter
            root_       = o.root_;
            keys_       = o.keys_;
            last_depth_ = o.last_depth_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~PatriciaTrie() = default;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-02a VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Organs. Damit ist die Form-B-Aussage der S5-Gate-Wache am
    /// Objekt pruefbar -- ein deklarierter allocator_type ohne reale Verdrahtung bliebe hier auf 0 stehen.
    /// NICHT im T3-Mess-Pfad (T3 misst compress/descend) und NICHT im T6-Pfad (T6 misst den Allokator DER
    /// KOMPOSITION, nicht diesen privaten Versorger) -- keine Doppelzaehlung.
    [[nodiscard]] typename allocator_type::snapshot_t trie_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

    // A8-S5-02a: allocator_ VOR nodes_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar).
    allocator_type                allocator_{};
    std::vector<Node, node_alloc> nodes_; ///< Knoten-Pool (index-basiert) -- innere Knoten + Blaetter.
    std::uint32_t                 root_ = kNil;
    std::uint64_t                 keys_ = 0; ///< Anzahl GESPEICHERTER (distinct) Schluessel = Anzahl Blaetter.
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
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — nodes_.push_back kann allozieren/werfen
    // ([[allocation-failure-exception]]: werfen statt terminate).
    void insert_key(std::uint64_t key) {
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
