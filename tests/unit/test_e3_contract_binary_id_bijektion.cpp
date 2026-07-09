// Schicht E3 Paket B: Contract-Test fuer StaticBinaryView als Fake-E2.
// Kein DLL-Bau: geprueft wird nur die statische Baum-/View-Struktur und ihre Mixed-Radix-Bijektion.

#include <builder/experiment_tree/experiment_tree.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;

namespace {

int g_fail = 0;

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

std::vector<ex::AxisLevel> static_levels() {
    return {
        ex::AxisLevel{"search_algo", {"k_ary", "interpolation", "eytzinger"}, true, "", "search_algo"},
        ex::AxisLevel{"node_type", {"node4", "node16"}, true, "", "node_type"},
        ex::AxisLevel{"memory_layout", {"aos", "soa"}, true, "", "memory_layout"},
    };
}

std::vector<ex::AxisLevel> all_levels_with_dynamic() {
    std::vector<ex::AxisLevel> levels = static_levels();
    levels.push_back(ex::AxisLevel{"concurrency", {"1", "4", "8"}, false, "thread_count", "concurrency"});
    levels.push_back(ex::AxisLevel{"prefetch", {"0", "64"}, false, "prefetch_distance", "prefetch"});
    return levels;
}

ex::ExperimentTree make_tree(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    return tree;
}

std::string expected_binary_id(std::vector<ex::AxisLevel> const& levels, std::vector<std::size_t> const& tuple) {
    std::string out;
    for (std::size_t d = 0; d < levels.size(); ++d) {
        if (!out.empty()) out += '/';
        out += levels[d].axis;
        out += '=';
        out += levels[d].values[tuple[d]];
    }
    return out;
}

void check_static_bijection() {
    std::cout << "== StaticBinaryView: Produkt, Reihenfolge, Bijektion ==\n";
    std::vector<ex::AxisLevel> const levels = static_levels();
    ex::ExperimentTree               tree   = make_tree(levels);
    ex::StaticBinaryView const       view   = tree.static_binary_view();
    std::size_t const product = levels[0].values.size() * levels[1].values.size() * levels[2].values.size();

    check_eq("binary_count == Produkt 3*2*2", tree.binary_count(), product);
    check_eq("StaticBinaryView.size == binary_count", view.size(), tree.binary_count());
    check_eq("level_count == 3", view.level_count(), std::size_t{3});
    check_eq("level_size[0] == 3", view.level_size(0), std::size_t{3});
    check_eq("level_size[1] == 2", view.level_size(1), std::size_t{2});
    check_eq("level_size[2] == 2", view.level_size(2), std::size_t{2});

    bool                  roundtrip_ok = true;
    bool                  order_ok     = true;
    std::set<std::string> ids;
    for (std::size_t i = 0; i < view.size(); ++i) {
        ex::BinarySpec const     spec = view[i];
        std::vector<std::size_t> tuple;
        tuple.reserve(levels.size());
        order_ok = order_ok && (spec.axes.size() == levels.size()) && (spec.index == i);
        for (std::size_t d = 0; d < levels.size() && d < spec.axes.size(); ++d) {
            order_ok      = order_ok && (spec.axes[d].first == levels[d].axis);
            auto const it = std::find(levels[d].values.begin(), levels[d].values.end(), spec.axes[d].second);
            order_ok      = order_ok && (it != levels[d].values.end());
            tuple.push_back(static_cast<std::size_t>(it - levels[d].values.begin()));
        }
        if (tuple.size() == levels.size()) {
            roundtrip_ok = roundtrip_ok && (view.flat_index(tuple) == i);
            order_ok     = order_ok && (spec.binary_id == expected_binary_id(levels, tuple));
        } else {
            roundtrip_ok = false;
        }
        ids.insert(spec.binary_id);
    }

    check("encode(decode(i)) == i fuer alle Indizes", roundtrip_ok);
    check("Alle binary_ids eindeutig", ids.size() == view.size());
    check("Signatur-Achsen folgen der Level-Deklaration", order_ok);
    check_eq("view[0] Signatur", view[0].binary_id, std::string{"search_algo=k_ary/node_type=node4/memory_layout=aos"});
    check_eq("view[1] Signatur", view[1].binary_id, std::string{"search_algo=k_ary/node_type=node4/memory_layout=soa"});
    check_eq("view[2] Signatur", view[2].binary_id,
             std::string{"search_algo=k_ary/node_type=node16/memory_layout=aos"});
    check_eq("view[4] Signatur", view[4].binary_id,
             std::string{"search_algo=interpolation/node_type=node4/memory_layout=aos"});
    check_eq("view[11] Signatur", view[11].binary_id,
             std::string{"search_algo=eytzinger/node_type=node16/memory_layout=soa"});
}

void check_dynamic_levels_do_not_change_static_count() {
    std::cout << "== DynamicDims: is_static=false veraendert binary_count nicht ==\n";
    ex::ExperimentTree         static_tree = make_tree(static_levels());
    std::vector<ex::AxisLevel> all_levels  = all_levels_with_dynamic();
    ex::ExperimentTree         full_tree   = make_tree(all_levels);

    check_eq("statischer binary_count", static_tree.binary_count(), std::size_t{12});
    check_eq("binary_count mit DynamicDims bleibt gleich", full_tree.binary_count(), static_tree.binary_count());
    check_eq("StaticBinaryView.size mit DynamicDims bleibt gleich", full_tree.static_binary_view().size(),
             static_tree.static_binary_view().size());
    check_eq("dynamic_filter findet 2 Runtime-Dimensionen", full_tree.dynamic_filter().size(), std::size_t{2});
    check_eq("experiment_setting_count = 12*3*2", full_tree.experiment_setting_count(), std::size_t{72});
    check_eq("static_filter enthaelt nur die 3 statischen Ebenen", full_tree.static_filter().size(), std::size_t{3});
}

// Review-Fix (wf_6e518da1, CONFIRMED-minor): size==1-Level aktivieren den pinned_signature-Zweig
// (experiment_tree.hpp:279-282, KF-15 Paper-Wiedererkennung) und die Radix-1-Arithmetik ((i/div)%1) —
// beides war vom 3/2/2-Fixture nicht gedeckt und wird hier explizit asserted.
void check_pinned_signature_and_radix1() {
    std::cout << "== Gepinnte Levels: pinned_signature + Radix-1 ==\n";
    std::vector<ex::AxisLevel> const levels = {
        ex::AxisLevel{"search_algo", {"k_ary", "eytzinger"}, true, "", "search_algo"},
        ex::AxisLevel{"allocator", {"pool_v1"}, true, "", "allocator"},
        ex::AxisLevel{"filter", {"bloom_v2"}, true, "", "filter"},
        ex::AxisLevel{"memory_layout", {"aos", "soa"}, true, "", "memory_layout"},
    };
    ex::ExperimentTree         tree = make_tree(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();

    check_eq("Radix-1: binary_count == 2*1*1*2", view.size(), std::size_t{4});
    bool bijektion_ok = true;
    bool pin_ok       = true;
    for (std::size_t i = 0; i < view.size(); ++i) {
        ex::BinarySpec const spec = view[i];
        pin_ok                    = pin_ok && (spec.pinned_signature == "allocator=pool_v1/filter=bloom_v2");
        pin_ok = pin_ok && (spec.binary_id.find("allocator=pool_v1/filter=bloom_v2") != std::string::npos);
        std::vector<std::size_t> tuple;
        for (std::size_t d = 0; d < levels.size(); ++d) {
            auto const it = std::find(levels[d].values.begin(), levels[d].values.end(), spec.axes[d].second);
            tuple.push_back(static_cast<std::size_t>(it - levels[d].values.begin()));
        }
        bijektion_ok = bijektion_ok && (view.flat_index(tuple) == i);
    }
    check("Radix-1: encode(decode(i)) == i fuer alle Indizes", bijektion_ok);
    check("pinned_signature = genau die size==1-Segmente (KF-15)", pin_ok);
    check_eq("view[0] Signatur mit Pins", view[0].binary_id,
             std::string{"search_algo=k_ary/allocator=pool_v1/filter=bloom_v2/memory_layout=aos"});
    check_eq("view[3] Signatur mit Pins", view[3].binary_id,
             std::string{"search_algo=eytzinger/allocator=pool_v1/filter=bloom_v2/memory_layout=soa"});
}

} // namespace

int main() {
    std::cout << "==== E3 Contract: StaticBinaryView Binary-ID-Bijektion ====\n";
    check_static_bijection();
    check_dynamic_levels_do_not_change_static_count();
    check_pinned_signature_and_radix1();
    std::cout << "\n==== E3 Binary-ID-Bijektion: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
