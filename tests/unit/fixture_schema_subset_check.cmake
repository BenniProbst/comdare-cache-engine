# fixture_schema_subset_check.cmake (ce-seitig) -- Rider aus KF-Design Posten #95, gefuehrt im
# #16-Auftrag ('Werkzeug kommt erst mit dem Golden-Fenster'). Schwester der SUPER-seitigen Wache
# Code/tests/fixture_schema_subset_check.cmake (B14-NB3+, xmllint-basiert); DIESE Datei bewacht die
# ce-EIGENE Fixture tests/unit/thesis_tiere/experiment_golden_kern.xml auch dann, wenn ce OHNE den
# super-Nachbarn gebaut wird (eigene ce-CI) -- bis hierher hatte die ce-Fixture in der ce-CI KEINE
# Wache (A7-Befund: 'das KANONISCHE File hat im landenden Stand gar keine Wache').
#
# DIE DREI CODEX-AUFLAGEN AUS #95, HIER UMGESETZT:
#   (1) NICHTLEER-PRUEFUNG: leere/abgeschnittene XML war in der NB2-Fassung stumm-gruen (Codex
#       :138); hier ist eine leere oder element-lose Datei FATAL.
#   (2) CDATA-KANTE: KEIN Selbstbau-Textparser (die NB2-Fassung fiel an '<![CDATA[' im Kommentar,
#       Codex BLOCKER) -- gelesen wird ausschliesslich ueber xmllint; die einzige Textfrage dieser
#       Datei (DOCTYPE-Sperre) ist bewusst FAIL-CLOSED: ein '<!DOCTYPE'-Vorkommen in Kommentar/
#       CDATA macht faelschlich ROT, nie faelschlich GRUEN (deklarierte Asymmetrie).
#   (3) SKIP-BEI-FEHLEND=ROT: die FIXTURE ist repo-EIGEN -- fehlt sie, ist das ein kaputtes Repo
#       bzw. eine falsch verdrahtete add_test-Zeile, KEIN fehlender Nachbar => FATAL, kein Skip.
#       Fehlt xmllint, ist das ebenso FATAL (eine Wache ohne Werkzeug ist abgeschaltet, nicht
#       gruen). NUR das SCHEMA-Bein darf skippen, und nur aus dem EINEN strukturellen Grund:
#       die Single-Source-XSD lebt im SUPER-Repo (Code/test_data_xml/experiment_schema.xsd);
#       eine Kopie in ce waere eine ZWEITE Quelle und genau die Zwei-Sprachen-Falle, gegen die
#       die super-Wache gebaut ist. Steht der super-Nachbar (Submodul-Layout), laeuft das Bein
#       HART; steht er nicht (ce standalone/eigene ce-CI), wird der Ausfall als benannter
#       Halb-Skip gemeldet -- die Beine (1)/(2)/Wohlform bleiben immer hart.
#
# Erwartete -D Variablen: FIXTURE (Pflicht) -- die ce-Fixture; SCHEMA (optional) -- explizite XSD;
# ohne -DSCHEMA wird der super-Nachbar relativ zur ce-Wurzel gesucht (CE_ROOT Pflicht dafuer).

set(_ctx "fixture_schema_subset_check_ce")

find_program(COMDARE_XMLLINT_EXE xmllint)
if(NOT COMDARE_XMLLINT_EXE)
    message(FATAL_ERROR
        "${_ctx}: xmllint FEHLT auf diesem Host. Die Wache stellt ohne Werkzeug keine Aussage -- "
        "ein Skip waere ihr Abschalten (Codex-Auflage #95/3). Paket: libxml2-utils (Debian/Ubuntu) "
        "bzw. libxml2 (Arch/Fedora).")
endif()

if(NOT DEFINED FIXTURE)
    message(FATAL_ERROR
        "${_ctx}: -DFIXTURE fehlt -- Verdrahtungsfehler in der add_test-Zeile (die Wache haette "
        "sonst keinen Gegenstand).")
endif()
if(NOT EXISTS "${FIXTURE}")
    message(FATAL_ERROR
        "${_ctx}: FIXTURE '${FIXTURE}' EXISTIERT NICHT. Sie ist repo-EIGEN (tests/unit/"
        "thesis_tiere/) -- das ist KEIN fehlender Nachbar-Checkout, also KEIN Skip "
        "(Codex-Auflage #95/3: Skip-bei-fehlend=rot).")
endif()

# (1) NICHTLEER, Stufe Datei-Groesse: 0 Bytes sind ohne Parser entscheidbar.
file(SIZE "${FIXTURE}" _fixture_bytes)
if(_fixture_bytes EQUAL 0)
    message(FATAL_ERROR
        "${_ctx}: FIXTURE '${FIXTURE}' ist LEER (0 Bytes) -- abgeschnittene/leere XML war die "
        "stumm-gruene Klasse der NB2-Fassung (Codex-Auflage #95/1).")
