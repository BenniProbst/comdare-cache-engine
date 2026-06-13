# GOAL 2026-06-13 — Autonome Abarbeitung: Messung + Audit-Wellen + interpretierbarer LaTeX-Appendix

> **Auftrag (User, 2026-06-13, verbatim-sinngemäß):** „Vielleicht lassen wir den Voll-Lauf auch
> einfach laufen, aber machen gleich mit den übrigen Aufgaben autonom weiter. Bitte formuliere ein
> goal, um alle in der letzten Session definierten Aufgaben und Audits autonom abzuarbeiten."
>
> **Kippschalter gegenüber dem Vorgänger-Goal:** `GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` sagte
> „Steuerung erfolgt MANUELL durch den User". **Dieses Goal hebt das auf: AUTONOME Ausführung** —
> nicht fragen, fortfahren, je Welle committen+pushen+Submodul-Bump. Die TODO-Substanz (M1-M3, L1-L8,
> A1-A5, Gates G1-G4) bleibt im Vorgänger-Goal autoritativ; dieses Dokument ist der **Ausführungs-
> Charter** darüber (Reihenfolge, Autonomie-Grenzen, M2-Sicherheits-Regeln, Stop-Bedingungen).

## §0 Autonomie-Mandat & Leitplanken (bindend)

1. **Autonom fortfahren ohne Rückfrage** für alles, was aus dem Auftrag folgt und reversibel ist.
   Destruktive Ops NUR in den 3 Thesis-Repos (Diplomarbeit / cache-engine / prt-art) erlaubt, sofern
   **Tag + Commit + Push** (Memory `feedback_destructive_autonomy_3repos_with_tag`). Infra/andere Repos
   bleiben bestätigungspflichtig.
2. **Zwei-Phasen-Cache-Warmup bleibt PFLICHT** (Mess-Gültigkeit). Der „Second-Execution"-Audit-Einwand
   wird NUR dokumentiert/dem User vorgelegt (A5), NIE stillschweigend umgesetzt.
3. **Pattern-Direktive** (`feedback_lehrbuch_design_patterns_only_zero_cost_metaprog`): neue Strukturen
   nur als benannte Lehrbuch-/erweiterte Patterns + Benennungskonvention (web-verifiziert); Metaprog nur
   zero-cost. Gilt für JEDEN Code dieses Goals (v.a. Welle 4 + Phase-L-Tools).
4. **Relative Pfade** in allen Thesis-/LaTeX-/Skript-Referenzen (git-clone-fest); **TU-Dresden-/ZIH-
   diplominf-Vorlage** (`zihpub.cls`) unangetastet; **EN≡DE-Äquivalenz** der Thesis-Builds.
5. **Keine Erfolgsmarke ohne literale Tool-Ausgabe** (`feedback_no_success_marks_without_literal_output`);
   je Welle Commit+Push + 3-Repo-Submodul-Sync. Roh-CSVs → lokale benannte Kopie + robuste NAS-Ablage;
   in git nur Aggregate/Appendix/PDF.
6. **Audit-Lehren anwenden** (`audit-sicherung-20260612/ERKENNTNISSE.md` §7): Capability-Pfade per
   static_assert über die ZIEL-Population (die 320) absichern, nicht nur Referenz-Kompositionen;
   Konfiguration pinnen statt stiller Defaults; Diff-Beweise nur mit nachweislich verschiedenen Pfaden.

## §1 M2-Sicherheits-Regeln (während der Voll-Lauf läuft)

Der M2-Resume-Lauf (`cowmem-v1`-DLLs, Hintergrund) misst über die **bereits kompilierten** 320 .dll auf
Disk. Daraus folgt für die parallele autonome Arbeit:

