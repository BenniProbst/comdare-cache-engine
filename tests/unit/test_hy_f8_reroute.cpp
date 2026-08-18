// test_hy_f8_reroute.cpp -- HY-A2 F8-MINIMAL-DoD: der Reroute-Roundtrip ueber GENAU EIN Standard-Dock
// auf ZWEI plain-Tier-Ziele.
//
// WAS DIESER TEST BEWEIST, und warum jeder Teil noetig ist:
//   (1) ROUNDTRIP-KOEDER DURCH DAS DOCK -- ein GEWUERFELTES Token laeuft ueber den Antriebs-Zeiger,
//       den der DOCK-SLOT haelt (proxy.antrieb(slot), IObservableTier -> IDriveableTier), ins Ziel
//       und kommt literal gleich zurueck. A2.5-Fix 5 (Review #15): vorher nahm der Koeder einen
//       LOKALEN Stack-Zeiger auf das Ziel und lief am Dock VORBEI -- ein Dock, das den gebundenen
//       Zeiger verliert oder nie speichert, waere gruen geblieben. Ein fester Wert waere zudem kein
//       Koeder: er koennte aus einem Default, einem Nullwert oder einem stehengebliebenen Puffer
//       stammen und saehe genauso aus. Der Wert wird deshalb aus einem deterministisch geseedeten
//       PRNG gezogen (reproduzierbar, aber nicht vorhersehbar) und VOR/NACH dem Dock verglichen.
//   (2) UMSCHALTUNG -- dasselbe Dock traegt nacheinander ZWEI verschiedene Ziele. Ohne den zweiten
//       Durchgang bewiese der Test nur, dass irgendein Zeiger irgendwo hinzeigt; erst der Wechsel
//       zeigt, dass der Umschaltpunkt WIRKT und das Ergebnis dem NEUEN Ziel folgt.
//   (3) DOCK-ZAHL VORHER/NACHHER -- die Abnahme-Zahl der DoD, literal gedruckt.
//   (4) DELEGATION statt Eigenleben -- der Proxy meldet den Lebenszyklus des ZIELS. Die Probe
//       schaltet das Ziel auf Running und liest die Antwort AM PROXY: kaeme sie aus einem eigenen
//       state_, bliebe sie Idle. Genau das ist die zweite Wahrheit, die der Delegations-Schnitt
//       verhindert (K2).
//   (5) DIE B4-PAARE -- beide Ebenen, im selben Lauf, damit die Zahlen nicht in zwei Berichten
//       auseinanderlaufen.
//
// DIE CT-SPERRE steht NICHT hier, sondern am Eigentuemer (hybrid_binary_proxy.hpp:62, RerouteZiel primaer
// undefiniert, + :96-115 die Negativ-Pins): ein nicht
// deklariertes Reroute-Ziel ist per Konstruktion unbaubar, und ein unbaubarer Aufruf laesst sich in
// einer Test-TU nur als auskommentierter Text ablegen, der nichts prueft. Was hier geprueft wird,
// ist die BEDINGUNG, unter der die Sperre bestehen bleibt -- dieselbe Bauform wie bei S-6b.

#include <hybrid/hybrid_binary_proxy.hpp>

#include <anatomy/observable_tier.hpp> // IObservableTier (Dock-Flaeche) + SnapshotSink/push_snapshot_to_visitor
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <iostream> // D-F8: std::cout wird hier direkt benutzt -- Include gehoert in DIESE TU
#include <map>
#include <random>
#include <string_view>

