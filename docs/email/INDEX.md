# docs/email/ — INDEX

**Kategorie:** Email-Kommunikation mit externen Autoren / TUD-internen Co-Autoren
**Aktualisiert:** 2026-05-08

## Dokumente

| Datei | Zweck |
|-------|-------|
| [20260508-1800-email_kontakte.md](20260508-1800-email_kontakte.md) | **Hauptdokument:** Kontakt-Liste + Email-Vorlagen pro Werk (REV 2 mit Vollangaben-Regel nach Habich-Treffen) |
| [20260508-1700-email_habich_folge_klarstellung.md](20260508-1700-email_habich_folge_klarstellung.md) | Folge-Mail an Habich mit klar getrennten Werk-Bloecken (Klarstellung der ersten Anfrage zu Ungethuem 2017 / Schmidt 2025 / Berthold 2023) |
| [20260508-1900-mailverlauf_kuehn_p28_eingang.md](20260508-1900-mailverlauf_kuehn_p28_eingang.md) | Mailverlauf-Doku Roland Kuehn (TU Dortmund DBIS) — Code-Zusage erhalten + drei architektur-relevante Erkenntnisse (Cache-Kohaerenz-Ping-Pong, LeafOnly-Counter, Sampling-Variante) |
| [20260508-2000-email_p26_qzhang_en.md](20260508-2000-email_p26_qzhang_en.md) | **EN** Email-Vorlage P26 Qian Zhang (ECNU) — REV 1, ⚠️ ersetzt durch REV 2 |
| [20260508-2000-email_p27_tzhang_en.md](20260508-2000-email_p27_tzhang_en.md) | **EN** Email-Vorlage P27 Tingji Zhang (Tsinghua) — REV 1, ⚠️ verworfen (Tingji Zhang ist NICHT Corresponding Author!) |
| [20260508-2100-recherche_zhang_kontakte_und_copyright.md](20260508-2100-recherche_zhang_kontakte_und_copyright.md) | **Recherche-Notiz:** Email-Verifikation, Corresponding-Author-Identifikation, Copyright-Status der zwei Paper |
| [20260508-2100-email_p26_qzhang_en.md](20260508-2100-email_p26_qzhang_en.md) | **EN REV 2** Email-Vorlage P26 mit verifizierten Kontakten + Senior-CC (Xueqing Gong) — versand-vorbereitet |
| [20260508-2100-email_p27_qu_zhang_en.md](20260508-2100-email_p27_qu_zhang_en.md) | **EN REV 2** Email-Vorlage P27 mit Peng Qu + Youhui Zhang als TO — VERSCHICKT 2026-05-08 19:50, Mail an `qupeng@tsinghua.edu.cn` BOUNCED |
| [20260508-2200-email_p27_bounce_followup_en.md](20260508-2200-email_p27_bounce_followup_en.md) | **EN Folge-Mail nach Bounce** — kurze Entschuldigung + Bitte um Weiterleitung an Peng Qu (Domain ist `mails.tsinghua.edu.cn`, NICHT `tsinghua.edu.cn`) |

## Vollangaben-Regel (Lessons Learned 2026-05-08)

Bei Outbound-Mails ueber wissenschaftliche Werke pro Werk im Mail-Body
einen klaren Werk-Block am Anfang:

```
WERK
   Titel:    "..."
   Autoren:  Vorname1 Nachname1, Vorname2 Nachname2, ...
   Venue:    Konferenz/Journal Vollname + Jahr (+ Ort/DOI)
```

NIEMALS verwenden:
- Interne P-IDs (P01-P33)
- Kurzformen wie "Ungethuem 2017" / "Schmidt 2025" / "VAMPIR 2023"

Begruendung: Selbst Co-Autoren haben Dutzende Veroeffentlichungen pro Jahr und
koennen ohne Vollangaben nicht zuordnen, welches Werk gemeint ist.

Memory-Anker: `feedback_outbound_scientific_references.md`

## Status der Anfragen

| Werk | Status |
|------|--------|
| Ungethuem 2017 BTW (Hardware Optimizations) | ✅ Habich verschickt; ⏳ REV 2 Folge-Mail empfohlen |
| Schmidt 2025 DIMES (To Stride or Not) | ✅ Habich verschickt; ⏳ REV 2 Folge-Mail empfohlen |
| Berthold 2023 SPP2377 (VAMPIR Poster) | ✅ Habich verschickt; ⏳ REV 2 Folge-Mail empfohlen |
| Schmeisser 2022 Datenbank-Spektrum (B²-Tree) | ✅ verschickt; Code erhalten 2026-05-08 |
| Kuehn 2023 DaMoN (B+-Tree Cache Optimization) | ✅ verschickt |
| Q. Zhang 2024 FGCS (Prefetching Indexing) | 📝 Vorlage ready; nicht verschickt |
| T. Zhang 2025 ASPLOS (Hierarchical Prefetching) | 📝 Vorlage ready; nicht verschickt |