- **ERLAUBT während M2** (berührt M2-Binärdateien NICHT): Quell-Edits an `libs/cache_engine/**`
  (Header, gegen die die DLLs einst kompilierten — die fertigen .dll ändern sich dadurch nicht) +
  Unit-Test-Bau/-Lauf in **separaten** Build-Bäumen (`build/msvc-release`, nicht `build/thesis_tiere`);
  die komplette Phase-L-Tool-/Thesis-Arbeit (`Code/04..06`, `thesis/diplomarbeit/**`).
- **VERBOTEN während M2** (würde den laufenden Lauf/Resume korrumpieren): Stamp-Format-/CSV-Schema-/
  Harness-Matrix-Änderungen (invalidieren die 63 fertigen Stamps → Re-Measure-Sturm); ein zweiter
  Mess-Lauf in `build/thesis_tiere`; Neubau der 320 perm-DLLs.
- **Konsequenz:** Welle-1 *Harness/Stamp/CSV*-Punkte werden **gebatcht für den M3-Neubau** (cowfix-v1),
  NICHT mid-M2 eingespielt. Welle-2 *libs*-Edits laufen sicher parallel (verifiziert über Unit-Tests).
- M2 ist **Entwicklungs-Substrat + Sicherheitsnetz**, nicht die Abgabe-Daten. Stirbt M2 (Reboot o.ä.),
  genügt derselbe Harness-Befehl (Resume) — kein Datenverlust (per-Binary-Stamps). **Die Abgabe-Daten
  kommen aus M3 (cowfix-v1).**

## §2 Autonome Ausführungs-Reihenfolge (mit Begründung)

```
[läuft] M2-Resume (cowmem-v1)         → Interim-Matrix + L-Entwicklungsdaten + Netz   (tokenfrei, Hintergrund)
  ║ parallel, token-gebunden, autonom:
  ├─ Phase A (Audit-Wellen → cowfix-v1) = GATE ZU GÜLTIGEN DATEN     [Schwerpunkt: libs + Tests]
  │    A1 Welle1-Rest (mess-kritisch; Harness/Stamp-Teile gebatcht für M3)
  │    A2 Welle2 Apparat-Reinheit + CoW-real-für-die-320 (restore_statistics → 4 Pilot-Wrapper +
  │       static_assert über echte Pilot-Komposition) + echter Policy-Allocator + Iterator-Scan
  │    A3 Welle3 Dimension/Validität (RC nominal bis User-Entscheid; Scrambling; Konformitäts-Gate)
  │    A4 Welle4 Pattern-Hygiene (Etiketten/Naming gemäß Pattern-Direktive; kein Mess-Einfluss)
  └─ Phase L (Auswertungs-Pipeline) = baut Tools gegen Interim-Schema, läuft final gegen M3
       L1✓(Spalten da) · L2 3D-Surfaces je Interface-Fn · L3 Achsen-Austauschbarkeits-longtables
       L4 Appendix (ALLE Werte + ehrliche Limitierungs-Tabelle) · L5 thesis-Integration (relativ, EN≡DE)
       L6 Generalprobe (Teil-CSV → Test-PDF)
↓ nach Phase A komplett + M2 fertig/abgelöst:
M3 finaler Voll-Lauf (cowfix-v1, alle 21 Profile valide) → ersetzt M2-Daten
↓
L7/L8 Finale: EIN Experiment-Kommando → Aggregat → Appendix → build.ps1 beide Sprachen → fertige PDF
              + NAS-Ablage der Roh-Matrix
```

**Warum Phase A VOR der finalen Auswertung:** Die Achsen-Austauschbarkeits-Belege (Professor-Kern)
brauchen Daten, in denen Achsen-Unterschiede ECHT sind, nicht Apparat-Artefakt. cowmem-v1 trägt K1
(RC ×18→×3 degeneriert), K3 (CoW-Capability tot → Memento-Pfad für alle 320 identisch), K5 (Apparat-
dominierte Wall-Clock). Darum: Tools gegen das Interim-Schema bauen (richtige Form), aber die
**interpretierbaren Endbelege aus cowfix-v1 (M3)** ziehen. Das Interim dient L-Entwicklung + Generalprobe.