namespace {

namespace hy     = ::comdare::cache_engine::hybrid;
namespace cea    = ::comdare::cache_engine::anatomy;
namespace cee    = ::comdare::cache_engine::execution_engine;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;

/// Ein plain Tier-ZIEL. Bewusst winzig: der Test misst die REROUTE-Naht, nicht eine Anatomie. Es
/// erfuellt genau die zwei Flaechen, die der Proxy braucht -- IAnatomyBase (Delegations-Ziel/basis)
/// und IObservableTier (die Antriebs-Flaeche, die der DOCK-SLOT fuehrt; sie erbt IDriveableTier,
/// worueber der funktionale Koeder laeuft). A2.5-Fix 5: vorher erbte das Fixture nur IDriveableTier
/// und konnte gar nicht als Antrieb gebunden werden -- der Test band (slot, nullptr, &ziel) und der
/// Dock-Pfad blieb ungemessen. Der genus-Parameter existiert fuer die Fremd-Genus-Ablehnungs-Probe
/// (ziel_binden-Wache (b)); der Default ist das deklarierte Reroute-Ziel dieses Tests.
class PlainZiel final : public cea::IAnatomyBase, public cea::IObservableTier {
public:
    explicit PlainZiel(std::string_view name, std::size_t organe,
                       cea::AnatomyGenus genus = cea::AnatomyGenus::SearchAlgorithm) noexcept
        : name_{name}, organe_{organe}, genus_{genus} {}

    // -- IExecutionEngine (das ECHTE Eigenleben -- der Proxy hat keines) --
    [[nodiscard]] std::string_view          engine_name() const noexcept override { return name_; }
    [[nodiscard]] cee::EngineLifecycleState lifecycle_state() const noexcept override { return zustand_; }
    void                                    warm_up() override { zustand_ = cee::EngineLifecycleState::Warming; }
    void                                    run() override { zustand_ = cee::EngineLifecycleState::Running; }
    void                                    reset() override { zustand_ = cee::EngineLifecycleState::Idle; }
    void                                    shutdown() override { zustand_ = cee::EngineLifecycleState::Shutdown; }

    // -- IAnatomyBase --
    [[nodiscard]] std::string_view  composition_name() const noexcept override { return name_; }
    [[nodiscard]] std::string_view  paper_id() const noexcept override { return "plain-fixture"; }
    [[nodiscard]] cea::AnatomyGenus genus() const noexcept override { return genus_; }
    [[nodiscard]] std::size_t       organ_count() const noexcept override { return organe_; }

    // -- IObservableTier: die Mess-Flaeche der Dock-Bindung, hier minimal aber EHRLICH bedient --
    void tier_observe(cea::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out == nullptr) return;
        *out                 = cea::ComdareTierObserverSnapshot{};
        out->tier_fill_level = static_cast<std::uint64_t>(daten_.size());
    }
    void tier_measure_accept(cea::IMessVisitor& v) const noexcept override {
        // Die EINE Umsetzung POD -> Ereignis-Strom (kein eigener Emissions-Block, s. observable_tier.hpp).
        cea::ComdareTierObserverSnapshot s{};
        tier_observe(&s);
        cea::push_snapshot_to_visitor(s, v, static_cast<std::uint64_t>(genus_));
    }
    void tier_reset_statistics() noexcept override {} // Fixture fuehrt keine kumulativen Zaehler

    // -- IDriveableTier: der funktionale Antrieb, ueber den der Koeder laeuft --
    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
        daten_[key] = value;
        return true;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        auto const it = daten_.find(key);
        if (it == daten_.end()) return false;
        if (out_value != nullptr) *out_value = it->second;
        return true;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t key) noexcept override { return daten_.erase(key) != 0; }
    void                        tier_clear() noexcept override { daten_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(daten_.size());
    }

private:
    std::string_view                       name_;
    std::size_t                            organe_;
    cea::AnatomyGenus                      genus_;
    cee::EngineLifecycleState              zustand_ = cee::EngineLifecycleState::Idle;
    std::map<std::uint64_t, std::uint64_t> daten_{};
};

/// Der Koeder-Wert. Deterministisch geseedet (reproduzierbar), aber nicht als Literal im Code --
/// waere er eins, koennte ein Vergleich gegen einen stehengebliebenen Puffer zufaellig passen.
[[nodiscard]] std::uint64_t gewuerfeltes_token(std::uint64_t seed) {
    std::mt19937_64                              rng{seed};
    std::uniform_int_distribution<std::uint64_t> dist{1, ~std::uint64_t{0}};
    return dist(rng);
}

} // namespace

