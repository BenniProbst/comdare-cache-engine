// test_warn2a_block_id_bidirektional -- Warnungs-Review Runde 1c/2a (09.08.2026).
//
// DER BEFUND, den dieser Test festhaelt: `AxisLevel::block_id` ist die Rueck-Referenz eines Knotens
// auf seinen AxisBlock ("Bidirektionalitaet", experiment_tree.hpp:196). Die Konvention lautet
// block_id == axis. Der COMPILE-TIME-Pfad setzt sie (axis_reflect.hpp:42), und drei Tests pruefen
// sie -- test_br1_subset, test_br1_full22_count ("jeder Knoten block_id()==axis()") und
// test_harness_compile ("!d.block_id().empty()").
//
// Der PROFIL-Pfad (build_axis_levels) setzte sie NICHT. Aufgefallen ist das als
// -Wmissing-field-initializers an sieben Stellen. Als Speicher-Warnung war das Rauschen --
// std::string wird wertinitialisiert, es gibt keinen unbestimmten Wert. Als FACHLOGIK war es ein
// Riss: profil-erzeugte Baeume verletzten eine Zusicherung, die drei Tests fuehren -- nur bauen
// jene Tests ihre Baeume ueber den compile-time-Pfad und konnten den Bruch nie sehen. Ein
// richtiges Messgeraet am falschen Gegenstand.
//
// WARUM EIN TEST UND NICHT NUR DIE GEHEILTE WARNUNG: der Uebersetzer sah SIEBEN Stellen, es sind
// ACHT. Die tier-Ebene default-konstruiert ihr AxisLevel und weist danach zu, statt zu aggregat-
// initialisieren -- -Wmissing-field-initializers greift dort strukturell nicht, block_id blieb
// aber genauso leer. Die Warnung war der Anlass; die Zusicherung ist der Gegenstand.
//
// GOLDEN-NEUTRAL, nachgesehen statt vermutet: block_id geht nicht in den Schluessel ein
// (StaticAxisNode::serialize() == `axis_ + "=" + value_`), also weder in die binary_id noch in
// den golden-CRC.

#include <builder/experiment_tree/experiment_tree.hpp>
#include <builder/experiment_tree/profile_to_tree.hpp>

#include "xml_config_parser/xml_config_parser.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;
namespace cx = comdare::builder::xml;

namespace {

/// Ein Profil, das JEDEN Erzeugungszweig von build_axis_levels trifft: tier, die drei
/// cacheline-Unterachsen, node_width, die zwei alloc_hw-Unterachsen und eine Organ-Achse.
/// Werte sind ueberall EXPLIZIT, damit keine AxisRegistry noetig ist (der Test haengt an keiner XML).
[[nodiscard]] cx::ThesisProfile profil_mit_allen_zweigen() {
    cx::ThesisProfile tp;
    tp.id = "warn2a_probe";

    cx::ThesisTier tier;
    tier.id = "T_probe";
    tp.base_tiers.push_back(tier);

    cx::ThesisAxisSpec cacheline;
    cacheline.ref               = "cacheline";
    cacheline.line_sizes        = {"64", "128"};
    cacheline.alignments        = {"cacheline_aligned"};
    cacheline.sw_prefetch_hints = {"T0"};
    tp.permute_axes.push_back(cacheline);

    cx::ThesisAxisSpec node_width;
    node_width.ref            = "node_width";
    node_width.width_in_lines = {"1", "4"};
    tp.permute_axes.push_back(node_width);

    cx::ThesisAxisSpec alloc_hw;
    alloc_hw.ref              = "alloc_hw";
    alloc_hw.alloc_numa_nodes = {"auto"};
    alloc_hw.alloc_pages      = {"4k"};
    tp.permute_axes.push_back(alloc_hw);

    cx::ThesisAxisSpec organ; // eine der 18 Organ-Kompositions-Achsen
    organ.ref    = "allocator";
    organ.values = {"mimalloc", "jemalloc"};
    tp.permute_axes.push_back(organ);

    return tp;
}

/// Das ORAKEL. Es kommt NICHT aus dem Prueflind: die Regel ist aus experiment_tree.hpp/axis_reflect.hpp
/// abgelesen und hier eigenstaendig formuliert -- block_id ist gesetzt und gleich der Achse.
[[nodiscard]] bool traegt_seinen_block(ex::AxisLevel const& l) { return !l.block_id.empty() && l.block_id == l.axis; }

struct Bilanz {
    std::size_t              statische = 0; // der NENNER
    std::size_t              konform   = 0; // der ZAEHLER
    std::vector<std::string> verletzer;
};

[[nodiscard]] Bilanz bilanziere(std::vector<ex::AxisLevel> const& levels) {
    Bilanz b;
    for (auto const& l : levels) {
        if (!l.is_static) continue;
        ++b.statische;
        if (traegt_seinen_block(l))
            ++b.konform;
        else
            b.verletzer.push_back(l.axis + " -> block_id='" + l.block_id + "'");
    }
    return b;
}

} // namespace

