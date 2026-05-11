#pragma once
// MeasurementCategory + AlgoDetail enums (F1)
// Termin 7 / 03_uml_measurement §2

#include <cstdint>

namespace comdare::cache_engine::measurement {

enum class MeasurementCategory : std::uint8_t {
    CLU                   = 0,    // Cache-Line-Utilization
    CACHE_MISS_L1         = 1,
    CACHE_MISS_L2         = 2,
    CACHE_MISS_L3         = 3,
    DTLB_MISS             = 4,
    MEMORY_FOOTPRINT      = 5,
    BRANCH_MISS           = 6,
    IPC_CPI               = 7,
    LATENCY_MEAN          = 8,
    LATENCY_P50           = 9,
    LATENCY_P95           = 10,
    LATENCY_P99           = 11,
    LATENCY_P999          = 12,
    THROUGHPUT            = 13,
    ENERGY_J              = 14,
    FILL_BUFFER_OCCUPANCY = 15,
};

enum class AlgoDetail : std::uint16_t {
    UNKNOWN              = 0,
    ART_NODE4            = 1,
    ART_NODE16           = 2,
    ART_NODE48           = 3,
    ART_NODE256          = 4,
    HOT_COMPOUND_K32     = 5,
    HOT_BINODE           = 6,
    MASSTREE_INTERNAL    = 7,
    MASSTREE_BORDER      = 8,
    COCO_MACRONODE       = 9,
    START_MULTIBYTE      = 10,
    B2TREE_DECISION      = 11,
    B2TREE_SPAN          = 12,
    SURF_LOUDS_DENSE     = 13,
    SURF_LOUDS_SPARSE    = 14,
    CSS_NODE             = 15,
    CSB_NODEGROUP        = 16,
    WIDER_HANKINS        = 17,
    PRTART_DENSEBYTE     = 100,
    PRTART_EXTENDEDDENSE = 101,
    PRTART_SPARSEPATRICIA= 102,
    PRTART_REDIRECT      = 103,
    PRTART_CUSTOMCACHE   = 104,
};

}  // namespace comdare::cache_engine::measurement