TEST(HyF8Reroute, EinDockZweiZieleRoundtripKoeder) {
    hy::HybridBinaryProxy<cea::AnatomyGenus::SearchAlgorithm> proxy;

    // (3) DOCK-ZAHL VORHER/NACHHER -- die Abnahme-Zahl der F8-DoD.
    EXPECT_EQ(proxy.belegte_docks(), 0u) << "vor dem attach traegt der Proxy kein Dock";
    int const slot = proxy.dock_anlegen();
    ASSERT_GE(slot, 0) << "attach lieferte -status statt eines Slot-Index";
    EXPECT_EQ(proxy.belegte_docks(), 1u) << "F8-DoD: GENAU EIN Standard-Dock";
    EXPECT_EQ(proxy.dock_kapazitaet(), 1u);
    std::cout << "  F8-DoD Dock-Zahl: vorher 0 -> nachher " << proxy.belegte_docks() << " (Kapazitaet "
              << proxy.dock_kapazitaet() << ")\n";

    // Ungebunden: ehrliche Sentinels statt Dereferenzierung im Fenster zwischen attach und Bindung.
    EXPECT_FALSE(proxy.ist_gebunden());
    EXPECT_EQ(proxy.engine_name(), std::string_view{"hybrid_ungebunden"});
    EXPECT_EQ(proxy.organ_count(), 0u);

    PlainZiel ziel_a{"plain_ziel_a", 18};
    PlainZiel ziel_b{"plain_ziel_b", 18};

    // ---- (1)+(2) ROUNDTRIP-KOEDER durch das Dock, zweimal, auf ZWEI Ziele --------------------
    // VOID-Lambda mit Ausgabe-Parameter, nicht bool-Rueckgabe: ASSERT_* expandiert zu `return;` und
    // waere in einem Lambda mit Rueckgabetyp ein Typfehler (am Objekt gemessen: "inconsistent types
    // 'void' and 'bool' deduced for lambda return type"). Der Befund wandert deshalb ueber eine
    // Referenz heraus -- die Abbruch-Semantik der ASSERTs bleibt damit erhalten.
    auto koeder_durch_das_dock = [&](PlainZiel& ziel, std::uint64_t seed, char const* etikett, bool& gleich) {
        auto const token = gewuerfeltes_token(seed);
        auto const key   = gewuerfeltes_token(seed ^ 0x5DEECE66Dull);

        // A2.5-Fix 5: BEIDE Zeiger auf DASSELBE Objekt (der Vertrag von ziel_binden) -- Antrieb als
        // IObservableTier, Basis als IAnatomyBase. Die fruehere Bindung (slot, nullptr, &ziel) war
        // ein GEMISCHTES Paar und wird seit Fix 4 mit hybrid_status_bindung_inkonsistent abgewiesen.
        cea::IObservableTier* obs = &ziel;
        ASSERT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), obs, &ziel), hy::hybrid_status_ok);

        // DER DOCK-GRIFF: der Antrieb kommt AUS DEM SLOT, nicht aus einer lokalen Variablen. Nur so
        // misst der Koeder die Dock-Weiterleitung -- ein Slot, der den Zeiger nicht speichert oder
        // nicht zurueckgibt, faellt HIER (frueherer toter ASSERT_NE auf &ziel ist gestrichen; diese
        // Gleichheit ist die lebendige Fassung derselben Frage und deckt nullptr mit ab).
        auto* am_dock = proxy.antrieb(static_cast<std::size_t>(slot));
        ASSERT_EQ(am_dock, obs) << etikett << ": der Dock-Slot haelt NICHT den gebundenen Antrieb";
        cea::IDriveableTier* antrieb = am_dock; // IObservableTier erbt den funktionalen Antrieb

        ASSERT_TRUE(antrieb->tier_insert(key, token)) << etikett << ": insert durch das Dock";
        std::uint64_t zurueck = 0;
        ASSERT_TRUE(antrieb->tier_lookup(key, &zurueck)) << etikett << ": lookup durch das Dock";
        EXPECT_EQ(zurueck, token) << etikett << ": das Token kam NICHT literal gleich zurueck";
        std::cout << "  Koeder " << etikett << ": key=" << key << " token=" << token << " zurueck=" << zurueck
                  << (zurueck == token ? "  [GLEICH]" : "  [ABWEICHUNG]") << "\n";

        // Die DELEGATION am selben Griff: der Proxy nennt jetzt DIESES Ziel.
        EXPECT_TRUE(proxy.ist_gebunden());
        EXPECT_EQ(proxy.engine_name(), ziel.engine_name()) << etikett << ": engine_name wird NICHT delegiert";
        EXPECT_EQ(proxy.organ_count(), ziel.organ_count());
        gleich = (zurueck == token);
    };

    bool gleich_a = false;
    bool gleich_b = false;
    koeder_durch_das_dock(ziel_a, 0xA11CEull, "ziel_a", gleich_a);
    koeder_durch_das_dock(ziel_b, 0xB0B0ull, "ziel_b", gleich_b);
    EXPECT_TRUE(gleich_a) << "Roundtrip ueber ziel_a: Token kam nicht literal gleich zurueck";
    EXPECT_TRUE(gleich_b) << "Roundtrip ueber ziel_b: Token kam nicht literal gleich zurueck";

    // Die UMSCHALTUNG hat wirklich stattgefunden -- der Proxy nennt das zweite Ziel, nicht das erste.
    EXPECT_EQ(proxy.engine_name(), std::string_view{"plain_ziel_b"})
        << "nach dem Umbinden meldet der Proxy noch das ERSTE Ziel -- der Umschaltpunkt wirkt nicht";

    // ---- (4) DELEGATION statt Eigenleben ------------------------------------------------------
    // Der Proxy hat KEIN state_. Wer den Lebenszyklus am ZIEL dreht, sieht es am PROXY.
    EXPECT_EQ(proxy.lifecycle_state(), cee::EngineLifecycleState::Idle);
    ziel_b.run();
    EXPECT_EQ(proxy.lifecycle_state(), cee::EngineLifecycleState::Running)
        << "der Proxy fuehrt einen EIGENEN Lebenszyklus -- das ist die zweite Wahrheit, gegen die der "
           "Delegations-Schnitt gebaut ist (an warm_up haengt die Mess-Semantik)";
    // Und umgekehrt: der Proxy-Aufruf landet am Ziel, nicht in einem eigenen Feld.
    proxy.reset();
    EXPECT_EQ(ziel_b.lifecycle_state(), cee::EngineLifecycleState::Idle)
        << "proxy.reset() hat das Ziel NICHT erreicht -- die Delegation ist einseitig";

    // ---- WEG C: genus() ist das GEERBTE Ziel-Genus, nie der Reroute-Wert ----------------------
    EXPECT_EQ(proxy.genus(), cea::AnatomyGenus::SearchAlgorithm);
    EXPECT_NE(proxy.genus(), cea::AnatomyGenus::FunctionInterfaceReroute)
        << "genus() liefert den Reroute-Genus -- damit sucht der Host ein Pruef-Dock, das es nicht "
           "gibt (Weg C, Owner-Entscheid E-1 final)";

    // ---- LOESEN vor dem Entladen (destroy-vor-dlclose) ---------------------------------------
    // VOR dem Loesen haelt der Slot den Antrieb des ZULETZT gebundenen Ziels -- erst dieses Paar
    // aus Vorher- und Nachher-Messung macht die nullptr-Erwartung unten zu einem UEBERGANG statt
    // zu einem Dauerzustand (A2.5-Fix 5: vorher war nie ein Antrieb gebunden, die Zeile mass nichts).
    EXPECT_EQ(proxy.antrieb(static_cast<std::size_t>(slot)), static_cast<cea::IObservableTier*>(&ziel_b))
        << "vor dem Loesen muss der Slot den Antrieb von ziel_b halten";
    ASSERT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), nullptr, nullptr), hy::hybrid_status_ok);
    EXPECT_FALSE(proxy.ist_gebunden());
    EXPECT_EQ(proxy.antrieb(static_cast<std::size_t>(slot)), nullptr)
        << "der Slot haelt nach dem Loesen noch einen Zeiger -- das waere ein use-after-dlclose mit "
           "voll plausiblem Verhalten bis zum ersten Zugriff";
    EXPECT_EQ(proxy.organ_count(), 0u) << "ohne Ziel traegt der Proxy keine Organe";
}

