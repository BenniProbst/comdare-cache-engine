#pragma once
// abi/tier_hybrid_stempel.hpp -- F2-3 (S-5-ERBINNEN-REST, #15-Nachlande 20.08.2026): die Tier- und
// Hybrid-ERBINNEN des CRTP-Stempel-Vertrags (S-1, abi/stempel_basis.hpp) nach dem P4/P5-Muster
// (PlanerStempel, profile_facade/planner/planner_version.hpp; CebStempel, builder/ceb_version_stamp.hpp).
//
// WARUM VORLAGEN STATT ZWEIER KONKRETER STRUCTS: Planer und CEB haben je GENAU EINEN einkompilierten
// Wert-Satz -- ihre Erbinnen delegieren auf Datei-Konstanten. Ein TIER-Binary hat seinen Wert-Satz JE
// MODUL (die static-constexpr-Traeger der Makro-Expansion COMDARE_ANATOMY_VERSION_STAMP_M: kM/kSC/kO/
// kFP/kNM, anatomy_module_abi_v1.hpp; im Baum seit L1/#15: Layout 7, Format 6, GliedCount 11). Die
// Erbin ist deshalb ueber einem PROVIDER parametrisiert -- derselbe Delegations-Gedanke, nur ist die
// Wert-Quelle ein Template-Parameter statt einer Datei-Konstante. S-1-Doktrin unveraendert: die Erbin
// aendert KEINEN Stempel-WERT, sie BINDET Bestand an.
//
// DER ZWANG (S-1/P1.2) WANDERT AN DIE INSTANZIIERUNG: eine Vorlage kann nicht unter ihrer Definition
// mit static_assert(StempelVertrag<...>) schliessen (deferred-consteval, die Probe braucht die Werte).
// Jede INSTANZIIERUNG schliesst deshalb mit
//     static_assert(StempelVertrag<TierStempel<P>, StempelTraeger::Tier>);
//     static_assert(stempel_name_vertrag<TierStempel<P>>());
// direkt unter ihrer Provider-Definition (Vertragstest: test_s1_stempel_basis_vertrag.cpp, Abschnitt
// F2-3). Matrix (kStempelZulassungsMatrix) und 28-Zellen-Pin bleiben BYTE-UNBERUEHRT -- die Erbinnen
// ERFUELLEN die Zeilen Tier/Hybrid, sie aendern keine Zelle.
//
// DER name()-VERTRAG (E-A + V-05R "Ja, integrieren", KON101): DER NAME ist der EIGENE SHA-256 (64
// lowercase hex) ueber DASSELBE Preimage, das den SHA-512-Fingerprint speist -- ausdruecklich NICHT
// dessen erste 64 hex ("EIGENER Hash, KEIN fingerprint[0:64]", Ledger :27981-27986; die Quell-Wache
// dazu ist die Ungleichheits-Wache in abi/anatomy_fingerprint.hpp, der POD-Traeger ist
// name_line/name_len in AnatomyVersionLines, Layout 7). Hier steht die ERBIN-SEITE dieses Vertrags:
// stempel_name_vertrag<Erbin>() prueft 64-lowercase-hex UND die Praefix-UNGLEICHHEIT am Wert-Paar der
// Erbin. F7 (06.08.2026, KON100-S4) hat den CT-Namen als EIGENES Stempel-Interface festgelegt; er ist
// hier deshalb PFLICHT-Teil der Tier-/Hybrid-Provider-Flaeche ("Erbin verschaerft, nie aufweicht").
//
// TEIL-DEKLARATION (18.6-Muster) -- WAS HIER AUSDRUECKLICH NICHT GEBAUT WIRD, mit Beleg:
//   * Der MATRIX-EINZUG des Namens als 8. Interface-ZEILE ueber alle vier Traeger (28 -> 32 Zellen)
//     haengt an der offenen NAMENS-/SHA-FORM der Traeger PLANER und CEB: V-08R hat fuer den Planer
//     "SHA256 ueber die PLANER-VERSIONSNUMMER" entschieden, die FORM-Frage ist owner-gated offen
//     (Wellenplan par.23.1 F2-7: "Planer-SHA owner-gated P5 [...] P5 im Vorlagen-Fenster";
//     planner_version.hpp traegt kFingerprintShaBewusstLeer als lauten Riegel). Eine Matrix-Zeile,
//     die Planer-/CEB-Zellen belegen muesste, waere geraten -- sie faellt ins Vorlagen-Fenster.
//   * Die VERDRAHTUNG der Erbinnen in die Makro-Naht (COMDARE_ANATOMY_VERSION_STAMP_M) und die
//     Stempel-PFLICHT je Schalter sind B-8b/B5 (Wellenplan F2-4: Fall-Deklaration mit F2-3-Kopplung)
//     -- B-8b zieht die Provider-Form von test_s1/erbin_bestand in die Makro-Expansion, NICHT hier.
//
// LAYER: abi -- wie die Basis (Stufe-0-Querschnitt, KON43-01(1)); nur stempel_basis + std.
// header-only, C++23, ASCII-only.

