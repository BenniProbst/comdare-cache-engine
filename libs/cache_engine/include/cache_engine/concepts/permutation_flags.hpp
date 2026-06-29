#pragma once
// PermutationFlags — F10-K Strukturiertes Flag-System (CPUID-Vorbild)
// Termin 7 / 04_uml_hardware_isa §2 + 05_uml_engine_choice_builder §3 + REV 5 §3
// 10 Banks (REV 5 erweitert telemetry_bank fuer Achse 11 Kuehn)

#include <cstdint>
#include <string>
#include <string_view>

namespace comdare::cache_engine {

namespace flags::page_bank {
inline constexpr uint64_t DENSEBYTE_ART256       = 1ULL << 0;
inline constexpr uint64_t SPARSE_NODE4_ART       = 1ULL << 1;
inline constexpr uint64_t COMPOUND_HOT           = 1ULL << 2;
inline constexpr uint64_t MULTIBYTE_START        = 1ULL << 3;
inline constexpr uint64_t MACRO_COCO             = 1ULL << 4;
inline constexpr uint64_t DECISION_B2TREE        = 1ULL << 5;
inline constexpr uint64_t LOUDS_DENSE_SURF       = 1ULL << 6;
inline constexpr uint64_t LOUDS_SPARSE_SURF      = 1ULL << 7;
inline constexpr uint64_t METATRIEHT_WORMHOLE    = 1ULL << 8;
inline constexpr uint64_t BPLUS_MASSTREE         = 1ULL << 9;
inline constexpr uint64_t SPAN_B2TREE            = 1ULL << 10;
inline constexpr uint64_t LOUDS_JACOBSON         = 1ULL << 11;
inline constexpr uint64_t CSS_NODE               = 1ULL << 12;
inline constexpr uint64_t CSB_NODEGROUP          = 1ULL << 13;
inline constexpr uint64_t WIDER_HANKINS          = 1ULL << 14;
inline constexpr uint64_t CONFIGTABLE_SAMUEL     = 1ULL << 15;
inline constexpr uint64_t PREFETCH_CHEN          = 1ULL << 16;
inline constexpr uint64_t FRACTAL_CHEN           = 1ULL << 17;
inline constexpr uint64_t ADAPTIVE_BTREESAREBACK = 1ULL << 18;
inline constexpr uint64_t BART_HYBRID_P06        = 1ULL << 19;
inline constexpr uint64_t HATTRIE_P06            = 1ULL << 20;
inline constexpr uint64_t PBTREESTATIC_P06       = 1ULL << 21;
inline constexpr uint64_t PRTART_DENSEBYTE       = 1ULL << 56;
inline constexpr uint64_t PRTART_EXTENDEDDENSE   = 1ULL << 57;
inline constexpr uint64_t PRTART_SPARSEPATRICIA  = 1ULL << 58;
inline constexpr uint64_t PRTART_REDIRECT        = 1ULL << 59;
inline constexpr uint64_t PRTART_CUSTOMCACHE     = 1ULL << 60;
} // namespace flags::page_bank

namespace flags::node_bank {
inline constexpr uint64_t ART_NODE4           = 1ULL << 0;
inline constexpr uint64_t ART_NODE16          = 1ULL << 1;
inline constexpr uint64_t ART_NODE48          = 1ULL << 2;
inline constexpr uint64_t ART_NODE256         = 1ULL << 3;
inline constexpr uint64_t HOT_COMPOUND_K32    = 1ULL << 4;
inline constexpr uint64_t HOT_BINODE          = 1ULL << 5;
inline constexpr uint64_t MASSTREE_INTERNAL   = 1ULL << 6;
inline constexpr uint64_t MASSTREE_BORDER     = 1ULL << 7;
inline constexpr uint64_t MACRONODE_COCO      = 1ULL << 8;
inline constexpr uint64_t MULTIBYTE_START     = 1ULL << 9;
inline constexpr uint64_t DECISION_B2TREE     = 1ULL << 10;
inline constexpr uint64_t SPAN_B2TREE         = 1ULL << 11;
inline constexpr uint64_t HATTRIE_HASHBUCKET  = 1ULL << 12;
inline constexpr uint64_t PBTREE_STATIC       = 1ULL << 13;
inline constexpr uint64_t CSS_NODE            = 1ULL << 14;
inline constexpr uint64_t CSB_NODEGROUP       = 1ULL << 15;
inline constexpr uint64_t WIDER_BPLUS_HANKINS = 1ULL << 16;
inline constexpr uint64_t PRTART_REDIRECT     = 1ULL << 56;
inline constexpr uint64_t PRTART_BPLUS        = 1ULL << 57;
} // namespace flags::node_bank

namespace flags::traversal_bank {
inline constexpr uint64_t BYTEBYBYTE             = 1ULL << 0;  // P01
inline constexpr uint64_t DISCRIMINATIVE_BITS    = 1ULL << 1;  // P02
inline constexpr uint64_t LAYER_SLICE            = 1ULL << 2;  // P03
inline constexpr uint64_t MACRO_NODE             = 1ULL << 3;  // P04
inline constexpr uint64_t MULTIBYTE_SPAN         = 1ULL << 4;  // P05
inline constexpr uint64_t EMBEDDED_DEC_TREE      = 1ULL << 5;  // P06a
inline constexpr uint64_t HATTRIE_HASH_THEN_TRIE = 1ULL << 6;  // P06b
inline constexpr uint64_t PBTREE_STATIC_PREFETCH = 1ULL << 7;  // P06b
inline constexpr uint64_t BART_HYBRID_DISPATCH   = 1ULL << 8;  // P06b
inline constexpr uint64_t HASH_ANCHOR            = 1ULL << 9;  // P07
inline constexpr uint64_t RANK_SELECT            = 1ULL << 10; // P09/P10
inline constexpr uint64_t LOWER_BOUND            = 1ULL << 11; // P10
inline constexpr uint64_t POINTER_ARITHMETIC     = 1ULL << 12; // P11
inline constexpr uint64_t OFFSET                 = 1ULL << 13; // P12
inline constexpr uint64_t PREFETCH_AHEAD         = 1ULL << 14; // P21
inline constexpr uint64_t ADAPTIVE_PREFETCH_DIST = 1ULL << 15; // P23
inline constexpr uint64_t PRTART_HOT_PATH        = 1ULL << 56;
inline constexpr uint64_t PRTART_PREFETCH_AWARE  = 1ULL << 57;
} // namespace flags::traversal_bank

namespace flags::value_handle_bank {
inline constexpr uint64_t INLINE_HANDLE    = 1ULL << 0;
inline constexpr uint64_t POINTER_HANDLE   = 1ULL << 1;
inline constexpr uint64_t REDIRECT_TO_NODE = 1ULL << 2;
inline constexpr uint64_t REDIRECT_TO_PAGE = 1ULL << 3;
inline constexpr uint64_t DYNAMIC_HANDLE   = 1ULL << 4;
inline constexpr uint64_t CHAINREF_HANDLE  = 1ULL << 5; // PRT-ART Multi-Value
} // namespace flags::value_handle_bank

namespace flags::memory_layout_bank {
inline constexpr uint64_t CACHELINE_ALIGNED    = 1ULL << 0;
inline constexpr uint64_t HUGEPAGE_2MB_ALIGNED = 1ULL << 1;
inline constexpr uint64_t HUGEPAGE_1GB_ALIGNED = 1ULL << 2;
inline constexpr uint64_t POINTER_FREE_CONTIG  = 1ULL << 3; // P11
inline constexpr uint64_t SIBLING_CLUSTER      = 1ULL << 4; // P12
inline constexpr uint64_t INLINE_SLOT          = 1ULL << 5;
inline constexpr uint64_t METADATA_HEADER      = 1ULL << 6;
inline constexpr uint64_t EMBEDDED_TREE        = 1ULL << 7;  // P06
inline constexpr uint64_t PROBABILITY_LAYOUT   = 1ULL << 8;  // P16
inline constexpr uint64_t CACHE_OBLIVIOUS      = 1ULL << 9;  // P17
inline constexpr uint64_t LAYOUT_INVARIANT     = 1ULL << 10; // P19
inline constexpr uint64_t MULTI_LEVEL_RELOC    = 1ULL << 11; // P18
} // namespace flags::memory_layout_bank

namespace flags::allocator_bank {
inline constexpr uint64_t POOL_PER_PAGETYPE = 1ULL << 0;
inline constexpr uint64_t ARENA_PER_SUBTREE = 1ULL << 1;
inline constexpr uint64_t SLAB              = 1ULL << 2;
inline constexpr uint64_t HBM               = 1ULL << 3; // P32/P33
inline constexpr uint64_t DEFAULT_MALLOC    = 1ULL << 4;
inline constexpr uint64_t TCMALLOC          = 1ULL << 5;
inline constexpr uint64_t MIMALLOC          = 1ULL << 6;
inline constexpr uint64_t SNMALLOC          = 1ULL << 7;
inline constexpr uint64_t REWIRING          = 1ULL << 8; // P05 START
} // namespace flags::allocator_bank

namespace flags::prefetch_bank {
inline constexpr uint64_t NONE                = 1ULL << 0;
inline constexpr uint64_t SOFTWARE_FIXED      = 1ULL << 1; // P21
inline constexpr uint64_t ADAPTIVE_DISTANCE   = 1ULL << 2; // P23
inline constexpr uint64_t HIERARCHICAL_BUNDLE = 1ULL << 3; // P27
inline constexpr uint64_t FILL_BUFFER_AWARE   = 1ULL << 4; // P25
inline constexpr uint64_t HOT_PATH            = 1ULL << 5; // P26
} // namespace flags::prefetch_bank

namespace flags::concurrency_bank {
inline constexpr uint64_t NONE            = 1ULL << 0;
inline constexpr uint64_t OLC             = 1ULL << 1; // P01/P08
inline constexpr uint64_t ROWEX           = 1ULL << 2; // P02/P08
inline constexpr uint64_t RCU             = 1ULL << 3; // P29 comdare-rcu
inline constexpr uint64_t HAZARD_POINTERS = 1ULL << 4; // P30
inline constexpr uint64_t EPOCH_BASED     = 1ULL << 5;
} // namespace flags::concurrency_bank

namespace flags::isa_bank {
inline constexpr uint64_t SCALAR_X86_64 = 1ULL << 0;
inline constexpr uint64_t SCALAR_ARM64  = 1ULL << 1;
inline constexpr uint64_t SCALAR_RISCV  = 1ULL << 2;
inline constexpr uint64_t X86_SSE2      = 1ULL << 3;
inline constexpr uint64_t X86_SSE4_2    = 1ULL << 4;
inline constexpr uint64_t X86_AVX2      = 1ULL << 5;
inline constexpr uint64_t X86_AVX512F   = 1ULL << 6;
inline constexpr uint64_t X86_BMI2      = 1ULL << 7;
inline constexpr uint64_t X86_POPCNT    = 1ULL << 8;
inline constexpr uint64_t X86_CRC32     = 1ULL << 9;
inline constexpr uint64_t ARM_NEON      = 1ULL << 10;
inline constexpr uint64_t ARM_SVE       = 1ULL << 11;
inline constexpr uint64_t ARM_SVE2      = 1ULL << 12;
inline constexpr uint64_t RISCV_V       = 1ULL << 13;
} // namespace flags::isa_bank

// Achse 11 NEU 2026-05-09 (Kuehn-Erkenntnisse)
namespace flags::telemetry_bank {
inline constexpr uint64_t PER_NODE_COUNTER        = 1ULL << 0; // P28 Original (WARNING)
inline constexpr uint64_t LEAFONLY_COUNTER        = 1ULL << 1; // Kuehn NEU
inline constexpr uint64_t LEAFONLY_SAMPLED        = 1ULL << 2; // Kuehn NEU
inline constexpr uint64_t RETROACTIVE_AGGREGATION = 1ULL << 3; // Kuehn NEU (BARRIER)
inline constexpr uint64_t PATH_READ_COUNTER       = 1ULL << 4; // P26 Zhang FGCS
inline constexpr uint64_t PROBABILITY_HINTS       = 1ULL << 5; // PRT-ART eigen T4
} // namespace flags::telemetry_bank

// F10-K PermutationFlags — Bank-Layout aus Flag_System.txt
struct PermutationFlags {
    uint64_t page_bank          = 0;
    uint64_t node_bank          = 0;
    uint64_t traversal_bank     = 0;
    uint64_t value_handle_bank  = 0;
    uint64_t memory_layout_bank = 0;
    uint64_t allocator_bank     = 0;
    uint64_t prefetch_bank      = 0;
    uint64_t concurrency_bank   = 0;
    uint64_t isa_bank           = 0;
    uint64_t telemetry_bank     = 0; // NEU REV 5 (Achse 11 Kuehn)