TEST(HyF8Reroute, ZielBindenWeistGemischtesPaarUndFremdGenusAb) {
    // A2.5-Fix 4/5 (Review #15, KOPPLUNG): die zwei ziel_binden-Wachen werden HIER gepinnt -- die
    // Proxy-Haelfte (hybrid_binary_proxy.hpp) landete im selben Zug, dieser Test ist ihre Messung.
    hy::HybridBinaryProxy<cea::AnatomyGenus::SearchAlgorithm> proxy;
    int const                                                 slot = proxy.dock_anlegen();
    ASSERT_GE(slot, 0);

    PlainZiel             ziel{"plain_ziel_wache", 18};
    cea::IObservableTier* obs = &ziel;

    // (a) GEMISCHTES PAAR, beide Richtungen: "nullptr in BEIDEN heisst geloest" -- einer allein ist
    // keiner der zwei dokumentierten Zustaende und muss VOR jeder Mutation abgewiesen werden.
    EXPECT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), nullptr, &ziel), hy::hybrid_status_bindung_inkonsistent)
        << "antrieb=nullptr bei gesetzter basis: antrieb und ziel_ zeigten fortan auf verschiedene Wahrheiten";
    EXPECT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), obs, nullptr), hy::hybrid_status_bindung_inkonsistent)
        << "basis=nullptr bei gesetztem antrieb: die Gegenrichtung desselben Verstosses";
    EXPECT_FALSE(proxy.ist_gebunden()) << "die Ablehnung kam NACH einer Mutation -- ziel_ wurde beschrieben";
    EXPECT_EQ(proxy.antrieb(static_cast<std::size_t>(slot)), nullptr)
        << "die Ablehnung kam NACH einer Mutation -- der Dock-Slot wurde beschrieben";

    // (b) FREMD-GENUS: ein basis mit genus() != ZielGenus wuerde die Loader-Riegel-Garantie
    // (Symbol == genus()) am gebundenen Proxy nachtraeglich entwerten -- genus() delegiert an ziel_.
    PlainZiel fremd{"plain_ziel_fremd", 13, cea::AnatomyGenus::Set};
    EXPECT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), static_cast<cea::IObservableTier*>(&fremd), &fremd),
              hy::hybrid_status_kein_zielfaehiges_genus)
        << "ein Set-Ziel am SearchAlgorithm-Proxy: genus() wechselte still auf das Fremd-Genus";
    EXPECT_FALSE(proxy.ist_gebunden());
    EXPECT_EQ(proxy.antrieb(static_cast<std::size_t>(slot)), nullptr);

    // GEGENPROBE: das dokumentierte Paar bindet weiterhin, und das Loesen-Paar bleibt zulaessig --
    // sonst haette die Wache mehr verboten, als der Vertrag beschreibt.
    EXPECT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), obs, &ziel), hy::hybrid_status_ok);
    EXPECT_TRUE(proxy.ist_gebunden());
    EXPECT_EQ(proxy.ziel_binden(static_cast<std::size_t>(slot), nullptr, nullptr), hy::hybrid_status_ok);
    EXPECT_FALSE(proxy.ist_gebunden());
    std::cout << "  ziel_binden-Wachen: gemischtes Paar -> " << hy::hybrid_status_bindung_inkonsistent
              << ", Fremd-Genus -> " << hy::hybrid_status_kein_zielfaehiges_genus << ", Paar/Loesen -> ok\n";
}

