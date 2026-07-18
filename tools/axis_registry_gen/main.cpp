// INC-A (2026-07-14) - axis_registry_gen: erzeugt die ce-Registry-XML
// `cache_engine_axis_registry.xml` per COMPILE-TIME-REFLEKTION der realen Achsen-Registry.
//
// WAS: eigenstaendiges C++-Executable (KEINE CMake-Generierungs-Logik). Es reflektiert je der
// 19 Kompositions-Achsen (optional + 7 Build/Shape-Achsen via --with-extra-axes) die
// Enabled*/StaticAxisVariants*-mp_list (mp_filter<is_enabled, All*>) und emittiert je Baustein
// name()/FQ-Typ/Kurz-Typ/enabled/golden_wired.
//
// PFLICHT-LEITPLANKE (verifiziert, Fakten V1/V2 20260713-verify-registry-facts):
//   Es wird ueber die Enabled*-Listen reflektiert (die axes26::T*-Aliase aus
//   registry_to_axis_levels.hpp = TopicConfigSet::StaticAxisVariants* = mp_filter<is_enabled,All*>),
//   NIE ueber die rohen All*-mp_lists. Reflektierte man All*, emittierten default-OFF Paper/Vendor-
//   Wrapper konditionale name()-Formen (z.B. "original_art(disabled)", "mimalloc(real=std)") - das
//   divergiert byte-genau von serialize_composition_path (das nie disabled-Varianten sieht) und
//   braeche die Round-Trip-Garantie. => Reflektion ausschliesslich ueber Enabled*.
//   Zusaetzlich ein Laufzeit-GUARD: kein emittierter name() darf '(' oder ' ' enthalten (faengt
//   jede konditionale (disabled)/(real=std)-Form + Family-Namen-Leaks ab).
//
// KONSEQUENZ (honest): die Baustein-Zahlen sind die ENABLED-Zahlen der aktuellen Build-Konfiguration
//   (Default-Standalone), NICHT die All*-Registry-Zahlen (search_algo=22/allocator=26/queuing_q1=15 aus
//   der Fakten-Tabelle). Vendor-Allocatoren sind HAVE-gated, Paper-/per-K-search_algo-Wrapper Default-OFF
//   (opt-in). Mit vollem ENABLE-Flag-Satz + vendor-HAVE waechst die Reflektion (weiterhin clean, weil
//   enabled==clean name()) auf die All*-Zahlen. Der Generator ist damit flag-parametrisch + immer round-
//   trip-sicher.
//
// header=...: seit INC-A #6 tragen die CE-Achsen-Wrapper COMDARE_DEFINE_ORGAN_LOCATION (cpp_type_name+
//   header_include, anatomy/organ_location.hpp) - analog dem prt-art-R-B-Fix. 'header' wird jetzt aus
//   W::header_include gelesen (organ_header<W>(), nur falls HasOrganLocation<W>), NIE aus dem Typ-Namen
//   abgeleitet (das waere fragil/fabriziert). Wrapper OHNE Location liefern weiterhin header="".
//
// genus="SearchAlgorithm": die 19 Kompositions-Achsen sind die 19 Organe der SearchAlgorithm-Anatomie
//   (AnatomyGenus::SearchAlgorithm, anatomy_base.hpp:68 = "vollst. 19-Achsen-Anatomie"). Die Achsen-
//   Wrapper tragen selbst kein per-Organ-genus-Member; 'genus' ist daher der Anatomie-Genus der Komposition.
//   F30 (WP-4, 2026-07-16): der Wert wird nicht mehr als String-Literal emittiert, sondern via
//   anat::genus_name(AnatomyGenus::SearchAlgorithm) aus der Enum-Single-Source reflektiert (byte-identisch).
//
// type= (F30, WP-4 2026-07-16): '::'-praefixierter FQ-Typ nach der Makro-Konvention (organ_location.hpp:42,
//   wie die prt-art-Registry via W::cpp_type_name) — derselbe Typ ist damit in BEIDEN Registries identisch
//   buchstabiert. BEWUSST NICHT das cpp_type_name-Literal selbst: bei Template-Wrappern (ObservableNodeType/
//   ObservableMemoryLayout/...) traegt das Literal KEINE Template-Argumente — eine 1:1-Emission kollabierte
//   z.B. alle 4 node_type-Bausteine auf denselben type-String (Informationsverlust). Stattdessen wird der
//   volle, argument-erhaltende type_name<W>() emittiert ('::'-praefixiert, elaborated-normalisiert, F24) und
//   per GUARD gegen das Literal geprueft (b.type muss mit W::cpp_type_name beginnen — faengt Literal-Drift).
//
// golden_wired: ein Baustein ist golden-verdrahtet, wenn er in den golden-320-Katalog (FullSourceCatalog =
//   CatalogAxes<4,4,5,4>, profile_facade/source_catalog.hpp:83-113) eingeht: die ersten K je Achse
//   (search_algo=4, node_type=4, memory_layout=5, prefetch=4; die uebrigen 15 Achsen je 1). Hier inline
//   ueber mp_take_c<Enabled*, K> auf DENSELBEN Enabled-Listen berechnet (byte-identisch zur Katalog-Take).
//
// C++23. Ausfuehren: `axis_registry_gen --out <pfad> [--with-extra-axes]`. TABU (permutation_axes.xml,
// golden-320, POD-1416, kV3AxisSchema, ABI) wird NICHT beruehrt - der Generator LIEST nur Typen.

