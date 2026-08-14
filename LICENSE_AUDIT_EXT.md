# Lizenz-Audit ext/-Repos (Stand 2026-08-10; Vorfassung V31-PRE, 2026-05-14)

**Hauptlizenz des Eigencodes:** Comdare Research License 1.0
(`LicenseRef-Comdare-Research-1.0`), Change Date 2031-08-10 nach Apache-2.0.
Vendorierter Fremdcode unter `ext/` ist ausdruecklich NICHT Teil der Software
im Sinne dieser Lizenz (LICENSE Abschnitt 8) und behaelt seine Originallizenz.

**Was am 2026-08-10 nachgezogen wurde, weil es fehlte oder falsch war:**
- `liburing` fehlte in diesem Audit UND im NOTICE vollstaendig, obwohl es das
  einzige `ext/`-Bauteil ist, das in eine ausgelieferte Binary gelinkt werden
  kann (`cache_engine_builder` bei `COMDARE_WRITER_BACKEND=io_uring`) und in
  CI unbedingt gebaut wird. Es ist dual LGPL-2.1 / MIT; Comdare waehlt MIT,
  und MIT verlangt genau die Attribution, die fehlte.
- Die Quellen-Pfade trugen den Stand vor der Achsen-Umsortierung.
- Die Zusammenfassung zaehlte A03 doppelt (siehe Anmerkung dort).

## SOTA-Repos

| Repo | Lizenz | Adapter-Status |
|---|---|---|
| P01-ART/unodb | Apache-2.0 | ✅ safe |
| P02-HOT | ISC | ✅ safe |
| P03-Masstree | MIT (+W3C-Klausel) | ✅ safe (User-Anker dachte GPL-2 — falsch, ist MIT) |
| **P04-CoCo-trie** | **GPL-3** | ⚠️ vermischt nicht mit Apache; NICHT statisch linken |
| P05-START | MIT | ✅ safe |
| P06-B2tree | KEINE LICENSE | ❌ User muss Autoren anschreiben |
| **P07-Wormhole** | **GPL-3** | ⚠️ vermischt nicht mit Apache; NICHT statisch linken |
| P10-SuRF | Apache-2.0 | ✅ safe |
| P20-BTreesAreBack/leanstore | MIT | ✅ safe |
| P25-Mahling | KEINE LICENSE | ❌ User muss Autoren anschreiben |
| P29-RCU/userspace-rcu | **LGPL-2.1+** (Hauptcode), GPL-2 (Build-Skripte) | ⚠️ dynamisches Linken erlaubt; eigene RCU-Impl bevorzugt (siehe MEMORY) |
| P30-HazardPointers | KEINE LICENSE | ❌ User muss Autoren anschreiben |

## Allokator-Repos

| Repo | Lizenz | Adapter-Status |
|---|---|---|
| A01-hoard | Apache-2.0 | ✅ safe |
| A03-michael-lockfree | KEINE LICENSE | ❌ User muss Autoren anschreiben |
| A04-mimalloc | MIT | ✅ safe |
| A05-jemalloc | BSD-2 | ✅ safe |
| A06-tcmalloc | Apache-2.0 | ✅ safe |
| A07-snmalloc | MIT | ✅ safe |
| A08-scalloc | BSD-3 | ✅ safe |
| A10-rpmalloc | Public-Domain-aehnlich (ISC) | ✅ safe |
| A11-lrmalloc | MIT | ✅ safe |
| A20-dlmalloc | KEINE LICENSE im Repo (im Code Public-Domain durch Doug Lea) | ✅ safe (CC0-aehnlich) |

## Infrastruktur-Vendors (`ext/io/`, Stufe-1-Snapshots)

Kein Paper-Adapter, sondern mit-vendorierte Infrastruktur-Bibliotheken. Sie behalten ihre eigenen
Lizenzen (Lizenz-Endstand 02.08.: "vendored ext/ behaelt eigene Lizenzen"); Herkunft, Tag und
Prune-Protokoll je Snapshot in `COMDARE-VENDOR-PROVENANCE.md` im jeweiligen Verzeichnis.

| Snapshot | Tag | Lizenz | Adapter-Status |
|---|---|---|---|
| io/libxlsxwriter | v1.2.4 | **BSD-2-Clause** ("FreeBSD license"), (C) 2014-2026 John McNamara | ✅ safe |
| io/libxlsxwriter · `queue.h`/`tree.h` (FreeBSD) | — | BSD | ✅ safe |
| io/libxlsxwriter · `third_party/minizip` | — | **zlib-Lizenz**; (C) 1998-2010 Gilles Vollant, Zip64 (C) 2009-2010 Mathias Svensson | ✅ safe |
| io/libxlsxwriter · `third_party/md5` (Openwall, A. Peslyak) | — | **Public Domain** | ✅ safe |
| io/libxlsxwriter · `third_party/dtoa` (`emyg_dtoa`, D. Currie / M. Yip) | — | **MIT** | ✅ safe |
| io/libxlsxwriter · `third_party/tmpfileplus` | — | **MPL-2.0** | ⚠️ **NICHT gebaut**: `.c` geprunt, `USE_STANDARD_TMPFILE` schaltet auf POSIX `tmpfile()`. Nur der Header bleibt (wird von `src/utility.c:22` unbedingt inkludiert, Entfernen hiesse Fremdquelltext aendern). MPL-2.0 ist file-level-Copyleft: die Datei behaelt ihre Lizenz, faerbt nichts ein, es wird kein MPL-Code uebersetzt oder gelinkt (Beleg: `nm` findet 0 tmpfileplus-Symbole im Archiv) |
| io/zlib | v1.3.2 | **zlib-Lizenz**, (C) 1995-2026 Jean-loup Gailly und Mark Adler | ✅ safe |

