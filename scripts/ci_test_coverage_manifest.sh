# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- CI-Test-Abdeckung: DIE EINE WAHRHEITSQUELLE (R4, 2026-08-06)
# =============================================================================
# WOZU. Bis R4 lebte jede ctest-Auswahl als Zeichenkette IM Job (.gitlab-ci.yml):
# 14 unabhaengig gepflegte -R-/-LE-Listen, deren VEREINIGUNG niemand geprueft hat.
# Ergebnis (live gemessen, 2026-08-06, 406 registrierte Tests): 9 Tests liefen in
# KEINEM Job -- gebaut, aber nie ausgefuehrt. Das war kein Einzelfall, sondern eine
# Fehlerklasse: sie war am 2026-07-13 schon einmal (M-CE-25, 21 Tests) von Hand
# geflickt worden und in drei Wochen erneut nachgewachsen, weil das Anlegen eines
# Tests und das Eintragen in einen Job zwei getrennte, nur menschlich gekoppelte
# Schritte sind.
#
# WIE DIESE DATEI DAS LOEST. Sie ist die EINZIGE Stelle, an der eine ctest-Auswahl
# steht. Jeder Job, der Tests ausfuehrt, laedt diese Datei (POSIX '.') und fragt
# SEINE Auswahl hier ab -- er traegt sie nicht mehr selbst. Dieselbe Datei ist die
# Eingabe der Abdeckungs-Wache scripts/ci_test_coverage_guard.sh, die die
# Vereinigung aller aktiven Auswahlen gegen die LIVE-Inventur (ctest -N) haelt.
# Damit ist die Wache nicht mehr auf ein Abbild der Jobs angewiesen (kein Parsen
# der YAML, kein Regex-Raten): Job und Wache lesen dasselbe Objekt.
#
# EINEN JOB HINZUFUEGEN / AENDERN (der einzige Weg):
#   1. Hier einen Block CE_COV_MODE_<id> / CE_COV_PATT_<id> / CE_COV_GATE_<id>
#      anlegen und <id> in CE_COV_JOBS eintragen.
#   2. Im Job in .gitlab-ci.yml die Auswahl NUR ueber ce_ctest bzw. ce_ctest_args
#      beziehen (Beispiele stehen in jedem umgestellten Job).
#   Wer Schritt 1 vergisst, bekommt Schritt 2 nicht zum Laufen (ce_ctest bricht bei
#   unbekannter Kennung mit Exit 3 ab) -- Vergessen ist damit laut, nicht still.
#
# GATE-KLASSEN (was zaehlt als Deckung?). Eine Auswahl deckt nur, wenn ihr Job im
# LAUFENDEN Pipeline-Lauf auch wirklich faehrt. Die Wache wertet die Gates gegen die
# echte Umgebung aus (die GitLab-Variablen liegen im Job-Environment) statt gegen
# eine zweite Abschrift der rules-Zeilen:
#   always          -- Job ohne rules-Gate: laeuft immer, deckt immer.
#   declared:VAR    -- Job laeuft, solange die Cluster-Deklaration VAR nicht LEER
#                      ist (PMC-Lanes-Doktrin 23.07.: eine leere Lane-Menge schaltet
#                      die Hardware-Jobs bewusst ab). VAR ungesetzt = Lauf ausserhalb
#                      der CI -> die Wache nimmt den Job als deklariert an und sagt
#                      das im Bericht. VAR gesetzt-aber-leer = KEINE Deckung.
#   optin:VAR=WERT  -- Job ist per Default AUS (z.B. ISA-Matrix). Deckung nur, wenn
#                      die Umgebung den Wert wirklich traegt. Ein opt-in-Job darf
#                      also NIE die einzige Deckung eines Tests sein.
#
# ASCII-only (Leitplanke). Keine Umlaute, kein UTF-8.
# =============================================================================

# Reihenfolge = Reihenfolge in .gitlab-ci.yml (Lesbarkeit; fuer die Mengenlehre egal).
CE_COV_JOBS="contract pmc arm64_smoke sanitize_asan contract_durability contract_conformance contract_pool_flip contract_harness contract_profile_coverage contract_experiment_driver contract_node_shape chaos_drift test_unit sanitize_tsan"

