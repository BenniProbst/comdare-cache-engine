#pragma once
// AP15-3 (#263-Rest, Strang C) -- HOST-only CRTP-Kompositionsschicht fuer Map-Dagger-Ops.
//
// Generische Kompositions-Schicht: Saemtliche Map-Zugriffe ruhen auf EINEM Ordnungs-Such-Primitiv + fester
// Komposition (ENTWICKLER-FUNCTION-HANDLE-HOPS.md:410-414, asec:if-decomp). IDriveableTier bleibt ABI-stabil;
// darum lebt diese CRTP-Schicht im Pruefdock-Host, nicht im Organ. Kuenftige Gattungen bekommen SIBLING-Contracts
// (DriveableSetContract/...) und erben NICHT von Map.

#include <anatomy/idriveable_tier.hpp>
#include <anatomy/scannable_tier.hpp>

#include <concepts>
#include <cstdint>
#include <limits>

namespace comdare::cache_engine::builder::pruef_dock {

namespace detail {

template <class T>
concept DriveableTierScan =
    requires(T const& tier, std::uint64_t start_key, std::uint64_t max_count, std::uint64_t* out_checksum) {
        { tier.tier_scan(start_key, max_count, out_checksum) } -> std::convertible_to<std::uint64_t>;
    };

template <class T>
concept DriveableScannerProbe = requires(T const& tier) {
    { tier.has_scanner() } -> std::convertible_to<bool>;
};

} // namespace detail

template <class Derived>
class DriveableMapContract {
public:
    using key_type    = std::uint64_t;
    using mapped_type = std::uint64_t;

    struct ScanResult {
        bool          available = false;
        std::uint64_t count     = 0;
        std::uint64_t checksum  = 0;
    };

    struct EqualRangeResult {
        ScanResult first{};
        ScanResult second{};
    };

    [[nodiscard]] bool insert_if_absent(std::uint64_t key, std::uint64_t value) noexcept {
        std::uint64_t old = 0;
        if (derived().tier_lookup(key, &old)) return false;
        return derived().tier_insert(key, value);
    }

    [[nodiscard]] bool insert_or_assign(std::uint64_t key, std::uint64_t value) noexcept {
        return derived().tier_insert(key, value);
    }

    [[nodiscard]] bool bracket(std::uint64_t key, std::uint64_t& out) noexcept {
        if (!derived().tier_lookup(key, &out)) { (void)derived().tier_insert(key, 0); }
        return derived().tier_lookup(key, &out);
    }

    [[nodiscard]] std::uint64_t bracket(std::uint64_t key) noexcept {
        std::uint64_t out = 0;
        (void)bracket(key, out);
        return out;
    }

    [[nodiscard]] bool at(std::uint64_t key, std::uint64_t& out) const noexcept {
        return derived().tier_lookup(key, &out);
    }

    [[nodiscard]] std::uint64_t count(std::uint64_t key) const noexcept {
        std::uint64_t old = 0;
        return derived().tier_lookup(key, &old) ? 1u : 0u;
    }

    [[nodiscard]] bool contains(std::uint64_t key) const noexcept {
        std::uint64_t old = 0;
        return derived().tier_lookup(key, &old);
    }

    [[nodiscard]] bool empty() const noexcept { return derived().tier_size() == 0u; }

    [[nodiscard]] std::uint64_t size() const noexcept { return derived().tier_size(); }

    [[nodiscard]] bool erase(std::uint64_t key) noexcept { return derived().tier_erase(key); }

    void clear() noexcept { derived().tier_clear(); }

    [[nodiscard]] bool has_ordering_interface() const noexcept {
        if constexpr (detail::DriveableTierScan<Derived>) {
            if constexpr (detail::DriveableScannerProbe<Derived>) {
                return derived().has_scanner();
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

    [[nodiscard]] ScanResult scan_from(std::uint64_t start_key, std::uint64_t max_count) const noexcept
        requires detail::DriveableTierScan<Derived>
    {
        if (!has_ordering_interface()) return {};
        std::uint64_t got_sum   = 0;
        auto const    got_count = derived().tier_scan(start_key, max_count, &got_sum);
        return ScanResult{true, got_count, got_sum};
    }

    [[nodiscard]] ScanResult lower_bound(std::uint64_t start_key, std::uint64_t max_count = 1u) const noexcept
        requires detail::DriveableTierScan<Derived>
    {
        return scan_from(start_key, max_count);
    }

    [[nodiscard]] ScanResult upper_bound(std::uint64_t key, bool key_present,
                                         std::uint64_t max_count = 1u) const noexcept
        requires detail::DriveableTierScan<Derived>
    {
        if (key == std::numeric_limits<std::uint64_t>::max()) return scan_from(key, 0u);
        auto const upper_start = key_present ? key + 1u : key;
        return scan_from(upper_start, max_count);
    }

    [[nodiscard]] EqualRangeResult equal_range(std::uint64_t key, bool key_present) const noexcept
        requires detail::DriveableTierScan<Derived>
    {
        return EqualRangeResult{lower_bound(key, 1u), upper_bound(key, key_present, 1u)};
    }

private:
    [[nodiscard]] Derived&       derived() noexcept { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const& derived() const noexcept { return static_cast<Derived const&>(*this); }
};

class DriveableMapView final : public DriveableMapContract<DriveableMapView> {
public:
    explicit DriveableMapView(anatomy::IDriveableTier& tier) noexcept
        : tier_(tier), scanner_(dynamic_cast<anatomy::IScannableTier*>(&tier)) {}

    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept {
        return tier_.tier_insert(key, value);
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept {
        return tier_.tier_lookup(key, out_value);
    }

    [[nodiscard]] bool tier_erase(std::uint64_t key) noexcept { return tier_.tier_erase(key); }

    void tier_clear() noexcept { tier_.tier_clear(); }

    [[nodiscard]] std::uint64_t tier_size() const noexcept { return tier_.tier_size(); }

    [[nodiscard]] bool has_scanner() const noexcept { return scanner_ != nullptr; }

    [[nodiscard]] std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                                          std::uint64_t* out_checksum) const noexcept {
        if (scanner_ == nullptr) return 0u;
        return scanner_->tier_scan(start_key, max_count, out_checksum);
    }

private:
    anatomy::IDriveableTier& tier_;
    anatomy::IScannableTier* scanner_ = nullptr;
};

} // namespace comdare::cache_engine::builder::pruef_dock
