# =============================================================================
# comdare-cache-engine -- Makefile (GNU-Bauweg, Owner-Ansage 08.08.2026)
# =============================================================================
# WOZU. Der offizielle Linux-Bauweg aus Quellen ist
#   ./configure.sh && make && make check && make install
# (Owner 08.08.2026; Ledger DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md
# :11370-11391). Dieses Makefile ist der GNU-Vorbau -- es BAUT NICHT SELBST,
# sondern reicht an CMake weiter. Das bauende Skript bleibt CMakeLists.txt.
#
# ZIELE nach GNU Makefile Conventions (Standard Targets):
#   all install check installcheck uninstall clean distclean
#   mostlyclean maintainer-clean
# Der GNU-Name des Test-Ziels ist 'check', nicht 'test' -- "Perform self-tests
# (if any). The user must build the program before running the tests, but need
# not install the program."
#
# WOHER DIE KONFIGURATION KOMMT. Ausschliesslich aus config.status, das
# configure.sh schreibt. Dieses Makefile enthaelt bewusst KEINE Vorgabewerte
# fuer Praefix oder Bauverzeichnis: gaebe es sie, waeren sie eine zweite
# Wahrheit neben config.status und wuerden bei jedem Vergessen von
# ./configure.sh still danebenlaufen.
#
# ASCII-only (Leitplanke). Keine Umlaute, kein UTF-8.
# =============================================================================

CONFIG_STATUS = config.status

# Aus config.status gelesen (leer = nicht konfiguriert). Der ':='-Zuweiser
# wertet EINMAL beim Einlesen aus, nicht bei jeder Benutzung.
BUILDDIR   := $(shell test -f ./$(CONFIG_STATUS) && . ./$(CONFIG_STATUS) && printf '%s' "$$COMDARE_BUILDDIR")
SRCDIR     := $(shell test -f ./$(CONFIG_STATUS) && . ./$(CONFIG_STATUS) && printf '%s' "$$COMDARE_SRCDIR")
CMAKE      := $(shell test -f ./$(CONFIG_STATUS) && . ./$(CONFIG_STATUS) && printf '%s' "$$COMDARE_CMAKE")
PREFIX     := $(shell test -f ./$(CONFIG_STATUS) && . ./$(CONFIG_STATUS) && printf '%s' "$$COMDARE_PREFIX")
BINDIR     := $(shell test -f ./$(CONFIG_STATUS) && . ./$(CONFIG_STATUS) && printf '%s' "$$COMDARE_BINDIR")

# Die Planer-CLI: das ist das Werkzeug, das der Anwender laut Owner nach dem
# Installieren aufruft ("Der Anwender ruft also die CLI des Planers auf,
# nachdem er sie per install kompiliert hat").
PLANER_CLI = comdare-experiment-planner

# Parallelitaet an CMake durchreichen. Ohne Angabe entscheidet CMake selbst;
# 'make -jN' wirkt NICHT automatisch auf den Unterbau, deshalb explizit.
BUILD_JOBS ?=
ifneq ($(strip $(BUILD_JOBS)),)
  BUILD_PAR = --parallel $(BUILD_JOBS)
else
  BUILD_PAR =
endif

.DEFAULT_GOAL := all

.PHONY: all install check installcheck uninstall clean distclean \
        mostlyclean maintainer-clean konfiguriert help

# -- Wache: ohne configure.sh geht nichts, und zwar LAUT -----------------------
konfiguriert:
	@test -n "$(BUILDDIR)" || { \
	  echo "FEHLER: nicht konfiguriert -- es gibt kein $(CONFIG_STATUS)."; \
	  echo "  Zuerst:  ./configure.sh [--prefix=PFAD] [--enable-NAME] ..."; \
	  echo "  Hilfe:   ./configure.sh --help"; \
	  exit 1; }
	@test -d "$(BUILDDIR)" || { \
	  echo "FEHLER: Bauverzeichnis '$(BUILDDIR)' fehlt, obwohl $(CONFIG_STATUS) es nennt."; \
	  echo "  Erneut konfigurieren:  ./configure.sh"; \
	  exit 1; }

