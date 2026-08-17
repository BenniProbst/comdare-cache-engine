# axis_q2_queuing — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** mix D/E — ueberwiegend Engineering-Pattern + Lehrbuch-Policies (EagerFlush/LazyFlush = Write-Through/Write-Back-Lehrbuch); kein is_original-faehiger Code in dieser Achse.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle is_original = ✗). EagerFlush und LazyFlush sind Lehrbuch-Policies (Write-Through bzw. Write-Back) ohne Paper. WatermarkFlush, TimedFlush und AdaptiveLsmFlush sind Engineering-/Re-Impl-Wrapper mit verwandten Paper-Ankern (LSM-Tree, Kafka/Spark, RocksDB); die jeweilige Referenzimplementierung ist entweder nicht in C/C++ (Kafka/Spark = Scala/Java) oder CE-eigene Logik (EWMA in AdaptiveLsmFlush).

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| EagerFlush | Eager (write-through) Flush | — (Lehrbuch Write-Through-Policy) | — | — | nein | none | ✗ |
| LazyFlush | Lazy (write-back/deferred) Flush | — (Lehrbuch Write-Back-Policy) | — | — | nein | none | ✗ |
| WatermarkFlush | Watermark/Threshold-Flush (fill ≥ X%, Default 75%) | — (verwandt LSM-Tree Memtable-Trigger, O'Neil 1996) | Acta Informatica 1996 (verwandt) | 10.1007/s002360050048 | nein | none | ✗ |
| TimedFlush | Zeit-Window (Micro-Batching) Flush | Kafka: a Distributed Messaging System…; verwandt D-Streams (Spark) | NetDB 2011; SOSP 2013 | cwiki.apache.org/.../Kafka-netdb-06-2011.pdf ; 10.1145/2517349.2522737 | ? | Apache-2.0 (Scala/Java) | ✗ |
| AdaptiveLsmFlush | Adaptiver Watermark-Flush via EWMA (selbst-tunend) | RocksDB Dynamic Leveled Compaction (kein Paper); LSM-Wurzel O'Neil 1996 | RocksDB Eng.; Acta Inf. 1996 | github.com/facebook/rocksdb/wiki/Leveled-Compaction ; 10.1007/s002360050048 | OSS | Apache-2.0 OR GPLv2 | ✗ |

## §3 Compliance-Status
Alle 5 Wrapper haben entweder eine Paper-Referenz (WatermarkFlush, TimedFlush, AdaptiveLsmFlush) oder sind explizit als Lehrbuch-/Baseline-Policy gekennzeichnet (EagerFlush = Write-Through, LazyFlush = Write-Back) → Habich-Pflicht erfuellt.

Hinweise:
- is_original-Kandidaten (Map §3): **keine** in dieser Achse (alle is_original_eligible = false).
- Lizenz-blockierte Codes: keine. AdaptiveLsmFlush referenziert RocksDB (Apache-2.0 OR GPLv2, permissive Option verfuegbar), TimedFlush referenziert Kafka/Spark (Apache-2.0) — beide jedoch ohne C/C++-Referenzimpl (Scala/Java) bzw. CE-eigene EWMA-Logik, daher kein Original-Linking.
- Offene/unsichere Punkte (Map §5): WatermarkFlush (confidence medium — kein dediziertes Watermark-Flush-Paper, nur verwandtes LSM-Konzept); TimedFlush (confidence high, aber Referenzimpl Scala/Java, kein C/C++-Standalone); AdaptiveLsmFlush (confidence medium — EWMA-Logik CE-eigen, kein dediziertes Paper).
- Header-Korrektur (Map §4, KRITISCH): AdaptiveLsmFlush — der alte Code-Kommentar behauptet "Levandoski 2013" (= Bw-Tree, KEIN LSM-Flush). Diese Fehlattribution ist zu entfernen; konzeptuell korrekt sind RocksDB Dynamic Level + O'Neil LSM-Tree 1996 (s. §2).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): keine P-ID fuer axis_q2_queuing (Queuing-Achsen referenzieren externe OSS-Repos oder sind reine CE-Baselines/Standards — Map §6).
