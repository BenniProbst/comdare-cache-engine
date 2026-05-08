# tools/gitlab_ci_zih_push — GitLab CI-Job pusht Binaries direkt ans ZIH (F-EXTRA-3)

**Architektur-Erweiterung** fuer automatisierte Workflow-Tests.
Abhaengig von Fortigate-31G-Edge-Migration des Kubernetes-Clusters
(Cluster-Migrations-Plan, siehe `Projekte/Cluster/sessions/`).

## Status: Skelett (NEU, abhaengig von Infra-Job)

## Architektur

1. GitLab CI-Pipeline triggert nach Cross-Compile-Stage einen ZIH-Push-Job.
2. ZIH-Push-Job nutzt eigene statische IP (jetzt verfuegbar) als Source.
3. Fortigate 31G als neues Edge-Geraet im Cluster.
4. Binaries werden direkt in ZIH-Workspace gepusht.

## Erwartete `.gitlab-ci.yml`-Snippet (zu implementieren)

```yaml
zih-push:
  stage: deploy
  script:
    - tools/gitlab_ci_zih_push/push.sh build/cache_engine_builder
  only:
    - main
    - tags
  environment:
    name: zih-barnard
```

## Voraussetzungen (Infra-Job)

- ✅ Eigene statische IP-Adresse (verfuegbar)
- ⏳ Kubernetes-Cluster-Umbau mit Fortigate 31G als Edge
- ⏳ Firewall-Regeln fuer ZIH-Workspace-SOCKS5-Endpunkt
