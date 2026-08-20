// E-24 C10 (a) -- die DEFEKTE Set-Permutations-DLL: das Negativ-Beweismittel des V5-Konformitaets-Gates
// UEBER DIE DLL-GRENZE.
//
// WARUM ES SIE GEBEN MUSS: test_e24_c4_genus_pruef_docks belegt dock_status_conformance_failed (4) an
// einem IN-PROCESS gebauten Modul (DuplicateAcceptingSetModule). Das ist der richtige Beweis fuer die
// Gate-REIHENFOLGE, aber nicht fuer den Lade-Pfad: in-process kennt das Dock den Kompositions-Typ nicht
// erst ueber dlopen + dynamic_cast, und ein Fehler in der Symbol-/vtable-Naht bliebe unsichtbar. C10
// verlangt die Negativ-Proben am DLL-Pfad -- also braucht es ein Modul, das ECHT geladen wird und ECHT
// durchfaellt.
//
// WIE ES DURCHFAELLT (ohne die ABI zu luegen): das Modul ist eine voellig regulaere Set-Permutation --
// vier Pflicht-Symbole, richtiger Major, richtige Magic, richtige Gattung, ISetTier vorhanden. Defekt ist
// allein das search_algo-KERN-ORGAN: MultisetKeyBag legt Duplikate ab, statt sie zu verwerfen. Damit
// waechst occupied_count auch beim zweiten insert(7), SetAnatomy::insert meldet is_new == true
// (set_anatomy.hpp: is_new := after > before) und das std::set-Orakel faellt an SRF3
// (genus_conformance_gate.hpp: "insert(duplikat)"). Genau eine Verletzung der Mengen-Semantik, an genau
// der Stelle, an der das Orakel sie sucht.
//
// Das Modul ist BEWUSST kein Muell-Modul: ein kaputtes Symbol oder eine falsche Magic wuerde schon der
// Loader abweisen und das Gate nie erreichen -- dann pruefte die Probe den Loader, nicht das Gate.

#include <cache_engine/abi/set_module_abi_v1.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

/// MultisetKeyBag -- search_algo-API-konform (key_type + insert/lookup/erase/occupied_count/clear),
/// aber mit MULTISET-Semantik: jeder insert legt ab. Form bewusst nah an SortedArrayKeySet
/// (set_default_organ.hpp) -- der einzige Unterschied ist die fehlende Duplikat-Abwehr im insert.
struct MultisetKeyBag {
    using key_type = std::uint64_t;

    void insert(std::uint64_t k, std::uint64_t /*v*/) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        keys_.insert(it, k); // DEFEKT (absichtlich): keine Duplikat-Abwehr
    }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        return (it != keys_.end() && *it == k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void erase(std::uint64_t k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it != keys_.end() && *it == k) keys_.erase(it);
    }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return keys_.size(); }
    void                      clear() noexcept { keys_.clear(); }

private:
    std::vector<std::uint64_t> keys_; // aufsteigend sortiert, Duplikate erlaubt
};

} // namespace

// Slot-Belegung identisch zu genus_module_set.cpp (13 Slots, INC-2d) -- nur der Kern ist ausgetauscht.
COMDARE_DEFINE_SET_MODULE(MultisetKeyBag, int, int, int, int, int, int, int, int, int, int, int, int)

// A-11/golden-102 (19.08.2026): STEMPEL-PFLICHT -- der Loader weist stempellose Module mit status 13
// (version_lines_symbol_missing) ab; Emission ohne Stempel faellt. Diese Fixture traegt deshalb ihre
// EIGENEN, ehrlichen Zeilen (2-arg-Form: KEINE Mess-Deklaration -- sie kompiliert kein Tooling ein).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
COMDARE_ANATOMY_VERSION_STAMP("system_fixture=perm_set_defekt_d13@1.0.0.c",
                              "fixture_kern=multiset_key_bag_defekt@1.0.0.c")