# -- all: uebersetzt alles ----------------------------------------------------
all: konfiguriert
	$(CMAKE) --build "$(BUILDDIR)" $(BUILD_PAR)

# -- check: Selbsttests -------------------------------------------------------
# DIE AUSWAHL WIRD EINGEBUNDEN, NICHT KOPIERT. scripts/ci_test_coverage_manifest.sh
# ist seit R4 (06.08.2026) die EINE Wahrheitsquelle fuer jede ctest-Auswahl; jeder
# CI-Job fragt seine Auswahl dort ab. 'make check' tut dasselbe und nimmt die
# Kennung 'test_unit' -- die Voll-Suite ohne die Hardware-Klasse 'pmc' (die
# braucht Performance-Counter und eigene Vendor-Lanes, siehe Manifest). Eine
# eigene Auswahl hier waere die 15. Wahrheit gewesen und genau die Fehlerklasse,
# deren Beseitigung R4 gekostet hat.
#
# 'ce_ctest' statt 'ctest' direkt: die Funktion quotet Modus und Muster selbst
# korrekt (Muster wie '[1-9]' duerfen nicht globben) und bricht bei unbekannter
# Kennung mit Exit 3 ab -- Vergessen ist damit laut, nicht still.
check: all
	$(CMAKE) --build "$(BUILDDIR)" $(BUILD_PAR) --target comdare_tests
	@. "$(SRCDIR)/scripts/ci_test_coverage_manifest.sh" && \
	  ce_ctest test_unit "$(BUILDDIR)" --output-on-failure

# -- install: DESTDIR-faehig --------------------------------------------------
# DESTDIR wird dem Praefix VORANGESTELLT (GNU: staged install; CMake dokumentiert
# dasselbe Verhalten fuer 'cmake --install'). Es wird hier bewusst NICHT gesetzt,
# nur durchgereicht -- GNU: "DESTDIR should not be set within the Makefile".
# Vorbehalt, den CMake selbst nennt: DESTDIR traegt auf Unix, nicht auf Windows
# (dort steckt ein Laufwerksbuchstabe im Praefix). Der offizielle Weg ist laut
# Owner ohnehin der Linux-Weg.
install: all
	DESTDIR="$(DESTDIR)" $(CMAKE) --install "$(BUILDDIR)"

# -- installcheck: prueft die INSTALLIERTE Fassung ----------------------------
# Das ist der eigentliche Owner-Nachweis: nach 'make install' muss die Planer-CLI
# unter bindir liegen und ansprechbar sein.
installcheck:
	@_cli="$(DESTDIR)$(BINDIR)/$(PLANER_CLI)"; \
	test -x "$$_cli" || { \
	  echo "FEHLER: '$$_cli' fehlt oder ist nicht ausfuehrbar."; \
	  echo "  Zuerst 'make install' (ggf. mit DESTDIR=...)."; \
	  exit 1; }; \
	echo "installcheck: $$_cli vorhanden"; \
	"$$_cli" --help >/dev/null 2>&1 || "$$_cli" help >/dev/null 2>&1 || { \
	  echo "FEHLER: '$$_cli' beantwortet weder --help noch help."; \
	  exit 1; }; \
	echo "installcheck: Planer-CLI antwortet -- OK"

