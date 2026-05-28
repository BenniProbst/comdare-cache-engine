# axis_14_value_handle — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #731 R7.6 Paper-Identifikation
**Klasse:** C (license-blockiert) — alle is_original_module = false

## §1 Pflicht-Note (Goldstandard)

Pro Wrapper-Klasse: vollstaendige Paper-Referenz (Titel + Autoren + Venue + Jahr
+ DOI). Klasse C: kein direktes Original-Code-Linking (RCU LGPL-2.1, Hazard
Pointers NO LICENSE) — Re-Impl mit ehrlicher is_original=false-Deklaration.

## §2 Wrapper-Paper-Mapping

### §2.1 InlineValueHandle
- **Titel:** "Making B+-Trees Cache Conscious in Main Memory"
- **Autoren:** Jun Rao, Kenneth A. Ross
- **Venue:** SIGMOD 2000
- **DOI:** 10.1145/342009.335449 (P11 — CSS-Tree, kein Repo)
- **is_original_module:** false (Re-Impl, Pseudocode-Quelle)

### §2.2 ExternalPoolValueHandle
- **Quelle:** Oracle In-Memory (Industrie-Standard, kein Paper)
- **is_original_module:** false

### §2.3 ImmutableSharedRefValueHandle
- **Titel:** "Making Data Structures Persistent"
- **Autoren:** Driscoll, Sarnak, Sleator, Tarjan
- **Venue:** Journal of Computer and System Sciences (JCSS) 1989
- **DOI:** 10.1016/0022-0000(89)90034-2
- **Verwandt:** McKenney "RCU" OLS 2001 (P29, LGPL-2.1 → F2 eigene Impl Task #652)
- **is_original_module:** false (license-blockiert)

### §2.4 VersionedPointerValueHandle
- **Titel:** "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects"
- **Autoren:** Maged M. Michael
- **Venue:** IEEE Transactions on Parallel and Distributed Systems (TPDS) 2004
- **DOI:** 10.1109/TPDS.2004.8 (P30, NO LICENSE → Re-Impl; vgl. C++26 P0233R4)
- **is_original_module:** false (NO-LICENSE-blockiert)

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| InlineValueHandle | Rao+Ross SIGMOD 2000 | false | OK (Pseudocode) |
| ExternalPoolValueHandle | Oracle (Standard) | false | OK (kein Paper) |
| ImmutableSharedRefValueHandle | Driscoll+ JCSS 1989 / RCU | false | OK (LGPL) |
| VersionedPointerValueHandle | Michael TPDS 2004 | false | OK (NO LICENSE) |

## §4 Cross-Refs
- Doku 17 §4.5 Klasse C
- Task #652 F2 eigene RCU-Implementierung
