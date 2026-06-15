# Meilenstein: End-to-End-Mess-Kette verifiziert (KF-3 + KF-14, 2026-06-02)

> `/goal`: gebaute DLLs erzeugen Messwerte → Diplomarbeit-Kette → LaTeX-Auswertung. Dieser Meilenstein belegt
> die **funktionale Gesamtkette** mit echten Messdaten. Fundament-Meilenstein: `20260602-konfigurator-fundament-kf2-kf1-kf4.md`.

## KF-3 — per-Organ Cache-Line-Unterachse ✅ (Kernthema)

`libs/cache_engine/axes/cacheline/cacheline_config.hpp`: Compile-Time-Querschnitt-Komponente. Wertraum
(line_size{64,128,256} × alignment{none,aligned,padded} × sw_hint{none,T0,T1,T2,NTA} = 45) + Primitive
(`alignment_bytes`, `prefetch<Hint>` compile-time gebacken) + Concept `CacheLineConfigurable` + CRTP-Mixin
`CacheLineAware<Cfg>` (NTTP, **jedes Organ trägt seine EIGENE Config**) + `all_configs()`-Enumeration. Kein
Runtime-Switch (`if constexpr`). **Verifiziert:** Standalone-Test (cl) 10/10 + 2 static_asserts (OrganA{B128,Padded,T0}
≠ OrganB — per-Organ-Unabhängigkeit belegt). Commit ce `07f0019`. Organ-Verwebung = KF-5.

## KF-14 — End-to-End-Kette (DLLs → Messwerte → LaTeX) ✅

**Die vollständige Kette, mit echten gemessenen Daten:**

```
gebaute Perm-DLLs (43 smoke: SIMD×Layout×Allocator)
  → messung_driver        → measurements/all_permutations.bin   (magic 0xC0FFEE02, v2, 43 Records)
  → binary-to-csv          → measurements_cartesian.csv          (16-Spalten-Schema, 43 Datenzeilen)
  → generate_measurement_appendix.ps1  → anhang/{de,en}/tabellen/cartesian_smoke43_{table,diagram}.tex
  → A_measurements.tex (\input)  → build.ps1 -Lang en|de
  → diplomarbeit-{en,de}.pdf : Tabelle A.2 (S.31) + Abbildung A.2 (S.32), ECHTE Messwerte
```

**Verifiziert:** Thesis EN+DE je **50 Seiten, 0 fatale LaTeX-Fehler**; `.lot`→Table A.2 „Measurement series
cartesian\_smoke43", `.lof`→Figure A.2 „Cycles per organism permutation"; alle cartesian-Refs aufgelöst. Commit DA `d78f437`.

**Integrations-Befund:** Die Kette war NICHT gebrochen — `binary-to-csv.exe` war lediglich stale (status 11 gegen ein
älteres Record-Layout). Frischer Build löst die Konvertierung; Treiber (`MeasurementWriter`) + Stufe 03 nutzen
identisches `0xC0FFEE02`/v2-Format + dasselbe `comdare_measurement_record_v1` (`module_abi_v1.hpp`).

## Stand & verbleibende Tasks

| Bereich | Stand |
|---|---|
| Fundament (KF-2/1/4) | ✅ verifiziert |
| Cache-Line-Unterachse (KF-3) | ✅ verifiziert (Komponente; Organ-Verwebung KF-5 offen) |
| End-to-End-Kette (KF-14) | ✅ verifiziert mit echten Daten |

**Verbleibend autonom:** KF-9 (dynamische Enumeration aus dem Profil + 3-Modi-Maske + Fingerprint-Map) + KF-8
(C++/CEB-Generator: Profil → perm_<id>.cpp) → verbinden das NEUE `comdare_thesis_profile` mit der bewährten Kette;
KF-5 (Cache-Line in alle Organ-Algorithmen, groß), KF-6 (node echt verdrahten), KF-7 (Laufzeit-Durchlauf am Prüf-Dock),
KF-10 (Wiederholungen ×3, separat), KF-11 (Telemetrie silent-mode), KF-15 (inverse Auswertungslogik).
**Cluster-gated (nicht autonom):** KF-12 (SLURM-Launcher), KF-13 (ZIH-Skalierung) — brauchen die ZIH-Umgebung.

**Nächster Schritt:** KF-9 + KF-8 — die profil-getriebene DLL-Erzeugung, damit `cacheline_study.profile.xml` direkt
in dieselbe verifizierte Kette mündet.