endif()

# DOCTYPE-/Entity-Sperre (fail-closed, s. Kopf): der Byte-Scan kennt kein XML -- er macht bei
# einem DOCTYPE in Kommentar/CDATA faelschlich ROT und ist damit auf der billigen Seite des
# Irrtums. --nonet unten blockiert Netz-Loads zusaetzlich.
file(READ "${FIXTURE}" _fixture_roh)
string(FIND "${_fixture_roh}" "<!DOCTYPE" _doctype_pos)
if(NOT _doctype_pos EQUAL -1)
    message(FATAL_ERROR
        "${_ctx}: FIXTURE '${FIXTURE}' traegt '<!DOCTYPE' (Byte-Position ${_doctype_pos}) -- "
        "DTD/Entities sind fuer die Fixture nicht zugelassen (Entity-Expansion waere ein "
        "Parser-Nebenweg an der Wache vorbei). Fail-closed: auch ein Kommentar-Vorkommen ist ROT.")
endif()

# (2) WOHLGEFORMTHEIT ueber xmllint -- die CDATA-/Kommentar-/PI-Kanten beantwortet der echte
# Parser, nicht ein Text-Scan.
execute_process(
    COMMAND "${COMDARE_XMLLINT_EXE}" --noout --nonet "${FIXTURE}"
    RESULT_VARIABLE _wf_rc ERROR_VARIABLE _wf_err ERROR_STRIP_TRAILING_WHITESPACE TIMEOUT 10)
if(NOT _wf_rc EQUAL 0)
    message(FATAL_ERROR
        "${_ctx}: FIXTURE '${FIXTURE}' ist NICHT wohlgeformt (xmllint RC=${_wf_rc}):\n${_wf_err}")
endif()

# (1b) NICHTLEER, Stufe Struktur: eine wohlgeformte Datei ohne ein einziges Element traegt keine
# Aussage. xmllint --xpath 'count(//*)' liefert die Zahl auf stdout.
execute_process(
    COMMAND "${COMDARE_XMLLINT_EXE}" --nonet --xpath "count(//*)" "${FIXTURE}"
    RESULT_VARIABLE _cnt_rc OUTPUT_VARIABLE _n_elem OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET TIMEOUT 10)
if(NOT _cnt_rc EQUAL 0 OR _n_elem MATCHES "^0" OR _n_elem STREQUAL "")
    message(FATAL_ERROR
        "${_ctx}: FIXTURE '${FIXTURE}' traegt keine Elemente (count(//*) = '${_n_elem}', "
        "RC=${_cnt_rc}) -- leere Huelle, keine Fixture (Codex-Auflage #95/1).")
endif()
message(STATUS "${_ctx}: FIXTURE wohlgeformt, ${_fixture_bytes} Bytes, ${_n_elem} Elemente, kein DOCTYPE.")

# SCHEMA-Bein (Teilmengen-Aussage gegen die Single-Source-XSD des super-Repos).
if(NOT DEFINED SCHEMA)
    if(DEFINED CE_ROOT)
        set(SCHEMA "${CE_ROOT}/../../test_data_xml/experiment_schema.xsd")
    else()
        set(SCHEMA "")
    endif()
endif()
if(SCHEMA STREQUAL "" OR NOT EXISTS "${SCHEMA}")
    message(STATUS "${_ctx}: SCHEMA-Bein uebersprungen -- super-Nachbar nicht im Baum "
                   "(gesucht: '${SCHEMA}'). Die Single-Source-XSD lebt im super-Repo; eine "
                   "ce-Kopie waere eine zweite Quelle. Im super-Kontext laeuft dieses Bein hart.")
    message(STATUS "COMDARE-XML-WACHE-SKIP-SCHEMA-BEIN")
    return()
endif()
execute_process(
    COMMAND "${COMDARE_XMLLINT_EXE}" --noout --nonet --schema "${SCHEMA}" "${FIXTURE}"
    RESULT_VARIABLE _sch_rc ERROR_VARIABLE _sch_err ERROR_STRIP_TRAILING_WHITESPACE TIMEOUT 10)
if(NOT _sch_rc EQUAL 0)
    message(FATAL_ERROR
        "${_ctx}: VOKABULAR-/STRUKTUR-DRIFT -- die ce-Fixture ist NICHT gueltig gegen die "
        "Single-Source-XSD.\n  FIXTURE: ${FIXTURE}\n  SCHEMA:  ${SCHEMA}\n"
        "  BEFUND von xmllint (RC=${_sch_rc}):\n${_sch_err}\n"
        "Fix: Schema-Evolution zuerst in der XSD (super Code/test_data_xml/SCHEMA.md) oder die "
        "Fixture auf das Schema-Vokabular ziehen.")
endif()
message(STATUS "${_ctx}: OK -- Fixture gueltig gegen die Single-Source-XSD (${SCHEMA}).")
