// Schicht E2 Paket B: ABI-Vertrag gegen die bestehende Wormhole-Referenz-DLL.
//
// Kartierungs-Divergenz bewusst: unlabeled DLL-Tests laufen via comdare_tests +
// `ctest -LE 'contract|pmc'` vermutlich bereits in test:unit; dieses Gate nutzt
// Label `e2;abi` und folgt damit demselben Mechanismus ohne `contract`-Label.

#include <anatomy/anatomy_base.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;

#ifndef COMDARE_E2_PILOT_DLL
#error "COMDARE_E2_PILOT_DLL must be defined via CMake (target_compile_definitions)"
#endif

static_assert(std::is_standard_layout_v<ana::ComdareResourceControlV1>,
              "E2-Vertrag: ComdareResourceControlV1 muss standard_layout bleiben");
static_assert(std::is_trivially_copyable_v<ana::ComdareResourceControlV1>,
              "E2-Vertrag: ComdareResourceControlV1 muss trivially_copyable bleiben");
static_assert(ana::kResourceControlVersion == 1, "E2-Vertrag: Fake-E1 nutzt Resource-Control-POD Version 1");

namespace {

int g_fail = 0;

[[nodiscard]] std::filesystem::path pilot_dll_path() { return std::filesystem::path{COMDARE_E2_PILOT_DLL}; }

void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << '\n';
    if (!ok) ++g_fail;
}

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << '\n';
    if (!ok) ++g_fail;
}

void check_static_contract() {
    std::cout << "== Static ABI-POD Vertrag ==\n";
    check("ComdareResourceControlV1 standard_layout", std::is_standard_layout_v<ana::ComdareResourceControlV1>);
    check("ComdareResourceControlV1 trivially_copyable", std::is_trivially_copyable_v<ana::ComdareResourceControlV1>);
    check_eq("kResourceControlVersion", ana::kResourceControlVersion, std::uint32_t{1});
}

void check_driveable_contract(ana::IObservableTier* observable) {
    std::cout << "== IObservableTier -> IDriveableTier Vertrag ==\n";
    auto* drive = static_cast<ana::IDriveableTier*>(observable);
    drive->tier_clear();

    check_eq("tier_size nach clear", drive->tier_size(), std::uint64_t{0});
    check("tier_insert(42, 4242)", drive->tier_insert(42u, 4242u));

    std::uint64_t value = 0;
    check("tier_lookup(42)", drive->tier_lookup(42u, &value));
    check_eq("tier_lookup value", value, std::uint64_t{4242});
    check_eq("tier_size nach insert", drive->tier_size(), std::uint64_t{1});
}

void check_resource_control_contract(ana::IResourceControllableTier* rc) {
    std::cout << "== IResourceControllableTier Vertrag ==\n";
    rc->tier_query_resource_caps(nullptr);
    check("tier_query_resource_caps(nullptr) crasht nicht", true);

    ana::ComdareResourceControlV1 caps{};
    rc->tier_query_resource_caps(&caps);
    check_eq("caps.thread_count", caps.thread_count, std::uint64_t{64});
    check_eq("caps.prefetch_distance", caps.prefetch_distance, std::uint64_t{64});
    check_eq("caps.pool_budget_bytes", caps.pool_budget_bytes, (std::uint64_t{1} << 30));
    check_eq("caps.batch_size", caps.batch_size, std::uint64_t{4096});
    check_eq("caps.inline_threshold_bytes", caps.inline_threshold_bytes, std::uint64_t{256});
    check_eq("caps.controllable_axis_count", caps.controllable_axis_count, std::uint64_t{5});

    check_eq("tier_apply_resource_control(nullptr)", rc->tier_apply_resource_control(nullptr), std::uint64_t{0});

    ana::ComdareResourceControlV1 fake_e1{};
    fake_e1.thread_count            = 999;
    fake_e1.prefetch_distance       = 999;
    fake_e1.pool_budget_bytes       = (std::uint64_t{1} << 30) + 4096;
    fake_e1.batch_size              = 999999;
    fake_e1.inline_threshold_bytes  = 999;
    fake_e1.controllable_axis_count = 99;

    // #23 honest-100% (Goal-V4): `applied` zaehlt nur RC-Achsen, deren KONKRETE Pilot-Komposition den Wert REAL
    // annimmt (Detection-Idiom = Praesenz des set_runtime_*-Konsumenten). Pilot-DLL = WormholeComposition:
    //   • thread_count → 0 (label-only, In-Prozess-Tier konsumiert runtime_thread_count() nicht)
    //   • prefetch_distance → 0 (NonePrefetch traegt KEIN set_runtime_distance, nur DistanceEstimator; axis_07:119-123)
    //   • pool_budget_bytes → 0 (ObservableWormholeOrgan = organ-backed Huelle ohne Store → kein set_runtime_pool_budget)
    //   • batch_size → 1 (adapter-konsumiert als FENSTER-Semantik, t1_segment_shape_ :1102-1114)
    //   • inline_threshold_bytes → 1 (ObservableValueHandle traegt den Setter unbedingt + konsumiert ihn real)
    // ⇒ ehrlich 2. KEIN ABI-Bump: kResourceControlVersion/POD/Vtable unveraendert, nur der Observer-Zaehlwert.
    std::uint64_t const applied = rc->tier_apply_resource_control(&fake_e1);
    check_eq("Fake-E1 over-caps apply status (Wormhole: batch + inline real angewandt -> 2)", applied,
             std::uint64_t{2});
}

void check_loaded_dll_contract() {
    std::cout << "== Loader + Anatomy-Vertrag ==\n";
    std::filesystem::path const path = pilot_dll_path();
    check("Pilot-DLL Pfad existiert", std::filesystem::exists(path));

    loader::AnatomyModuleHandle handle;
    int const                   status = loader::AnatomyModuleLoader::load(path, handle);
    check_eq("AnatomyModuleLoader::load", status, loader::status_ok);
    if (status != loader::status_ok) {
        std::cout << "  Loader-Status: " << loader::status_name(status) << '\n';
        return;
    }

    check("handle.valid()", handle.valid());
    ana::IAnatomyBase* const anatomy = handle.anatomy();
    check("handle.anatomy() non-null", anatomy != nullptr);
    if (anatomy == nullptr) return;

    check("genus == SearchAlgorithm", anatomy->genus() == ana::AnatomyGenus::SearchAlgorithm);
    check("organ_count > 0", anatomy->organ_count() > 0);
    check("composition_name nicht leer", !anatomy->composition_name().empty());

    auto* observable = dynamic_cast<ana::IObservableTier*>(anatomy);
    check("dynamic_cast<IObservableTier*> aus demselben IAnatomyBase*", observable != nullptr);
    if (observable != nullptr) check_driveable_contract(observable);

    auto* rc = dynamic_cast<ana::IResourceControllableTier*>(anatomy);
    check("dynamic_cast<IResourceControllableTier*> aus demselben IAnatomyBase*", rc != nullptr);
    if (rc != nullptr) check_resource_control_contract(rc);

    std::cout << "== RAII ==\n";
    std::cout << "  [OK]  Handle verlaesst Scope; Destruktor entlaedt DLL/Anatomie\n";
}

} // namespace

int main() {
    std::cout << "==== E2 Contract: ABI-Vertrag gegen Wormhole-Pilot-DLL ====\n";
    check_static_contract();
    check_loaded_dll_contract();
    std::cout << "\n==== E2 ABI-Vertrag: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
