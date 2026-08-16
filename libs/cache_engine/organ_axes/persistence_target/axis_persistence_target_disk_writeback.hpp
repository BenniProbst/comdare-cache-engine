#pragma once
// STRUKT-R ORG-18 axis_persistence_target DiskWritebackTarget (vollstaendiger Baustein, per option() AUS)
// @topic io @achse persistence_target @subaxis PT2 writeback
//
// Session-Doc §5 (Owner-KERN): 2. Baustein der Achse -- "auf Platte zurueckschreiben muss".
// Owner-Entscheid Q-1 = FALL B: der Baustein existiert VOLLSTAENDIG (Typ, Registry-Eintrag, Stempel,
// binary_id-Segment-Faehigkeit), ist aber enabled=false, damit EnabledTargets 1-elementig bleibt und der
// Voll-Bau-Raum bei 2^17 statt 2^18 liegt (Bauplan N-2: 448 GB / 48 h gegen 224 GB / 24 h).
//
// ===================== EHRLICHKEITS-DEKLARATION (bitte NICHT stillschweigend aufschalten) ============
// Dieser Baustein hat KEINEN echten Geraete-Schreibpfad. has_device_writeback_path() meldet das
// maschinenlesbar als false. Die Mess-Op unten misst AUSSCHLIESSLICH die Rueckschreib-VORBEREITUNG
// (Staging: Records in einen Writeback-Puffer sammeln + Pruefsumme), NICHT die Platte. Das ist dieselbe
// bewusste Abgrenzung, die axis_io_dispatch_observable.hpp fuer die io_dispatch-Achse dokumentiert
// ("reine IN-MEMORY-Dispatch-SIMULATION, KEIN echtes IO" -- echte Platten-Wartezeit im DRAM-Benchmark
// verfaelschte die Algo-Charakteristik). Der Wert ist real und strategie-abhaengig, KEINE erfundene
// Konstante -- aber er ist eine Staging-Zahl und darf NIE als Disk-Messung berichtet werden.
//
// Wer diesen Baustein per COMDARE_AXIS_PERSISTENCE_ENABLE_DISK_WRITEBACK=ON aufschaltet, verdoppelt den
// Voll-Bau-Raum (2^17 -> 2^18) UND misst weiterhin nur Staging, solange has_device_writeback_path()
// false meldet. Beides muss dann bewusst entschieden sein.
// =====================================================================================================

#include "axis_persistence_target_strategy_base.hpp"
#include "axis_persistence_target_subaxes_pt1_to_pt2.hpp"
#include "concepts/axis_persistence_target_cache_engine_permutation_concept.hpp"
#include <organ_axes/persistence_target/axis_persistence_target_flags.hpp>
#include <topics/io/concepts/topic_io_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::persistence_target {

/// DiskWritebackTarget -- der Index muss auf ein Block-Geraet zurueckgeschrieben werden.
/// Zustand STRUKT-R: Baustein vollstaendig, Geraete-Pfad NICHT implementiert (siehe Kopf-Deklaration).
class DiskWritebackTarget : public PersistenceTargetStrategyBase<DiskWritebackTarget> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::writeback_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::disk_writeback_enabled;

    [[nodiscard]] static constexpr bool             writes_back_to_disk() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "persistence_disk_writeback"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::persistence_target::DiskWritebackTarget",
                                  "organ_axes/persistence_target/axis_persistence_target_disk_writeback.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "DiskWritebackTarget (Rueckschreib-Staging; Geraete-Pfad noch nicht implementiert)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DISK_WRITEBACK"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    /// Maschinenlesbare Ehrlichkeits-Marke (Gegenstueck zur Kopf-Deklaration): meldet, ob hinter diesem
    /// Baustein ein echter Geraete-Schreibpfad steht. Solange false, ist jede Zahl dieser Achse eine
    /// Staging-Zahl -- Mess-Berichte duerfen das Segment dann NICHT als Disk-Messung etikettieren.
    /// BEWUSST kein static_assert gegen `enabled`: Owner-Auflage Q-1 verlangt, dass der Raum "jederzeit
    /// ohne Code-Aenderung per option() aufschaltbar" bleibt.
    [[nodiscard]] static constexpr bool has_device_writeback_path() noexcept { return false; }

    // Verhaltens-tragende Mess-Op der persistence_target-Achse (F15-operativ, Seg-Timer-Traeger).
    // EHRLICHKEIT: gemessen wird die Rueckschreib-VORBEREITUNG, nicht die Platte (siehe Kopf-Deklaration).
    // Reale, strategie-abhaengige Last: jeder Record wird - wie vor jedem echten Rueckschreiben - in einen
    // Staging-Puffer gesammelt und in die laufende Pruefsumme gefaltet. Anders als MemoryOnlyTarget
    // (`return 0`, kein Pfad) entsteht hier echte Kopier-/Faltungs-Arbeit -> der Seg-Timer trennt die
    // beiden Bausteine messbar, ohne Disk-Wartezeit zu behaupten.
    [[nodiscard]] static std::uint64_t persistence_writeback_scan(unsigned char const* buf, std::size_t n,
                                                                  std::size_t record_size) noexcept {
        // Staging-Fenster bewusst klein + stack-lokal: kein Allokator-Einfluss auf die Messung, kein
        // Seiteneffekt auf den Index. 64 Byte = eine Cache-Line (Cache-Line-Direktive).
        // Null-Init ist Pflicht, nicht Kosmetik: bei record_size < sizeof(uint32_t) deckt die Staging-Kopie
        // nicht alle 4 gelesenen Bytes ab -> ohne Init waere der Rest ein unbestimmter Wert. Aufrufer ist
        // heute kRecordSize=48 (abi_adapter.hpp:405), aber die Op ist public und wird auch aus Tests gerufen.
        constexpr std::size_t kStageBytes = 64;
        unsigned char         stage[kStageBytes]{};
        std::uint64_t         s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t const chunk = record_size < kStageBytes ? record_size : kStageBytes;
            std::memcpy(stage, buf + i * record_size, chunk); // Staging-Kopie (Rueckschreib-Vorbereitung)
            std::uint32_t v;
            std::memcpy(&v, stage, sizeof(v));
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::persistence_target

namespace comdare::cache_engine::persistence_target {
static_assert(concepts::PersistenceTargetStrategy<DiskWritebackTarget>);
static_assert(concepts::CacheEnginePermutationStrategy<DiskWritebackTarget>);
// Ehrlichkeits-Marke ist compile-time abfragbar (Mess-/Report-Seite kann sie zementiert auswerten).
static_assert(!DiskWritebackTarget::has_device_writeback_path(),
              "Sobald ein echter Geraete-Schreibpfad existiert: has_device_writeback_path() auf true "
              "heben UND diesen static_assert hier entfernen (er ist die Erinnerung, nicht die Sperre).");
} // namespace comdare::cache_engine::persistence_target
