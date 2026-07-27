// tests/unit/test_o4_machine_identity.cpp -- Lane C, Paket O-4 (Bauplan IV.2.6, Ledger §70.6).
// V-4 (27.07.): REGISTRIERT im ctest-Gate. Der Test lief bis dahin nur per Hand-Bau; der einzige
// genannte Grund war Paket-Disziplin des O-4-Pakets, kein inhaltlicher -- und ein Identitaets-Test
// ohne Gate faengt eine Regression erst, wenn ihn jemand zufaellig von Hand baut.
//
// WAS HIER BEWIESEN WIRD: dass die Maschinen-Identifikation die zwei Identitaets-Begriffe SAUBER
// trennt (Hostname = Instanz-Lookup, Eigenschafts-Tupel = Klassen-Identitaet, §70.6), dass sie NIE
// still auf eine fremde Identitaet zurueckfaellt, und dass die Live-Abfrage auf dieser Maschine
// wirklich das liefert, was deklariert ist.

#include <cache_engine/measurement/machine_identity.hpp>
// Der Abgrenzungs-Test unten greift auf die Gate-Hooks zu. machine_identity.hpp zieht das Gate
// ABSICHTLICH nicht (O-4 haengt sich nirgends ein) -- wer die Hooks pruefen will, holt sie selbst.
#include <cache_engine/measurement/simd_build_gate.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace meas = comdare::cache_engine::measurement;

