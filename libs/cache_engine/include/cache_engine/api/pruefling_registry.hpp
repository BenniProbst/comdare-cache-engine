// SPDX-License-Identifier: Apache-2.0
// V41.E11 (2026-05-29) — Konkrete Plugin-Controller Pruefling-Registry (header-only).
//
// Implementiert das in i_pruefling_factory.hpp definierte Abstract-Factory-Slot-Pattern:
// die cache-engine HÄLT (per Konfiguration registrierte) Prüflinge wie prt-art und vermittelt
// sie an das Mess-Projekt (Diplomarbeit). Header-only + inline-Singleton, da die cache-engine
// kein zentrales add_library-Target hat (header-only/INTERFACE + builder-Komponenten).

#pragma once

#include "i_pruefling_factory.hpp"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::api {

/// In-Process-Registry der konfigurierten Prüflinge. Prüfling-Repos (prt-art, ...) registrieren
/// ihre Factory beim Bootstrap (CMake-konfiguriert via COMDARE_CE_PRUEFLINGE).
class PrueflingRegistry final : public IPrueflingRegistry {
public:
    void register_factory(std::unique_ptr<IPrueflingFactory> factory) override {
        if (factory) factories_.push_back(std::move(factory));
    }

    [[nodiscard]] std::vector<IPrueflingFactory*> all_factories() override {
        std::vector<IPrueflingFactory*> out;
        out.reserve(factories_.size());
        for (auto& f : factories_) out.push_back(f.get());
        return out;
    }

    [[nodiscard]] IPrueflingFactory* find(std::string_view pruefling_name) override {
        for (auto& f : factories_) {
            if (f && f->pruefling_name() == pruefling_name) return f.get();
        }
        return nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept { return factories_.size(); }

private:
    std::vector<std::unique_ptr<IPrueflingFactory>> factories_{};
};

/// Globaler Registry-Zugriff (Function-local-static Singleton — ODR-sicher header-only).
[[nodiscard]] inline IPrueflingRegistry& get_pruefling_registry() {
    static PrueflingRegistry registry;
    return registry;
}

} // namespace comdare::cache_engine::api
