#pragma once
// resolve_baustein - Compile-time-Fallback fuer PRT-ART → CacheEngine (REV 7 §6.2)
//
// PRT-ART deklariert seine Bausteine in configuration_permutation_type.
// Wenn ein angefragter Bausteine-Typ im PRT-ART-Stack NICHT vorhanden ist
// → Compile-time-Fallback auf den CacheEngine-Stack.

#include <type_traits>

namespace comdare {

// Concept: hat Algo eine member `baustein_t<Tag>`?
template <typename Algo, typename BausteineTag>
concept has_member_baustein = requires {
    typename Algo::template baustein_t<BausteineTag>;
};

// CacheEngine-Default-Baustein-Lookup (Fallback-Quelle)
namespace cache_engine {
    template <typename BausteineTag>
    struct baustein_t {
        // Default-Wert: void → "kein default-Baustein" → Pruefling muss eigenen liefern
        using type = void;
    };
}

// Resolve-Helper: liefere den Baustein-Typ entweder aus Algo (PRT-ART)
// oder per Fallback aus cache_engine
template <typename Algo, typename BausteineTag>
struct resolve_baustein {
private:
    template <typename A>
    static auto choose() {
        if constexpr (has_member_baustein<A, BausteineTag>) {
            return typename A::template baustein_t<BausteineTag>{};
        } else {
            return typename cache_engine::baustein_t<BausteineTag>::type{};
        }
    }

public:
    using type = decltype(choose<Algo>());
};

template <typename Algo, typename BausteineTag>
using resolve_baustein_t = typename resolve_baustein<Algo, BausteineTag>::type;

}  // namespace comdare
