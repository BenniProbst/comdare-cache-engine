#pragma once

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include <cache_engine/platform_probe/cpuid_probe.hpp>
#include <provenance/build_provenance.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder {

namespace detail {

inline void append_enabled_allocator(std::string& out, std::string_view name, bool enabled) {
    if (!enabled) { return; }
    if (!out.empty()) { out += ','; }
    out += name;
}

[[nodiscard]] inline std::string enabled_allocators_manifest_value() {
    namespace flags = ::comdare::cache_engine::alloc::flags;

    std::string allocators;
    append_enabled_allocator(allocators, "std", flags::std_enabled);
    append_enabled_allocator(allocators, "mimalloc", flags::mimalloc_enabled);
    append_enabled_allocator(allocators, "snmalloc", flags::snmalloc_enabled);
    append_enabled_allocator(allocators, "pmr", flags::pmr_enabled);
    append_enabled_allocator(allocators, "pool", flags::pool_enabled);
    append_enabled_allocator(allocators, "jemalloc", flags::jemalloc_enabled);
    append_enabled_allocator(allocators, "tcmalloc", flags::tcmalloc_enabled);
    append_enabled_allocator(allocators, "dlmalloc", flags::dlmalloc_enabled);
    append_enabled_allocator(allocators, "hoard", flags::hoard_enabled);
    append_enabled_allocator(allocators, "slab", flags::slab_enabled);
    append_enabled_allocator(allocators, "michael_lf", flags::michael_lf_enabled);
    append_enabled_allocator(allocators, "scalloc", flags::scalloc_enabled);
    append_enabled_allocator(allocators, "numalloc", flags::numalloc_enabled);
    append_enabled_allocator(allocators, "rpmalloc", flags::rpmalloc_enabled);
    append_enabled_allocator(allocators, "lrmalloc", flags::lrmalloc_enabled);
    append_enabled_allocator(allocators, "cama", flags::cama_enabled);
    append_enabled_allocator(allocators, "starmalloc", flags::starmalloc_enabled);
    append_enabled_allocator(allocators, "tcmalloc_wh", flags::tcmalloc_wh_enabled);
    append_enabled_allocator(allocators, "hmalloc", flags::hmalloc_enabled);
    append_enabled_allocator(allocators, "pim_malloc", flags::pim_malloc_enabled);
    append_enabled_allocator(allocators, "crystalline", flags::crystalline_enabled);
    append_enabled_allocator(allocators, "exgen", flags::exgen_enabled);
    append_enabled_allocator(allocators, "buddy", flags::buddy_enabled);
    append_enabled_allocator(allocators, "ptmalloc2", flags::ptmalloc2_enabled);
    append_enabled_allocator(allocators, "vmem_mag", flags::vmem_mag_enabled);
    append_enabled_allocator(allocators, "vampir_nfp", flags::vampir_nfp_enabled);
    return allocators.empty() ? std::string{"none"} : allocators;
}

[[nodiscard]] inline char const* bool01(bool value) noexcept { return value ? "1" : "0"; }

[[nodiscard]] inline std::string
isa_ran_on_manifest_value(::comdare::cache_engine::platform_probe::CpuidProbeResults const& cpu) {
    std::ostringstream os;
    os << "vendor=" << cpu.vendor << ";brand=" << cpu.brand_string << ";sse2=" << bool01(cpu.has_sse2)
       << ";sse42=" << bool01(cpu.has_sse42) << ";avx=" << bool01(cpu.has_avx) << ";avx2=" << bool01(cpu.has_avx2)
       << ";avx512f=" << bool01(cpu.has_avx512f) << ";avx512bw=" << bool01(cpu.has_avx512bw)
       << ";avx512vl=" << bool01(cpu.has_avx512vl) << ";bmi1=" << bool01(cpu.has_bmi1)
       << ";bmi2=" << bool01(cpu.has_bmi2) << ";neon=" << bool01(cpu.has_neon) << ";sve=" << bool01(cpu.has_sve)
       << ";sve2=" << bool01(cpu.has_sve2) << ";family=" << cpu.cpu_family << ";model=" << cpu.cpu_model
       << ";stepping=" << cpu.cpu_stepping << ";cache_line_bytes=" << cpu.cache_line_bytes;
    return os.str();
}

} // namespace detail

[[nodiscard]] inline std::string serialize_provenance_manifest() {
    namespace build = ::comdare::cache_engine::provenance;

    auto const cpu = ::comdare::cache_engine::platform_probe::probe_cpuid();

    std::ostringstream os;
    os << "compiler_id=" << build::compiler_id << '\n';
    os << "compiler_version=" << build::compiler_version << '\n';
    os << "cxx_flags=" << build::cxx_flags << '\n';
    os << "isa_built_for=" << build::isa_built_for << '\n';
    os << "isa_ran_on=" << detail::isa_ran_on_manifest_value(cpu) << '\n';
    os << "cpu_vendor=" << cpu.vendor << '\n';
    os << "cpu_brand=" << cpu.brand_string << '\n';
    os << "cpu_family=" << cpu.cpu_family << '\n';
    os << "cpu_model=" << cpu.cpu_model << '\n';
    os << "cpu_stepping=" << cpu.cpu_stepping << '\n';
    os << "cache_line_bytes=" << cpu.cache_line_bytes << '\n';
    os << "allocators=" << detail::enabled_allocators_manifest_value() << '\n';
    os << "git_sha_super=" << build::git_sha_super << '\n';
    os << "git_sha_cache_engine=" << build::git_sha_cache_engine << '\n';
    os << "git_sha_prt_art=" << build::git_sha_prt_art << '\n';
    os << "git_sha_thesis=" << build::git_sha_thesis << '\n';
    return os.str();
}

[[nodiscard]] inline bool write_provenance_manifest(std::filesystem::path const& path) {
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out) { return false; }
    out << serialize_provenance_manifest();
    return static_cast<bool>(out);
}

} // namespace comdare::cache_engine::builder