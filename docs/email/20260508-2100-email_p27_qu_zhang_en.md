# Email Vorlage P27 — Peng Qu + Youhui Zhang (Tsinghua, ASPLOS 2025) — English Version (REV 2)

**Stand:** 2026-05-08 21:00 (REV 2 nach Recherche)
**Sprache:** English (Tsinghua + Edinburgh)
**Werk:** "Hierarchical Prefetching: A Software-Hardware Instruction Prefetcher for Server Applications" (ASPLOS 2025)

---

## ⚠️ Korrektur gegenueber REV 1 (`20260508-2000-email_p27_tzhang_en.md`)

**Tingji Zhang ist Erstautor, aber NICHT Corresponding Author.** Corresponding Authors sind
laut publizierter Version **Peng Qu UND Youhui Zhang** (beide Tsinghua, mit Asterisk markiert).

Daher diese REV 2 mit korrekten Empfaengern. Die alte Vorlage `20260508-2000-email_p27_tzhang_en.md`
ist verworfen.

---

## Empfaenger-Routing

| Rolle | Name | Email | Verifiziert? |
|-------|------|-------|--------------|
| **TO** | Prof. Youhui Zhang (Senior, Corresponding) | `zyh02@tsinghua.edu.cn` | ✅ verifiziert |
| **TO** | Dr. Peng Qu (Junior-Senior, Corresponding) | `qupeng@tsinghua.edu.cn` (zu verifizieren) | ⏳ vermutet |
| **CC** | Prof. Boris Grot (Co-Autor, Edinburgh) | `boris.grot@ed.ac.uk` | ✅ verifiziert |
| **CC** | Tingji Zhang (Erstautor, Ph.D.) | `zhangtj22@mails.tsinghua.edu.cn` | ⏳ vermutet |
| **CC** | Prof. Dr. Dirk Habich (Diplomarbeits-Betreuer) | `dirk.habich@tu-dresden.de` | ✅ |

---

## Email Text

```
Subject: Methodology question — bundle selection in hierarchical prefetching (ASPLOS '25)

Dear Prof. Zhang, Dear Dr. Qu,

I am working on my diploma thesis at TU Dresden, supervised by Prof. Dr. Dirk
Habich. The thesis title is "Active Cache-Aware Hardware Adaptation Cache
Engine for Trie-Based Index Structures".

PUBLICATION OF INTEREST
   Title:    "Hierarchical Prefetching: A Software-Hardware Instruction
              Prefetcher for Server Applications"
   Authors:  Tingji Zhang, Boris Grot, Wenjian He, Yashuai Lv, Peng Qu*,
             Fang Su, Wenxin Wang, Guowei Zhang, Xuefeng Zhang, Youhui Zhang*
   Venue:    ASPLOS '25, 30th ACM International Conference on Architectural
             Support for Programming Languages and Operating Systems, Volume 2
   DOI:      10.1145/3676641.3716260
   License:  CC-BY (open access via Edinburgh Research Explorer)

Your bundle mechanic for hierarchical prefetching is a direct conceptual
inspiration for one of the prefetch strategies in my cache engine — it sits
on axis 7 (Prefetch) of our building-block cross-permutation matrix.

I have already located your repository CRAFT-THU/gem5-hp (BSD-3-Clause), which
is a gem5 fork. Since my work focuses on CPU-only database index structures
(not on instruction prefetching), I do not need the full gem5 simulator but
would like to transfer the bundle concept to a software-prefetch strategy
on data structures.

Question: Is there an isolated description of the bundle-selection algorithm
(for example as pseudocode, a short code snippet, or an isolated module
outside the gem5 integration) that I may use as the basis for a PRT-ART-
internal implementation via an adapter? A short technical note or a pointer
to a self-contained reference implementation would be very helpful.

I will of course cite your work and preserve the BSD-3-Clause license on
any derived components.

Best regards,
Benjamin-Elias Probst (s2631336@tu-dresden.de)

CC: Prof. Boris Grot (boris.grot@ed.ac.uk, Co-Author, EASE Lab Edinburgh)
    Tingji Zhang (first author, CRAFT Lab Tsinghua, for information)
    Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de, advisor)
```

## Anrede-Hinweise

- **"Dear Prof. Zhang"** fuer Youhui Zhang (Full Professor, etablierte Position)
- **"Dear Dr. Qu"** fuer Peng Qu (Assistant Researcher, hat den Doktor-Titel)
- Beide gemeinsame Anrede beruecksichtigt die Senior/Junior-Hierarchie + Corresponding-Author-Status
- Boris Grot in CC ist hilfreich, weil er als Edinburgh-Forscher westlich erreichbar ist

## Bundle-Mechanik-Hinweis

Anders als die REV-1-Version verzichtet diese Vorlage auf den ausfuehrlichen Hinweis zur
Lizenz-Klaerung — Paper ist CC-BY, Repo BSD-3-Clause, beides ist klar. Die Anfrage zielt
ausschliesslich auf eine isolierte methodische Beschreibung der Bundle-Selection.

## Versand-Vorbereitung-Checkliste

Vor Versand pruefen:
- [ ] Peng Qu's Email-Adresse: `qupeng@tsinghua.edu.cn` cross-checken via Tsinghua CS Faculty
      (alternativ `pengqu@tsinghua.edu.cn`)
- [ ] Tingji Zhang's Email als CC: `zhangtj22@mails.tsinghua.edu.cn` cross-checken via
      CRAFT-Lab-Profil oder ASPLOS-Paper-PDF
- [ ] Boris Grot Email aktuell: `boris.grot@ed.ac.uk` ✅ (sollte konstant sein)
- [ ] Habich CC: `dirk.habich@tu-dresden.de` ✅