TEST(HyF8Reroute, CtSperreUnterscheidetNoch) {
    // Die Bedingung, unter der die Sperre bestehen BLEIBT (die Verletzung selbst ist unbaubar).
    // BEIDE zulaessigen und ALLE unzulaessigen -- ein Praedikat, das nur die Ja-Faelle prueft,
    // koennte auf immer-wahr degeneriert sein, ohne dass es hier auffiele.
    static_assert(hy::reroute_ziel_deklariert<cea::AnatomyGenus::SearchAlgorithm>());
    static_assert(hy::reroute_ziel_deklariert<cea::AnatomyGenus::Set>());
    static_assert(!hy::reroute_ziel_deklariert<cea::AnatomyGenus::FunctionInterfaceReroute>(),
                  "Gate S1: der Reroute-Genus ist NIE ein Ziel.");
    static_assert(!hy::reroute_ziel_deklariert<cea::AnatomyGenus::View>(),
                  "die Sperre darf nicht auf immer-wahr degenerieren.");
    static_assert(!hy::reroute_ziel_deklariert<cea::AnatomyGenus::Sequence>());
    static_assert(!hy::reroute_ziel_deklariert<cea::AnatomyGenus::Adapter>());
    static_assert(hy::kDeklarierteRerouteZiele == 2, "F8-DoD: GENAU ZWEI deklarierte Ziele.");
    SUCCEED();
}

