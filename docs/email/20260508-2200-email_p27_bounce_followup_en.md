# P27 — Bounce-Follow-up nach gescheiterter Zustellung an `qupeng@tsinghua.edu.cn`

**Stand:** 2026-05-08 22:00 (nach Bounce 2026-05-08 19:50)
**Anlass:** Erste Anfrage an Peng Qu wurde abgelehnt mit `550 Recipient verification failed`.
Die anderen Empfaenger (Youhui Zhang, Boris Grot, Tingji Zhang, Habich) haben die Mail aller
Wahrscheinlichkeit nach erhalten — kein Bounce fuer sie.

## Was wir wissen

| Empfaenger | Email | Status |
|------------|-------|--------|
| Prof. Youhui Zhang | `zyh02@tsinghua.edu.cn` | ✅ zugestellt |
| Dr. Peng Qu | `qupeng@tsinghua.edu.cn` | ❌ **BOUNCED** (Recipient verification failed) |
| Prof. Boris Grot | `boris.grot@ed.ac.uk` | ✅ zugestellt (CC) |
| Tingji Zhang | `zhangtj22@mails.tsinghua.edu.cn` | ✅ zugestellt (CC) |
| Prof. Dirk Habich | `dirk.habich@tu-dresden.de` | ✅ zugestellt (CC) |

## Recherche-Ergebnis

- Peng Qu's verifizierter Email-Domain laut Google Scholar: `mails.tsinghua.edu.cn`
  (NICHT `tsinghua.edu.cn`!)
- Genaue Adresse oeffentlich nicht auffindbar
- Tsinghua-Format-Konvention: `<Initialen><Zahl>@mails.tsinghua.edu.cn`
- Wahrscheinliche Kandidaten: `qupeng@mails.tsinghua.edu.cn`, `qup18@mails.tsinghua.edu.cn`,
  `qup13@mails.tsinghua.edu.cn` — alle nicht verifiziert

## Pragmatische Strategie

Statt drei Format-Varianten durchzuprobieren (mit weiteren Bounces): eine **kurze
Folge-Mail** an Youhui Zhang (Senior, Corresponding Author) und Boris Grot (Co-Autor,
westlich) schicken, die sich fuer den Bounce entschuldigt und um Weiterleitung +
korrekte Adresse bittet.

---

## Email Text (Bounce-Follow-up)

```
Subject: Apologies — Re: Methodology question — bundle selection in hierarchical prefetching (ASPLOS '25)

Dear Prof. Zhang, Dear Prof. Grot,

a brief follow-up to my earlier email of today: my message bounced for
Dr. Peng Qu, as the address qupeng@tsinghua.edu.cn that I had inferred
from the Tsinghua faculty conventions is apparently not active.

Could either of you kindly forward the request to Dr. Qu, or share his
correct email address (likely on the mails.tsinghua.edu.cn domain) so
that I may resend it directly?

Apologies for the inconvenience and thank you in advance.

Best regards,
Benjamin-Elias Probst (s2631336@tu-dresden.de)
CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de, advisor)
```

## Empfaenger-Routing (Folge-Mail)

| Rolle | Name | Email |
|-------|------|-------|
| **TO** | Prof. Youhui Zhang | ✅ `zyh02@tsinghua.edu.cn` |
| **TO** | Prof. Boris Grot | ✅ `boris.grot@ed.ac.uk` |
| **CC** | Prof. Dirk Habich | ✅ `dirk.habich@tu-dresden.de` |

**Tingji Zhang nicht in CC** — er hat die Erst-Mail bereits erhalten. Verschlankte
Folge-Mail nur an die zwei Senior-Empfaenger fuer schnelle Aufloesung.

## Falls keine Antwort innerhalb von ~1 Woche

Alternative Wege fuer Peng Qu's Email:
1. **Boris Grot direkt anschreiben** (er ist westlich erreichbar und antwortet
   typischerweise schnell)
2. **CRAFT Lab Contact-Adresse:** Tsinghua University, Zi Qiang Ke Ji Building,
   Beijing 100084 — postalisch oder Faculty-Verzeichnis-Anfrage
3. **ResearchGate-Direktnachricht** an Peng Qu (Profil-URL:
   https://www.researchgate.net/profile/Peng-Qu-8) — funktioniert oft fuer Adress-
   Anfragen
4. **DBLP-Korrekturanfrage:** ueber DBLP-Kontaktformular nach Peng Qu's Korrespondenz-
   Adresse fragen