#include <anatomy/anatomy_base.hpp>                            // AnatomyGenus + genus_name (F30: genus reflektiert)
#include <anatomy/organ_location.hpp>                          // HasOrganLocation<W> (INC-A #6: header_include)
#include <builder/codegen/adhoc_emitter.hpp>                   // strip_all_elaborated (F24: geteilter Helfer)
#include <builder/codegen/type_name.hpp>                       // type_name<W>() (FQ-Typ, compile-time)
#include <builder/experiment_tree/registry_to_axis_levels.hpp> // axes26::T* (Enabled*/StaticAxisVariants*)

#include <boost/mp11.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace cg   = ::comdare::cache_engine::builder::codegen;
namespace anat = ::comdare::cache_engine::anatomy;
namespace mp   = boost::mp11;

namespace {

struct Baustein {
    std::string name;    // Wrapper::name() - byte-genauer serialize-Schluessel
    std::string wrapper; // Kurz-Typ (letzte ::-Komponente vor einem eventuellen '<')
    std::string type;    // fully-qualified C++-Typ, '::'-praefixiert (F30; type_name<W>() + strip, F24)
    std::string header;  // Include-Pfad des Wrappers (W::header_include, falls HasOrganLocation<W>; sonst leer)
    // F30-GUARD: das cpp_type_name-Literal des Wrappers (COMDARE_DEFINE_ORGAN_LOCATION), falls vorhanden —
    // b.type MUSS damit beginnen (Literal ohne Template-Argumente == Praefix des vollen Typ-Namens).
    std::string location_literal;
    bool        golden = false;
};

struct AxisOut {
    std::string           slot;
    std::string           id;
    std::string           category;
    std::vector<Baustein> bausteine;
};

// Kurz-Typ aus dem FQ-Namen: alles vor einem eventuellen '<', dann hinter dem letzten "::".
[[nodiscard]] std::string short_name(std::string const& fq) {
    std::size_t const lt   = fq.find('<');
    std::string const head = (lt == std::string::npos) ? fq : fq.substr(0, lt);
    std::size_t const cc   = head.rfind("::");
    return (cc == std::string::npos) ? head : head.substr(cc + 2);
}

[[nodiscard]] std::string xml_escape(std::string const& s) {
    std::string o;
    o.reserve(s.size());
    for (char const c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            case '\'': o += "&apos;"; break;
            default: o += c;
        }
    }
    return o;
}