#include <cache_engine/abi/stempel_basis.hpp>

#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::abi {

// ================================================================================================
// Provider-Vertraege (die WERT-Quellen der Erbinnen; statisch, noexcept, string_view-konvertibel)
// ================================================================================================

/// Die Tier-Wert-Flaeche: die fuenf Matrix-Pflichten des Traegers TIER + der Name (F7/V-05R).
/// version_xyz fehlt BEWUSST (Matrix: VERBOTEN am Tier -- "die Binary hat KEINE eigene Version,
/// nur der PLANER hat eine", KON8/Q-A); ein Provider, der es doch traegt, faellt am Vertrag.
template <class P>
concept TierStempelWerte = requires {
    { P::mess_zeile() } noexcept -> std::convertible_to<std::string_view>;
    { P::system_zeile() } noexcept -> std::convertible_to<std::string_view>;
    { P::organ_zeile() } noexcept -> std::convertible_to<std::string_view>;
    { P::fingerprint_sha() } noexcept -> std::convertible_to<std::string_view>;
    { P::gesamt_stempel() } noexcept -> std::convertible_to<std::string_view>;
    { P::name() } noexcept -> std::convertible_to<std::string_view>;
};

/// Die Hybrid-Wert-Flaeche: Tier-Flaeche + der angeschlossene()-RT-Hook (KON47-02). Der Provider
/// liefert den Span STATISCH (der Datenbestand ist der Dock-Init-Cache, hybrid_dock_array.hpp --
/// statisch langlebig, nie ein Temporaer; API-Invariante wie am AngeschlosseneVertrag der Basis).
template <class P>
concept HybridStempelWerte = TierStempelWerte<P> && requires {
    { P::angeschlossene() } noexcept;
    requires ist_stempelzeilen_span<std::remove_cvref_t<decltype(P::angeschlossene())>>;
};

// ================================================================================================
// Die Erbinnen (P4/P5-Muster: reine DELEGATION, 0 virtual, keine Daten)
// ================================================================================================
// JEDER Forwarder traegt eine requires-KLAUSEL auf seinem Provider-Gegenstueck. Das ist KEIN Stil:
// ein unbedingter Forwarder EXISTIERTE auch dann als Deklaration, wenn der Provider das Mitglied
// nicht traegt -- die Absenz-Proben der Basis (hat_*/vorhanden_*) saehen ihn, und erst die
// KOERPER-Instanziierung im deferred-Default-Argument braeche HART (Koerper-Fehler sind kein
// SFINAE). Genau das verbietet die S-1-Doktrin ("der Vertrag wird FALSCH, die Basis bricht nie").
// Mit der Klausel FAELLT der Forwarder bei fehlendem Provider-Mitglied weg, und die Matrix lehnt
// per falschem Vertrag ab -- beweisbar an den Negativ-Proben in test_s1 (F2-3-Abschnitt).
// Die Klassen-Vorlagen selbst sind deshalb UNBESCHRAENKT; die benannten Provider-Concepts oben
// sind der POSITIV-Vertrag fuer Verdrahtungs-Stellen (B-8b prueft mit ihnen, bevor es einbaut).

