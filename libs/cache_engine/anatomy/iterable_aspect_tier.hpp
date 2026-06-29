#pragma once
// #188-4a-C (2026-06-29) — IIterableAspectTier: ABI-stabiler SEPARATER Laufzeit-Kanal fuer iterable_aspect-
// Achsen-Parameter (z.B. die k-ary-Arity K). BEWUSST GETRENNT vom Resource-Control-Kanal
// (IResourceControllableTier, resource_controllable_tier.hpp): K ist ein ALGORITHMISCHER Such-METHODE-Parameter
// (iterable_aspect), KEIN "Resource Control" (Threads/Pool/Prefetch/...). User-Entscheid 2026-06-29: eigener
// Kanal fuer saubere Semantik-Trennung (NICHT den RC-POD ueberladen). #221 (RC) = der parallele Kanal, gleiches Muster.
//
// ABI-SICHER nach EXAKT demselben Designprinzip wie IResourceControllableTier/IObservableTier:
//   - eigenstaendiges Sub-Interface; der Host fragt es via dynamic_cast<IIterableAspectTier*> ab;
//     alt-gebaute DLLs ohne Faehigkeit -> nullptr -> Host degradiert sauber (rein ADDITIV, kein ABI-Bruch).
//   - quert die Grenze als reine vtable + uint64-Skalare -> ABI-stabil.
//   - NIEMALS in-place an bestehenden Interfaces aendern (SEH-0xc0000005-Lektion, resource_controllable_tier.hpp:13).
//   - NICHT an COMDARE_MEASUREMENT_ON gebunden (wie IResourceControllableTier): steuert algorithmus-INTERN, nicht die Messung.
//
// Rolle im Permutations-B+-Baum (Dossier §12/§19): K ist eine DYNAMISCHE SUBACHSE. Der Builder
// (IterableAspectLoop, #188-4a-C4) iteriert die iterable_values einer Achse (aus dem Profil/XML) und ruft je Wert
// tier_apply_iterable_aspect ueber die GELADENE Binary auf — der State wohnt im Container (ComposedSearch::
// set_iterable_aspect), das Traversal-Organ bleibt STATELESS (reine Funktion von (Store, K)).

#include <cstdint>

namespace comdare::cache_engine::anatomy {

// Achs-IDs fuer den iterable-Kanal (welche iterable-Achse adressiert wird). Erweiterbar; 0 = search_algo (T0).
inline constexpr std::uint64_t kIterableAxisSearchAlgo = 0;

/// ABI-Version des iterable-Aspekt-Kanals. Major-Bump bei Signatur-Aenderung (neue Achse = additiv, kein Bump).
inline constexpr std::uint32_t kIterableAspectVersion = 1;

// ─────────────────────────────────────────────────────────────────────────────
// IIterableAspectTier — optionaler iterable-Aspekt-Steuer-Kanal (additiv, dynamic_cast, IMMER einkompiliert).
// ─────────────────────────────────────────────────────────────────────────────
class IIterableAspectTier {
public:
    virtual ~IIterableAspectTier() = default;

    /// Setzt den Laufzeit-Aspekt-Wert `value` (z.B. k-ary-Arity K) fuer die iterable-Achse `axis_id`.
    /// Gibt true, wenn die adressierte Achse den Wert real angenommen hat (false = Achse nicht iterable/unbekannt).
    /// noexcept (Steuer-Op darf den Lauf nicht werfen). Der Aufrufer uebergibt explizite Werte aus iterable_values().
    virtual bool tier_apply_iterable_aspect(std::uint64_t axis_id, std::uint64_t value) noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
