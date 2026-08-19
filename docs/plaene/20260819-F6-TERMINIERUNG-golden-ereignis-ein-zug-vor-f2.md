# F6-TERMINIERUNG: das golden-Ereignis faehrt als EIN Zug VOR F2

Datum: 2026-08-19 · Herkunft: A2.5-Fixstrecke-2, Posten G6 (Bump-Bruch #15) · Status: TERMINIERUNG
Traeger-Task: **benennt der Lead** (diese Notiz terminiert; sie ersetzt den Task nicht).

## Der Termin

Die vier unten genannten Posten fahren als **EIN gemeinsamer Zug im golden-Fenster**, und zwar
**VOR dem F2-Identitaets-Freeze am Freitag, 21.08.2026**. Nach F2 sind Identitaets-Umbauten
teuer (Flotten-Bestand waechst; vor F2 sind sie billig, danach invalidieren sie Gebautes).
Spaetester sinnvoller Landepunkt des Zuges ist damit Donnerstag, 20.08.2026.

## Die vier Teile des EINEN Zuges

1. **B-9 -- build_version wird identitaets-wirksam.** Heute ist die build_version Provenienz
   (Suffix); seit fingerprint_format=3 sind die CEB-Laufzeit-Hauptachsen und die
   Enabled-Mengen-Signatur bereits identitaets-wirksam (anatomy_fingerprint.hpp, Kopf).
   B-9 zieht den verbleibenden build_version-Anteil in die Identitaet nach.
2. **A-11 -- Stempel-Pflicht.** Emission ohne Stempel faellt; die Pflicht wird erzwungen,
   nicht empfohlen (golden-Folgezug laut Uebergabe zur Fixstrecke).
3. **B-10.3 -- CRC-Re-Anker.** Die durch B-9 + A-11 bewegten golden-Anker werden NEU geankert,
   im SELBEN Commit wie die Bewegung. Bekannte Anker-Stellen (welche der Zug konkret dreht,
   entscheidet der Traeger-Task am Objekt):
   - kNewGolden131072Crc64 = 0x56F1B721C72DC10E (TABU-CRC; source_catalog.hpp:191),
   - golden-Emissions-CRC 0xF1C1F26A1232073B (anatomy_fingerprint.hpp, Kopf-Kommentar),
   - die sha256-SOLLs der fuenf TABU-Dateien in scripts/pre_push_lande_gates.sh (Gate 6/6)
     -- Skript-SOLLs, Code-Anker und Datei-Stand wandern GEMEINSAM oder gar nicht.
4. **B-11.2 -- Bissprobe.** Unmittelbar nach dem Re-Anker beweist ein T-1-Durchgang, dass die
   Wachen am NEUEN Anker beissen (Wegwerf-Koeder rot, Gegenprobe gruen; Rot-Logs persistiert).
   Ein Re-Anker ohne Bissprobe ist eine Attrappe: er verschiebt die Zahl und prueft nichts.

## Warum EIN Zug (und nicht vier)

Jeder der vier Posten fuer sich invalidiert golden-Bestaende (Stempel-/Preimage-wirksam).
Vier getrennte Zuege bedeuteten vier Invalidierungen, vier Re-Anker, vier Bissproben und
drei Zwischenzustaende, in denen Anker und Bestand einander widersprechen. EIN Zug bedeutet
GENAU EINE Invalidierung mit GENAU EINEM Re-Anker und GENAU EINER Bissprobe. Das ist dieselbe
Harmonisierungs-Doktrin, die der Kopf von scripts/ci_test_inventory_floor.txt fuer den Floor
vorlebt (EINMAL LIVE neu messen, nie Deltas addieren) -- hier auf golden-Anker angewandt.

## Mechanischer Rueckhalt (seit G6/F9)

scripts/pre_push_lande_gates.sh, Gate [6/6] TABU-CRC-PROBE, stoppt JEDE Bewegung der fuenf
TABU-Dateien und jede Anker-Drift AUSSERHALB dieses Zuges hart (Exit 1, Meldung an den Lead,
niemals eigenmaechtiges Regenerieren). Der EINE Zug selbst aendert Code-Anker, Skript-SOLLs
und Dateistand in EINEM bewussten Commit -- damit erzwingt das Werkzeug die Ein-Zug-Form,
statt sie zu erbitten (K3/H6: Werkzeug schlaegt Disziplin).

## Abgrenzung

- Diese Notiz aendert KEINEN Anker und KEINE TABU-Datei; sie terminiert.
- Der Lock-Regen des Branches (axis_version_lock: 4 Digest-Drifts ohne Version-Bump, Stand
  19.08.2026) ist ein EIGENER Lande-Posten und gehoert NICHT in das golden-Ereignis; er hat
  mit den golden-Ankern nichts zu tun und blockiert Gate [4/6] bis zur Heilung.