// ── DIE ZUSICHERUNG ────────────────────────────────────────────────────────────────────────────
TEST(Warn2aBlockIdBidirektional, JedeProfilErzeugteStatischeEbeneTraegtIhrenBlock) {
    ex::AxisRegistry const leere_registry; // Werte sind explizit -> Registry wird nicht gebraucht
    auto const             levels = ex::build_axis_levels(profil_mit_allen_zweigen(), "", leere_registry);

    Bilanz const b = bilanziere(levels);

    // DER NENNER GEHOERT IN DIE AUSGABE -- eine nackte Null waere von einem echten Freispruch
    // nicht zu unterscheiden.
    std::string bericht = "block_id gesetzt bei " + std::to_string(b.konform) + " von " + std::to_string(b.statische) +
                          " statischen Ebenen";
    for (auto const& v : b.verletzer) bericht += "\n  VERLETZER: " + v;

    // Waechst der Baum nicht, misst der Test nichts: die Grundgesamtheit selbst wird geprueft.
    // 8 = tier + 3x cacheline + 1x node_width + 2x alloc_hw + 1x Organ-Achse.
    EXPECT_EQ(b.statische, 8u) << "Grundgesamtheit unerwartet -- " << bericht;
    EXPECT_EQ(b.konform, b.statische) << bericht;
}

// ── DER GEGENEINGANG (T-4): ein Eingang, bei dem die Zusicherung FAELLT ─────────────────────────
// Ohne ihn koennte `traegt_seinen_block` konstant true liefern und der Test waere blind.
// Hier wird GENAU der Zustand nachgestellt, den der Profil-Pfad vor der Heilung erzeugte.
TEST(Warn2aBlockIdBidirektional, DasOrakelUnterscheidetTatsaechlich) {
    std::vector<ex::AxisLevel> vorher_zustand;
    // exakt die alte Aggregat-Initialisierung: vier Elemente, block_id bleibt leer
    vorher_zustand.push_back(ex::AxisLevel{"cacheline.line_size", {"64"}, /*is_static=*/true, ""});
    vorher_zustand.push_back(ex::AxisLevel{"allocator", {"mimalloc"}, /*is_static=*/true, ""});

    Bilanz const b = bilanziere(vorher_zustand);
    EXPECT_EQ(b.statische, 2u);
    EXPECT_EQ(b.konform, 0u) << "Das Orakel haelt den UNGEHEILTEN Zustand faelschlich fuer konform "
                                "-- dann bewiese der Test oben nichts.";

    // Und die dritte Moeglichkeit: gesetzt, aber auf den FALSCHEN Block. Auch das muss auffallen,
    // sonst pruefte der Test nur "nicht leer" statt der Bidirektionalitaet.
    std::vector<ex::AxisLevel> falscher_block;
    falscher_block.push_back(ex::AxisLevel{"allocator", {"mimalloc"}, /*is_static=*/true, "", "prefetch"});
    EXPECT_EQ(bilanziere(falscher_block).konform, 0u)
        << "block_id != axis wurde durchgewunken -- die Zusicherung waere auf '!= leer' verduennt.";
}