// header_include eines Organs, wenn der Wrapper COMDARE_DEFINE_ORGAN_LOCATION traegt (HasOrganLocation<W>,
// INC-A #6). Wrapper OHNE per-Organ-Location liefern "" - eine Ableitung aus dem Typ-Namen waere
// fragil/fabriziert (die frueher bewusst leere 'header'-Emission, jetzt via Makro befuellt).
template <class W>
[[nodiscard]] std::string organ_header() {
    if constexpr (anat::HasOrganLocation<W>) {
        return std::string{W::header_include};
    } else {
        return std::string{};
    }
}

// Reflektiert eine Enabled*-mp_list in ihre Bausteine (name/FQ-Typ/Kurz-Typ) + markiert golden anhand
// der ersten GoldenK Eintraege derselben Liste (== FullSourceCatalog-Take). mp_identity vermeidet die
// Default-Konstruktion der Wrapper (nur der Typ wird benoetigt).
template <class List, std::size_t GoldenK>
[[nodiscard]] std::vector<Baustein> reflect_axis() {
    constexpr std::size_t kSize = mp::mp_size<List>::value;
    constexpr std::size_t kTake = (GoldenK < kSize) ? GoldenK : kSize;

    std::set<std::string> golden;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, mp::mp_take_c<List, kTake>>>([&](auto id) {
        using W = typename decltype(id)::type;
        golden.insert(std::string{W::name()});
    });

    std::vector<Baustein> out;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        Baustein b;
        b.name = std::string{W::name()};
        // F24 (WP-4, Voll-Audit 2026-07-16): type_name<W>() schaelt nur den AEUSSEREN MSVC-Elaborated-
        // Specifier; verschachtelte Template-Argumente (Outer<class NS::Inner>) behalten unter MSVC ihre
        // "class "/"struct "-Tokens -> die Registry-XML waere compiler-abhaengig (anderes Byte-Bild als die
        // committete GCC-Form). Derselbe geteilte Helfer wie im adhoc_emitter (Single-Source) normalisiert
        // ALLE Elaborated-Keywords; unter GCC/Clang ist das ein No-op (Byte-Bild unveraendert, verifiziert).
        // F30 (WP-4): '::'-Praefix nach der Makro-Konvention (organ_location.hpp:42) — identische
        // Buchstabierung wie die prt-art-Registry (W::cpp_type_name); Details s. Kopf-Doku 'type='.
        b.type    = "::" + cg::strip_all_elaborated(cg::type_name<W>());
        b.wrapper = short_name(b.type);
        b.header  = organ_header<W>();
        if constexpr (anat::HasOrganLocation<W>) {
            b.location_literal = std::string{W::cpp_type_name}; // F30-GUARD-Eingabe
        }
        b.golden = golden.contains(b.name);
        out.push_back(std::move(b));
    });
    return out;
}

template <class List, std::size_t GoldenK>
[[nodiscard]] AxisOut make_axis(char const* slot, char const* id, char const* category) {
    return AxisOut{slot, id, category, reflect_axis<List, GoldenK>()};
}

// GUARD: kein clean serialize-Schluessel enthaelt '(' oder ' '. Faengt jede konditionale
// (disabled)/(real=std)-Form + versehentliche Family-Namen-Leaks. Leerer name ebenfalls illegal.
[[nodiscard]] bool name_is_clean(std::string const& n) {
    if (n.empty()) return false;
    return n.find('(') == std::string::npos && n.find(' ') == std::string::npos;
}

} // namespace

