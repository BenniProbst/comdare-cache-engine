#pragma once
// V41 (Task #42-Folge) — MasstreeLayerNodePoolStore: B+Baum-of-Tries-Substrat mit eingebettetem kpermuter.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @paper P03 Masstree (Mao/Kohler/Morris EuroSys 2012)
//
// **Original-getreue Portierung** des kpermuter (ext/traversal/P03-Masstree/masstree-beta/kpermuter.hh,
// sized_kpermuter_info<2> fuer W=15 -> uint64-Arithmetik, MIT/BSD-artige Masstree-LICENSE) + des Knoten-Layouts
// (masstree_struct.hh: leaf/internode_width=15, layer_keylenx=128, inline keylenx=8). is_original=false
// ([[pseudocode-papers-fallback]]; masstree.hh ist template-only ohne extrahierbare Function-Bodies -> Re-Impl).
// Reine Such-LOGIK (Slice-Descent/ksearch/make_new_layer/Split) lebt im MasstreeLayerTraversalOrgan, NICHT hier.
//
// **kpermuter-DISTINKTION (cache-craftiness):** Die 15 Slot-Daten (slice/keylenx/value/layer) bleiben physisch
// an fester Position; das 64-Bit-Permutationswort `perm` traegt die SORTIERTE Reihenfolge. Insert schiebt nur
// Nibbles im Permutationswort (insert_from_back), KEIN memmove der Slot-Daten — die Store-API-Distinktion ggue.
// BTreeNodePoolStore (physischer Array-Shift).

#include "masstree_layer_pool_concept.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

class MasstreeLayerNodePoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    static constexpr int          kWidth         = 15;    // leaf_width == internode_width (masstree_struct.hh)
    static constexpr std::uint8_t kInlineKeylenx = 8;     // Slot haelt Inline-Wert
    static constexpr std::uint8_t kLayerKeylenx  = 128;   // Slot haelt Sub-Layer-Wurzel (struct.hh:262)

    enum Kind : std::uint8_t { kLeaf = 0, kInternode = 1 };
    [[nodiscard]] static constexpr std::size_t make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind        ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t ref_idx(std::size_t r)  noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── kpermuter<15> ueber uint64 (sized_kpermuter_info<2>), VERBATIM aus kpermuter.hh:78-195 ──
    // initial_value/full_value fuer max_width=15; make_empty/make_sorted/operator[]/back/insert_from_back/remove.
    struct Kperm15 {
        std::uint64_t x_ = 0;
        static constexpr std::uint64_t kInitial = 0x0123456789ABCDE0ULL;
        static constexpr std::uint64_t kFull    = 0xEDCBA98765432100ULL;

        [[nodiscard]] static std::uint64_t make_empty() noexcept {
            // p = initial >> ((max_width - W) << 2); return p & ~15;  (max_width==W==15 -> Shift 0)
            return kInitial & ~static_cast<std::uint64_t>(15);
        }
        [[nodiscard]] static std::uint64_t make_sorted(int n) noexcept {
            std::uint64_t const mask = ((n == kWidth) ? std::uint64_t{0} : (std::uint64_t{16} << (n << 2))) - 1;
            return (make_empty() << (n << 2)) | (kFull & mask) | static_cast<std::uint64_t>(n);
        }
        [[nodiscard]] int size() const noexcept { return static_cast<int>(x_ & 15); }
        [[nodiscard]] int operator[](int i) const noexcept { return static_cast<int>((x_ >> ((i << 2) + 4)) & 15); }
        [[nodiscard]] int back() const noexcept { return (*this)[kWidth - 1]; }
        // insert_from_back(i): allocate back() and insert at position i; return allocated phys slot.
        int insert_from_back(int i) noexcept {
            int const value = back();
            x_ = ((x_ + 1) & ((std::uint64_t{16} << (i << 2)) - 1))
               | (static_cast<std::uint64_t>(value) << ((i << 2) + 4))
               | ((x_ << 4) & ~((std::uint64_t{256} << (i << 2)) - 1));
            return value;
        }
        void remove(int i) noexcept {
            if (static_cast<int>(x_ & 15) == i + 1) {
                --x_;
            } else {
                int const rot_amount = ((static_cast<int>(x_ & 15) - i - 1) << 2);
                std::uint64_t const rot_mask = ((std::uint64_t{16} << rot_amount) - 1) << ((i + 1) << 2);
                x_ = ((x_ - 1) & ~rot_mask)
                   | (((x_ & rot_mask) >> 4) & rot_mask)
                   | (((x_ & rot_mask) << rot_amount) & rot_mask);
            }
        }
    };

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_root(std::size_t r) noexcept { root_ = r; }
    void inc_size()              noexcept { ++size_; }
    void dec_size()              noexcept { --size_; }
    void clear() noexcept {
        leaves_.clear(); inodes_.clear(); fl_leaf_.clear(); fl_inode_.clear();
        root_ = kNil; size_ = 0;
    }
    [[nodiscard]] bool is_leaf(std::size_t r) const noexcept { return ref_kind(r) == kLeaf; }
    void free_node(std::size_t r) noexcept {
        if (ref_kind(r) == kLeaf) fl_leaf_.push_back(ref_idx(r));
        else                      fl_inode_.push_back(ref_idx(r));
    }

    // ── Leaf (kpermuter-Slots) ──
    [[nodiscard]] std::size_t new_leaf() {
        std::size_t idx;
        if (!fl_leaf_.empty()) { idx = fl_leaf_.back(); fl_leaf_.pop_back(); leaves_[idx] = Leaf{}; }
        else { idx = leaves_.size(); leaves_.push_back(Leaf{}); }
        leaves_[idx].perm = Kperm15::make_empty();
        return make_ref(kLeaf, idx);
    }
    [[nodiscard]] int  leaf_size(std::size_t r)        const noexcept { return Kperm15{leaves_[ref_idx(r)].perm}.size(); }
    [[nodiscard]] int  leaf_perm_at(std::size_t r, int logical) const noexcept { return Kperm15{leaves_[ref_idx(r)].perm}[logical]; }
    [[nodiscard]] int  leaf_alloc_slot(std::size_t r)  const noexcept { return Kperm15{leaves_[ref_idx(r)].perm}.back(); }
    int  leaf_perm_insert(std::size_t r, int logical) noexcept {
        Kperm15 q{leaves_[ref_idx(r)].perm};
        int const phys = q.insert_from_back(logical);
        leaves_[ref_idx(r)].perm = q.x_;
        return phys;
    }
    void leaf_perm_remove(std::size_t r, int logical) noexcept {
        Kperm15 q{leaves_[ref_idx(r)].perm};
        q.remove(logical);
        leaves_[ref_idx(r)].perm = q.x_;
    }
    [[nodiscard]] std::uint64_t leaf_permutation(std::size_t r) const noexcept { return leaves_[ref_idx(r)].perm; }
    void          leaf_set_permutation(std::size_t r, std::uint64_t p) noexcept { leaves_[ref_idx(r)].perm = p; }
    // Setzt die Permutation auf make_sorted(n): logische Reihenfolge 0,1,..,n-1 == physische Slots 0..n-1.
    // Der Aufrufer fuellt Slots 0..n-1 (slice/keylenx/value/layer) VORHER (Split-Rebuild, Masstree-Praezedenz).
    void leaf_set_sorted_size(std::size_t r, int n) noexcept { leaves_[ref_idx(r)].perm = Kperm15::make_sorted(n); }

    [[nodiscard]] std::uint64_t leaf_slice_at(std::size_t r, int slot)   const noexcept { return leaves_[ref_idx(r)].slice[slot]; }
    void leaf_set_slice_at(std::size_t r, int slot, std::uint64_t s)     noexcept { leaves_[ref_idx(r)].slice[slot] = s; }
    [[nodiscard]] int  leaf_keylenx_at(std::size_t r, int slot)          const noexcept { return leaves_[ref_idx(r)].keylenx[slot]; }
    void leaf_set_keylenx_at(std::size_t r, int slot, std::uint8_t kl)   noexcept { leaves_[ref_idx(r)].keylenx[slot] = kl; }
    [[nodiscard]] value_type leaf_value_at(std::size_t r, int slot)      const noexcept { return leaves_[ref_idx(r)].lv_value[slot]; }
    void leaf_set_value_at(std::size_t r, int slot, value_type v)        noexcept { leaves_[ref_idx(r)].lv_value[slot] = v; }
    [[nodiscard]] std::size_t leaf_layer_at(std::size_t r, int slot)     const noexcept { return leaves_[ref_idx(r)].lv_layer[slot]; }
    void leaf_set_layer_at(std::size_t r, int slot, std::size_t ref)     noexcept { leaves_[ref_idx(r)].lv_layer[slot] = ref; }

    [[nodiscard]] std::size_t leaf_next(std::size_t r) const noexcept { return leaves_[ref_idx(r)].next; }
    [[nodiscard]] std::size_t leaf_prev(std::size_t r) const noexcept { return leaves_[ref_idx(r)].prev; }
    void leaf_set_next(std::size_t r, std::size_t n)   noexcept { leaves_[ref_idx(r)].next = n; }
    void leaf_set_prev(std::size_t r, std::size_t p)   noexcept { leaves_[ref_idx(r)].prev = p; }

    // ── Internode (B+Baum-Branch: n Slices, n+1 Kinder) ──
    [[nodiscard]] std::size_t new_internode() {
        std::size_t idx;
        if (!fl_inode_.empty()) { idx = fl_inode_.back(); fl_inode_.pop_back(); inodes_[idx] = Internode{}; }
        else { idx = inodes_.size(); inodes_.push_back(Internode{}); }
        return make_ref(kInternode, idx);
    }
    [[nodiscard]] int  inode_n(std::size_t r)               const noexcept { return inodes_[ref_idx(r)].n; }
    void inode_set_n(std::size_t r, int n)                  noexcept { inodes_[ref_idx(r)].n = n; }
    [[nodiscard]] std::uint64_t inode_slice_at(std::size_t r, int slot) const noexcept { return inodes_[ref_idx(r)].slice[slot]; }
    void inode_set_slice_at(std::size_t r, int slot, std::uint64_t s) noexcept { inodes_[ref_idx(r)].slice[slot] = s; }
    [[nodiscard]] std::size_t inode_child_at(std::size_t r, int slot) const noexcept { return inodes_[ref_idx(r)].child[slot]; }
    void inode_set_child_at(std::size_t r, int slot, std::size_t c)  noexcept { inodes_[ref_idx(r)].child[slot] = c; }

private:
    struct Leaf {
        std::uint64_t                     perm = 0;
        std::array<std::uint64_t, kWidth> slice{};
        std::array<std::uint8_t,  kWidth> keylenx{};
        std::array<std::uint64_t, kWidth> lv_value{};
        std::array<std::size_t,   kWidth> lv_layer{};
        std::size_t next = kNil, prev = kNil;
    };
    struct Internode {
        int                                 n = 0;
        std::array<std::uint64_t, kWidth>   slice{};        // bis zu 15 Slices
        std::array<std::size_t, kWidth + 1> child{};        // bis zu 16 Kinder
    };

    std::vector<Leaf>        leaves_{};
    std::vector<Internode>   inodes_{};
    std::vector<std::size_t> fl_leaf_{}, fl_inode_{};
    std::size_t root_ = kNil;
    std::size_t size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das MasstreeLayerNodePool-Concept.
static_assert(MasstreeLayerNodePool<MasstreeLayerNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