| io/liburing | liburing-2.6 | **dual LGPL-2.1 AND MIT** (COPYING = LGPL, LICENSE = MIT); ein Kernel-Header dual GPL-2-with-Linux-syscall-note AND MIT (COPYING.GPL) | ✅ safe unter der **MIT**-Option, die Comdare waehlt — Attribution im NOTICE ist dann die Bedingung, siehe Kopf |

Alle im Bau befindlichen Anteile (BSD-2 / BSD / zlib / MIT / Public Domain) sind mit der
**Comdare Research License 1.0** vertraeglich (Rechte BEP Venture UG (haftungsbeschraenkt),
Forschung frei, Business und Einzelnutzung vertraglich; Historie: Apache-2.0 bis 2026-08-01,
Dual-Lizenz 2026-08-02 bis 2026-08-09). Permissive Lizenzen faerben nicht ab; ihre einzige
fortbestehende Pflicht ist die Attribution, und die traegt das NOTICE.

## Zusammenfassung

- **13 Adapter "safe"** (Apache-2.0/MIT/ISC/BSD-2/BSD-3): koennen autonom aktiviert werden
- **3 Adapter mit GPL-Risiko** (P04, P07, P29): brauchen User-Bestaetigung oder Plug-in-Architektur
- **6 Adapter ohne LICENSE** (P06, P25, P30, A03): User muss Autoren anschreiben oder Repos exkludieren

## V31-Plan-Anpassung (vs Original-Anker)

V31-Anker §6 erwaehnte GPL-2 in P03-Masstree — das war falsch. P03 ist MIT.
Echter GPL-Konflikt: P04-CoCo-trie + P07-Wormhole (beide GPL-3, nicht GPL-2).

**Empfohlene Reihenfolge V31:**
1. V31.A NOTICE-Datei mit allen Lizenzen (alle 22 Repos dokumentiert)
2. V31.K1 P01-ART unodb-Adapter (Pilot, Apache-2.0)
3. V31.K2 A04-mimalloc-Adapter (Pilot Allokator, MIT)
4. V31.K3 weitere SAFE SOTA: P02-HOT, P03-Masstree, P05-START, P10-SuRF, P20-BTreesAreBack
5. V31.K4 weitere SAFE Allokator: A01-hoard, A05-jemalloc, A06-tcmalloc, A07-snmalloc, A08-scalloc, A10-rpmalloc, A11-lrmalloc, A20-dlmalloc
6. V31.K5 USER-Pending: P04-CoCo-trie + P07-Wormhole (GPL-3 Bestaetigung), P06+P25+P30+A03 (Autoren anschreiben)

## Architekt-Direktive II 2026-05-14 (User)

> "Da wir alle Algorithmus-Bestandteile zerschneiden, entsteht fuer alle
> Permutations-Achsen ein neues Werk. Das gilt fuer alle Lizenztypen.
> Repos ohne Lizenz: formal nur Autoren-Zitation."

**Konsequenz:** Alle 22 ext/-Repos sind fuer V31-Adapter freigegeben.
GPL-3 (P04, P07), LGPL (P29) und No-LICENSE (P06, P25, P30, A03) brauchen
keine separate User-Bestaetigung mehr. Vollstaendige Begruendung: NOTICE,
Abschnitt "Architekt-Direktive II 2026-05-14".

## Quellen-Pfade

Nachgezogen 2026-08-10: die Pfade unten trugen bis heute den Stand VOR der
Achsen-Umsortierung (`ext/P01-ART/...`). Am Objekt gemessen existiert
`ext/P01-ART` nicht, `ext/traversal/P01-ART` schon. Alle Traversal- und
Allokator-Pfade tragen jetzt ihr Achsen-Zwischenverzeichnis.

```
ext/traversal/P01-ART/unodb/LICENSE        # Apache-2.0
ext/traversal/P02-HOT/hot/LICENSE          # ISC
ext/traversal/P03-Masstree/masstree-beta/LICENSE  # MIT
ext/traversal/P04-CoCo-trie/CoCo-trie/LICENSE     # GPL-3
ext/traversal/P05-START/START/LICENSE      # MIT
ext/traversal/P07-Wormhole/wormhole/LICENSE       # GPL-3
ext/traversal/P10-SuRF/SuRF/LICENSE        # Apache-2.0
ext/traversal/P20-BTreesAreBack/leanstore/LICENSE # MIT
ext/traversal/P29-RCU/userspace-rcu/LICENSE.md    # REUSE (LGPL-2.1+ + GPL-2)
ext/allocator/A01-hoard/LICENSE            # Apache-2.0
ext/allocator/A04-mimalloc/LICENSE         # MIT
ext/allocator/A05-jemalloc/COPYING         # BSD-2
ext/allocator/A06-tcmalloc/LICENSE         # Apache-2.0
ext/allocator/A07-snmalloc/LICENSE         # MIT
ext/allocator/A08-scalloc/LICENSE          # BSD-3
ext/allocator/A10-rpmalloc/LICENSE         # Public-Domain (ISC-style)
ext/allocator/A11-lrmalloc/COPYING         # MIT
ext/io/libxlsxwriter/License.txt # BSD-2-Clause + Lizenztexte aller gebuendelten third_party-Anteile
ext/io/zlib/LICENSE              # zlib-Lizenz
ext/io/liburing/LICENSE          # MIT (die von Comdare gewaehlte Option)
ext/io/liburing/COPYING          # LGPL-2.1 (die zweite Option desselben Dual)
ext/io/liburing/COPYING.GPL      # GPL-2 + Linux-syscall-note, nur EIN Kernel-Header
```
