# axis_03m_mapping — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** E (Engineering-Baseline) — beide Wrapper sind triviale CE-eigene Mapping-Idiome (linear packing / based-pointer-Arena) mit nur loser Paper-Lineage; kein OSS-Code, kein is_original-Kandidat.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking. Beide (`DirectPlacement`, `PoolRelative`) sind CE-eigene Engineering-Baselines ohne externen C/C++-Code; die zugeordneten Paper dienen ausschliesslich als lose Lineage/Analogie (kein Algorithmus-Code aus dem Paper uebernommen).

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| DirectPlacement | Direct slot-to-absolute-offset mapping (linear packing) | Organization and Maintenance of Large Ordered Indexes (lose Lineage) | Acta Informatica 1(3) 1972 | 10.1007/BF00288683 | nein | none | ✗ |
| PoolRelative | Pool-relative offset mapping (based-pointer, O(1) rebase) | Making Data Structures Persistent (lose Analogie) | JCSS 38(1) 1989 | 10.1016/0022-0000(89)90034-2 | nein | none | ✗ |

## §3 Compliance-Status
Beide Wrapper sind als CE-eigene Engineering-Baselines mit loser Paper-Lineage gekennzeichnet und tragen jeweils eine Paper-Referenz → Habich-Pflicht erfuellt. Es gibt keine is_original-Kandidaten in dieser Achse (Map §3 fuehrt keinen axis_03m-Wrapper) und keine lizenz-blockierten Codes. Offene Punkte laut Map §5: `DirectPlacement` (Conf. medium, Bayer/McCreight 1972 nur lose Lineage, Wrapper triviale CE-Baseline) und `PoolRelative` (Conf. medium, Driscoll 1989 nur lose Analogie, eigentlich Standard-Arena/Based-Pointer-Idiom). Map §4 listet keine Header-Korrekturen fuer axis_03m.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_03m_mapping, §5 offene Punkte)
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/: keine P-ID fuer axis_03m (beide Wrapper ohne local_forschung_id)
