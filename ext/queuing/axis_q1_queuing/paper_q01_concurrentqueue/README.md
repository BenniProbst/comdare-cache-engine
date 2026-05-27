# paper_q01_concurrentqueue — Kuratierte moodycamel-Snapshot (P2.D.q.s2 Pilot)

**Source:** Cameron Desrochers — `moodycamel::ConcurrentQueue` (BSD-2 / Simplified BSD).
Repo: https://github.com/cameron314/concurrentqueue (header-only).

**Algorithmus-Klassifikation:** Lock-Free MPMC Block-Based Queue (Vyukov-inspiriert,
mit moodycamel-Erweiterungen: separate producer-queues, per-producer hash, atomic-fence-optimiert).
Real-world produktionserprobt (Microsoft, Facebook, Tencent etc.).

**API-Mapping (2/6 originall, 4/6 Lücken):**
- put          → `ConcurrentQueue<T>::enqueue(T const&)`  (originall)
- get          → `ConcurrentQueue<T>::try_dequeue(T&)`    (originall)
- emplace      → **LUECKE** (concurrentqueue hat keine emplace-Methode — Cache-Engine
  Re-Impl via `enqueue(std::move(T{args...}))`, is_original_emplace()=false)
- peek_front   → **LUECKE** (concurrentqueue ist async, kein peek im Paper-Design —
  Re-Impl via `try_dequeue + try_re-enqueue`, is_original_peek_front()=false)
- peek_back    → **LUECKE** (analog peek_front)
- clear        → **LUECKE** (kein clear in concurrentqueue — Re-Impl via
  Drain-Loop `while(try_dequeue(_));`, is_original_clear()=false)

**Wrapper-Klasse:** `OriginalLockFreeMpmcConcurrentQueue` (parallel zu existing
`LockFreeMPMCBuffer` Re-Impl).

**is_original_module()** = false (weil 4/6 Lücken). PermutationEngine kann via
`PaperOriginalValidated`-Concept-Filter ausschliessen, oder via `HasOriginalCode`
explizit zulassen (Paper-Bindung partiell).

**P2.D.q.s2 Pilot:** Erster externer git-Submodule-basierter Wrapper im queuing-Topic
(im Gegensatz zu allen anderen Q1/Q2-Wrappern die reine Cache-Engine Re-Impl sind).
