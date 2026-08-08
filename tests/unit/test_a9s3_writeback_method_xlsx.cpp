// test_a9s3_writeback_method_xlsx -- A9-S3: WritebackMethod hat jetzt VIER Werte (Csv/LatexTable/
// ComparisonMetrics/Xlsx). Die compile-time-Drift-Wachen sitzen bereits im Header selbst
// (writeback_method_registry.hpp, static_assert-Batterie) -- dieser Test ist der LAUFZEIT-Beleg, dass
// die Registry auch als Wert benutzbar ist (Iteration, Lookup), nicht nur beim Uebersetzen wahr bleibt.

#include <cache_engine/measurement/writeback_method_registry.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

namespace ms = comdare::cache_engine::measurement;

TEST(A9S3WritebackMethodXlsx, RegistryHatVierEintraegeMitXlsxAlsViertem) {
    EXPECT_EQ(ms::kWritebackMethodCount, 4u);
    ASSERT_EQ(ms::kWritebackMethodRegistry.size(), 4u);
    EXPECT_EQ(ms::kWritebackMethodRegistry[3].method, ms::WritebackMethod::Xlsx);
    EXPECT_EQ(ms::kWritebackMethodRegistry[3].id, std::string_view{"xlsx"});
    EXPECT_EQ(ms::kWritebackMethodRegistry[3].name, std::string_view{"Xlsx"});
}

TEST(A9S3WritebackMethodXlsx, WritebackMethodInfoLoestXlsxAuf) {
    auto const& info = ms::writeback_method_info(ms::WritebackMethod::Xlsx);
    EXPECT_EQ(info.id, std::string_view{"xlsx"});
    EXPECT_EQ(info.name, std::string_view{"Xlsx"});
}

TEST(A9S3WritebackMethodXlsx, IterationSiehtAlleVierEintraegeGenauEinmal) {
    std::size_t gesehen      = 0;
    bool        xlsx_gesehen = false;
    ms::for_each_writeback_method([&](ms::WritebackMethodInfo const& e) {
        ++gesehen;
        if (e.method == ms::WritebackMethod::Xlsx) xlsx_gesehen = true;
    });
    EXPECT_EQ(gesehen, 4u);
    EXPECT_TRUE(xlsx_gesehen);
}
