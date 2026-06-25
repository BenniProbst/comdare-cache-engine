# SESSION-ÜBERGABE — Implementierungs-Agent (2026-06-25): GitLab-CI/CD-Aufbau, Runner-Blocker, Code-Lags

> **Rolle:** Implementierungs-Agent (Thesis cache-engine, autonomer /goal). **Audience:** die nächste Impl-Session.
> **Diese Session war ungewöhnlich INFRA-/GitLab-lastig** (CI/CD-Aufbau) — die meisten Code-Aufgaben blieben deshalb offen und sind hier + im konsolidierten TODO-Doc gebündelt.
>
> 📌 **SINGLE-SOURCE der offenen Impl-TODOs (zuerst lesen!):** `(super) docs/sessions/20260625-IMPL-AGENT-KONSOLIDIERTE-TODOS-HANDOVERS-WARTBARKEIT.md` (Commit `bb5e5be`) — 28 deduplizierte Impl-Aufgaben (§A) + 3 neue User-Anforderungen (§B) + Reihenfolge & harte Regeln (§C). Dieses Übergabe-Doc ist der **Erzähl-Kontext** dazu, NICHT die TODO-Liste selbst.

---

## 0. Die 4 Repos + ihre GitLab/GitHub-Identität (NEU diese Session)

| Lokal | GitHub | **GitLab (NEU)** | gitlab-Remote gesetzt |
|---|---|---|---|
| super `Diplomarbeit - Datenbanken` | `probst-Diplomarbeit-cache-engine` | `comdare/research/probst-diplomarbeit-cache-engine` (id 288) | ✅ |
| cache-engine `Code/external/comdare-cache-engine` | `comdare-cache-engine` | `comdare/research/comdare-cache-engine` (id 286) | ✅ |
| prt-art `Code/external/comdare-prt-art` (Submodul) | `comdare-prt-art` | `comdare/research/comdare-prt-art` (id 287) | ✅ |
| thesis-text `thesis/diplomarbeit` (NEU Submodul) | `20260931-Overleaf-Diplomarbeit` | `comdare/research/20260931-overleaf-diplomarbeit` (id 289) | ✅ |

GitLab-Gruppen spiegeln `Desktop/Projekte/`: `comdare/{modules,products,research,cluster}`. cluster_development ≡ `Projekte/Cluster` (EIN Repo, 2 Klone) → `comdare/cluster/comdare-cluster-development` (privat).

---

## 1. Was diese Session GELIEFERT + committet/gepusht wurde

- **Phase B `LinuxPerfPmcSource`** (cache-engine `47fef0f`, super-Bump `ed701e6`): reale Cache-Miss-Quelle via `perf_event_open(2)` (+ PAPI-Fallback), additiv hinter `IPmcSource`, `#if COMDARE_ENABLE_PMC && __linux__`-geguardet, Windows-Build unberührt (CMake-Re-Configure Exit 0). ctest-Smoke `linux_perf_pmc_smoke` Linux-conditional. **Counter≠0-Beweis steht aus (prod-Runner).**
- **cache-engine CI REV8** (`1046587`, Datei `.gitlab-ci.yml`): Production-Bare-Metal-Flow `build:amd/intel → measure:amd/intel` (shell-Executor, `COMDARE_ENABLE_PMC=ON`, `linux_perf_pmc_smoke`). `workflow:`-Regeln unterdrücken Tag-Pipelines; Jobs via `needs:[]` entkoppelt; alte Debian-Docker-Stages entfernt.
- **super CI REV8** + **thesis-Submodul** + **relative Submodul-URLs** (`d91fda0`): `.gitmodules` alle 3 auf `../X.git` (air-gap-tauglich); thesis/diplomarbeit von ignoriert→echtes Submodul (absorbgitdirs); cache-engine-Pointer→`1046587`; neue `.gitlab-ci.yml`-Flow `verify:submodules → analyse:thesis-data (zieht cache-engine measure:amd-Artefakte via CI_JOB_TOKEN, lose Abhängigkeit + Fallback) → thesis:pdf`.
- **Konsolidiertes Impl-TODO-Doc** (`bb5e5be`, super docs/sessions) + Tasks **#176–#182** in der Task-DB.
- **Infra-Handovers** (im Cluster-Repo, vom User weitergeleitet): `2026-06-23-HANDOVER-…-CE-DL6-gitlab-v10-push.md`, `2026-06-23-SAMMEL-HANDOVER-impl-an-infra-alle-CE-items.md`, **`2026-06-24-HANDOVER-infra-RUNNER-AUSFALL-und-SECRET-ROTATIONS-AUDIT.md`**.

