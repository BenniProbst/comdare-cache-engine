#pragma once
// pmc_ceb_asymmetrie.hpp -- DIE EINE STELLE (Test-Seite), die die deklarierte CEB/TIER-Asymmetrie
// der PMC-Meta-Meta-Achse ausdrueckt.
//
// WOZU EIN EIGENER HEADER FUER DREI ZEILEN LOGIK: es gibt ZWEI Drift-Guards, die die CEB-Zeile gegen die
// TIER-Zeile halten, und sie liegen in ZWEI Dateien --
//     tests/unit/test_m_w12_stamp_bausteine.cpp  (A5CebVersionStamp..., die Vollmenge)
//     tests/unit/test_d4_ceb_schluessel_wahl.cpp (GleicheComboLiefertDenselbenSchluessel..., alle 7 Teilmengen)
// Beide muessen seit dem 10.08.2026 dieselbe eine Ausnahme kennen. Haette ich sie zweimal hingeschrieben,
// waere genau die Lage entstanden, gegen die die Guards ueberhaupt gebaut sind: zwei Fassungen einer
// Ordnungs-Zusage, die auseinanderlaufen koennen. Der Kopf von ceb_version_stamp.hpp nennt diesen
// Fehler beim Namen -- "der von O-8 Schritt 12 dokumentierte DRITTE Ableitungsweg". Hier ist er der VIERTE.
//
// DIE ZUSAGE, die diese Datei traegt, in einem Satz:
//     Die CEB-Mess-Zeile ist die TIER-Mess-Zeile PLUS GENAU EIN Glied "pmc=<vendor>@X.Y.Z" -- und sonst
//     nichts.
// Sie bleibt damit eine GLEICHHEIT. Wer ein zweites Glied einhaengt oder die Ordnung aendert, faellt in
// beiden Guards rot -- genauso wie vor dem 10.08. Eine Lockerung auf "enthaelt" haette dagegen jede
// kuenftige Drift durchgelassen.
//
// FREMDER NENNER (T-3): die Erwartung wird aus dem RUNTIME-Renderer + der REGISTRY konstruiert, nie aus
// dem CEB-Header abgeschrieben. Ein Abschreiben verglichen den Header mit sich selbst.
//
// Der Praeprozessor-Schalter COMDARE_CEB_HAT_PMC_GLIED kommt aus builder/ceb_version_stamp.hpp -- DIE
// EINE Bedingung, an der auch Laengen-Rechnung und Renderer haengen. Es gibt hier keine zweite.

#include "builder/ceb_version_stamp.hpp"                    // COMDARE_CEB_HAT_PMC_GLIED (die EINE Bedingung)
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>      // kMetaMetaGroupClose
#include <cache_engine/measurement/algo_semver.hpp>         // parse/render_algo_semver (DIE Grammatik)
#include <cache_engine/measurement/pmc_vendor_registry.hpp> // die ZWEI Hardware-Komponenten

#include <string>

namespace comdare_test_pmc {

/// Die einkompilierte Hardwareform DIESER Uebersetzung, als Registry-Zeiger. nullptr == kein PMC-Glied.
[[nodiscard]] inline ::comdare::cache_engine::measurement::PmcVendorInfo const* eingebauter_vendor() noexcept {
#if COMDARE_CEB_HAT_PMC_GLIED
    namespace cm = ::comdare::cache_engine::measurement;
#if defined(COMDARE_PMC_VENDOR_AMD)
    return &cm::pmc_vendor_info(cm::PmcVendor::Amd);
#else
    return &cm::pmc_vendor_info(cm::PmcVendor::Intel);
#endif
#else
    return nullptr;
#endif
}

/// ceb_erwartung_aus_tier_zeile(tier) -- die TIER-Zeile in die Form gebracht, die die CEB rendern MUSS.
/// Ohne Vendor-Makro ist das die IDENTITAET -- und genau das ist der Additivitaets-Beweis: die beiden
/// Drift-Guards vergleichen dann Byte fuer Byte dieselben Texte wie vor dem 10.08.2026.
/// Leere Zeile bleibt leer (die Leer-Semantik beider Wege ist dieselbe: kein Tooling => keine Zeile).
[[nodiscard]] inline std::string ceb_erwartung_aus_tier_zeile(std::string tier) {
    auto const* const v = eingebauter_vendor();
    if (v == nullptr || tier.empty()) return tier;
    namespace cm  = ::comdare::cache_engine::measurement;
    namespace abi = ::comdare::cache_engine::abi;
    // Das Glied ist ein ';'-GESCHWISTER INNERHALB der bestehenden Meta-Meta-Klammer (Owner-E2 "ans Ende
    // der Kette", RF-7 keine neue Zeile) -- also: Klammer aufschneiden, Glied anhaengen, Klammer zu.
    if (tier.back() != abi::kMetaMetaGroupClose) return tier; // Aufrufer prueft die Vorbedingung selbst
    tier.pop_back();
    tier += ';';
    tier += cm::kPmcStampName;
    tier += '=';
    tier += v->id;
    tier += '@';
    tier += cm::render_algo_semver(cm::parse_algo_semver(v->version)).view();
    tier += abi::kMetaMetaGroupClose;
    return tier;
}

} // namespace comdare_test_pmc
