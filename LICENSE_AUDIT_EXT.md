# Lizenz-Audit ext/-Repos (V31-PRE, 2026-05-14)

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

## Quellen-Pfade

```
ext/P01-ART/unodb/LICENSE        # Apache-2.0
ext/P02-HOT/hot/LICENSE          # ISC
ext/P03-Masstree/masstree-beta/LICENSE  # MIT
ext/P04-CoCo-trie/CoCo-trie/LICENSE     # GPL-3
ext/P05-START/START/LICENSE      # MIT
ext/P07-Wormhole/wormhole/LICENSE       # GPL-3
ext/P10-SuRF/SuRF/LICENSE        # Apache-2.0
ext/P20-BTreesAreBack/leanstore/LICENSE # MIT
ext/P29-RCU/userspace-rcu/LICENSE.md    # REUSE (LGPL-2.1+ + GPL-2)
ext/A01-hoard/LICENSE            # Apache-2.0
ext/A04-mimalloc/LICENSE         # MIT
ext/A05-jemalloc/LICENSE         # BSD-2
ext/A06-tcmalloc/LICENSE         # Apache-2.0
ext/A07-snmalloc/LICENSE         # MIT
ext/A08-scalloc/LICENSE          # BSD-3
ext/A10-rpmalloc/LICENSE         # Public-Domain (ISC-style)
ext/A11-lrmalloc/LICENSE         # MIT
```
