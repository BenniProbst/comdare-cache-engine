# 2026-06-13 — ARCHITEKTUR-KORREKTUR: Achsen-Austausch gehört in den B+-Baum (cache-engine), NICHT flach in die Auswertung

> **PAUSE (User 2026-06-13): „Bitte pausiere, wir müssen unbedingt ein stärkeres Modell verwenden."** Dieses Doc
> ist der saubere Übergabepunkt — das stärkere Modell setzt hier KORREKT fort. Vorheriger Fehler unten dokumentiert.

## 1. Der Verstoß (User-Korrektur, 2 Nachrichten)
- „Baust du gerade **illegale flache Tupel für die Achsen**?" → JA. L1 (Stufe 04) erzeugte
  `WideMeasurementRow.axes = vector<AxisAssignment>` per String-Parsing aus `binary_id` (`parse_axis_tuple`) und
  **duplizierte damit `ExperimentTree::parse_axes`**; der (gestoppte) L3-Agent sollte darüber flache
  kombinatorische Paarbildung machen. Beides umgeht den Baum = Verstoß gegen [[feedback_always_use_trees_for_search]].
- „Die Existenz des **B+-Experiment-Baums** für den Achsenaustausch als NICHT-flache Struktur … Es ist auch
  **nur dort das Framework anzusiedeln**." → Der Achsen-Austausch-Framework gehört AUSSCHLIESSLICH in die cache-engine.

## 2. Die korrekte non-flache Semantik (verifiziert an `builder/experiment_tree/experiment_tree.hpp`)
- Die **Achsen sind die Baum-Ebenen** (`AxisLevel{axis, values[]}`); ein Pfad Wurzel→Blatt = ein `binary_id` = ein Lebewesen.
- `StaticBinaryView` = **Mixed-Radix-Odometer**: `operator[](i)` (Z.244) und `flat_index(tuple)` (Z.237) sind
  **inverse Bijektionen**; `level_count()`/`level_size(d)` geben die Ebenen-Geometrie.
- **Achsen-Austausch ist tree-nativ**: nimm `tuple` eines Lebewesens, ändere **nur `tuple[d]`** (Ebene d = Achse a)
  von k→k', `flat_index` → das Geschwister-Lebewesen, das sich in **GENAU** Achse a unterscheidet. O(Geschwister),
  kein quadratischer Flach-Scan. Doc 26.
- Mess-Werte je Lebewesen liegen sparse in `value_map_` (`set_node_value`/`node_value`, key=`binary_id` → `NodeValue`).

## 3. User-ENTSCHEIDUNG (AskUserQuestion, 2026-06-13): **Option A — cache-engine-Stage erzeugt die Diff-Tabellen**
```
cache-engine (neue Stage/Komponente, NEBEN experiment_tree):
  axis_exchange_pairs(level d)  -> via StaticBinaryView.flat_index die Geschwister-Paare (nur 1 Achse diff)
  + join (Mess-Werte je binary_id × workload × Interface-Funktion)  -> Δns/op je (a, v->v')
  -> emittiert diff_tables.tex (longtable) + optional .csv
Stufe 04/05 (Superprojekt): NUR die 3D-Surfaces. KEIN binary_id-Parse, KEINE flachen Achsen-Tupel im Eval-Tool.
```

## 4. Re-Architektur-Auftrag fürs stärkere Modell (L3-Ersatz + L1/L2-Bereinigung)
**NEU (cache-engine):** Eine Achsen-Austausch-Komponente in `builder/experiment_tree/` (benanntes Pattern; nutzt
`StaticBinaryView`): je Achsen-Ebene d die Geschwister-Paare (i,i') aufzählen, die sich nur in `tuple[d]`
unterscheiden (Gruppierung = „alle Ebenen außer d fix"); je Paar+Workload+Interface-Funktion Δns/op aus den
Mess-Werten; Aggregat Median/IQR; LaTeX-longtable-Emitter (relative Pfade, de/en, body+standalone).
- ⚠️ **Daten-Join-Detail:** die per-Interface-Funktions-Latenzen (`op_<art>_p50_ns` je Workload) liegen in der
  **Matrix-CSV**, NICHT in `NodeValue` (das trägt `axis_stats[19][8]`+`seg_ns[19]`, keine op-Latenzen-je-Workload).
  → Entweder die Stage liest die CSV (binary_id×workload→op-Latenzen) und nutzt den Baum NUR für die
  Geschwister-Paarbildung, ODER `NodeValue`/`value_map_` wird um die op-Latenzen-je-Workload erweitert. Design
  vom stärkeren Modell entscheiden (Baum bleibt die Struktur-/Paarbildungs-Quelle — das ist der Kern der Direktive).
- `ExperimentTree` wird aus der Registry gebaut (`registry_to_axis_levels.hpp` → `AxisLevels`), wie im Mess-Lauf.

**BEREINIGEN (Stufe 04, L1):** `WideMeasurementRow.axes` (vector<AxisAssignment>) + `parse_axis_tuple` + die
`AxisAssignment`-Struktur ENTFERNEN (illegale Flach-Tupel-/parse-Duplikation von `ExperimentTree::parse_axes`).
**BEHALTEN (L1, korrekt — betrifft Mess-WERTE, nicht Achsen-Struktur):** `parse_wide_csv` header-getrieben,
`TestdataConfig` (workload/dyn-dims/n_ops), `array<OpLatency,6>` (18 op_-Spalten, Single-Source `kOpKindNames`),
`aggregate_tier_workload_per_op` (Median p50_ns je Interface-Fn, two_phase_valid).
**L2 (3D-Surfaces) BEHALTEN**, aber die **Lebewesen-y-Ordnung** vom Flach-Tupel-Lexikografie-Komparator auf den
**Baum-Index** (`StaticBinaryView`-Reihenfolge) umstellen, sobald die Stage den Index liefert.

## 5. Was KORREKT + committet ist (NICHT anfassen)
- **A2a/K3** (cache-engine, ce `4a64bc8`): `restore_statistics` in alle 17 produktiven `lookup::*SearchAlgo`-Roh-
  Wrapper → `organ_cow_capable_v` für die 320 aktiv; Verifikations-TU `test_cow_capable_wrappers` grün. RICHTIG.
- **L1 Mess-Wert-Teile** (super `c610354`) + **L2 Surfaces** (super `1ec5cfd`) — bis auf die §4-Bereinigung gültig.
- **M2-cowmem-v1-Voll-Lauf läuft** im Hintergrund (Resume, ~63+/320) — NICHT stören.
- Verifizierter Gesamt-Arbeitsplan: `20260613-autonom-A2a-done-verifizierter-arbeitsplan.md` (L4-L8/A2b-A5 unverändert;
  NUR L1-axes/L3 sind durch diese Korrektur ersetzt).

## 6. Übergabe-Status
PAUSIERT für stärkeres Modell. Nächster Schritt: die cache-engine-Achsen-Austausch-Stage (§4) bauen — gegen
den B+-Baum, mit static_assert/Test, literal verifiziert; danach L4 (Appendix) → L5/L6/L7 → batch_for_m3-Wellen.
Aktives /goal: `GOAL-AUTONOM-ABARBEITUNG-20260613.md` (+ §2.5 Audit-Backlog, §2.5.6 Rohdaten-Manifest).
