# axis_11_telemetry — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** mix — A (standalone is_original-faehig: LatencyHistogram via HdrHistogram_c, CC0-1.0) + E (Engineering/kontextuelle Telemetrie ohne benannten Algorithmus: DensityTracker, InsertCounter, LeafOnlyCounter).

## §1 Pflicht-Note
Nur `LatencyHistogram` hat echtes is_original-Linking-Potenzial (OSS + permissiv: HdrHistogram_c, CC0-1.0). Die uebrigen drei Wrapper (`DensityTracker`, `InsertCounter`, `LeafOnlyCounter`) sind kontextuelle CE-Telemetrie-Komponenten ohne benannten Telemetrie-Algorithmus im Paper — sie verweisen lediglich auf den Datenstruktur-Kontext (ART, HOT bzw. cache-freundliches B+-Layout) und sind Re-Impl/Baseline, kein Original-Code-Linking.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| DensityTracker | Per-Node Density/Slot-Occupancy Tracking | The Adaptive Radix Tree… (kontextuell; kein benannter Telemetrie-Algo) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | ✗ |
| InsertCounter | Globaler atomarer Insert-Zähler | HOT… (kontextuell; kein Insert-Counter-Algo) | SIGMOD 2018 | 10.1145/3183713.3196896 | lokal | MIT | ✗ |
| LatencyHistogram | HDR-Histogramm (p50/p95/p99 Lookup-Latenz) | HdrHistogram (Software/Konzept; Header zitiert fälschlich Wormhole) | ~2014 (Software) | github.com/HdrHistogram/HdrHistogram_c | OSS | CC0-1.0 | ✓ |
| LeafOnlyCounter | Counter nur in Blatt-Knoten (Anti-False-Sharing) | Towards Data-Based Cache Optimization of B+-Trees (Kuehn; beschreibt KEINEN Leaf-Counter) | DaMoN 2023 | 10.1145/3592980.3595303 | nein | none | ✗ |

## §3 Compliance-Status
Alle 4 Wrapper haben eine Paper-Ref ODER sind als kontextuelle CE-Telemetrie/Baseline gekennzeichnet → Habich-Pflicht erfuellt.

- **is_original-Kandidat (Map §3):** `LatencyHistogram` → Repo github.com/HdrHistogram/HdrHistogram_c, Lizenz CC0-1.0. Einziger Wrapper der Achse mit is_original_eligible = true (echtes Original-Code-Linking moeglich).
- **Lizenz-blockiert:** keiner in dieser Achse.
- **Korrekturen aus Map §4 angewendet (alte falsche Attributionen ersetzt):**
  - `InsertCounter`: Venue "PVLDB 2018" (HOT) → korrekt **SIGMOD 2018**.
  - `LatencyHistogram`: Code-Kommentar "Wormhole, Wu, SIGMOD 2019" (doppelt falsch) → korrekt **HdrHistogram (Gil Tene)**; Wormhole ist EuroSys 2019 und misst keine Latenz-Histogramme.
  - `LeafOnlyCounter`: Code-Kommentar "Kuehn DaMoN 2023 X1: Counter nur in Blatt-Knoten" → Fehlattribution; das Kuehn-Paper behandelt cache-freundliches B+-Layout, KEINEN Leaf-Counter.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_11_telemetry, §3 is_original-Kandidaten, §4 Header-Korrekturen, §6 lokaler Katalog)
- Doku 17 §4.5 (Klassifikation A/C/D/E)
- Lokaler Katalog Forschungsarbeiten/code/ (P-IDs, Map §6): P01 = ART/unodb (Leis ICDE 2013, Apache-2.0) → DensityTracker; P02 = HOT/hot (Binna SIGMOD 2018, ISC) → InsertCounter; P28 = Kuehn Cache-Optimization B+-Trees (DaMoN 2023, TU-Dortmund-intern, KEIN public Code) → LeafOnlyCounter.
