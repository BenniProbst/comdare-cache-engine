#pragma once
// D12 / L-76d (2026-06-02) — GraphBfs: erste produktive Virus-Engine (Graph-BFS, ECHTE Breitensuche über
// CSR-Adjacency). Schwester-Zweig zu den Anatomie-Gattungen (kein Achsen-System) — IVirusExecutionEngine.
// Algorithmus-Korrektheit (Memory feedback_algorithm_correctness_when_named): echte BFS, kein Stub.

#include "../execution_engine/virus_execution_engine.hpp"

#include <cstdint>
#include <queue>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::virus {

namespace eng = ::comdare::cache_engine::execution_engine;

/// GraphBfs — Breitensuche über einen CSR-Graphen (row_offsets[n+1], col_indices[m]). Virus (keine Achsen).
class GraphBfs final : public eng::IVirusExecutionEngine {
public:
    /// CSR-Graph setzen: row_offsets.size()==n+1, col_indices = konkatenierte Nachbarlisten.
    void set_graph(std::vector<std::uint64_t> row_offsets, std::vector<std::uint64_t> col_indices) {
        row_offsets_ = std::move(row_offsets);
        col_indices_ = std::move(col_indices);
    }

    /// ECHTE BFS ab start_node. Liefert die Zahl der erreichten Knoten; aktualisiert den Mess-Snapshot.
    std::uint64_t run_bfs(std::uint64_t start_node) {
        std::uint64_t const n       = node_count();
        std::uint64_t       visited = 0, edges = 0, checksum = 0;
        if (n != 0 && start_node < n) {
            std::vector<char>         seen(static_cast<std::size_t>(n), 0);
            std::queue<std::uint64_t> q;
            seen[static_cast<std::size_t>(start_node)] = 1;
            q.push(start_node);
            while (!q.empty()) {
                std::uint64_t const u = q.front();
                q.pop();
                ++visited;
                checksum += u;
                std::uint64_t const beg = row_offsets_[static_cast<std::size_t>(u)];
                std::uint64_t const end = row_offsets_[static_cast<std::size_t>(u) + 1];
                for (std::uint64_t e = beg; e < end; ++e) {
                    ++edges;
                    std::uint64_t const v = col_indices_[static_cast<std::size_t>(e)];
                    if (v < n && !seen[static_cast<std::size_t>(v)]) {
                        seen[static_cast<std::size_t>(v)] = 1;
                        q.push(v);
                    }
                }
            }
        }
        ++snap_.run_count;
        snap_.visited_nodes   = visited;
        snap_.edges_traversed = edges;
        snap_.result_checksum = checksum;
        return visited;
    }

    [[nodiscard]] std::uint64_t node_count() const noexcept {
        return row_offsets_.empty() ? 0 : static_cast<std::uint64_t>(row_offsets_.size() - 1);
    }

    // ── IExecutionEngine ──
    [[nodiscard]] std::string_view          engine_name() const noexcept override { return "GraphBFS"; }
    [[nodiscard]] eng::EngineLifecycleState lifecycle_state() const noexcept override { return state_; }
    void                                    warm_up() override { state_ = eng::EngineLifecycleState::Warming; }
    void                                    run() override { state_ = eng::EngineLifecycleState::Running; }
    void                                    reset() override {
        state_ = eng::EngineLifecycleState::Idle;
        snap_  = {};
    } // Statistik-Reset
    void shutdown() override { state_ = eng::EngineLifecycleState::Shutdown; }

    // ── IVirusExecutionEngine (engine_kind() final = Virus in der Basis) ──
    [[nodiscard]] std::string_view algorithm_family() const noexcept override { return "GraphBFS"; }
    void                           virus_observe(eng::VirusMeasurementSnapshotV1* out) const noexcept override {
        if (out != nullptr) *out = snap_;
    }

private:
    std::vector<std::uint64_t>      row_offsets_{};
    std::vector<std::uint64_t>      col_indices_{};
    eng::EngineLifecycleState       state_{eng::EngineLifecycleState::Uninitialized};
    eng::VirusMeasurementSnapshotV1 snap_{};
};

} // namespace comdare::cache_engine::virus