---

## 2. 🔴 KRITISCHER BLOCKER (P0): prod-Bare-Metal-Runner crash-on-start

**Production baut NICHT.** id=16 (prod1/AMD) + id=17 (prod2/Intel) starten, kontaktieren GitLab **genau einmal**, dann tot — `contacted_at` friert ein, Jobs bleiben `pending`. **3× beobachtet** (22:14 → 21:15 → 08:16, je gefolgt von Daemon-Tod). „online" zeigt GitLab nur wegen ~1 h-Schwelle. **Restart hilft nicht** — der Daemon crasht beim Start. Infra-Aktion: `journalctl -u gitlab-runner -n 150` auf prod1/prod2 (config.toml / shell-User / `concurrent`). Alles Runner-Gated (#156, CE-DL3/DL4, #162-Reihe-C, #152/#163/#165) wartet darauf. Der **Cache** (MinIO) ist eingerichtet, aber moot bis die Daemons halten.

---

## 3. ⚠️ FALLSTRICKE (hart erarbeitet — nächste Session beachten)

- **GitLab-Push vom V10-Laptop (funktioniert):** (1) Auth = OAuth-Password-Grant als `root` an `/oauth/token` (PW im Vault) → kurzlebiger Bearer; PATs im Vault meist 401. (2) **curl** braucht `--ssl-no-revoke --cacert keys/gitlab-ca-ROOTCA-20260621.crt` (Schannel scheitert sonst an `CERT_TRUST_REVOCATION_STATUS_UNKNOWN` — interne CA ohne CRL; **KEIN** insecureSkipVerify). (3) **git** braucht `-c http.sslBackend=openssl -c http.sslCAInfo=<CA>` (Schannel ignoriert sslCAInfo) + Token via Credential-Helper (`oauth2:token`), **NIE Token in Output/Config** (sed-Redaction im Push-Output).
- **Erst-Push großer Repos NICHT atomar** → Gitaly-HTTP-500 bzw. `pre-receive hook declined` auf den GESAMT-Push. Lösung: **Refs EINZELN** pushen (`main` zuerst, dann Branches/Tags).
- **Pipeline-Selbst-DoS:** mirror-Push aller Tags = 1 Pipeline pro Tag (→ 100 pending). `workflow:`-Regel `if $CI_COMMIT_TAG when:never` verhindert das. **Cancel-API wirkt im degradierten Zustand NICHT** (Sidekiq-Stau) — Admin/Rails-Console nötig.
- **thesis-Submodul = bewegliches Ziel** (Overleaf-Sync, HEAD wandert während der Arbeit) → beim Pinnen den aktuellen HEAD ERST auf GitLab pushen, dann `git add thesis/diplomarbeit`, sofort committen.
- **Projekt-Transfer-API:** `POST /groups/:gid/projects/:pid` (das `POST /projects/:id/transfer` gibt hier **404**). **Registry-Image-Tags blocken Transfer** (samba-ad-bind9dlz hängt deshalb in root/).
- **GitLab cross-project `needs` erlaubt KEIN `optional:true`** → Mess-Artefakte per Skript holen (`${CI_API_V4_URL}/projects/<id>/jobs/artifacts/main/download?job=measure:amd` mit `JOB-TOKEN` + `||`-Fallback).
- **CI-Voraussetzungen (Infra), sonst läuft der Flow nicht:** Runner stabil · `perf_event_paranoid ≤ 1` · **TeX/latexmk** auf den Bare-Metal-Runnern (thesis:pdf) · **CI_JOB_TOKEN-Allowlist** super → die 3 Submodul-Projekte (sonst kein privater Submodul-Clone).
- **🔐 SECRETS:** **20 Dateien im HEAD von cluster_development haben Klartext-Secrets** (= jetzt auf GitHub) — Rotations-Audit im Runner-Handover, Rotation läuft beim Infra-Agenten. **Ich habe diese Session den GitLab-`root`-OAuth-Login (= das 4-fach-PW CdMgmt) wiederholt genutzt** (nie ausgegeben) → bei der Rotation einplanen.

