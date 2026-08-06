#pragma once
// G2 Folge-B (Lager-Gate, Stempel-Paket A7-B) -- die Mengen-Signatur DIESES Treibers: build_variant_set_signature.hpp
// (registry-frei, generisch ueber Typlisten) angewandt auf die drei REALEN, CMake-enabled Registry-Listen
//   EnabledPageTypes  (axis_01, topics/nodes)      -- mp_filter ueber T::enabled aus axis_01_page_type_flags.hpp
//   EnabledExtensions (axis_09b, topics/hardware)  -- dito aus axis_09b_simd_extension_flags.hpp
//   EnabledPlatforms  (axis_12, topics/hardware)   -- dito aus axis_12_general_hardware_flags.hpp
// Die Flags-Header sind CMake-generiert (configure_file -> ${PROJECT_BINARY_DIR}/generated/topics/...), d.h. die
// Signatur ist eine direkte Funktion der Build-Konfiguration DIESER Maschine. Genau darum ist sie das
// Cross-Maschinen-Gate: eine per-Maschine restringierte Enable-Menge ergibt eine andere Signatur -> .variant-Sidecar-
// Mismatch -> Neubau. Bei identischer (Voll-)Enable-Menge ist sie identisch und damit inert.
//
// DIESER Header ist von build_variant_set_signature.hpp GETRENNT, weil erst er die generierten Flags-Header
// nachzieht: wer nur die generische CT-Maschinerie braucht (Tests mit synthetischen Listen), soll die Registry-
// Abhaengigkeit nicht miterben.
//
// ZIRKULARITAETS-VERBOT: die Erwartungsseite kommt IMMER aus dieser aktuellen Treiber-Konfiguration, NIE aus der
// alten DLL oder deren Sidecar. Ein Sidecar darf nur verglichen, niemals als Soll gelesen werden.

#include "build_variant_set_signature.hpp" // variant_set_signature<PageList,SimdList,HwList>()

#include <cache_engine/abi/anatomy_fingerprint.hpp> // O-2/C-2: kAnatomyFingerprintBvsetMax (Budget-Nachweis)

#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_registry.hpp>   // EnabledExtensions
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_registry.hpp> // EnabledPlatforms
#include <topics/nodes/axis_01_page_type/axis_01_page_type_registry.hpp>                  // EnabledPageTypes

#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// Die Mengen-Signatur dieses Treibers als compile-time Konstante.
inline constexpr std::string_view kDriverBuildVariantSignature =
    variant_set_signature<nodes::axis_01_page_type::EnabledPageTypes,
                          hardware::axis_09b_simd_extension::EnabledExtensions,
                          hardware::axis_12_general_hardware::EnabledPlatforms>();

// CT-Wachen: die Signatur ist nicht leer, traegt den Format-/POD-Versions-Kopf und ALLE DREI Achsen-Klammern
// (§66-N3 Punkt 4). Schlaegt eine dieser Zusicherungen fehl, ist die Signatur strukturell kaputt und das Gate wuerde
// still falsch vergleichen -- deshalb bricht hier der Bau statt der Messung.
static_assert(!kDriverBuildVariantSignature.empty());
static_assert(kDriverBuildVariantSignature.starts_with("bvset="));
static_assert(kDriverBuildVariantSignature.find(";page_type[") != std::string_view::npos);
static_assert(kDriverBuildVariantSignature.find(";simd_extension[") != std::string_view::npos);
static_assert(kDriverBuildVariantSignature.find(";general_hardware[") != std::string_view::npos);
// Mindestens eine Variante je Achse -- die Registries erzwingen das bereits per eigenem static_assert
// (mp_size<Enabled*> > 0); hier wird zusaetzlich geprueft, dass sie auch WIRKLICH in der Signatur landet
// (eine leere Achse waere `page_type[]`, also unmittelbar von `[]` gefolgt).
static_assert(kDriverBuildVariantSignature.find("page_type[]") == std::string_view::npos);
static_assert(kDriverBuildVariantSignature.find("simd_extension[]") == std::string_view::npos);
static_assert(kDriverBuildVariantSignature.find("general_hardware[]") == std::string_view::npos);