    [[nodiscard]] constexpr bool any_set() const noexcept {
        return (page_bank | node_bank | traversal_bank | value_handle_bank | memory_layout_bank | allocator_bank |
                prefetch_bank | concurrency_bank | isa_bank | telemetry_bank) != 0;
    }

    // ConstraintFilter: pruefe inkohaerente Bit-Kombinationen (F4)
    [[nodiscard]] constexpr bool is_valid_combination() const noexcept {
        // Block AJ Abhaengigkeit: LeafOnlyCounter erfordert RetroactiveAggregation
        if ((telemetry_bank & flags::telemetry_bank::LEAFONLY_COUNTER) &&
            !(telemetry_bank & flags::telemetry_bank::RETROACTIVE_AGGREGATION))
            return false;
        if ((telemetry_bank & flags::telemetry_bank::LEAFONLY_SAMPLED) &&
            !(telemetry_bank & flags::telemetry_bank::RETROACTIVE_AGGREGATION))
            return false;
        // ART-Pages erfordern ART-Node-Familie
        constexpr uint64_t art_pages = flags::page_bank::DENSEBYTE_ART256 | flags::page_bank::SPARSE_NODE4_ART;
        constexpr uint64_t art_nodes = flags::node_bank::ART_NODE4 | flags::node_bank::ART_NODE16 |
                                       flags::node_bank::ART_NODE48 | flags::node_bank::ART_NODE256;
        if ((page_bank & art_pages) && !(node_bank & art_nodes)) return false;
        // B2Tree-Page erfordert EMBEDDED_TREE-Layout
        if ((page_bank & flags::page_bank::DECISION_B2TREE) &&
            !(memory_layout_bank & flags::memory_layout_bank::EMBEDDED_TREE))
            return false;
        return true;
    }

    // Kompakte hexadezimale Identifikation aus allen 10 Banks
    [[nodiscard]] inline std::string to_identifier() const {
        auto to_hex = [](uint64_t v) -> std::string {
            constexpr char digits[] = "0123456789abcdef";
            std::string    out(16, '0');
            for (int i = 15; i >= 0; --i) {
                out[i] = digits[v & 0xfULL];
                v >>= 4;
            }
            return out;
        };
        std::string s;
        s.reserve(176);
        s += to_hex(page_bank);
        s += '.';
        s += to_hex(node_bank);
        s += '.';
        s += to_hex(traversal_bank);
        s += '.';
        s += to_hex(value_handle_bank);
        s += '.';
        s += to_hex(memory_layout_bank);
        s += '.';
        s += to_hex(allocator_bank);
        s += '.';
        s += to_hex(prefetch_bank);
        s += '.';
        s += to_hex(concurrency_bank);
        s += '.';
        s += to_hex(isa_bank);
        s += '.';
        s += to_hex(telemetry_bank);
        return s;
    }

    [[nodiscard]] friend constexpr bool operator==(PermutationFlags const&, PermutationFlags const&) = default;
};

} // namespace comdare::cache_engine
