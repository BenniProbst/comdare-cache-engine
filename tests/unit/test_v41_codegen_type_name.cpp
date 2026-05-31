// V41.F.6.1 R5.G — type_name<T>() Verifikation (Auto-Emitter-Grundlage).
//
// Prüft, dass der constexpr-FQ-Typ-Name-Helper für reale Achsen-Vendor-Typen einen im Codegen
// nutzbaren, fully-qualified Namen liefert (ohne "class/struct"-Prefix, mit Namespace, unterscheidbar).
// Das beantwortet die Planrunden-Frage, ob die __FUNCSIG__/__PRETTY_FUNCTION__-Technik den
// für COMDARE_DEFINE_ANATOMY_MODULE_ADHOC nötigen Typ-String produziert.

#include <builder/codegen/type_name.hpp>

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <string_view>

namespace cg    = ::comdare::cache_engine::builder::codegen;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;

// Kalibrierung: ein fundamentaler Typ muss exakt round-trippen.
TEST(R5G_TypeName, FundamentalTypeRoundtrips) {
    EXPECT_EQ(cg::type_name<int>(), std::string_view{"int"});
}

// Achsen-Vendor-Typen: FQ-Name nutzbar für Codegen.
TEST(R5G_TypeName, ExtractsFqNameOfAxisVariants) {
    constexpr std::string_view a = cg::type_name<ce03a::Array256SearchAlgo>();
    constexpr std::string_view v = cg::type_name<ce03a::VectorU8U8SearchAlgo>();

    // Enthält Typ + Namespace.
    EXPECT_NE(a.find("Array256SearchAlgo"), std::string_view::npos);
    EXPECT_NE(a.find("lookup"), std::string_view::npos);  // V41.F.2: axis_03a physisch → lookup-Namespace
    EXPECT_NE(v.find("VectorU8U8SearchAlgo"), std::string_view::npos);
    // Kein führendes Elaborated-Keyword (MSVC "class "/"struct ") — sonst nicht codegen-nutzbar.
    EXPECT_NE(a.substr(0, 6), std::string_view{"class "});
    EXPECT_NE(a.substr(0, 7), std::string_view{"struct "});
    // Unterscheidbar (Voraussetzung, um Permutationen im Codegen auseinanderzuhalten).
    EXPECT_NE(a, v);

    // Diagnose: exakte FQ-Strings (so würden sie im generierten Modul-.cpp landen).
    std::cout << "[R5G type_name] Array256SearchAlgo  => '" << a << "'\n";
    std::cout << "[R5G type_name] VectorU8U8SearchAlgo => '" << v << "'\n";
}