int main(int argc, char** argv) {
    std::string out_path   = "cache_engine_axis_registry.xml";
    bool        with_extra = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view const a = argv[i];
        if (a == "--out" && i + 1 < argc) {
            out_path = argv[++i];
        } else if (a == "--with-extra-axes") {
            with_extra = true;
        } else if (a == "--help" || a == "-h") {
            std::cout << "axis_registry_gen --out <file> [--with-extra-axes]\n";
            return 0;
        } else {
            std::cerr << "axis_registry_gen: unbekanntes Argument '" << a << "'\n";
            return 2;
        }
    }

    std::vector<AxisOut> axes;
    // -- Die 19 KOMPOSITION-Achsen T00..T18 (serialize_composition_path-Reihenfolge, kCompositionAxisNames). --
    axes.push_back(make_axis<ex::axes26::T00_search_algo, 4>("T00", "search_algo", "composition"));
    axes.push_back(make_axis<ex::axes26::T01_cache_traversal, 1>("T01", "cache_traversal", "composition"));
    axes.push_back(make_axis<ex::axes26::T02_mapping, 1>("T02", "mapping", "composition"));
    axes.push_back(make_axis<ex::axes26::T03_path_compression, 1>("T03", "path_compression", "composition"));
    axes.push_back(make_axis<ex::axes26::T04_node_type, 4>("T04", "node_type", "composition"));
    axes.push_back(make_axis<ex::axes26::T05_memory_layout, 5>("T05", "memory_layout", "composition"));
    axes.push_back(make_axis<ex::axes26::T06_allocator, 1>("T06", "allocator", "composition"));
    axes.push_back(make_axis<ex::axes26::T07_prefetch, 4>("T07", "prefetch", "composition"));
    axes.push_back(make_axis<ex::axes26::T08_concurrency, 1>("T08", "concurrency", "composition"));
    axes.push_back(make_axis<ex::axes26::T09_serialization, 1>("T09", "serialization", "composition"));
    axes.push_back(make_axis<ex::axes26::T11_value_handle, 1>("T10", "value_handle", "composition"));
    // Bau-INC-2d: isa ist Target-ISA-System-Achse (system_axis, s.u.) — kein composition-Slot mehr.
    axes.push_back(make_axis<ex::axes26::T13_index_organization, 1>("T11", "index_organization", "composition"));
    axes.push_back(make_axis<ex::axes26::T14_io_dispatch, 1>("T12", "io_dispatch", "composition"));
    axes.push_back(make_axis<ex::axes26::T15_migration_policy, 1>("T13", "migration_policy", "composition"));
    axes.push_back(make_axis<ex::axes26::T16_filter, 1>("T14", "filter", "composition"));
    axes.push_back(make_axis<ex::axes26::T20_queuing_q1, 1>("T15", "queuing_q1", "composition"));
    axes.push_back(make_axis<ex::axes26::T21_queuing_q2, 1>("T16", "queuing_q2", "composition"));

    // -- Optional: Build/Shape-/System-Achsen (NICHT im serialize-Pfad; golden_wired stets false). --
    if (with_extra) {
        axes.push_back(make_axis<ex::axes26::T10_telemetry, 0>("ext", "telemetry", "system_axis"));
        axes.push_back(
            make_axis<ex::axes26::T12_isa, 0>("ext", "isa", "system_axis")); // Bau-INC-2d: Target-ISA-System-Achse
        axes.push_back(make_axis<ex::axes26::T17_page_type, 0>("ext", "page_type", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T18_simd_extension, 0>("ext", "simd_extension", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T19_general_hardware, 0>("ext", "general_hardware", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T22_btree_order, 0>("ext", "btree_order", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T23_skip_list_shape, 0>("ext", "skip_list_shape", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T24_bst_shape, 0>("ext", "bst_shape", "build_shape"));
        axes.push_back(make_axis<ex::axes26::T25_hash_probe_shape, 0>("ext", "hash_probe_shape", "build_shape"));
    }

    // -- GUARD: kein name() darf '(' / ' ' / leer sein (Round-Trip-/Enabled*-Leitplanke). --
    std::size_t total = 0;
    for (auto const& ax : axes) {
        for (auto const& b : ax.bausteine) {
            ++total;
            if (!name_is_clean(b.name)) {
                std::cerr << "axis_registry_gen: GUARD-BRUCH - unsauberer name() '" << b.name << "' in Achse '" << ax.id
                          << "'. KEINE Datei geschrieben (Enabled*-Leitplanke verletzt).\n";
                return 3;
            }
            // -- F30-GUARD: b.type ('::'-praefixiert) MUSS mit dem COMDARE_DEFINE_ORGAN_LOCATION-Literal des
            //    Wrappers beginnen (das Literal traegt keine Template-Argumente -> Praefix-Beziehung). Faengt
            //    Literal-Drift zwischen Makro-Deklaration und realem Typ (Kopf-Doku 'type='). --
            if (!b.location_literal.empty() && b.type.compare(0, b.location_literal.size(), b.location_literal) != 0) {
                std::cerr << "axis_registry_gen: F30-GUARD-BRUCH - type '" << b.type
                          << "' beginnt nicht mit dem cpp_type_name-Literal '" << b.location_literal << "' (Achse '"
                          << ax.id << "'). KEINE Datei geschrieben (Makro-Literal driftet vom realen Typ).\n";
                return 5;
            }
        }
    }

    std::ofstream f{out_path, std::ios::binary};
    if (!f) {
        std::cerr << "axis_registry_gen: kann Ausgabedatei nicht oeffnen: " << out_path << "\n";
        return 4;
    }

    f << "<comdare_axis_registry engine=\"cache_engine\" schema=\"1\">\n";
    f << "  <!-- GENERIERT von tools/axis_registry_gen (INC-A) per compile-time-Reflektion der Enabled*/\n";
    f << "       StaticAxisVariants*-Listen (mp_filter<is_enabled, All*>). NICHT von Hand editieren. -->\n";
    f << "  <!-- Baustein-Zahlen = ENABLED-Inventar dieser Build-Konfiguration (Vendor-HAVE-/Flag-abhaengig),\n";
    f << "       NICHT die All*-Registry-Zahlen. Reflektion ueber Enabled* ist Pflicht (Round-Trip-Garantie). -->\n";
    f << "  <!-- 'header' = W::header_include aus COMDARE_DEFINE_ORGAN_LOCATION (INC-A #6, analog prt-art R-B),\n";
    f << "       gelesen via HasOrganLocation<W>; NIE aus dem Typ-Namen abgeleitet. Wrapper ohne per-Organ-\n";
    f << "       Location liefern header=\"\" (kein Fabrizieren). -->\n";
    // F30: genus aus der Enum-Single-Source reflektiert (anatomy_base.hpp genus_name), nicht mehr Literal.
    std::string const genus{anat::genus_name(anat::AnatomyGenus::SearchAlgorithm)};
    for (auto const& ax : axes) {
        f << "  <axis id=\"" << xml_escape(ax.id) << "\" slot=\"" << xml_escape(ax.slot) << "\" category=\""
          << xml_escape(ax.category) << "\" genus=\"" << xml_escape(genus) << "\" baustein_count=\""
          << ax.bausteine.size() << "\">\n";
        for (auto const& b : ax.bausteine) {
            f << "    <baustein name=\"" << xml_escape(b.name) << "\" wrapper=\"" << xml_escape(b.wrapper)
              << "\" type=\"" << xml_escape(b.type) << "\" header=\"" << xml_escape(b.header)
              << "\" enabled=\"true\" golden_wired=\"" << (b.golden ? "true" : "false") << "\"/>\n";
        }
        f << "  </axis>\n";
    }
    f << "</comdare_axis_registry>\n";
    f.flush();
    if (!f) {
        std::cerr << "axis_registry_gen: Schreibfehler an " << out_path << "\n";
        return 4;
    }

    std::cout << "axis_registry_gen: " << axes.size() << " Achsen, " << total << " Bausteine -> " << out_path << "\n";
    for (auto const& ax : axes) {
        std::size_t golden = 0;
        for (auto const& b : ax.bausteine)
            if (b.golden) ++golden;
        std::cout << "  " << ax.slot << " " << ax.id << " : " << ax.bausteine.size() << " (golden " << golden << ")\n";
    }
    return 0;
}