// =================================================================================================
// 1. DIE BEGRIFFS-TRENNUNG (der Kern von §70.6)
// =================================================================================================
TEST(O4MachineIdentity, HostnameIstNichtDerSchluessel) {
    // Die XML fuehrt id="prod1" und hostname_hint="prod1". Beides sind NAMEN und duerfen die
    // Identitaet nicht aufloesen -- sonst waere der Stempel host-abhaengig und §70.6 gebrochen
    // ("gleiche Kombination => gleicher Stempel").
    EXPECT_EQ(meas::resolve_machine_by_properties("prod1", "ddr5_2x32"), nullptr);
    EXPECT_EQ(meas::resolve_machine_by_properties("prod1_zen5", "ddr5_2x32"), nullptr);
    // Nur das Eigenschafts-Tupel loest auf.
    ASSERT_NE(meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32"), nullptr);
    EXPECT_EQ(meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32")->machine_id,
              std::string_view{"prod1_zen5"});
}

TEST(O4MachineIdentity, DasTupelGiltALSGANZES) {
    // "exakter Eigenschafts-Match" (§70.6): ein halber Treffer ist kein Treffer.
    // E-2 (27.07.): prod1 UND prod2 tragen jetzt BEIDE ram_pair="ddr5_2x32". Die Kreuzprobe kann
    // deshalb nicht mehr ueber das RAM-Etikett laufen -- "intel_avx2 + ddr5_2x32" ist seither prod2,
    // also ein VOLL-Treffer. Geprueft werden stattdessen die zwei echten Halb-Treffer-Richtungen.
    EXPECT_EQ(meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr4_2x32"), nullptr)
        << "bekannte CPU-Fabrikation, aber ein ram_pair, das keine Zeile mehr fuehrt";
    EXPECT_EQ(meas::resolve_machine_by_properties("gibt_es_nicht", "ddr5_2x32"), nullptr)
        << "bekanntes ram_pair allein bindet NICHT -- sonst waere das Tupel halbiert";
    EXPECT_NE(meas::resolve_machine_by_properties("intel_avx2", "ddr5_2x32"), nullptr);
    // Die eigentliche E-2-Folge: seit beide Zeilen dasselbe ram_pair tragen, haengt ihre Trennung
    // ALLEIN an cpu_fabrication. Faellt diese Zusicherung, loesen prod1 und prod2 auf dieselbe Zeile auf.
    EXPECT_NE(meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32"),
              meas::resolve_machine_by_properties("intel_avx2", "ddr5_2x32"));
}

TEST(O4MachineIdentity, WiederverwendbarkeitUeberMaschinenTypenHinweg) {
    // Owner-Kern §70.6: "bei exaktem Eigenschafts-Match gilt die Achse als WIEDERVERWENDBAR (auch
    // ueber formal verschiedene Maschinentypen)". Zwei verschieden BENANNTE Hosts mit demselben
    // Tupel muessen auf DIESELBE Zeile zeigen -- der Resolver kennt Namen ja gar nicht.
    auto const* a = meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32");
    auto const* b = meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a, b) << "Gleiches Tupel MUSS dieselbe Identitaet liefern, unabhaengig vom Host.";
}

// =================================================================================================
// 2. KEIN STILLER FALLBACK
// =================================================================================================
TEST(O4MachineIdentity, UnbekannteMaschineLiefertKEINEIdentitaetUndKEINESignatur) {
    auto const r = meas::identify_machine("gibt_es_nicht", "auch_nicht", meas::kProd1Zen5Core);
    EXPECT_EQ(r.machine, nullptr);
    EXPECT_EQ(r.verdict, meas::MachineIdentityVerdict::UnbekannteMaschine);
    EXPECT_TRUE(r.signature().empty()) << "Ohne bestaetigte Identitaet darf NIE eine Signatur herausfallen.";
    // Und es ist ausdruecklich KEIN Fehler: ohne Identitaet gilt die Basis (CPU-only), die per
    // Halbordnung ueberall laeuft.
    EXPECT_FALSE(meas::error_class_of_verdict(r.verdict).has_value());
}

TEST(O4MachineIdentity, Prod2KernKennungIstMitO4aErhobenUndDeklariert) {
    // O-8 Schritt 5b (O-4a-VOLLZUG): DIESER Test stand hier invertiert -- er sicherte zu, dass prod2
    // NichtDeklariert meldet, solange die Kern-Kennung nicht erhoben war. Sie IST jetzt erhoben
    // (Infra-Handout, GenuineIntel/6/151/2 = Alder Lake-S, i9-12900K), also kehrt sich die Zusicherung
    // um. Die Umkehr ist der Punkt: der alte Test war kein Wunsch, sondern die ehrliche Beschreibung
    // eines Zwischenzustands, und der ist beendet.
    auto const* m = meas::resolve_machine_by_properties("intel_avx2", "ddr5_2x32");
    ASSERT_NE(m, nullptr);
    EXPECT_TRUE(m->core.declared) << "prod2s Kern-Kennung ist mit O-4a erhoben.";
    EXPECT_EQ(m->core.vendor, std::string_view{"GenuineIntel"});
    EXPECT_EQ(m->core.family, 6);
    EXPECT_EQ(m->core.model, 151);
    EXPECT_EQ(m->core.stepping, 2);
    // Der Brand bleibt BEWUSST leer: der live gelesene String von prod2 liegt nicht vor, und ein aus
    // der Modell-Bezeichnung konstruierter Praefix wuerde eine falsche Abweichung melden koennen.
    EXPECT_TRUE(m->core.brand.empty());
}

TEST(O4MachineIdentity, Prod2RamFrequenzIstErhobenUndCasBleibtEhrlichOffen) {
    // P7-Nachzug 27.07.: der zweite Teil der prod2-Erhebung. `dmidecode --type 17` liefert zwei
    // bestueckte Slots a 32 GB mit Speed == Configured Memory Speed == 4800 MT/s -- dieses Zusammenfallen
    // IST die Aussage "JEDEC-Basistakt, kein XMP/EXPO", also ein Ist-Wert und kein Modul-Etikett.
    // Die CAS-Latenz bleibt 0: SMBIOS Type 17 fuehrt sie nicht, decode-dimms ist auf prod2 nicht
    // installiert. Beide Zusicherungen zusammen sind der Punkt -- ein erhobenes Glied wird deklariert,
    // ein nicht erhobenes wird NICHT geschaetzt, nur weil daneben eine Zahl steht.
    auto const* m = meas::resolve_machine_by_properties("intel_avx2", "ddr5_2x32");
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->ram_frequency_mhz, 4800u);
    EXPECT_EQ(m->cas_latency_cl, 0u) << "0 heisst 'nicht erhoben' -- CL0 gibt es als realen Wert nicht.";

    // prod1 ist von dieser Erhebung NICHT beruehrt und bleibt vollstaendig undeklariert. Ohne diese
    // Gegenprobe koennte eine spaetere Sammel-Deklaration prod1 stillschweigend mitnehmen.
    auto const* p1 = meas::resolve_machine_by_properties("amd_zen5_avx512", "ddr5_2x32");
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1->ram_frequency_mhz, 0u);
    EXPECT_EQ(p1->cas_latency_cl, 0u);
}