---

## 4. ARCHITEKTUR-KONSOLIDIERUNGEN

- **CI/CD-Gesamtfluss (NEU):** cache-engine baut+misst auf Bare-Metal (lose externe Abhängigkeit) → super zieht `measure:amd`-Artefakte → kompiliert Diplomarbeit-PDF mit realen Cache-Miss-Werten. **prt-art = dynamisch gekoppeltes Prüfling-Modul**, **thesis-text = Submodul**, cache-engine = Submodul. Relative URLs machen das air-gap-tauglich.
- **I1 / EINE Architektur (Code-Lag, höchste Code-Prio = #176/#177):** `search_engine<Collection,ConfigPermutation> : execution_engine<>` (`abi/search_engine.hpp:19-21`) und `SearchAlgorithmAbiAdapter : IAnatomyBase→IExecutionEngine` sind **ZWEI unverbundene Template-Bäume** desselben Lebewesens. Soll (Doc 36 §4, festgezogen in ch1–4): GENAU EINE Hierarchie; ABI-Such-Sicht nur über `SearchAlgorithmAbiAdapter` über der Anatomie; lowercase `search_engine` → variadisches `SearchEngine` (CamelCase). **Verifiziert 2026-06-25: noch im Defekt-Zustand.**
- **Leitprinzip (KRITISCH, User 2026-06-25):** Thesis **Kapitel 1–4 sind FESTGEZOGEN = der Soll-Zustand**. Wo der Code abweicht, ist **der Code im Rückstand** und wird nachgezogen — **die Thesis wird NICHT verwässert.** Doc 36 markiert offene Defekte (I1) ausdrücklich als „zu beheben, NICHT kanonisieren".

---

## 5. OFFENE TODOs (Kurz-Index — Details im konsolidierten Doc §A/§B)

**GATE-FREI, sofort machbar (infra-unabhängig):**
- **#176 / #177** — I1: `search_engine<>` → EINE Hierarchie + variadisches `SearchEngine`. **Höchste Code-Prio, zusammen.** Großer/riskanter Refactor → erst Grounding+Plan, vor Edit re-greppen.
- **#178 (A-Teil)** — `sota_catalog.hpp:18-24` A=Stufe1+2 korrigieren (B/C-Labels **blockiert** bis Text-Agent/User die ch1-FF3-vs-ch6-Drift auflöst).
- **#182 (TODO-7)** — Cross-Achsen-Delegation §3.3 „überall" (3 Lücken: search_organ_-Uniformität / T1+T2-Doppelzustand / T15-LayoutAware).
- **#180 (B2)** README/IDE-Einstieg (VS Code+CLion) · **#181 (B3)** Abhängigkeits-Baumdiagramm — **low-risk, sofort sichtbarer Durchklick-Nutzen.**
- **#179 (B1)** Wartbarkeits-/Lesbarkeits-Sweep ALLER C++-Dateien (datei-für-datei, Ledger, **keine Verhaltensänderung**) — groß/langlaufend.

**RUNNER-GATED (warten auf P0):** #156 (der EINE Mess-Lauf), CE-DL3/DL4, #162-Reihe-C, #163/#165-Infra-Teile.

**Schon erledigt diese Session:** CE-DL2-impl (LinuxPerfPmcSource), CE-DL4-impl (`.gitlab-ci.yml` geschrieben), GitLab-Struktur, Submodul-/Air-Gap-Restruktur.

---

## 6. DIREKTIVEN für die nächste Session (bei der enormen Komplexität)

1. **ZUERST das konsolidierte TODO-Doc (`bb5e5be`) lesen** + diese Übergabe. Single-Source.
2. **Vor JEDEM Code-Edit Zeilennummern re-greppen** — alle `file:line` hier sind Momentaufnahmen; der Submodul-Stand verschiebt sich.
3. **Messdaten NIE löschen** (ABI/Schema darf brechen, CSV nur additiv, cowfix-v1/tier150 unveränderlich).
4. **Je Einheit committen + 3-Repo-Submodul-Sync** (nach jedem cache-engine/prt-art-Push sofort super-Pointer bumpen) auf **BEIDE** Remotes (GitHub origin + GitLab). Destruktive Ops nur in den 3 Thesis-Repos mit Tag; **Remotes nie löschen**.
4b. **Build nach jeder Einheit grün halten** (besonders B1). **Keine Erfolgsmarke ohne literale Tool-Ausgabe.**
5. **Pattern-Pflicht** (benanntes Lehrbuch-Muster) · **schwerer offizieller Weg, keine Behelfswege** · zero-cost-Metaprog ok.
6. **Thesis NICHT verwässern** — ch4-Identifier/Figuren = Soll, Code hochziehen.
7. **ultracode ist AUS** → Workflow-Tool nur auf explizites User-Opt-in.
8. **Infra ≠ mein Job** — Runner/GitLab-Server/VLAN/Secrets gehen als Handover an den Infra-Agenten (User leitet weiter), nicht selbst ausführen. Aber GitLab-Push/Projekt-/CI-Operationen DARF ich (mache ich diese Session ständig).
9. **GitLab-Auth-Recipe** (s. §3) — sonst scheitert jeder Push/jede API.

---

## 7. ⭐ USER-ENTSCHEIDUNGEN (2026-06-25, Kontext-Ende) — für die nächste Session bindend

1. **START nächste Session = #176 (I1)** — `search_engine<>` → EINE Lebewesen-Hierarchie. NICHT mit B2/B3 starten. Vorgehen: **zuerst Grounding-/Plan-Runde** (re-grep `abi/search_engine.hpp:19-21`, `anatomy/abi_adapter.hpp:119`, `search_algorithm_anatomy.hpp:32` gegen aktuellen Stand + Doc 36 §4), Bruchstellen kartieren, DANN inkrementell umbauen + nach jeder Einheit Build grün + Commit. #177 (variadisches `SearchEngine`) als Abschluss von #176.
2. **GitLab-Auth-ZIEL = SSH-Deploy-Key** (`keys/id_ed25519_gitlab`/`id_ed25519_gitlab_user`) statt HTTPS-OAuth. ⚠️ **Voraussetzung:** ein **SSH-Präsentations-VIP im V10** für gitlab-shell (`→ 10.0.40.5:22`, mode tcp Passthrough) — der existiert noch NICHT (war als optionaler Teil in CE-DL6-§B). **Bis der Infra-Agent den SSH-VIP gebaut hat, bleibt der HTTPS-OAuth-Recipe aus §3 der EINZIGE funktionierende Push-Weg.** → Neuer Infra-Wunsch: „CE-DL6b: SSH-Deploy-Key-Pfad (V10-SSH-VIP zu gitlab-shell + Deploy-Keys in den 4 Projekten hinterlegen)". Übergangsweise weiter HTTPS-OAuth; sobald SSH steht, `gitlab`-Remotes auf `git@gitlab-push:comdare/research/<projekt>.git` (ProxyJump entfällt, wenn V10-VIP direkt) umstellen.

---
*Geschrieben 2026-06-25, Kontext-Ende-Übergabe. Verifiziert im Code: `search_engine.hpp:19-21` + `sota_catalog.hpp:18-24` noch im Defekt-Zustand. Vorgänger-Übergaben: `20260623-PIVOT-PLAN-156-…`, `20260625-UEBERGABE-impl-agent-ch4-grounding-codelag.md`. Konsolidierte TODOs: `(super) 20260625-IMPL-AGENT-KONSOLIDIERTE-…`.*