// --------------------------------------------------------------------------------------------
// DIE SECHS ABI-PFLICHT-SYMBOLE -- am GELADENEN Objekt, nicht am Symbolnamen
// --------------------------------------------------------------------------------------------
//
// WARUM NICHT `nm`: dass ein Symbol im Export-Verzeichnis STEHT, sagt nicht, dass es das Richtige
// tut. AnatomyModuleLoader::load ruft alle sechs der Reihe nach auf und erreicht status_ok nur, wenn
// jedes davon liefert. Seine Status-Namen benennen zugleich, WELCHES Symbol versagt hat --
// symbol_not_found (eines der vier Ur-Pflicht-Symbole fehlt), magic_mismatch / abi_major_mismatch
// (version/magic antworten falsch), gattung_symbol_missing / genus_symbol_missing (eines der zwei
// Identitaets-Symbole fehlt, Q2/V-06), factory_returned_null (create liefert nichts). Ein status_ok
// ist damit die staerkere Aussage, und sie kostet nichts, weil der Loader ohnehin existiert.
//
// WAS DIESER TEST ZUSAETZLICH ZEIGT und der In-Process-Test NICHT zeigen kann: dass die Gleichheit
// der sechs Symbolnamen wirklich EINE Ladestrecke ergibt. Derselbe Loader, der die plain-Tier-.so
// laedt, nimmt hier eine Hybrid-.so -- ohne Fallunterscheidung, ohne Gattungs-Wissen vorab.
TEST(HyF8Reroute, SechsAbiPflichtSymboleAmGeladenenModul) {
    struct Fall {
        char const*       pfad;
        cea::AnatomyGenus erwartetes_ziel_genus;
        char const*       etikett;
    };
    Fall const faelle[] = {
        {COMDARE_HY_MODUL_SEARCHALGORITHM, cea::AnatomyGenus::SearchAlgorithm, "hybrid_reroute_searchalgorithm"},
        {COMDARE_HY_MODUL_SET, cea::AnatomyGenus::Set, "hybrid_reroute_set"},
    };

    for (auto const& f : faelle) {
        loader::AnatomyModuleHandle handle;
        int const                   st = loader::AnatomyModuleLoader::load(f.pfad, handle);
        ASSERT_EQ(st, loader::status_ok) << f.etikett << ": load == " << loader::status_name(st)
                                         << " -- eines der sechs "
                                         << "ABI-Pflicht-Symbole fehlt oder liefert nicht (Pfad " << f.pfad << ")";

        auto* anatomie = handle.anatomy();
        ASSERT_NE(anatomie, nullptr) << f.etikett << ": Factory lieferte nullptr";

        // WEG C am LEBENDEN Objekt: genus() meldet das ZIEL, nie den Reroute-Wert ...
        EXPECT_EQ(anatomie->genus(), f.erwartetes_ziel_genus)
            << f.etikett << ": das geladene Modul meldet ein anderes Ziel-Genus als deklariert";
        EXPECT_NE(anatomie->genus(), cea::AnatomyGenus::FunctionInterfaceReroute)
            << f.etikett << ": genus() liefert den Reroute-Wert -- Weg C gebrochen";

        // ... und WEG A daneben: die Identitaet nennt trotzdem den HYBRID. Genau diese beiden
        // Antworten NEBENEINANDER sind die Trennung der zwei Ebenen -- an einem Objekt, das
        // wirklich durch dlopen gekommen ist.
        EXPECT_EQ(anatomie->composition_name(), std::string_view{"HybridBinaryProxy"})
            << f.etikett << ": die Hybrid-.so ist im Log von einem plain Tier ununterscheidbar";
        EXPECT_EQ(anatomie->paper_id(), std::string_view{"HY-A2"});

        // Das Standard-Dock kam MIT aus der Factory -- eine Hybrid-.so ohne Aufnahmestelle waere
        // eine Huelle, die erst beim ersten Reroute-Versuch auffiele.
        auto* proxy = dynamic_cast<hy::HybridBinaryProxy<cea::AnatomyGenus::SearchAlgorithm>*>(anatomie);
        if (f.erwartetes_ziel_genus == cea::AnatomyGenus::SearchAlgorithm) {
            ASSERT_NE(proxy, nullptr) << f.etikett << ": das geladene Objekt ist kein HybridBinaryProxy";
            EXPECT_EQ(proxy->belegte_docks(), 1u) << f.etikett << ": GENAU EIN Standard-Dock aus der Factory";
            EXPECT_FALSE(proxy->ist_gebunden()) << f.etikett << ": frisch geladen ist kein Ziel gebunden";
        } else {
            // A2.5-Fix 7 (Review #15): die REST-HAZARD-Zelle des R1/ENABLE_EXPORTS-Umbaus wird
            // GEMESSEN, nicht nur ausgefuehrt: der Cast, der NICHT treffen darf. Deterministisch
            // unter gcc und clang, -O0 wie -O2 -- genau die Zelle, in der clangs
            // -fassume-unique-vtables-Zeigervergleich frueher die falsche vtable-Kopie traf.
            EXPECT_EQ(proxy, nullptr)
                << f.etikett
                << ": die Set-.so darf NICHT als SearchAlgorithm-Proxy casten (final-Cast ueber die dlopen-Grenze)";
        }

        std::cout << "  ABI-Pflicht-Symbole OK: " << f.etikett << "  genus=" << static_cast<int>(anatomie->genus())
                  << "  composition_name=" << anatomie->composition_name() << "  paper_id=" << anatomie->paper_id()
                  << "\n";
        // handle-Destruktor: destroy_anatomy VOR dlclose -- das vierte Symbol im Vollzug.
    }
}

TEST(HyF8Reroute, B4AbnahmeBeideEbenenPaare) {
    // Beide Ebenen im SELBEN Lauf -- damit die Zahlen nicht in zwei Berichten auseinanderlaufen.
    std::cout << "  B4-Paare: kAlleGattungen=" << hy::kAlleGattungen.size()
              << "  kAlleGenera=" << hy::kAlleGenera.size()
              << "  (davon ABI-sichtbar=" << hy::abi_sichtbare_genus_anzahl()
              << ", Klassifikation=" << hy::klassifikations_genus_anzahl() << ")\n"
              << "  Reroute-CT-Slot-Deckel: " << hy::kRerouteGenusCtSlotCount << "\n";
    EXPECT_EQ(hy::kAlleGattungen.size(), 4u);
    EXPECT_EQ(hy::kAlleGenera.size(), 6u);
    EXPECT_EQ(hy::abi_sichtbare_genus_anzahl(), 5u) << "die Dock-Registry bleibt FUENF";
    EXPECT_EQ(hy::klassifikations_genus_anzahl(), 1u);
    EXPECT_EQ(hy::kRerouteGenusCtSlotCount, 32u);
}