/// TierStempel<Provider> -- die Erbin des CRTP-Stempel-Vertrags fuer den Traeger TIER (F2-3).
template <class Provider>
struct TierStempel : ::comdare::cache_engine::abi::StempelBasis<TierStempel<Provider>,
                                                                ::comdare::cache_engine::abi::StempelTraeger::Tier> {
    [[nodiscard]] static constexpr std::string_view mess_zeile() noexcept
        requires requires {
            { Provider::mess_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::mess_zeile();
    }
    [[nodiscard]] static constexpr std::string_view system_zeile() noexcept
        requires requires {
            { Provider::system_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::system_zeile();
    }
    [[nodiscard]] static constexpr std::string_view organ_zeile() noexcept
        requires requires {
            { Provider::organ_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::organ_zeile();
    }
    [[nodiscard]] static constexpr std::string_view fingerprint_sha() noexcept
        requires requires {
            { Provider::fingerprint_sha() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::fingerprint_sha();
    }
    [[nodiscard]] static constexpr std::string_view gesamt_stempel() noexcept
        requires requires {
            { Provider::gesamt_stempel() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::gesamt_stempel();
    }
    /// DER NAME (E-A/V-05R): 64-hex-SHA-256 -- Erbin-Zusatz ueber der Matrix ("verschaerft, nie
    /// aufweicht"); sein Wert-Vertrag ist stempel_name_vertrag<...>() an der Instanziierung.
    [[nodiscard]] static constexpr std::string_view name() noexcept
        requires requires {
            { Provider::name() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::name();
    }
};

/// HybridStempel<Provider> -- die Erbin fuer den Traeger HYBRID: Tier-Flaeche + angeschlossene().
template <class Provider>
struct HybridStempel
    : ::comdare::cache_engine::abi::StempelBasis<HybridStempel<Provider>,
                                                 ::comdare::cache_engine::abi::StempelTraeger::Hybrid> {
    [[nodiscard]] static constexpr std::string_view mess_zeile() noexcept
        requires requires {
            { Provider::mess_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::mess_zeile();
    }
    [[nodiscard]] static constexpr std::string_view system_zeile() noexcept
        requires requires {
            { Provider::system_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::system_zeile();
    }
    [[nodiscard]] static constexpr std::string_view organ_zeile() noexcept
        requires requires {
            { Provider::organ_zeile() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::organ_zeile();
    }
    [[nodiscard]] static constexpr std::string_view fingerprint_sha() noexcept
        requires requires {
            { Provider::fingerprint_sha() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::fingerprint_sha();
    }
    [[nodiscard]] static constexpr std::string_view gesamt_stempel() noexcept
        requires requires {
            { Provider::gesamt_stempel() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::gesamt_stempel();
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept
        requires requires {
            { Provider::name() } noexcept -> std::convertible_to<std::string_view>;
        }
    {
        return Provider::name();
    }
    /// Der RT-Hook (KON47-02-Signatur, AngeschlosseneVertrag der Basis): Rueckgabetyp ist EXAKT der
    /// Span des Providers (same_as-Disziplin der Basis; eine Container-Konversion faellt dort durch).
    [[nodiscard]] auto angeschlossene() const noexcept
        requires requires {
            { Provider::angeschlossene() } noexcept;
        }
    {
        return Provider::angeschlossene();
    }
};

// ================================================================================================
// Der name()-VERTRAG (E-A/V-05R) -- die Erbin-Seite der Ungleichheits-Wache
// ================================================================================================

/// 64 KLEINBUCHSTABEN-Hex-Zeichen -- dieselbe Zeichenklassen-Strenge wie ist_sha512_hex_128 der
/// Basis (128x'g' bestuende sonst den behaupteten SHA-256-Vertrag; Zweitlens-Klasse 12.08.).
[[nodiscard]] consteval bool stempel_name_ist_wohlgeformt(std::string_view name) noexcept {
    if (name.size() != 64) return false;
    for (char const c : name)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

/// "EIGENER Hash, KEIN fingerprint[0:64]" (Ledger :27981-27986): der Name darf NIE das
/// 64-Zeichen-Praefix des SHA-512-Fingerprints sein. Traegt die Erbin keinen 128-hex-Fingerprint
/// (BewusstLeer-Klasse), gibt es kein Praefix, mit dem er kollidieren koennte -- dann ist die
/// Praefix-Frage gegenstandslos (true), und die Fingerprint-Pflicht selbst prueft die Matrix.
[[nodiscard]] consteval bool stempel_name_ist_kein_fingerprint_praefix(std::string_view name,
                                                                       std::string_view fingerprint) noexcept {
    if (fingerprint.size() != 128) return true;
    return name != fingerprint.substr(0, 64);
}

/// Der EINE Einstieg fuer die Instanziierungs-Stelle: Form UND Ungleichheit am Wert-Paar der Erbin.
/// Fehlt name() oder fingerprint_sha(), ist der Vertrag FALSCH statt hart (S-1-Doktrin).
template <class ErbinT>
[[nodiscard]] consteval bool stempel_name_vertrag() {
    if constexpr (!requires {
                      { ErbinT::name() } noexcept -> std::convertible_to<std::string_view>;
                  })
        return false;
    else if constexpr (!requires {
                           { ErbinT::fingerprint_sha() } noexcept -> std::convertible_to<std::string_view>;
                       })
        return false;
    else
        return stempel_name_ist_wohlgeformt(std::string_view{ErbinT::name()}) &&
               stempel_name_ist_kein_fingerprint_praefix(std::string_view{ErbinT::name()},
                                                         std::string_view{ErbinT::fingerprint_sha()});
}

} // namespace comdare::cache_engine::abi
