// SPDX-License-Identifier: Apache-2.0
// Mock-Permutations-Modul fuer ModuleLoader-Tests (Phase 7.2.D)
//
// Wird als SHARED-Lib `comdare_perm_mock_test` gebaut und exportiert das ABI
// `comdare_get_module_v1` — der Test loaded diese und prueft, dass die
// Magic-Konstanten korrekt durchkommen.

#include <cache_engine/abi/module_abi_v1.hpp>

#if defined(_WIN32)
  #define COMDARE_MODULE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define COMDARE_MODULE_EXPORT __attribute__((visibility("default")))
#else
  #define COMDARE_MODULE_EXPORT
#endif

namespace {

constexpr std::uint64_t kMockFingerprint = 0xABCDEF0123456789ULL;

struct MockInstance {
    int dummy = 7;
};

void* create_instance(comdare_cache_engine_v1* /*engine*/) {
    return new MockInstance{};
}
void destroy_instance(void* inst) {
    delete static_cast<MockInstance*>(inst);
}
void run_workload(void* /*inst*/, comdare_workload_descriptor_v1 const* /*workload*/,
                  comdare_measurement_record_v1* out) {
    if (out) {
        *out = {};
        out->version      = COMDARE_ABI_VERSION;
        out->op_count     = 12345;
        out->total_cycles = 67890;
    }
}
void pull_live_counters(void* /*inst*/, comdare_hw_counters_v1* out) {
    if (out) { *out = {}; out->cycles = 99; }
}

comdare_permutation_module_v1 const k_module = {
    /* abi_version             */ COMDARE_ABI_VERSION,
    /* permutation_fingerprint */ kMockFingerprint,
    /* create_instance         */ create_instance,
    /* destroy_instance        */ destroy_instance,
    /* run_workload            */ run_workload,
    /* pull_live_counters      */ pull_live_counters,
};

}  // namespace

extern "C" COMDARE_MODULE_EXPORT comdare_permutation_module_v1 const*
comdare_get_module_v1(void) {
    return &k_module;
}
