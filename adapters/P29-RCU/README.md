# P29-RCU — Adapter (McKenney et al. 2001)

**Paper:** Read-Copy Update (OLS 2001)
**Original-Repo:** ext/P29-RCU (LGPL-2.1+ — eigene Re-Implementation in libs/common/concurrency/ geplant)
**Stand:** V25.C (2026-05-14) — Adapter-Skelett

## Aufgabe
RCU-basierte Reclamation, expected_workload=YCSB_B (95/5 read-update).
Eigene RCU-Implementation aus V104 (cache-engine eigen, statt liburcu).

## Status
- [ ] rcu_adapter.hpp
- [ ] rcu_grace_period_runner.hpp