## §3 Stop-Bedingungen (NUR hier den User einbeziehen — sonst autonom)

- **A3-RC-Entscheid** (RC-Organ-Hooks bauen ODER RC-Dimension ehrlich entfernen): bis zur User-Antwort
  wird RC in der Auswertung als **nominal** ausgewiesen und fortgefahren — **kein Blockieren**.
- **A5 Zwei-Phasen-„Second-Execution"-Grundsatzfrage**: NUR Optionen dokumentieren, Entscheidung User —
  **kein Blockieren** (Zwei-Phasen bleibt unverändert PFLICHT bis User anders entscheidet).
- **Echte Unklarheit / fehlende Stand-Technik-Doku** (`feedback_never_guess_always_lookup…`): Lookup +
  bei Unsicherheit teure Planungssession gegen Architektur/Doku/Ist VOR dem Bauen — dann autonom weiter.
- Alles andere: **fortfahren, nicht fragen.**

## §4 Definition of Done (Abschluss-Gates, aus Vorgänger-Goal)

- **G1:** Phase-L-Pipeline produziert aus einer Matrix-CSV reproduzierbar Appendix + bilinguale PDF
  (gegen Interim verifiziert = Generalprobe ✓).
- **G2:** Audit-Wellen 1-3 literal verifiziert; **cowfix-v1**-DLLs gebaut; M3-Matrix komplett
  (~120.960 Zeilen, 0 Fehlmessungen); NAS-Ablage ✓.
- **G3:** L8-E2E ✓ — EIN Experiment-Aufruf → fertige bilinguale Diplomarbeit-PDF mit vollständigem
  Mess-Appendix aus der finalen Matrix; frischer git-clone baut identisch (relative Pfade); ZIH-Vorlage
  unverändert; Achsen-Austauschbarkeits-Belege interpretierbar (3D-Surfaces + Diff-longtables).
- **G4:** Welle 4 Pattern-Hygiene abgeschlossen; Session-Doku + Memory + 3-Repo-Sync nach jeder Phase.

## §5 Referenz-Dokumente (autoritativ)

| Zweck | Pfad (repo-relativ ab cache-engine) |
|-------|-------------------------------------|
| TODO-Substanz M/L/A + Gates | `docs/sessions/GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` |
| Audit-Wissen (K1-K10, 4 Wellen, Lehren) | `docs/sessions/audit-sicherung-20260612/ERKENNTNISSE.md` |
| Audit-Synthese + Fix-Plan §4 | `docs/sessions/20260611-audit-ergebnisse-synthese.md` |
| Mess-Audit Volltexte (57 Befunde) | `docs/sessions/20260612-messaudit-endergebnis.json` |
| Pattern-Audit Volltexte (28 Befunde) | `docs/sessions/20260611-patternaudit-ergebnis.json` |
| Memento Rev.2 (CoW) + Resume | `docs/architecture/33_undolog_memento_und_mess_resume.md` |
| Letzte Übergabe (Einstieg) | `docs/sessions/20260612-session-uebergabe-m2-laeuft-phase-l.md` |
| Bestehender Appendix-Orchestrator | `thesis/diplomarbeit/generate_measurement_appendix.ps1` |
| Mess-Resume / NAS | `scripts/collect_partial_results.ps1` · `scripts/copy_results_to_nas.sh` |

## §6 Sitzungs-Wiedereinstieg (jede neue Session)

1. M2-Status prüfen: `pwsh scripts/collect_partial_results.ps1` (Stamp-Zahl); falls Lauf tot →
   Resume-Befehl erneut (siehe Harness-Aufruf, `-Resume $true`, gleiche Parameter).
2. Offene Welle/L-Stufe aus diesem Goal + Tasks (#142-Cluster) fortsetzen.
3. Je abgeschlossener Einheit: literal verifizieren → committen+pushen → Submodul-Bump → Memory/Doku.