TEST(O4MachineIdentity, Prod2MatchtDieEigeneDeklarationUndNichtsAnderes) {
    // HOST-UNABHAENGIG: die Gegenprobe laeuft gegen SYNTHETISCHE live-Werte, nicht gegen die CPU des
    // Testlaeufers -- sonst haenge das Ergebnis daran, auf welcher Kiste ctest gerade laeuft.
    auto const* m = meas::resolve_machine_by_properties("intel_avx2", "ddr5_2x32");
    ASSERT_NE(m, nullptr);

    meas::MachineCoreCpuId passend = m->core;
    passend.brand                  = "12th Gen Intel(R) Core(TM) i9-12900K"; // live-Brand darf reicher sein
    auto const treffer             = meas::identify_machine("intel_avx2", "ddr5_2x32", passend);
    EXPECT_EQ(treffer.verdict, meas::MachineIdentityVerdict::Match);
    EXPECT_FALSE(treffer.signature().empty()) << "Bei Match faellt die deklarierte Signatur heraus.";

    meas::MachineCoreCpuId fremd = m->core;
    fremd.model                  = 183; // andere Intel-Generation
    auto const abweichung        = meas::identify_machine("intel_avx2", "ddr5_2x32", fremd);
    EXPECT_EQ(abweichung.verdict, meas::MachineIdentityVerdict::Abweichung);
    EXPECT_TRUE(abweichung.signature().empty()) << "Ohne bestaetigte Identitaet NIE eine Signatur.";
}

TEST(O4MachineIdentity, NichtErhobeneLiveKennungWirdWEITERHIN_EHRLICHGemeldet) {
    // Die NichtDeklariert-Klasse bleibt erreichbar und ehrlich -- sie haengt jetzt an der LIVE-Seite
    // (CPUID nicht lesbar) statt an einer fehlenden Deklaration. Ohne diesen Test waere mit dem
    // O-4a-Nachzug die einzige Abdeckung des Zustands verschwunden.
    meas::MachineCoreCpuId const nicht_lesbar{}; // declared == false
    auto const                   r = meas::identify_machine("intel_avx2", "ddr5_2x32", nicht_lesbar);
    ASSERT_NE(r.machine, nullptr);
    EXPECT_EQ(r.verdict, meas::MachineIdentityVerdict::NichtDeklariert);
    EXPECT_TRUE(r.signature().empty());
    EXPECT_FALSE(meas::error_class_of_verdict(r.verdict).has_value())
        << "NichtDeklariert ist ein definierter Zustand, kein Fehler.";
}

TEST(O4MachineIdentity, AbweichungIstEinFehlerUndZwarD1) {
    // Ein Host, der etwas anderes ist als er deklariert: das ist genau
    // "ISA-/Beschleuniger-Erweiterung auf dem Host nicht verfuegbar" (axis_error.hpp:42).
    meas::MachineCoreCpuId fremd{};
    fremd.vendor   = "GenuineIntel";
    fremd.family   = 6;
    fremd.model    = 183;
    fremd.stepping = 1;
    fremd.declared = true;

    auto const r = meas::identify_machine("amd_zen5_avx512", "ddr5_2x32", fremd);
    ASSERT_NE(r.machine, nullptr);
    EXPECT_EQ(r.verdict, meas::MachineIdentityVerdict::Abweichung);
    EXPECT_TRUE(r.signature().empty()) << "Bei Abweichung darf keine Signatur freigegeben werden.";
    auto const err = meas::error_class_of_verdict(r.verdict);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt);
}

TEST(O4MachineIdentity, TaxonomieZahlIstDieDesEigenenPakets) {
    // HISTORIE: dieser Test hiess bis RF-3 "TaxonomieBleibtBeiVierFehlerklassen" und nagelte die 4 fest,
    // weil O-4 die Nummer 4 NICHT verbrauchen durfte -- sie war fuer RF-3 reserviert. Am 26.07. hat RF-3
    // (§70.3) sie eingezogen: BetriebssystemFeatureFehlt = 4, Count 4->5. Die Aussage des Tests bleibt
    // dieselbe und gilt unveraendert: O-4 selbst fuegt KEINE Fehlerklasse hinzu; die Zahl, die hier steht,
    // ist immer die des jeweils LETZTEN Taxonomie-Pakets.
    EXPECT_EQ(meas::kCompilerCompilerErrorClassCount, 5u);
}

