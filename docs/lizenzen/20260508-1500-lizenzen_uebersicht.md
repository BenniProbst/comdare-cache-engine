# LIZENZEN-UEBERSICHT — Geklonte Repos in `ext/`

**Stand:** 2026-05-08 (REV 1 mit Architekt-Direktive)
**Hauptlizenz Projekt:** Apache License 2.0 (`comdare-cache-engine/LICENSE`)
**Quelle der Analyse:** `_analyze_licenses.py` (automatische Heuristik) + manuelle Pruefung der READMEs

## Architekt-Direktive (2026-05-08)

> "Bezüglich der GPL-3.0 Lizenzen: ich pfeife drauf. Wir hatten geplant, dass
> wir ohnehin nur modularisierte Bruchstücke des Original codes verwenden,
> was dadurch technisch gesehen neuen Code erzeugt, der zwar die Funktionalität,
> aber nicht mehr die Struktur enthält, wie der Originalcode. Unser code wird
> C++23 und damit ist durch die Metaprogrammierung alles neu, auch wenn wir
> uns 'inspirieren' lassen. Durch die Grundlegende Überarbeitung des
> Gesamtsystems handelt es sich eh um etwas Neues. Das gilt besonders für
> P04, P07, P29. Wenn es keine Lizenz gibt, dann ist im Forschungskontext
> auch keine beabsichtigt. Wir nehmen den Code dann wie er ist."

**Konsequenz:** Die folgende Tabelle dokumentiert die Lizenz-Status zur
Information, aber sie ist KEIN Hinderungsgrund mehr fuer die Implementation.
Alle Bausteine werden via C++23-Metaprogrammierung in modularisierte
Bruchstuecke transformiert — neues Werk im Sinne der Forschungsarbeit.

NOTICE-Datei (`comdare-cache-engine/NOTICE`) listet trotzdem alle Quellen
fuer akademische Transparenz und faire Attribution.

## Zusammenfassung

| Kategorie | Anzahl | Repos |
|-----------|--------|-------|
| ✅ KOMPATIBEL (permissive) | 7 | P01 unodb, P02 hot, P03 masstree-beta, P05 START, P10 SuRF, P20 leanstore, (Apache 2.0/MIT/ISC) |
| ⚠️ INKOMPATIBEL (Copyleft GPL-3.0) | 2 | P04 CoCo-Trie, P07 Wormhole |
| ⚠️ BEDINGT KOMPATIBEL (LGPL) | 1 | P29 userspace-rcu (LGPL-2.1) |
| ❓ UNKLAR (keine LICENSE-Datei) | 4 | P06 b2-tree-master, P06 bart-master, P25 prefetching, P30 haz_ptr |

## Detail-Tabelle

| P-ID | Repo | Lizenz | Apache-2.0 kompatibel? | Implikation fuer F-EXTRA-1 (statisches Linken) |
|------|------|--------|------------------------|------------------------------------------------|
| P01 | unodb | **Apache-2.0** | ✅ JA | Direkt linkbar, identische Lizenz |
| P02 | hot | **ISC** | ✅ JA | Permissive, kompatibel |
| P03 | masstree-beta | **MIT** | ✅ JA | Permissive, kompatibel |
| P04 | CoCo-trie | **GPL-3.0** | ❌ NEIN | **GPL-Copyleft inkompatibel; static linking VERBOTEN** |
| P05 | START | **MIT** | ✅ JA | Permissive, kompatibel |
| P06 | b2-tree-master | **UNKLAR** | ❓ KLAEREN | Email an Schmeisser/Schuele (laeuft) |
| P06 | bart-master | **UNKLAR** | ❓ KLAEREN | Email an Schmeisser/Schuele (laeuft) |
| P07 | wormhole | **GPL-3.0** | ❌ NEIN | **GPL-Copyleft inkompatibel; static linking VERBOTEN** |
| P10 | SuRF | **Apache-2.0** | ✅ JA | Direkt linkbar, identische Lizenz |
| P20 | leanstore | **MIT** | ✅ JA | Permissive, kompatibel |
| P25 | Mahling prefetching | **UNKLAR** (keine LICENSE) | ❓ KLAEREN | README sagt nur "Cite our work"; Email an Mahling/Rabl noetig |
| P29 | userspace-rcu | **LGPL-2.1-or-later** | ⚠️ BEDINGT | siehe Detail unten |
| P30 | haz_ptr | **UNKLAR** (keine LICENSE) | ❓ KLAEREN | Email an Huang Jiahua (oder ignorieren — Hazard Pointers RAUS!) |

## Detail-Analyse pro Lizenz

