// V41.E11 Plugin-Controller Pruefling-Registry Test
// @doku 20 (Plugin-Controller: cache-engine HÄLT konfigurierte Prüflinge)

#include <gtest/gtest.h>

#include <cache_engine/api/pruefling_registry.hpp>

#include <memory>
#include <string_view>
#include <vector>

namespace api = ::comdare::cache_engine::api;

namespace {

// Dummy-Pruefling (steht stellvertretend fuer prt-art-Permutation).
class DummyPruefling final : public api::IPruefling {
public:
    explicit DummyPruefling(std::string_view axes) : axes_(axes) {}
    [[nodiscard]] std::string_view name() const override { return "dummy"; }
    [[nodiscard]] std::string_view version() const override { return "0.1.0"; }
    [[nodiscard]] std::string_view axes_signature() const override { return axes_; }
    [[nodiscard]] int              run(std::size_t n_ops, double& out_micros_per_op) override {
        out_micros_per_op = (n_ops == 0) ? 0.0 : 1.0;
        return 0;
    }

private:
    std::string_view axes_;
};

class DummyFactory final : public api::IPrueflingFactory {
public:
    [[nodiscard]] std::string_view              pruefling_name() const override { return "dummy"; }
    [[nodiscard]] std::vector<std::string_view> available_axes_combinations() const override {
        return {"compact|lazy", "sparse|eager"};
    }
    [[nodiscard]] std::unique_ptr<api::IPruefling> create(std::string_view axes) override {
        return std::make_unique<DummyPruefling>(axes);
    }
};

} // namespace

TEST(E11_PrueflingRegistry, RegisterFindAll) {
    api::PrueflingRegistry reg; // lokale Instanz (kein globaler State im Test)
    EXPECT_EQ(reg.size(), 0u);
    EXPECT_EQ(reg.find("dummy"), nullptr);

    reg.register_factory(std::make_unique<DummyFactory>());
    EXPECT_EQ(reg.size(), 1u);

    auto all = reg.all_factories();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0]->pruefling_name(), std::string_view{"dummy"});

    auto* f = reg.find("dummy");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->available_axes_combinations().size(), 2u);
    EXPECT_EQ(reg.find("nonexistent"), nullptr);
}

TEST(E11_PrueflingRegistry, FactoryCreatesRunnablePruefling) {
    DummyFactory f;
    auto         p = f.create("compact|lazy");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->axes_signature(), std::string_view{"compact|lazy"});
    double micros = -1.0;
    EXPECT_EQ(p->run(100, micros), 0);
    EXPECT_GE(micros, 0.0);
}

TEST(E11_PrueflingRegistry, GlobalSingletonStable) {
    auto& r1 = api::get_pruefling_registry();
    auto& r2 = api::get_pruefling_registry();
    EXPECT_EQ(&r1, &r2); // selbe Instanz (Function-local-static)
}