// -- O-2/C-2: DER LEBENDE BUDGET-NACHWEIS DES bvset-PREIMAGE-GLIEDS -----------------------------------
// Seit Format 3 ist diese Signatur ein Preimage-Glied ([6], F7-Spez (b)). Sie ist das EINZIGE Glied, dessen
// Laenge mit der Enable-Menge der Flotte WAECHST: je enabled Variante ein geklammertes Element mit Namen
// und Feldern. Ein Prosa-Budget waere hier wertlos -- es wuerde erst brechen, wenn jemand Achsen-Varianten
// hinzuschaltet, und dann als unerklaerlicher consteval-Fehler tief im Fingerprint. Deshalb misst diese
// Zeile die WIRKLICHE Signatur DIESES Treibers gegen genau die Konstante, aus der das Gesamt-Budget in
// anatomy_fingerprint.hpp gerechnet ist.
static_assert(kDriverBuildVariantSignature.size() <= ::comdare::cache_engine::abi::kAnatomyFingerprintBvsetMax,
              "O-2/C-2 BUDGET: die bvset-Signatur dieses Treibers ueberschreitet das fuer das Preimage-Glied "
              "[6] gerechnete Teil-Budget (kAnatomyFingerprintBvsetMax). Entweder das Teil-Budget UND den "
              "Gesamt-Nachweis in anatomy_fingerprint.hpp anheben oder die Enable-Menge begrenzen -- nicht "
              "still den Puffer vergroessern.");
// '\n'-FREIHEIT (OF-M3-1): die Signatur besteht aus Namen, Ziffern und den Trennern ;=[]{} -- kein
// Zeilenumbruch. Als Preimage-Glied ist das ab jetzt eine Injektivitaets-Pflicht, kein Zufall.
static_assert(kDriverBuildVariantSignature.find('\n') == std::string_view::npos,
              "Das bvset-Glied darf den Domain-Separator '\\n' nicht enthalten.");

// -- NB-3/T2-D: DIE PAAR-WACHE UEBER DIE VOLLEN REGISTRY-LISTEN ---------------------------------------
//
// build_variant_set_signature.hpp stellt die Namens-Eindeutigkeit fuer die Liste, die WIRKLICH emittiert
// wird -- das ist die enabled Menge, und mehr kann ein registry-freier Header nicht sehen. Genau dort
// bleibt aber eine Restluecke: zwei Wrapper mit demselben Namen, die sich per CMake-Flags gegenseitig
// ausschliessen, stehen NIE zusammen in einer Enabled-Liste. Die enabled-Wache sieht dann je Bau nur
// einen von beiden und schweigt -- waehrend die beiden Baue untereinander exakt kollidieren: gleiche
// Namen, gleiche Felder, gleiche Signatur, seit Format 3 also gleicher Fingerprint und ein falscher Skip
// GENAU ZWISCHEN DIESEN BEIDEN Konfigurationen. Der gefaehrlichste Fall ist damit der, den die untere
// Ebene per Konstruktion nicht sehen kann.
//
// DIESER Header ist der erste Ort, an dem die VOLLEN Registry-Listen sichtbar sind (er zieht sie ohnehin
// nach -- deshalb kostet die Wache keine neue Abhaengigkeit). Ueber All* geprueft, faellt ein Duplikat
// auf, sobald es DEKLARIERT wird, unabhaengig von jeder Enable-Kombination. Das ist die Stelle, an der
// die Zusage "der Name diskriminiert" wirklich gilt.
COMDARE_BVSET_PAAR_WACHE(nodes::axis_01_page_type::AllPageTypes, "page_type (volle Registry)");
COMDARE_BVSET_PAAR_WACHE(hardware::axis_09b_simd_extension::AllExtensions, "simd_extension (volle Registry)");
COMDARE_BVSET_PAAR_WACHE(hardware::axis_12_general_hardware::AllPlatforms, "general_hardware (volle Registry)");

/// Runtime-Naht fuer BuildConfig.build_variant_sig: liefert dieselbe CT-Konstante als std::string. Reine CT->RT-
/// Materialisierung (erlaubte Richtung); es wird nichts abgeleitet, geparst oder geraten.
[[nodiscard]] inline std::string driver_build_variant_signature() { return std::string{kDriverBuildVariantSignature}; }

} // namespace comdare::cache_engine::builder::experiment