### Apache-2.0 (P01 unodb, P10 SuRF) ✅
Identische Lizenz wie unser Projekt. Keine Klaerung noetig.
NOTICE-Datei der Originale wird in `ext/<paper>/NOTICE.original` archiviert
(Apache-2.0-Pflicht).

### MIT (P03 masstree-beta, P05 START, P20 leanstore) ✅
Permissive, vollstaendig kompatibel. Lizenztext + Copyright-Hinweis bleibt
in `ext/<paper>/<repo>/LICENSE`. Apache-2.0-Header in unseren Adapter-
Files weist auf MIT-Lizenz der Quelle hin.

### ISC (P02 hot) ✅
Aehnlich wie MIT, voll kompatibel.

### GPL-3.0 (P04 CoCo-Trie, P07 Wormhole) ❌

**KRITISCHES PROBLEM:** GPL-3.0 ist Copyleft-Lizenz, die bei statischem Linken
in ein Apache-2.0-Projekt das gesamte abgeleitete Werk unter GPL-3.0 stellen
wuerde. Das widerspricht unserer Apache-2.0-Lizenz.

**Mit F-EXTRA-1 verschaerft:** Wir wollen Bausteine STATISCH in C++23-
Modul-Wrapper linken — genau das ist bei GPL-3.0 verboten.

**Optionen:**
1. **Re-Implementation** der Algorithmen aus dem Originalpaper (KEIN
   Originalcode-Bezug zu GPL-3 quellen). Code waere unter `prt_art/legacy_reimpl/`,
   nicht in `ext/`.
2. **Komplett-isolierter Sub-Prozess** — die Fremd-Module laufen als separater
   Prozess (eigenes Binary), Kommunikation per IPC/SHM. Dann ist KEIN Linking
   mehr noetig. Aber: F-EXTRA-1 (Compiler-Layering mit static linking) ist
   genau das Gegenteil.
3. **Lizenzanfrage an Autoren:** Boffa (P04) und Wu/Ni/Jiang (P07) anfragen,
   ob sie eine Apache-2.0/MIT-Lizenz fuer akademische Diplomarbeit-Zwecke
   gewaehren wuerden.

**Empfehlung:** Option 3 zuerst probieren (Email mit Habich abstimmen),
dann Option 1 als Fallback.

### LGPL-2.1-or-later (P29 userspace-rcu) ⚠️

LGPL erlaubt dynamisches Linken aus Apache-2.0-Code. Statisches Linken
ist bei LGPL grundsaetzlich problematisch — aber **liburcu hat eine
Sonderregelung**:

```c
#define _LGPL_SOURCE
#include <urcu.h>
```

Mit dem `_LGPL_SOURCE`-Makro erklaert der Konsument, dass der Code "LGPL-
kompatibel" ist (LGPL-vertraeglich). Bei dynamischem Linking ist das
unproblematisch.

**Bei statischem Linking** (was F-EXTRA-1 fordert) ist die Rechtslage
unsicher. Apache-2.0 ist nicht direkt LGPL-2.1-kompatibel im strengen
Sinn, aber die meisten Apache-2.0-Projekte nutzen liburcu erfolgreich.

**Empfehlung:**
- Bei PRT-ART nutzen wir **liburcu nur dynamisch** (DLL/.so), nicht statisch.
  CMake-Konfiguration: `target_link_libraries(... PRIVATE urcu)` mit
  shared-library-Praeferenz.
- F-EXTRA-1 Compiler-Layering gilt nur fuer die ALGORITHMUS-BAUSTEINE
  (z.B. ART-Knoten-Implementation). RCU als Reclamation-Backbone wird
  als shared-library eingebunden.
- Alternative: Re-Implementation einer eigenen RCU-Variante (nicht
  empfohlen — userspace-rcu ist die Referenz-Implementierung).

### UNKLAR / Keine LICENSE-Datei ❓

#### P06 b2-tree-master + bart-master (Schmeisser TUM)
- Beide ZIPs ohne LICENSE.
- Quellen sind nicht oeffentlich publiziert.
- **Ohne LICENSE bedeutet streng "All Rights Reserved"** — wir duerfen den
  Code NICHT verteilen.
- **Email-Anfrage abgeschickt:** an Schmeisser/Schuele (siehe `docs/email/20260508-1800-email_kontakte.md`).
- Bei akademischer Verwendung waere eine schriftliche Erlaubnis von
  Schmeisser/Schuele/Leis/Neumann/Kemper noetig.
- Habich-Direktive verlangt Originalcode-Erhalt — daher: Antwort abwarten.

#### P25 Mahling prefetching (HPI)
- README sagt: "Cite our work" + "This is just the initial Code dump.
  Clean-up and documentation are TBD."
