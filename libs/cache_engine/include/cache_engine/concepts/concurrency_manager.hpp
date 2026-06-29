#pragma once
// ConcurrencyManager — F6 A+C Komposition via Policy-Template (zero-overhead)
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/i_concurrency_discipline.hpp>
#include <cache_engine/concepts/i_concurrency_mechanic.hpp>

#include <tuple>
#include <type_traits>

namespace comdare::cache_engine {

template <typename T>
concept ConcurrencyDisciplineConcept = std::is_base_of_v<IConcurrencyDiscipline, T>;

template <typename T>
concept ConcurrencyMechanicConcept = std::is_base_of_v<IConcurrencyMechanic, T>;

// Compile-Time Composition: 1..N Disziplinen + 1 Mechanic
template <ConcurrencyMechanicConcept M, ConcurrencyDisciplineConcept... Ds>
class ConcurrencyManager {
public:
    void dispatch(Event const& event) noexcept {
        std::apply([&](auto&... d) noexcept { (d.on_event(event), ...); }, disciplines_);
    }

    [[nodiscard]] std::size_t discipline_count() const noexcept { return sizeof...(Ds); }

    [[nodiscard]] M&       mechanic() noexcept { return mechanic_; }
    [[nodiscard]] M const& mechanic() const noexcept { return mechanic_; }

    template <std::size_t I>
    [[nodiscard]] auto& discipline() noexcept {
        return std::get<I>(disciplines_);
    }

private:
    std::tuple<Ds...> disciplines_{};
    M                 mechanic_{};
};

// Type-erased Variante (fuer dynamische Konfiguration via PermutationFlags)
class IConcurrencyManager {
public:
    virtual ~IConcurrencyManager()                                                      = default;
    virtual void                                  dispatch(Event const& event) noexcept = 0;
    [[nodiscard]] virtual ConcurrencyMechanicKind mechanic_kind() const noexcept        = 0;
    [[nodiscard]] virtual std::size_t             discipline_count() const noexcept     = 0;
};

} // namespace comdare::cache_engine
