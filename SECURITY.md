# Security Policy

Project: **comdare-cache-engine**
Vendor / rights holder: **BEP Venture UG (haftungsbeschraenkt)**, trademark Comdare
Security Contact: **bep.venture.ug@gmail.com**

## 1. Reporting a Vulnerability
Please report suspected vulnerabilities **privately** to **bep.venture.ug@gmail.com** with the
subject "Security Report: comdare-cache-engine". Include:
- A clear description of the issue and potential impact.
- Minimal, reproducible steps or proof-of-concept.
- Affected revision(s) and environment (OS, compiler, configuration, build switches).
- Any logs, stack traces, or screenshots.

**Acknowledgement timeline:** We aim to acknowledge your report within **14 calendar days**.
We will then coordinate on investigation, remediation, and coordinated disclosure timelines.

> **Please do not** create public GitHub or GitLab issues for security reports.

## 2. Supported Versions
There are no versioned releases before the thesis submission. Security fixes are applied to
the `development` branch and forwarded to `main`. Older revisions are out of support.

## 3. Scope
In scope:
- The own source code under `libs/`, `apps/`, `tools/`, `adapters/`, `cmake/`, `scripts/` and
  the CI configuration, as published by **BEP Venture UG (haftungsbeschraenkt)**.
- Default configuration and documented build switches.

Out of scope:
- Vendored third-party code under `ext/` (report to the respective upstream maintainers; the
  default build links only the components named in `NOTICE`).
- User-modified builds, forks, or unapproved patches.
- Social engineering, physical attacks, or attacks on infrastructure not owned by
  **BEP Venture UG (haftungsbeschraenkt)**.

## 4. Ground Rules for Testing (Responsible Research)
- Do not exploit the vulnerability beyond the minimal proof-of-concept needed to demonstrate impact.
- Do not access, modify, or exfiltrate data that does not belong to you.
- Do not degrade availability for other users (no denial-of-service or brute-force testing
  against production services).
- Perform testing only in your own environment or with explicit permission.
- Comply with applicable law at all times. Reading and auditing the source is expressly
  permitted by `LICENSE` Section 2.

## 5. Coordinated Disclosure
Please allow us a reasonable time to investigate and release a fix before any public
disclosure. We will provide credit in the changelog (optional, with your consent). Do not
share details publicly without our written approval until a fix is available.

## 6. Safe Harbor
If you follow this policy in good faith, we will not pursue legal action for security research
activities that align with the "Ground Rules for Testing", and we will consider such
activities authorized for the limited purpose of reporting vulnerabilities to us.

## 7. No Bounty by Default
We do not operate a bug-bounty program at this time. We may, at our discretion, offer a token
of appreciation or credit.

## 8. Contact & Legal
- Security contact: **bep.venture.ug@gmail.com**
- Preferred language: English or German
- Jurisdiction: Germany (informational only; does not waive rights)

---

*Version: 2026-08-25.*
