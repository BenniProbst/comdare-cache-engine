#pragma once
// V41.F.6.1.R7.5.j axis_09b CudaGh200SimdExtension (NVIDIA Grace Hopper GPU-Beschleuniger)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// CudaGh200SimdExtension — NVIDIA CUDA auf Hopper GH200 (Grace Hopper Superchip).
/// ZIH-Cluster: NVIDIA GH200 Grace Hopper Superchip (480 GB LPDDR5X CPU +
/// 96 GB HBM3 GPU). NVLink-C2C 900 GB/s zwischen Grace-CPU und Hopper-GPU.
/// Kompatibel mit Aarch64 (Grace) UND x86_64 (Hopper + Intel/AMD Host).
/// "vector_width_bits" hier metaphorisch -1 (massiv parallel statt Vector).
class CudaGh200SimdExtension : public SimdExtensionStrategyBase<CudaGh200SimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::accelerator_type_tag;
    using family_id = std::integral_constant<int, 7>;

    static constexpr bool enabled = flags::cuda_gh200_enabled;

    [[nodiscard]] static constexpr bool is_active() noexcept { return true; }
    [[nodiscard]] static constexpr int  vector_width_bits() noexcept { return -1; }     // massive parallel, GPU
    [[nodiscard]] static constexpr bool compatible_with_x86() noexcept { return true; } // Hopper-only Host
    [[nodiscard]] static constexpr bool compatible_with_arm() noexcept { return true; } // Grace+Hopper
    [[nodiscard]] static constexpr bool compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool compatible_with_powerpc() noexcept { return true; } // IBM AC922 (Power9 + V100)
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "simd_ext_cuda_gh200"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CudaGh200SimdExtension (NVIDIA Hopper GH200, NVLink-C2C 900GB/s, ZIH Grace Hopper)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CUDA_GH200"; }

    // ─── R7.7.c Topologie: GPU "per Socket" macht keinen Sinn ────────────────
    // -1 = massive parallel (96 GB HBM3, ~16896 CUDA Cores im H100 GPU-Die).
    // shared_among_cores=false: GPU ist separate Device, nicht CPU-intern shared.
    // accessible_from_efficiency_cores=true: irrelevant (GPU-Bus statt CPU-Core).
    [[nodiscard]] static constexpr int  units_per_socket() noexcept { return -1; }
    [[nodiscard]] static constexpr bool shared_among_cores() noexcept { return false; }
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
static_assert(concepts::SimdExtensionStrategy<CudaGh200SimdExtension>);
static_assert(concepts::CacheEnginePermutationStrategy<CudaGh200SimdExtension>);
} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