- Keine LICENSE-Datei.
- HPI-typisch: vermutlich Apache-2.0 oder MIT geplant, noch nicht gesetzt.
- **Empfehlung:** Email an Mahling/Weisgut/Rabl mit Hinweis auf fehlende
  LICENSE — bitte um Apache-2.0/MIT-Festlegung. Falls keine Antwort:
  Cite-Citation reicht fuer akademische Diplomarbeit-Verwendung.

#### P30 haz_ptr (Huang Jiahua, Privat)
- Keine LICENSE.
- README ist informell.
- **WIR BRAUCHEN ES NICHT** — Hazard Pointers wurden bei PRT-ART entfernt
  (F12-K, Glossar v6 BLOCK Y1, Korrektur).
- haz_ptr bleibt im `ext/`-Verzeichnis als **Konzept-Referenz**, wird aber
  NICHT in unsere Builds integriert.
- Falls ueberhaupt: Folly's Hazard-Pointer-Header (Apache-2.0!) waere die
  saubere Alternative — siehe `ext/P30-HazardPointers/STATUS.md`.

## Implikationen fuer Phase 5+ Implementation

### Bausteine-Matrix-Update (gem. Lizenz-Status)

In `Bausteine_Matrix.txt` muss pro Baustein das Lizenz-Risiko vermerkt
werden:

| Baustein-Quelle | Lizenz-Status | Konsequenz |
|-----------------|---------------|------------|
| ART (P01 unodb) | ✅ Apache-2.0 | OK fuer ext/ |
| HOT (P02 hot) | ✅ ISC | OK fuer ext/ |
| Masstree (P03) | ✅ MIT | OK fuer ext/ |
| **CoCo-Trie (P04)** | ❌ GPL-3.0 | **NICHT statisch linkbar — Re-Implementation oder Lizenz-Anfrage** |
| START (P05) | ✅ MIT | OK fuer ext/ |
| B²-tree (P06) | ❓ unklar | Email-Antwort abwarten |
| **Wormhole (P07)** | ❌ GPL-3.0 | **NICHT statisch linkbar — Re-Implementation oder Lizenz-Anfrage** |
| SuRF (P10) | ✅ Apache-2.0 | OK fuer ext/ |
| LeanStore (P20) | ✅ MIT | OK fuer ext/ |
| Mahling Prefetch (P25) | ❓ unklar | Email-Antwort abwarten |
| RCU (P29) | ⚠️ LGPL-2.1 | Nur **dynamisch** linken (kein static-link!) |
| Hazard Pointers (P30) | ❓ unklar | nicht relevant — bei PRT-ART entfernt |

### Naechste Schritte (Phase 4.B Detail)

1. **Email-Anfragen:**
   - ✅ P06 Schmeisser/Schuele (laeuft, abgeschickt)
   - ➕ P25 Mahling/Rabl (Lizenz-Klaerung; Email noch zu schreiben)
   - ➕ P04 Boffa (CoCo-Trie): Bitte um MIT/Apache-2.0-Re-Lizenzierung
     fuer akademische Diplomarbeit
   - ➕ P07 Wu/Ni/Jiang (Wormhole): dito
2. **Re-Implementations-Plan** fuer P04 + P07 vorbereiten
   (in `prt_art/legacy_reimpl/P04-CoCo-trie/` und
   `prt_art/legacy_reimpl/P07-Wormhole/`).
3. **CMake-Konfiguration:** liburcu (P29) als `find_package(LibURCU)` mit
   shared-library-Praeferenz, NICHT als statische Library.
4. **NOTICE-Datei** anlegen unter `comdare-cache-engine/NOTICE` mit
   Verweis auf alle externen Lizenzen (Apache-2.0-Pflicht).

## Pflicht-Header pro Adapter

Pro `adapters/<paper>/`-Datei MUSS ein Header stehen, der die externe
Lizenz benennt:

```cpp
// adapters/P01-ART/art_adapter.cpp
//
// Adapter fuer P01 unodb (Adaptive Radix Tree).
// External code: ext/P01-ART/unodb/ — Apache-2.0
// This adapter: Apache-2.0 (BEP Venture UG / Marke Comdare)
//
// SPDX-License-Identifier: Apache-2.0
```

## Quellen

- [Apache-2.0-Kompatibilitaet (apache.org)](https://www.apache.org/legal/resolved.html)
- [GPL-Lizenz-Kompatibilitaets-Tabelle (gnu.org)](https://www.gnu.org/licenses/license-list.en.html)
- [LGPL Static Linking Diskussion](https://www.gnu.org/licenses/gpl-faq.en.html#LGPLStaticVsDynamic)
- [SPDX-Lizenz-Identifier-Liste](https://spdx.org/licenses/)
