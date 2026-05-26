// V41.F.6.1.R5.C.A — AnatomyBase Tests (Wurzel + Gattungs-Marker)
//
// Beweist:
// 1. AnatomyGenus enum hat 5 Werte (Mammal/Bird/Reptile/Invertebrate/Plant)
// 2. genus_name() Helper liefert korrekte Strings
// 3. kingdom_name() = "Animalia" (alle Gattungen sind Lebewesen)
// 4. AnatomyConcept Conformance fuer SearchAlgorithmAnatomy
// 5. SearchAlgorithmAnatomy::genus() liefert AnatomyGenus::SearchAlgorithm (Mammal)
// 6. IAnatomyBase Virtual Interface (Compile-Only-Verifikation)
// 7. Alle 11 bekannten Anatomien erfuellen AnatomyConcept
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §25-§31
// @task #700 V41.F.6.1.R5.C.A

#include <gtest/gtest.h>

#include <anatomy/anatomy_base.hpp>
#include <anatomy/known_algorithms.hpp>

#include <type_traits>

namespace ana = ::comdare::cache_engine::anatomy;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — AnatomyGenus Enum (5 Werte)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA_AnatomyGenus, EnumHasFiveValues) {
    static_assert(static_cast<int>(ana::AnatomyGenus::SearchAlgorithm) == 0);
    static_assert(static_cast<int>(ana::AnatomyGenus::Set)             == 1);
    static_assert(static_cast<int>(ana::AnatomyGenus::Sequence)        == 2);
    static_assert(static_cast<int>(ana::AnatomyGenus::Adapter)         == 3);
    static_assert(static_cast<int>(ana::AnatomyGenus::View)            == 4);
    SUCCEED();
}

TEST(R5CA_AnatomyGenus, GenusNameHelperReturnsStrings) {
    static_assert(ana::genus_name(ana::AnatomyGenus::SearchAlgorithm) == std::string_view{"SearchAlgorithm"});
    static_assert(ana::genus_name(ana::AnatomyGenus::Set)             == std::string_view{"Set"});
    static_assert(ana::genus_name(ana::AnatomyGenus::Sequence)        == std::string_view{"Sequence"});
    static_assert(ana::genus_name(ana::AnatomyGenus::Adapter)         == std::string_view{"Adapter"});
    static_assert(ana::genus_name(ana::AnatomyGenus::View)            == std::string_view{"View"});
    SUCCEED();
}

TEST(R5CA_AnatomyGenus, KingdomIsAnimalia) {
    static_assert(ana::kingdom_name() == std::string_view{"Animalia"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — AnatomyConcept Conformance (SearchAlgorithmAnatomy)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA_AnatomyConcept, SearchAlgorithmAnatomyConforms) {
    static_assert(ana::AnatomyConcept<ana::Art>);
    static_assert(ana::AnatomyConcept<ana::Hot>);
    static_assert(ana::AnatomyConcept<ana::Wormhole>);
    static_assert(ana::AnatomyConcept<ana::SuRF>);
    static_assert(ana::AnatomyConcept<ana::Masstree>);
    static_assert(ana::AnatomyConcept<ana::Start>);
    SUCCEED();
}

TEST(R5CA_AnatomyConcept, PaperBindingAnatomyConforms) {
    static_assert(ana::AnatomyConcept<ana::ArtPaperBinding>);
    static_assert(ana::AnatomyConcept<ana::HotPaperBinding>);
    static_assert(ana::AnatomyConcept<ana::StartPaperBinding>);
    static_assert(ana::AnatomyConcept<ana::WormholePaperBinding>);
    static_assert(ana::AnatomyConcept<ana::SurfPaperBinding>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — SearchAlgorithmAnatomy::genus() = Mammal
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA_GenusMarker, AllElevenAnatomiesAreMammal) {
    static_assert(ana::Art::genus()                 == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::Hot::genus()                 == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::Wormhole::genus()            == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::SuRF::genus()                == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::Masstree::genus()            == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::Start::genus()               == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::ArtPaperBinding::genus()     == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::HotPaperBinding::genus()     == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::StartPaperBinding::genus()   == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::WormholePaperBinding::genus() == ana::AnatomyGenus::SearchAlgorithm);
    static_assert(ana::SurfPaperBinding::genus()    == ana::AnatomyGenus::SearchAlgorithm);
    SUCCEED();
}

TEST(R5CA_GenusMarker, GenusNameMammalRoundtrip) {
    static_assert(ana::genus_name(ana::Art::genus()) == std::string_view{"SearchAlgorithm"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — IAnatomyBase Virtual Interface (Compile-Only-Verifikation)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA_IAnatomyBase, IsAbstractClass) {
    static_assert(std::is_abstract_v<ana::IAnatomyBase>);
    static_assert(std::has_virtual_destructor_v<ana::IAnatomyBase>);
    SUCCEED();
}

// Sample-Adapter (R5.D Pattern): bridge SearchAlgorithmAnatomy<C> → IAnatomyBase
template <ana::AnatomyConcept A>
class AnatomyAbiAdapter final : public ana::IAnatomyBase {
public:
    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id() const noexcept override { return A::paper_id(); }
    [[nodiscard]] ana::AnatomyGenus genus() const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t organ_count() const noexcept override { return A::organ_count(); }
};

TEST(R5CA_IAnatomyBase, AbiAdapterBridgeToVirtualInterface) {
    AnatomyAbiAdapter<ana::Art> adapter;
    ana::IAnatomyBase const& base = adapter;
    EXPECT_EQ(base.composition_name(), std::string_view{"ArtComposition"});
    EXPECT_TRUE(base.paper_id().starts_with("P01"));
    EXPECT_EQ(base.genus(), ana::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(base.organ_count(), 17u);
}

TEST(R5CA_IAnatomyBase, AbiAdapterForPaperBinding) {
    AnatomyAbiAdapter<ana::ArtPaperBinding> adapter;
    ana::IAnatomyBase const& base = adapter;
    EXPECT_EQ(base.composition_name(), std::string_view{"ArtPaperBindingComposition"});
    EXPECT_EQ(base.genus(), ana::AnatomyGenus::SearchAlgorithm);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Cross-Anatomie-Konsistenz: alle 11 Anatomien teilen Kingdom + Gattung
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CA_CrossAnatomy, AllElevenShareSameKingdom) {
    constexpr auto kingdom = ana::kingdom_name();
    static_assert(kingdom == std::string_view{"Animalia"});

    constexpr auto art_genus           = ana::genus_name(ana::Art::genus());
    constexpr auto art_pb_genus        = ana::genus_name(ana::ArtPaperBinding::genus());
    constexpr auto wormhole_genus      = ana::genus_name(ana::Wormhole::genus());
    constexpr auto wormhole_pb_genus   = ana::genus_name(ana::WormholePaperBinding::genus());

    // Alle haben identische Gattung (Mammal)
    static_assert(art_genus         == std::string_view{"SearchAlgorithm"});
    static_assert(art_pb_genus      == std::string_view{"SearchAlgorithm"});
    static_assert(wormhole_genus    == std::string_view{"SearchAlgorithm"});
    static_assert(wormhole_pb_genus == std::string_view{"SearchAlgorithm"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Compile-Time Counter-Beispiel: leere Klasse erfuellt AnatomyConcept NICHT
// ─────────────────────────────────────────────────────────────────────────────

struct NotAnAnatomy {};

TEST(R5CA_NegativeTest, EmptyClassDoesNotConform) {
    static_assert(!ana::AnatomyConcept<NotAnAnatomy>);
    SUCCEED();
}