// =================================================================================================
// 3. DIE LIVE-ABFRAGE (auf dieser Maschine wirklich ausgefuehrt)
// =================================================================================================
TEST(O4MachineIdentity, LiveCpuIdLiefertEinePlausibleKernKennung) {
    auto const live = meas::live_core_cpu_id();
#if defined(__x86_64__) || defined(__i386__)
    ASSERT_TRUE(live.declared) << "Auf x86 muss CPUID eine Kennung liefern.";
    EXPECT_FALSE(live.vendor.empty());
    EXPECT_TRUE(live.vendor == "AuthenticAMD" || live.vendor == "GenuineIntel")
        << "Unerwarteter Vendor: " << live.vendor;
    EXPECT_GT(live.family, 0u);
    EXPECT_FALSE(live.brand.empty()) << "Der Marken-String kommt aus Leaf 0x80000002..4.";
#else
    EXPECT_FALSE(live.declared) << "Ohne x86-CPUID muss ehrlich 'nicht deklariert' herauskommen.";
#endif
}

TEST(O4MachineIdentity, LiveHostnameIstAbfragbar) {
    // Nur Lookup-Schluessel -- der Test prueft die Abfragbarkeit, nicht den Wert (der ist je Host anders).
    EXPECT_FALSE(meas::live_hostname().empty()) << "Hostname muss auf einer POSIX-Maschine abfragbar sein.";
}

TEST(O4MachineIdentity, AufProd1StimmtDieDeklarationMitDerEchtenCpuUEBEREIN) {
    // DIE End-zu-End-Probe: laeuft dieser Test auf prod1, MUSS die dort deklarierte Kern-Kennung
    // (live abgeleitet am 26.07.2026, gegen /proc/cpuinfo gegengeprueft) mit der echten CPU
    // uebereinstimmen. Auf jedem anderen Host wird die Probe uebersprungen statt falsch behauptet.
    if (meas::live_hostname() != "prod1") {
        GTEST_SKIP() << "Nicht auf prod1 (Host: " << meas::live_hostname() << ") -- Live-Abgleich uebersprungen.";
    }
    auto const live = meas::live_core_cpu_id();
    ASSERT_TRUE(live.declared);
    auto const r = meas::identify_machine("amd_zen5_avx512", "ddr5_2x32", live);
    ASSERT_NE(r.machine, nullptr);
    EXPECT_EQ(r.verdict, meas::MachineIdentityVerdict::Match)
        << "prod1 deklariert " << meas::kProd1Zen5Core.vendor << "/" << meas::kProd1Zen5Core.family << "/"
        << meas::kProd1Zen5Core.model << "/" << meas::kProd1Zen5Core.stepping << ", live gemessen wurde " << live.vendor
        << "/" << live.family << "/" << live.model << "/" << live.stepping;
    // Und ERST bei Match faellt die Signatur heraus.
    EXPECT_FALSE(r.signature().empty());
    EXPECT_EQ(r.signature().size(), meas::Prod1Zen5Signature::signature().size());
}

// =================================================================================================
// 4. ABGRENZUNG: O-4 SCHALTET DAS GATE NICHT SCHARF
// =================================================================================================
TEST(O4MachineIdentity, GateZustandNachC3a) {
    // URSPRUENGLICH hiess dieser Test "DieDreiGateStubsBleibenLeer" und war die Abgrenzung von O-4
    // gegen C-3a. Er war als TRIPWIRE gebaut und hat bei der Scharfschaltung korrekt gebrochen --
    // das ist kein Defekt, sondern der beabsichtigte Zweck. Nachgezogen auf den C-3a-Stand:
    EXPECT_TRUE(meas::active_organ_required().empty())
        << "Die Organ-Seite bleibt der dominante Schalter und ist leer (simd_organ_requirement.hpp:88).";
    EXPECT_FALSE(meas::active_organ_meaningful().empty())
        << "C-3a fuellt die Sinnhaftigkeits-Obergrenze mit der echten Vereinigung.";
    // O-4s eigentliche Zusage haelt weiter: OHNE belegte Maschinen-Deklaration keine Signatur.
    meas::reset_active_machine_declaration_for_test();
    EXPECT_TRUE(meas::active_machine_signature().empty())
        << "Ohne CEB-Belegung darf die Identifikation nichts freigeben (natuerlicher Kill-Switch).";
}