# -- Job 'contract' (Template .contract, ABI-Contract-Smoke) --
CE_COV_MODE_contract="-R"
CE_COV_PATT_contract='^test_abi_interface$'
CE_COV_GATE_contract="always"

# -- Jobs 'pmc:amd' / 'pmc:intel' (Template .pmc; EIN Skript, zwei Vendor-Lanes) --
# R4-AENDERUNG: vorher '^(linux_perf_pmc_smoke|m3v2_pmc_smoke)$' -- eine Namensliste,
# die bei jedem neuen pmc-Test von Hand haette wachsen muessen (und genau das ist am
# 2026-07-13 fuer m3v2_pmc_smoke schon einmal schiefgegangen, M-CE-25/Muster-F).
# Jetzt deckt der Job die KLASSE: alles, was das Label 'pmc' traegt. Die Ziel-Liste
# des Jobs bleibt explizit -- ein neuer pmc-Test ohne Ziel-Eintrag laesst ctest die
# fehlende Binary melden (Job ROT mit Namen), er verschwindet nicht mehr still.
CE_COV_MODE_pmc="-L"
CE_COV_PATT_pmc='pmc'
CE_COV_GATE_pmc="declared:COMDARE_PMC_LANES"

# -- Job 'build:arm64-smoke' (ISA-Teilmatrix, opt-in) --
CE_COV_MODE_arm64_smoke="-R"
CE_COV_PATT_arm64_smoke='^(test_cpuid_probe|test_conformance_gate)$'
CE_COV_GATE_arm64_smoke="optin:COMDARE_ISA_MATRIX=true"

# -- Job 'sanitize:asan-ubsan' --
CE_COV_MODE_sanitize_asan="-R"
CE_COV_PATT_sanitize_asan='^(test_abi_interface|test_config_durability|test_conformance_gate|test_experiment_driver_v13|test_chaos_drift_gate)$'
CE_COV_GATE_sanitize_asan="always"

# -- Job 'contract:durability' --
CE_COV_MODE_contract_durability="-R"
CE_COV_PATT_contract_durability='^test_config_durability$'
CE_COV_GATE_contract_durability="always"

# -- Job 'contract:conformance' --
CE_COV_MODE_contract_conformance="-R"
CE_COV_PATT_contract_conformance='^(test_conformance_gate|test_188_4c0_known_compositions_conformance|test_188_4cii_flat_wrapper_traversal|test_216h2_stat_reset_after_load|test_v41_anatomy_module_abi|test_cmd1_a_axis_command_base|test_ap15_2_get_allocator_proxy|test_ap15_3_driveable_map_contract|test_29_container_framework|test_v41_cache_engine_facade)$'
CE_COV_GATE_contract_conformance="always"

# -- Job 'contract:pool_flip' --
CE_COV_MODE_contract_pool_flip="-R"
CE_COV_PATT_contract_pool_flip='^(test_188_4bbV_pool_adapter_flip_compile|test_188_4a_eytzinger_organ|test_s7_([1-9]|10)_)'
CE_COV_GATE_contract_pool_flip="always"

# -- Job 'contract:harness' --
CE_COV_MODE_contract_harness="-R"
CE_COV_PATT_contract_harness='^(test_harness_compile|test_lazy_resume_binary)$'
CE_COV_GATE_contract_harness="always"

# -- Job 'contract:profile_coverage' --
CE_COV_MODE_contract_profile_coverage="-R"
CE_COV_PATT_contract_profile_coverage='^(test_profile_coverage|test_smoke_coverage_profile|test_wdk_datasets_fairness|test_sota_st2_dedup|test_validate_profile|test_h2_score_akte|test_profile_roundtrip|test_experiment_parser|test_measurement_categories|test_axis_registry_roundtrip)$'
CE_COV_GATE_contract_profile_coverage="always"

# -- Job 'contract:experiment_driver' --
CE_COV_MODE_contract_experiment_driver="-R"
CE_COV_PATT_contract_experiment_driver='^test_experiment_driver_v13$'
CE_COV_GATE_contract_experiment_driver="always"

# -- Job 'contract:node_shape' --
CE_COV_MODE_contract_node_shape="-R"
CE_COV_PATT_contract_node_shape='^test_234_'
CE_COV_GATE_contract_node_shape="always"