# -- uninstall ----------------------------------------------------------------
# CMake bringt kein eigenes uninstall mit, schreibt aber beim Installieren ein
# install_manifest.txt. Genau diese Liste wird hier zurueckgebaut -- nichts
# darueber hinaus, damit ein uninstall nie mehr entfernt als sein install
# angelegt hat. Die Pfade im Manifest tragen KEIN DESTDIR (am Objekt geprueft),
# deshalb wird es hier vorangestellt.
#
# '|| [ -n "$$_f" ]' IST NICHT KOSMETIK. CMake schliesst install_manifest.txt
# NICHT mit einem Zeilenumbruch ab (geprueft: letztes Byte ist das 'r' von
# "planner"). Ein blankes 'while IFS= read -r' liefert fuer die letzte Zeile
# false und bricht ab, BEVOR der Schleifenkoerper laeuft -- die letzte Datei
# des Manifests bliebe stehen. Beim ersten Lauf war das ausgerechnet
# bin/comdare-experiment-planner, also genau das Werkzeug, um das es geht:
# 8 von 9 Dateien entfernt, die wichtigste blieb liegen, Exit-Code 0.
uninstall:
	@_man="$(BUILDDIR)/install_manifest.txt"; \
	test -f "$$_man" || { \
	  echo "FEHLER: '$$_man' fehlt -- ohne Installations-Manifest wird nichts entfernt."; \
	  exit 1; }; \
	while IFS= read -r _f || [ -n "$$_f" ]; do \
	  test -n "$$_f" || continue; \
	  if test -e "$(DESTDIR)$$_f" || test -L "$(DESTDIR)$$_f"; then \
	    rm -f -- "$(DESTDIR)$$_f" && echo "entfernt: $(DESTDIR)$$_f"; \
	  fi; \
	done < "$$_man"

# -- clean / mostlyclean ------------------------------------------------------
clean: konfiguriert
	$(CMAKE) --build "$(BUILDDIR)" --target clean

mostlyclean: clean

# -- distclean ----------------------------------------------------------------
# GNU: "Delete all files ... that are created by configuring or building the
# program ... 'make distclean' should leave only the files that were in the
# distribution."
#
# ENTFERNT WIRD GENAU DAS IN config.status VERMERKTE BAUVERZEICHNIS -- kein
# fest verdrahtetes 'rm -rf build'. Grund, benannt statt stillschweigend: bis
# ce eb96b76a lag unter build/ eine GETRACKTE Mess-CSV
# (build/thesis_tiere/tier150_measurements.csv); sie ist seither nach
# docs/archiv/messdaten/ umgezogen, und 'git ls-files build/' liefert 0. Die
# Messdaten-Doktrin (Messdaten werden nie geloescht) bleibt trotzdem der Grund
# fuer die enge Fassung dieses Ziels: es loescht, was configure.sh angelegt hat,
# und nichts sonst. Die zweite, AKTIVE Kopie unter tests/unit/thesis_tiere/ ist
# von hier gar nicht erreichbar -- sie liegt ausserhalb des Bauverzeichnisses
# und ist per Groessen-Pin (test_org18_persistence_target) bewusst bewacht.
distclean:
	@test -n "$(BUILDDIR)" || { \
	  echo "nichts zu tun: kein $(CONFIG_STATUS) vorhanden."; exit 0; }
	@test "$(BUILDDIR)" != "." && test "$(BUILDDIR)" != "/" || { \
	  echo "FEHLER: unplausibles Bauverzeichnis '$(BUILDDIR)' -- abgebrochen."; exit 1; }
	rm -rf -- "$(BUILDDIR)"
	rm -f -- ./$(CONFIG_STATUS)

maintainer-clean: distclean
	@echo "maintainer-clean: identisch zu distclean -- dieses Projekt haelt keine"
	@echo "  eingecheckten Generate, die nur Betreuer neu erzeugen koennen."

# -- help ---------------------------------------------------------------------
help:
	@echo "comdare-cache-engine -- GNU-Bauweg"
	@echo ""
	@echo "  ./configure.sh [--prefix=PFAD] ...   konfigurieren (--help zeigt alles)"
	@echo "  make                                 uebersetzen"
	@echo "  make check                           Selbsttests (Auswahl 'test_unit')"
	@echo "  make install [DESTDIR=PFAD]          installieren"
	@echo "  make installcheck                    installierte Planer-CLI pruefen"
	@echo "  make uninstall                       per install_manifest zurueckbauen"
	@echo "  make clean                           Bauartefakte entfernen"
	@echo "  make distclean                       Bauverzeichnis + config.status entfernen"
	@echo ""
	@echo "  BUILD_JOBS=N                         Parallelitaet an cmake --build"