# -- Job 'chaos:drift' --
CE_COV_MODE_chaos_drift="-R"
CE_COV_PATT_chaos_drift='^test_chaos_drift_gate$'
CE_COV_GATE_chaos_drift="always"

# -- Job 'test:unit' (Template .test, Voll-Suite ueber das Sammel-Target comdare_tests) --
# R4-AENDERUNG (der Wurzelschnitt): vorher '-LE contract|pmc'. Der pmc-Ausschluss ist
# technisch begruendet (Performance-Counter-Hardware, eigene Vendor-Lanes, siehe oben);
# der contract-Ausschluss war es NICHT -- alle contract:*-Jobs konfigurieren mit exakt
# derselben Flag-Zeile wie test:unit, ohne Sanitizer, ohne Sonder-Tag. Er stammte aus
# #278 (2026-07-06) und war reine Report-Hygiene ("Not Run"-Zeilen im Sammel-Lauf),
# festgemacht an der Behauptung, jeder contract-Test habe seinen dedizierten Job. Genau
# diese Behauptung hat sich still zersetzt. Der Ausschluss faellt damit weg: test:unit
# faehrt jetzt ALLES ausser der Hardware-Klasse. Die contract:*-Jobs bleiben als
# schnelle, frueh scheiternde Einzelgates bestehen (Doppel-Ausfuehrung ist gewollt:
# sie melden fruehe Stufe + eigenen Build; test:unit ist das Vollstaendigkeits-Netz).
CE_COV_MODE_test_unit="-LE"
CE_COV_PATT_test_unit='pmc'
CE_COV_GATE_test_unit="always"

# -- Job 'sanitize:tsan' --
CE_COV_MODE_sanitize_tsan="-R"
CE_COV_PATT_sanitize_tsan='^(test_concurrency_disciplines|test_rcu|test_v41_axis_08_concurrency)$'
CE_COV_GATE_sanitize_tsan="always"

# =============================================================================
# Zugriff (das ist die gesamte oeffentliche Schnittstelle dieser Datei)
# =============================================================================

# ce_cov_lookup <FELD> <kennung> -> Wert auf stdout; Exit 3 bei unbekannter Kennung.
# FELD ist MODE, PATT oder GATE.
ce_cov_lookup() {
    eval "_ce_probe=\${CE_COV_${1}_${2}+gesetzt}"
    if [ "${_ce_probe:-}" != "gesetzt" ]; then
        echo "FEHLER (ci_test_coverage_manifest.sh): unbekannte Job-Kennung '${2}' fuer Feld '${1}'." >&2
        echo "  Erlaubte Kennungen: ${CE_COV_JOBS}" >&2
        echo "  -> Wer einen Job neu anlegt, traegt ihn HIER ein; die Auswahl lebt nirgends sonst." >&2
        return 3
    fi
    eval "printf '%s' \"\${CE_COV_${1}_${2}}\""
}

# ce_ctest_args <kennung> -> "<modus> <muster>" auf stdout (fuer Templates, die sich
# ihre ctest-Zeile selbst bauen, z.B. .test via COMDARE_TEST_CTEST_ARGS).
ce_ctest_args() {
    _ce_mode=$(ce_cov_lookup MODE "$1") || return 3
    _ce_patt=$(ce_cov_lookup PATT "$1") || return 3
    printf '%s %s' "$_ce_mode" "$_ce_patt"
}

# ce_ctest <kennung> <build-verzeichnis> [weitere ctest-Argumente...]
# Fuehrt ctest mit der deklarierten Auswahl aus -- vollstaendig gequotet (kein
# Wort-Splitting, kein Globbing an Mustern wie '[1-9]').
ce_ctest() {
    _ce_id=$1
    _ce_bld=$2
    shift 2
    _ce_mode=$(ce_cov_lookup MODE "$_ce_id") || return 3
    _ce_patt=$(ce_cov_lookup PATT "$_ce_id") || return 3
    echo "+ ctest --test-dir ${_ce_bld} ${_ce_mode} ${_ce_patt} $*   [Auswahl aus scripts/ci_test_coverage_manifest.sh, Kennung ${_ce_id}]"
    ctest --test-dir "$_ce_bld" "$_ce_mode" "$_ce_patt" "$@"
}
