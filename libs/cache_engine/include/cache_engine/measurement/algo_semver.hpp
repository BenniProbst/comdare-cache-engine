// measurement/algo_semver.hpp -- X.Y.Z-Interpretation des algo_version-Strings + die FLAG-GRAMMATIK v2.
//
// Section43.b (User-Direktive): Versionen sind X.Y.Z mit X.Y = Feature-Version, Z = Debug-Revision im selben
// Feature-Stand -- fuer jeden Achsen-Algorithmus (alle Typen) einzeln + den Planer statisch.
//
// ================================================================================================
// FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026) -- SIE ERSETZT DIE Q3-GRAMMATIK VOLLSTAENDIG
// ================================================================================================
// Bau-Vorlage: super docs/plaene/20260807-DESIGN-flag-grammatik-v2-punkt-notation-komposit.md,
// Abschnitte 7 (E-1..E-6) und 9 (F-1..F-10 inkl. der F-2-Korrektur "c{p.e}").
//
//   version := UINT '.' UINT '.' UINT [ '.' <flag> ]*
//   flag    := <basis> [ '{' <sub> [ '.' <sub> ]* '}' ] | <companion>
//   basis   := 'c' | 'g' | 'f' | 'n' | 'x128' | 'x256' | 'x512'
//   sub     := <token> [ '{' <sub> [ '.' <sub> ]* '}' ]        -- REKURSIV
//
// Beispiel: memory_layout=SoaMemoryLayout@1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni
//
// DIE ACHT REGELN, jede einzeln durchgesetzt (und unten je einzeln CT-bewiesen):
//   (R1) KEIN 'v'-PRAEFIX MEHR (Owner-E4 Frage 5: "Ja das v faellt jetzt"). Rohe und gerenderte Form
//        FALLEN DAMIT ZUSAMMEN -- deshalb gibt es ab jetzt GENAU EINEN Parser (s. "EIN PARSER" unten).
//   (R2) PUNKT VOR JEDEM FLAG, auch dem ersten: "1.0.0.c", nie "1.0.0c" (Owner-E4 Frage 1).
//   (R3) BASIS DIREKT AN IHRER KLAMMER, ohne Punkt: "c{p.e}", nie "c.{p.e}" (Owner F-2-Korrektur).
//   (R4) HINTER '{' NIE EIN FUEHRENDER PUNKT (Owner, Klammer-Regel) -- und nie eine LEERE Gruppe.
//   (R5) NUR DER PUNKT TRENNT (Owner F-7: "Ja wir verwenden NUR den Punkt"). Die Unterstriche der
//        Katalog-Token fallen weg: avx512_vbmi2 -> "x512{vbmi2}".
//   (R6) KARDINALITAET 1 -> n: mehrere Flags sind erlaubt. Die Reihenfolge ist SACHLICH egal, wird aber
//        FORMAL eingehalten -- der Parser ist deterministisch und bewahrt die Eingabe-Reihenfolge, weil
//        der Stempel Identitaet ist (s. "WARUM NICHT SORTIERT/NORMALISIERT" unten).
//   (R7) 'e' BEDEUTET EFFICIENCY CORE (Owner F-2). Das Konzept "experimental" ist ERSATZLOS DEPRECATED:
//        take_experimental, AlgoSemVer::experimental und alle daran haengenden Wachen sind ENTFALLEN.
//   (R8) 'p' UND 'e' SIND SUB-FLAGS UNTER DER BASIS 'c'; "{p}" ist der Default, d.h. "c" == "c{p}"
//        SEMANTISCH (Owner F-2). Der Parser NORMALISIERT das NICHT -- s. unten.
//
// -- EIN PARSER, NICHT ZWEI -------------------------------------------------------------------------
// Bis zur v2 gab es parse_algo_semver (rohe Form "vX.Y.Z...") und parse_dotted_semver (gerenderte Form
// "X.Y.Z..."). Mit dem Wegfall des 'v' sind beide Formen ZEICHENGLEICH; zwei Namen fuer eine Grammatik
// waeren genau die Drift-Quelle, gegen die dieser Header seit A13-M1b argumentiert ("damit die Grammatik
// nicht zweimal existiert und nicht driften kann"). parse_dotted_semver ist deshalb ERSATZLOS ENTFALLEN;
// alle Aufrufer rufen parse_algo_semver. Es gibt keinen Alias und keine Uebergangs-Form -- ein Parser, der
// beide Schreibweisen akzeptiert, waere ein Behelfsweg (Owner: "ich moechte die alten Wege komplett
// ersetzen").
//
// -- EIN RENDERER, NICHT ZWEI -----------------------------------------------------------------------
// Symmetrisch dazu: render_algo_semver() ist die EINE Rueckrichtung. Sie ist constexpr und schreibt in ein
// FESTES Array, damit der consteval-Zwilling in builder/ceb_version_stamp.hpp sie unveraendert benutzen
// kann. Vor der v2 rechnete dieser Zwilling seine Laenge (ceb_digits + ceb_flag_len) und seine Zeichen
// (put_num + Flag-Schwanz) SELBST nach -- zwei Renderer fuer eine Form. Mit einer Flag-LISTE waere das die
// sichere Drift; deshalb ist es beim Umbau zusammengelegt worden. algo_semver_string() ist nur noch die
// std::string-Bequemlichkeit ueber demselben Renderer.
//
// -- WARUM NICHT SORTIERT UND NICHT NORMALISIERT ----------------------------------------------------
// Owner: "Reihenfolge sachlich egal, formal eingehalten" und "{p} ist der DEFAULT ... '1.0.0.c' ==
// '1.0.0.c{p}' SEMANTISCH". Beides sind Aussagen ueber die BEDEUTUNG, nicht ueber die BYTES. Der Parser
// haelt deshalb die Eingabe-Reihenfolge und faltet "c" NICHT auf "c{p}":
//   * Der Stempel IST Identitaet. Wuerde der Parser sortieren oder Defaults auffuellen, ergaebe der
//     Roundtrip parse -> render eine ANDERE Zeichenfolge als das Quell-Literal -- die SHA512-Zeile und die
//     Lager-Identitaet haetten dann zwei Werte fuer dieselbe Quelle.
//   * Die Aussage "c bedeutet c{p}" braucht den KATALOG (sie weiss, dass p ein Sub von c ist). Der Katalog
//     ist ausdruecklich NICHT Teil dieser Scheibe (s. S2-KATALOG-ANDOCKSTELLE unten).
// Folge, die ehrlich benannt gehoert: "1.0.0.c" und "1.0.0.c{p}" sind hier ZWEI VERSCHIEDENE Werte. Wer
// sie gleichsetzen will, entscheidet damit ein Byte-Ereignis -- das ist ein Owner-Entscheid, kein
// Nebenprodukt dieses Parsers.
//
// -- WAS DIESER PARSER SELBST NICHT PRUEFT -- UND WER ES SEIT S2 TUT ---------------------------------
// parse_algo_semver prueft die FORM, nicht den KATALOG: "x512{quatsch}" und "zzz" sind FUER DEN PARSER
// wohlgeformt. Ob 'vl' ein echtes AVX-512-Subset ist, ob 'gfni' ein zulaessiges Companion-Flag ist und ob
// ein Token unter SEINER Basis steht, entscheidet die KATALOG-WACHE flag_catalog_is_satisfied() -- seit
// dem 07.08.2026 gebaut (S2), auf measurement/flag_grammar_catalog.hpp gestuetzt und in
// ce_owned_version_is_wellformed() eingehaengt. Die Trennung bleibt: der PARSER kennt keinen Katalog (er
// liefert "1.0.0.x512{quatsch}" weiter als geparsten Wert), die WACHE lehnt ihn ab. Wer nur parst, sieht
// die Form; wer eine ce-eigene Version PRUEFT, bekommt beides. Der Grep-Anker der Naht bleibt
// S2-KATALOG-ANDOCKSTELLE.
//
// == UNVERAENDERT AUS DER Q3-WELT (die Zahlen-Ebene) ================================================
//   UINT := '0' | [1-9][0-9]{0,5}  -- KEINE fuehrende Null (ausser der Komponente "0" selbst), HOECHSTENS
//   kMaxSemVerComponentDigits Ziffern (B11, Codex-Review 02.08.2026). Beides verhindert ALIAS-IDENTITAETEN:
//   "01.0.0.c" und "4294967297.0.0.c" stempelten sonst wie "1.0.0.c" (Leading-Zero bzw. stiller
//   Modulo-2^32-Umlauf), waehrend compose_algo_signature die rohen Literale VERBATIM serialisiert -- ein
//   Lager-Key-Zusammenfall bei roh verschiedenen Binaries. Beide Formen sind Sentinel.
//
// KURZFORM VERBOTEN (Owner-Q3, in der v2 unveraendert gueltig): "1" und "1.2" sind SENTINEL. Versionen sind
// einheitlich und immer dreistellig.
//
// SENTINEL-WACHE (A13-Review-Auflage K-5, in der v2 unveraendert gueltig): das NULL-TRIPEL IST der Sentinel
// -- auch mit Flags. "0.0.0.c" parst exakt auf AlgoSemVer{} (leere Flag-Liste), damit die bestehenden
// CT-Wachen (parse != Sentinel) nicht ueber die Flag-Formen ausgehebelt werden koennen. is_sentinel() prueft
// zusaetzlich NUR das x/y/z-Tripel (nicht den ganzen Struct) -- die Wachen bleiben auch dann dicht, wenn eine
// spaetere Aenderung ein Flag doch durch den Sentinel-Pfad durchreicht.
//
// Metaprog: reines constexpr, kein std::variant, keine vtable, KEINE dynamische Allokation (der
// Anatomie-Fingerprint ist consteval -- s. abi/anatomy_fingerprint.hpp), keine schweren Includes.

#pragma once

// Der KATALOG der Grammatik (S2, 07.08.2026): welche Token es gibt und unter welcher Basis jedes stehen
// darf. Er liegt bewusst in einer eigenen Datei -- die Grammatik ist eine Aussage ueber ZEICHEN, der
// Katalog eine ueber HARDWARE. Diese Zeile ist die EINE Naht zwischen beiden.
#include <cache_engine/measurement/flag_grammar_catalog.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

/// SCHARFSCHALTUNG der Owner-PFLICHT "jede ce-eigene Version traegt das CPU-Flag" (Q3, in der v2 als F-10
/// bestaetigt und praezisiert: "ce-eigene Achsen tragen mindestens 'c'").
/// DEFAULT ON seit A13-M3/C4 (03.08.2026): der Migrations-Commit hat die Bestands-Strings gezogen UND dieses
/// Define im SELBEN Commit auf 1 gesetzt (ein Byte-Ereignis, ein Neuanker). Wer es heute auf 0 zuruecksetzt,
/// laesst die Owner-Pflicht undurchgesetzt; die Bestands-Literale bleiben davon unberuehrt.
/// MIGRATIONS-NAHT-LISTE (A13-M1b-Fixup Review-BEFUND-2; Klasse (d) aus A13-M2 Review-BEFUND-3).
/// VOLLZUG (A13-M3/C4 fuer die Q3-Form, FLAG-GRAMMATIK-v2 07.08.2026 fuer die Punkt-Notation): alle unten
/// gefuehrten Klassen (a)-(h) stehen migriert da, ENFORCE ist scharf. Die Liste bleibt trotzdem stehen: sie
/// sagt, WELCHE Quellen-KLASSEN es gibt und woran eine neue erkannt wird. Jede KUENFTIGE ce-Versions-Quelle
/// gehoert hier hinein und traegt die beiden B12-Wachen -- sonst entsteht wieder eine Luecke wie bei (e)/(f).
/// DOKTRIN VORWEG: diese Liste ist AUS DEM DIFF/GREP ABZULEITEN, NIE HANDGEPFLEGT. Genau das Fortschreiben
/// von Hand ist beim M2-Bau unterblieben (BEFUND-3). Ueberall dort, wo die Fundstellen ZAEHLBAR sind und
/// mit dem Bestand WACHSEN, steht deshalb unten das KOMMANDO, mit dem der Migrations-Commit seine
/// Fundstellen selbst erhebt -- die genannten Zahlen sind Momentaufnahmen und altern, die Kommandos nicht.
///
/// DAS ERSTE, BINDENDE KOMMANDO IST DER GENERISCHE WACHEN-GREP (A13-M3/C2, Befund GA-05/Z-10 vom
/// 03.08.2026). Er steht VOR den klassen-spezifischen Kommandos, weil er die Klassen VOLLSTAENDIG liefert,
/// statt sie an Datei-NAMEN zu erraten:
///     grep -rn 'ce_owned_version_satisfies_cpu_enforce\|ce_owned_version_is_wellformed' --include=*.hpp --include=*.cpp libs tests tools apps
///     (09.08.2026 EINZEILIG gezogen: die Fortsetzung stand als '\' am Zeilenende und setzte damit
///      den // -Kommentar fort -- GCC meldete "multi-line comment" [-Wcomment]. Mit 476 von 2447
///      Vorkommen war das der GROESSTE Einzelposten der Warnungs-Erhebung. Hier folgte zufaellig
///      wieder eine ///-Zeile, es ging also nichts verloren; waere die Folgezeile Code gewesen,
///      waere sie STILL verschwunden. Genau deshalb wird das Muster beseitigt, nicht unterdrueckt.)
/// Begruendung (das ist keine Stil-Frage): jede ce-EIGENE Versions-Quelle traegt per B12-Doktrin genau
/// diese beiden Wachen -- wer eine neue Quelle ohne sie anlegt, hat keine Naht angelegt, sondern eine
/// Luecke. BELEG, warum dieser grep noetig wurde: die Klassen (e) kPlannerVersion und (f) kOsProbeVersion
/// fielen durch ALLE DREI frueher als bindend deklarierten Kommandos hindurch -- (a) greppt drei namentlich
/// genannte Dateien, (d) greppt 'axis_code_version *=', der Absicherungs-grep greppt
/// 'meta_meta_version_cpu_pflicht'. Die Selbst-Erhebung war damit nachweislich unvollstaendig, obwohl sie
/// als "nie handgepflegt" deklariert war. Die klassen-spezifischen Kommandos unten bleiben stehen: sie
/// sagen, WAS an der jeweiligen Fundstelle zu tun ist; der generische grep sagt, WELCHE es gibt.
///   (a) 7 Nicht-Organ-Literale: abi/system_axis_code_versions.hpp (3x), measurement/
///       measurement_tooling_registry.hpp (3x), measurement/measurement_framework_registry.hpp (1x) --
///       alle tragen dieselbe gated ENFORCE-Wache und brechen beim Scharfschalten mit.
///       Erhebung: grep -n '"[0-9]' auf genau diese drei Dateien (Feld `version` der Registry-Tabellen).
///   (b) builder/ceb_version_stamp.hpp: der konsteval-Zwilling rendert die Version seit der v2 ueber
///       DENSELBEN render_algo_semver() wie der Runtime-Weg -- es gibt dort keinen eigenen Renderer und
///       keine eigene Laengen-Rechnung mehr, die driften koennte.
///   (c) compose_algo_signature (axis_variant_version_table.hpp) serialisiert W::algo_version VERBATIM:
///       jede Literal-Migration ist damit AUCH ein .algos-Sidecar-Byte-Ereignis (Skip-/Rebuild-Kaskade der
///       Sidecar-Welt) und gehoert in dasselbe Byte-Ereignis-Fenster wie die Stempel-Migration.
///   (d) A13-M2 (Review-BEFUND-3): die META-META-Code-Versionen. Seit dem Owner-Entscheid E2
///       ("Die Meta-Meta-Achsen und deren Stempel-Eintraege sind wie alle Hauptachsen PFLICHT") traegt
///       JEDE Meta-Meta ein eigenes `axis_code_version`-Literal -- eine VIERTE Quelle neben (a)-(c),
///       die es beim A13-M1b-Fixup noch nicht gab. Zwei Sorten, und der Unterschied ist der Grund,
///       warum diese Klasse ueberhaupt eigens genannt wird:
///         * MECHANISCH GESICHERT (bricht beim Scharfschalten von selbst mit): jede Stelle, an der
///           neben dem ungated meta_meta_version_wohlgeformt<> auch der gated
///           meta_meta_version_cpu_pflicht<>-Zwilling steht -- heute
///           measurement/external_utils_family_axis.hpp (SimdExternalUtilsFamily).
///         * UNGESICHERT (bricht NICHT mit, muss aktiv gefunden werden): test-lokale Meta-Metas ohne
///           gated Wache -- heute tests/unit/test_striktheit_axis_dach_guard.cpp (ProofOrganMetaMeta)
///           und tests/unit/test_meta_meta_halbordnung.cpp (Avx512MetaMeta, GpuMetaMeta,
///           GpuClusterMetaMeta). Diese Sorte waechst mit jeder neuen Test-Meta-Meta.
///       GREP-ANWEISUNG (bindend):
///           grep -rn 'axis_code_version *=' --include=*.hpp --include=*.cpp libs tests tools apps
///       liefert die vollstaendige Klasse (d); ihre gated Absicherung findet
///           grep -rn 'meta_meta_version_cpu_pflicht' libs tests
///       Wer eine Meta-Meta hinzufuegt, ohne den gated Zwilling danebenzustellen, legt eine neue
///       UNGESICHERTE Naht -- deshalb ist der erste grep der Migrations-Einstieg, nicht diese Liste.
///   (e) CX-W5 (Codex-Doppelreview 02.08.2026): die PLANER-SELBST-Version. profile_facade/planner/
///       planner_version.hpp traegt kPlannerVersion als EIGENEN ce-Versionspfad (Section43.b "den Planer
///       statisch"). MECHANISCH GESICHERT: die gated static_assert in planner_version.hpp bricht beim
///       Scharfschalten von selbst mit.
///       Erhebung: grep -rn 'kPlannerVersion *=' --include=*.hpp profile_facade/planner
///   (f) OS-U3 (Commit d115e4cc; Befund GA-05/Z-10 vom 03.08.2026, in A13-M3/C2 nachgetragen): die
///       OS-PROBE-VERFAHRENS-Version. measurement/operating_system_probe.hpp fuehrt kOsProbeVersion als
///       EIGENES ce-Literal -- EIN Literal, aber DREI probe_ids ("os_probe.<familie>@1.0.0.c", je Familie
///       eine). MECHANISCH GESICHERT: die gated ENFORCE-Wache in operating_system_probe.hpp bricht beim
///       Scharfschalten von selbst mit. WARUM DIE KLASSE HIER FEHLTE (die Lehre): das Literal heisst weder
///       algo_version noch axis_code_version und liegt in keiner der drei namentlich genannten
///       Registry-Dateien -- es fiel damit durch jedes einzelne der frueheren Kommandos. Genau deshalb
///       steht der generische Wachen-grep jetzt VORNE.
///       ABGRENZUNG: eine Migration ist hier ein probe_id-BYTE-Ereignis (die Provenienz-Kette des
///       OS-U4-Token-Tripels sieht "os_probe.<fam>@1.0.0.c"), aber KEIN Stempel-Ereignis -- die
///       A-15-Neutralitaet der Probe (kein erhobener Wert und keine probe_id reist in den Binary-Stempel)
///       bleibt unangetastet.
///       Erhebung: der generische Wachen-grep oben faengt sie (operating_system_probe.hpp).
///   (g) OD-10-RT (03.08.2026): die NUMA/PAGE-ERHEBUNGS-Verfahrens-Version.
///       measurement/numa_page_probe.hpp fuehrt kNumaPageProbeVersion als EIGENES ce-Literal --
///       wieder EIN Literal und DREI probe_ids ("numa_page_probe.<familie>@1.0.0.c", je Familie eine).
///       MECHANISCH GESICHERT: der Header traegt beide B12-Wachen (ungated
///       ce_owned_version_is_wellformed im K5-Vertrag, gated ce_owned_version_satisfies_cpu_enforce
///       hinter COMDARE_VERSION_HW_FLAG_ENFORCE).
///       WARUM SIE HIER STEHT, OBWOHL DER GENERISCHE GREP SIE FINDET: die Liste sagt, WELCHE
///       Quellen-KLASSEN es gibt und woran eine neue erkannt wird. (f) und (g) sind DIESELBE Klasse
///       "Erhebungs-Verfahrens-Version" mit zwei Auspraegungen -- wer eine dritte Probe baut, sieht
///       hier, dass sie dazugehoert, statt sie fuer einen Sonderfall zu halten.
///       ABGRENZUNG wie bei (f): probe_id-Ereignis, KEIN Stempel-Ereignis -- die A-15-Neutralitaet
///       der Probe bleibt unangetastet, und die System-Achsen-Code-Version "target_isa" wird nicht
///       gebumpt.
///       Erhebung: der generische Wachen-grep oben faengt sie (numa_page_probe.hpp).
///   (h) OD-11-RT (06.08.2026): die KERN-KLASSEN-/PROZESS-LOKALITAETS-Verfahrens-Version.
///       measurement/numa_cpu_pin_process_probe.hpp fuehrt kNumaCpuPinProcessProbeVersion als EIGENES
///       ce-Literal -- wieder EIN Literal und DREI probe_ids
///       ("numa_cpu_pin_process_probe.<familie>@1.0.0.c", je Familie eine). Sie ist die DRITTE Auspraegung
///       derselben Klasse wie (f) und (g) und belegt damit, dass die in (g) beschriebene Klasse
///       "Erhebungs-Verfahrens-Version" wirklich eine Klasse ist und kein Paar von Sonderfaellen.
///       MECHANISCH GESICHERT: der Header traegt beide B12-Wachen, dazu die Praefix-DISJUNKTHEITS-Wache
///       gegen "numa_page_probe." -- ohne sie liese ein Log-Grep auf die eine Erhebungs-Familie die
///       andere mit.
///       ABGRENZUNG wie bei (f)/(g): probe_id-Ereignis, KEIN Stempel-Ereignis. Die Unter-Achse
///       core_class haengt an target_isa (binary_id="never"); die A-15-Neutralitaet ist damit nicht nur
///       zugesichert, sondern strukturell erzwungen.
///       Erhebung: der generische Wachen-grep oben faengt sie (numa_cpu_pin_process_probe.hpp).
#ifndef COMDARE_VERSION_HW_FLAG_ENFORCE
#define COMDARE_VERSION_HW_FLAG_ENFORCE 1
#endif

namespace comdare::cache_engine::measurement {

// == DIE FLAG-LISTE: REPRAESENTATION UND IHRE BEGRUENDUNG ==========================================
//
// GEWAEHLT: EIN FLACHES, VORORDNUNGS-SORTIERTES KNOTEN-ARRAY MIT EXPLIZITER TIEFE.
// Also: std::array<FlagToken, kMaxFlagNodes> + Belegungszaehler, wobei jeder Knoten sein TOKEN und seine
// TIEFE traegt und die Knoten in der Reihenfolge stehen, in der sie im Text auftreten (Pre-Order).
//
// WARUM NICHT (a) "std::array<FlagEntry, N> mit FlagEntry{Basis, std::array<SubToken,M>, count}":
//   Diese Form kann GENAU zwei Ebenen. Die Grammatik ist aber rekursiv (`sub := token [ '{' ... '}' ]`,
//   Owner: "p und e sind je ein Komposit-Flag", koennen also selbst Sub-Flags tragen). Heute uebt das
//   keine Stelle aus -- aber sobald eine es tut, waere die Struktur ein ZWEITER Umbau an derselben Naht,
//   und zwar an der Naht, die im Fingerprint-Preimage haengt. Der Auftrag sagt dazu ausdruecklich: eine
//   Tiefe > 1 darf spaeter keinen zweiten Umbau erzwingen.
//
// WARUM NICHT (b) "Basis-Enum + uint32_t-Bitmaske je Basis":
//   Eine Bitmaske DECKELT den Katalog auf 32 Sub-Token je Basis und verlangt, dass der Katalog HIER
//   bekannt ist. Beides ist falsch: der Katalog ist ausdruecklich noch nicht recherchiert (Owner F-5/F-6),
//   und Owner F-3 verlangt fuer die POD-Kodierung ausdruecklich einen HASH statt einer Bitmaske ueber
//   einen festen Katalog, "damit die Katalog-Groesse nicht gedeckelt ist". Eine Bitmaske im Parser waere
//   dieselbe Deckelung eine Ebene frueher.
//
// WARUM EIN FLACHES ARRAY UND KEINE ZEIGER-BAEUME:
//   Der Anatomie-Fingerprint ist consteval (abi/anatomy_fingerprint.hpp, anatomy_fingerprint_hex). Es darf
//   deshalb weder dynamische Allokation noch std::vector/std::string geben. Ein Knoten-Array mit
//   Belegungszaehler ist die einzige Form, die zugleich constexpr-faehig, wertsemantisch vergleichbar
//   (operator== ohne Zeiger-Identitaet) und ohne Nachbau renderbar ist.
//
// WARUM PRE-ORDER + TIEFE STATT ELTERN-INDIZES:
//   Der Text IST Pre-Order. Parsen und Rendern sind damit beide ein einziger linearer Durchlauf, und die
//   Rekonstruktion der Zeichenfolge ist VERLUSTFREI: Tiefensprung nach oben -> '{', gleiche Tiefe -> '.',
//   Tiefensprung nach unten -> so viele '}' wie Ebenen, dann '.'. Ein Eltern-Index waere redundante,
//   zweite Wahrheit ueber dieselbe Information (parent_flag_node_index() unten LEITET ihn aus der Tiefe ab,
//   speichert ihn aber nicht).
//
// DER TIEFEN-DECKEL IST DOKUMENTIERT UND STATIC_ASSERT-GESICHERT, NICHT UNENDLICH:
//   kMaxFlagDepth == 4 ist die Zusage dieses Headers. Heute uebt der Bestand Tiefe 1 aus ("c{p}"), die
//   Grammatik erlaubt mehr. Eine UNBEGRENZTE Tiefe waere in einer constexpr-Rekursion ohnehin nur bis zum
//   Compiler-Limit unbegrenzt -- also ein unbenannter Deckel statt eines benannten. Wer tiefer schachtelt,
//   bekommt den Sentinel und bricht damit an den CT-Wachen; das ist die laute Form.

/// Laengen-Deckel EINES Flag-Tokens. AM ECHTEN KATALOG BEMESSEN (SIMD-Recherche 07.08.2026): das laengste
/// vorkommende Token ist "3dnowprefetch" (13), danach "vp2intersect" (12) und "vpclmulqdq" (10). 16 laesst
/// drei Zeichen Luft, ohne den Knoten unnoetig gross zu machen.
/// ABGRENZUNG: die langen cpuinfo-Namen ("avx512_vp2intersect", 19) sind NICHT unsere Token -- s. die
/// DREI-NAMENSRAEUME-Notiz weiter unten.
inline constexpr std::size_t kMaxFlagTokenLen = 16;

/// Deckel der GESAMTZAHL an Flag-Knoten einer Version (Basen + alle Sub-Token auf allen Ebenen).
///
/// AM ECHTEN KATALOG BEMESSEN, nicht geschaetzt. Die SIMD-Recherche (Owner-Auftrag F-5/F-6, Ergebnis
/// 07.08.2026) liefert die Vollmenge; der teuerste konstruierbare Fall ist eine Version, die ALLES
/// deklariert:
///     c{p.e}                                        3 Knoten
///   + x128{sse.sse2.sse3.ssse3.sse41.sse42.sse4a.aes.pclmulqdq.sha}     1 + 10 = 11
///   + x256{avx.avx2.fma.f16c.vnni.ifma.vnniint8.vnniint16.neconvert.sha512.sm3.sm4}  1 + 12 = 13
///   + x512{f.cd.vl.dq.bw.ifma.vbmi.vbmi2.vnni.bitalg.vpopcntdq.vp2intersect.bf16.fp16}  1 + 14 = 15
///   + Companion/Skalar (gfni vaes vpclmulqdq popcnt bmi1 bmi2 abm movbe adx rdrand rdseed)   11
///   + die basislose MMX-Familie (mmx mmxext 3dnow 3dnowext 3dnowprefetch)                     5
///                                                                                    ------------
///                                                                                            58
/// Der ERSTE Ansatz stand auf 32 -- das haette eine vollstaendig deklarierte Organ-Version ABGELEHNT
/// (der Deckel ist fail-loud: FlagList::append liefert false, die Version wird Sentinel und bricht an den
/// CT-Wachen). Ein Deckel, der eine LEGITIME Eingabe verwirft, ist ein Defekt und keine Wache -- deshalb
/// steht hier 96: deutlich ueber den 58, ohne unbegrenzt zu sein, mit Luft fuer die naechste ISA-Generation
/// (AVX10 und die APX-Nachbarn stehen in der Recherche bereits als Reserve).
inline constexpr std::size_t kMaxFlagNodes = 96;

/// Deckel der SCHACHTELUNGS-TIEFE. 0 == Flag direkt hinter dem Tripel, 1 == Sub-Flag in dessen Klammer.
/// Der Bestand uebt heute hoechstens 1 aus; die Grammatik ist rekursiv, deshalb ist der Deckel groesser
/// als der Bedarf -- aber BENANNT (s. Begruendung oben).
inline constexpr std::uint8_t kMaxFlagDepth = 4;

static_assert(kMaxFlagDepth >= 1, "die Grammatik braucht mindestens eine Klammer-Ebene ('c{p}').");
static_assert(kMaxFlagNodes >= 1 && kMaxFlagNodes <= 255, "der Belegungszaehler ist ein uint8_t.");
static_assert(kMaxFlagTokenLen >= 1 && kMaxFlagTokenLen <= 255, "die Token-Laenge ist ein uint8_t.");

/// EIN Knoten der Flag-Liste: sein Token und die Klammer-Tiefe, in der es steht.
/// Das Token liegt als KOPIE im Knoten, nicht als string_view in die Eingabe. Begruendung: AlgoSemVer wird
/// als WERT herumgereicht, verglichen und in constexpr-Variablen abgelegt -- eine Sicht in einen
/// Eingabe-String, der ein Temporary sein kann (algo_semver_string nimmt einen string_view entgegen),
/// waere eine haengende Referenz, die im constexpr-Pfad zufaellig noch funktioniert und im Laufzeit-Pfad
/// nicht.
struct FlagToken {
    std::array<char, kMaxFlagTokenLen> text{};
    std::uint8_t                       len   = 0;
    std::uint8_t                       depth = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view{text.data(), len}; }
};

[[nodiscard]] constexpr bool operator==(FlagToken const& a, FlagToken const& b) noexcept {
    if (a.len != b.len || a.depth != b.depth) return false;
    for (std::size_t i = 0; i < a.len; ++i)
        if (a.text[i] != b.text[i]) return false;
    return true;
}

/// Die Flag-Liste einer Version: die Knoten in TEXT-Reihenfolge (Pre-Order) + Belegungszaehler.
struct FlagList {
    std::array<FlagToken, kMaxFlagNodes> nodes{};
    std::uint8_t                         count = 0;

    [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }

    /// Haengt ein Token an. Liefert false, wenn der Deckel erreicht ist oder das Token zu lang ist --
    /// der Aufrufer faellt dann auf den Sentinel (nie stilles Kuerzen: ein gekuerztes Flag waere ein
    /// FALSCHER Stempel, und der Stempel ist Identitaet).
    [[nodiscard]] constexpr bool append(std::string_view tok, std::uint8_t depth) noexcept {
        if (count >= kMaxFlagNodes || tok.empty() || tok.size() > kMaxFlagTokenLen) return false;
        FlagToken& n = nodes[count];
        for (std::size_t i = 0; i < tok.size(); ++i) n.text[i] = tok[i];
        n.len   = static_cast<std::uint8_t>(tok.size());
        n.depth = depth;
        ++count;
        return true;
    }
};

[[nodiscard]] constexpr bool operator==(FlagList const& a, FlagList const& b) noexcept {
    if (a.count != b.count) return false;
    for (std::size_t i = 0; i < a.count; ++i)
        if (!(a.nodes[i] == b.nodes[i])) return false;
    return true;
}

/// X.Y.Z-Tripel (X.Y = Feature, Z = Debug-Revision) + die FLAG-LISTE der Grammatik v2.
/// Was hier NICHT MEHR steht und warum: das frueher eigene Feld `bool experimental` (Owner-E2 02.08.2026)
/// ist ERSATZLOS ENTFALLEN -- 'e' bedeutet seit dem Owner-KERN vom 07.08.2026 EFFICIENCY CORE und ist damit
/// ein Flag wie jedes andere (R7). Das frueher eigene Feld `HardwareFlag hardware` ist in der Flag-Liste
/// aufgegangen: 'c'/'g'/'f'/'n' sind Basen unter den Flags, nicht mehr ein Skalar-Enum (R6, Kardinalitaet
/// 1 -> n).
struct AlgoSemVer {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
    FlagList      flags{};

    /// K-5-Sentinel-Praedikat: der Sentinel ist das NULL-TRIPEL -- unabhaengig von den Flags. CT-Wachen
    /// pruefen HIERMIT, nicht per Struct-Vergleich gegen AlgoSemVer{}, damit eine Flag-Fehlform sie nie
    /// umgeht.
    [[nodiscard]] constexpr bool is_sentinel() const noexcept { return x == 0u && y == 0u && z == 0u; }

    /// Traegt die Version ueberhaupt ein Flag? (Die Owner-PFLICHT verlangt zusaetzlich, dass 'c' DARUNTER
    /// ist -- dafuer version_satisfies_cpu_only_policy().)
    [[nodiscard]] constexpr bool has_flags() const noexcept { return !flags.empty(); }

    /// Traegt die Version das genannte Token als TOP-LEVEL-Flag (Tiefe 0)? Das ist die Frage, die die
    /// B12-Politik stellt ("traegt mindestens 'c'"), und sie ist bewusst auf Tiefe 0 beschraenkt: ein
    /// 'c' als SUB-Token unter einer anderen Basis waere etwas anderes als die Basis 'c'.
    [[nodiscard]] constexpr bool has_top_level_flag(std::string_view token) const noexcept {
        for (std::size_t i = 0; i < flags.count; ++i)
            if (flags.nodes[i].depth == 0u && flags.nodes[i].view() == token) return true;
        return false;
    }
};

[[nodiscard]] constexpr bool operator==(AlgoSemVer const& a, AlgoSemVer const& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.flags == b.flags;
}

/// A13-M1b/B11 ZIFFERN-DECKEL einer Versions-KOMPONENTE (X, Y bzw. Z). Sechs Stellen (bis 999999) liegen
/// weit ueber jedem realen Achsen-/Tooling-Stand; alles darueber ist keine Version, sondern ein Tippfehler
/// oder ein Angriff auf die Stempel-Identitaet. Der Deckel ist die FACHLICHE Wache; die arithmetische
/// Ueberlauf-Wache in take_uint ist das zweite, deckel-unabhaengige Netz.
inline constexpr std::size_t kMaxSemVerComponentDigits = 6;

namespace detail {
/// Konsumiert eine Dezimalzahl am Anfang von s (modifiziert s NUR im Erfolgsfall). Rueckgabe {wert, gefunden}.
///
/// B11-WACHEN (Codex-Review 02.08.2026) -- ohne sie erzeugte der Parser ALIAS-IDENTITAETEN, also zwei ROH
/// verschiedene Versions-Literale mit demselben Stempel-Segment (und damit demselben SHA512-/Lager-Key,
/// waehrend die .algos-Sidecar-Signatur die Literale roh verschieden serialisiert):
///   (1) UEBERLAUF: "4294967297.0.0.c" wickelte modulo 2^32 auf 1 um und stempelte wie "1.0.0.c".
///   (2) FUEHRENDE NULL: "01.0.0.c" stempelte wie "1.0.0.c".
/// Beide sind SENTINEL -- die Registry-Wachen (parsbar-Pflicht) brechen darauf compile-time.
///
/// Reihenfolge der Netze: erst der Ziffern-Deckel (fachlich), dann die Ueberlauf-Wache (arithmetisch).
/// Bei kMaxSemVerComponentDigits <= 9 kann der zweite Zweig rechnerisch nicht mehr greifen -- er steht
/// bewusst trotzdem da, weil er bei einer spaeteren Deckel-Anhebung OHNE Nacharbeit weitertraegt (die
/// Klasse "stiller Ueberlauf" darf nicht an einer Konstante haengen).
[[nodiscard]] constexpr std::pair<std::uint32_t, bool> take_uint(std::string_view& s) noexcept {
    if (s.empty() || s.front() < '0' || s.front() > '9') return {0u, false};
    bool const       fuehrende_null = (s.front() == '0');
    std::string_view rest           = s; // erst pruefen, dann committen: nie ein halb konsumierter Rest
    std::uint32_t    n              = 0;
    std::size_t      ziffern        = 0;
    while (!rest.empty() && rest.front() >= '0' && rest.front() <= '9') {
        if (++ziffern > kMaxSemVerComponentDigits) return {0u, false}; // (1a) Ziffern-Deckel
        auto const d = static_cast<std::uint32_t>(rest.front() - '0');
        if (n > (std::numeric_limits<std::uint32_t>::max() - d) / 10u) return {0u, false}; // (1b) Ueberlauf
        n = n * 10u + d;
        rest.remove_prefix(1);
    }
    // (2) LEADING-ZERO-VERBOT: exakt "0" ist erlaubt (der Sentinel und jede echte Null-Komponente),
    //     "01"/"007" nicht -- sonst gaebe es zu jeder Version unendlich viele rohe Schreibweisen.
    if (fuehrende_null && ziffern > 1) return {0u, false};
    s = rest;
    return {n, true};
}

/// Konsumiert EIN Flag-Token: die maximale Folge aus [a-z0-9] am Anfang von s, die MINDESTENS EINEN
/// Buchstaben enthaelt.
///
/// DAS IST DIE STELLE, AN DER DER PARSER NICHT ZEICHENWEISE RAET (Auftrag, ausdruecklich): 'x512' ist EINE
/// mehrzeichige Basis und 'gfni' EIN Companion-Flag -- ein zeichenweiser Parser haette 'g' (GPU) plus einen
/// Rest 'fni' gelesen. Die Token-Grenze ist ausschliesslich '.', '{', '}' oder das Ende.
/// GROSSBUCHSTABEN und UNTERSTRICHE sind KEINE Token-Zeichen und beenden das Token damit NICHT-leer, aber
/// unverbraucht -- der Aufrufer laeuft danach in seine Rest-Pruefung und faellt auf den Sentinel. Genau so
/// ist Owner F-7 gemeint ("Ja wir verwenden NUR den Punkt"): "x512_vbmi2" ist kein gueltiges Flag, die
/// Notation heisst "x512{vbmi2}".
/// Ein Token laenger als kMaxFlagTokenLen wird NICHT gekuerzt, sondern abgelehnt (leerer Rueckgabewert):
/// ein gekuerztes Flag waere ein falscher Stempel.
///
/// WARUM ZIFFERN ERLAUBT SIND, EIN TOKEN AUS LAUTER ZIFFERN ABER NICHT -- das ist keine Stil-Frage,
/// sondern eine ALIAS-WACHE derselben Klasse wie B11. Sie schliesst ZWEI Loecher, und beide sind am
/// echten SIMD-Katalog (Recherche 07.08.2026) belegt:
///   * Ziffern MUESSEN vorkommen duerfen: 'x512', 'vbmi2', 'f16c', 'sse41', 'vnniint16' tragen sie, und
///     '3dnow'/'3dnowext'/'3dnowprefetch' sogar an ERSTER Stelle. Ein "Token beginnt mit einem
///     Buchstaben" waere also falsch.
///   * LOCH 1 (Tiefe 0): ein Token aus lauter Ziffern macht "1.2.3.4" zugleich zu einer (verbotenen)
///     VIERSTELLIGEN Version und zu einer dreistelligen mit dem Flag "4". Ein Tippfehler in der
///     Bezifferung wuerde still zu einer gueltigen, aber ANDEREN Identitaet.
///   * LOCH 2 (jede Tiefe): der Katalog enthaelt Namen MIT PUNKT -- 'avx10.1' und 'avx10.2' (AVX10
///     versioniert sich so). Der Punkt IST unser Trenner. Waere "1" ein gueltiges Token, dann laese sich
///     "x512{avx10.1}" STILL als zwei Geschwister 'avx10' und '1' -- wieder eine gueltige, aber andere
///     Identitaet. Mit dieser Regel bricht die Form stattdessen laut (Sentinel).
/// Die Bedingung ist rein FORMAL (mindestens ein [a-z]) und kennt keinen Katalog.
///
/// WAS SIE KOSTET -- OFFEN GELEGT, WEIL ES EIN OWNER-ENTSCHEID IST UND KEIN NEBENEFFEKT:
/// die Recherche schlaegt fuer die Klammer eine PRAEFIX-STRIPPING-Kurzform vor ("x128{2.3.41.42}" statt
/// "x128{sse2.sse3.sse41.sse42}"), mit dem Argument, dass die Basis innerhalb der Klammer disambiguiert.
/// Diese Kurzform ist unter der Regel oben NICHT SCHREIBBAR. Das ist Absicht, aber es ist eine
/// FORM-Entscheidung mit KATALOG-Wirkung, und sie gehoert deshalb sichtbar hierher:
///   -- die Recherche fuehrt die Kurzform selbst als "Owner-Entscheid noetig", nicht als Festlegung;
///   -- die VOLLNAMEN-Form ("x128{sse2.sse41}") traegt denselben Katalog vollstaendig und ist unter
///      dieser Regel gueltig -- es geht also keine Ausdruckskraft verloren, nur eine Abkuerzung;
///   -- die Kurzform zuzulassen hiesse, Loch 2 wieder zu oeffnen (s.o.), und der Owner hat den lauten
///      Bruch ausdruecklich zum wichtigsten Merkmal dieses Umbaus erklaert.
/// Wer die Kurzform will, hebt diese Regel auf und nimmt dafuer die stille Fehllesung von 'avx10.1' in
/// Kauf. Das ist eine Abwaegung, keine Rechenaufgabe -- deshalb steht sie hier und wird nicht entschieden.
[[nodiscard]] constexpr std::string_view take_flag_token(std::string_view& s) noexcept {
    std::size_t n            = 0;
    bool        hat_buchstab = false;
    while (n < s.size() && ((s[n] >= 'a' && s[n] <= 'z') || (s[n] >= '0' && s[n] <= '9'))) {
        if (s[n] >= 'a' && s[n] <= 'z') hat_buchstab = true;
        ++n;
    }
    if (n == 0 || n > kMaxFlagTokenLen || !hat_buchstab) return std::string_view{};
    std::string_view const tok = s.substr(0, n);
    s.remove_prefix(n);
    return tok;
}

/// node := <token> [ '{' <node> [ '.' <node> ]* '}' ]   -- die EINE rekursive Produktion.
///
/// WARUM EINE PRODUKTION UND NICHT ZWEI: die Grammatik notiert `flag := <basis> [ '{' ... '}' ] |
/// <companion>` und `sub := <token> [ '{' ... '}' ]`. Beide haben DIESELBE Form "Token, optional gefolgt
/// von einer Klammer-Gruppe"; der einzige Unterschied ist, ob das Token aus der geschlossenen Basis-Menge
/// stammt oder nicht -- und genau das ist die KATALOG-Frage, die hier ausdruecklich nicht gestellt wird
/// (s. S2-KATALOG-ANDOCKSTELLE). Zwei Produktionen mit identischer Form waeren zwei Wahrheiten ueber
/// dieselbe Syntax.
///
/// Die Sentinel-Faelle dieser Funktion, jeder als Grammatik-Regel und nicht als Sonderfall:
///   * leeres Token           -> "1.0.0.{p}" (Klammer ohne Basis), "1.0.0.c{.p}" (fuehrender Punkt hinter
///                               '{', R4), "1.0.0.c{}" (leere Gruppe, R4), "1.0.0.c{p..e}" (Doppelpunkt)
///   * fehlendes '}'          -> "1.0.0.c{p" (unbalancierte Klammer)
///   * Tiefe > kMaxFlagDepth  -> der benannte Deckel
///   * Knoten-/Token-Deckel   -> FlagList::append liefert false
[[nodiscard]] constexpr bool take_flag_node(std::string_view& s, FlagList& out, std::uint8_t depth) noexcept {
    if (depth > kMaxFlagDepth) return false;
    std::string_view const tok = take_flag_token(s);
    if (tok.empty()) return false;
    if (!out.append(tok, depth)) return false;
    if (s.empty() || s.front() != '{') return true; // R3: die Klammer haengt DIREKT an der Basis, ohne Punkt
    s.remove_prefix(1);
    for (;;) {
        if (!take_flag_node(s, out, static_cast<std::uint8_t>(depth + 1))) return false;
        if (!s.empty() && s.front() == '.') {
            s.remove_prefix(1);
            continue;
        }
        break;
    }
    if (s.empty() || s.front() != '}') return false;
    s.remove_prefix(1);
    return true;
}

/// K-5: baut das Ergebnis-Tripel und faltet JEDES Null-Tripel auf den reinen Sentinel AlgoSemVer{} zurueck --
/// "0.0.0.c" ist der Sentinel, nicht ein "CPU-Sentinel".
[[nodiscard]] constexpr AlgoSemVer make_semver(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                               FlagList const& flags) noexcept {
    if (x == 0u && y == 0u && z == 0u) return AlgoSemVer{};
    return AlgoSemVer{x, y, z, flags};
}

/// Der FLAG-SCHWANZ hinter dem X.Y.Z-Tripel -- die EINE Stelle, an der R2 und R6 durchgesetzt werden.
/// R2 (Punkt vor JEDEM Flag) ist hier genau eine Zeile: jeder Durchlauf verlangt zuerst den '.'. Dass auch
/// das ERSTE Flag ihn braucht, ist damit keine Sonderregel, sondern der Normalfall -- genau die Symmetrie,
/// die Owner-E4 Frage 1 wollte ("dann ist jedes Segment gleich behandelt").
/// Jeder unverbrauchte Rest (z.B. ein ueberzaehliges '}', ein Grossbuchstabe, ein Unterstrich) faellt hier
/// auf den Sentinel, weil er die '.'-Bedingung nicht erfuellt.
[[nodiscard]] constexpr AlgoSemVer take_flag_tail(std::string_view& s, std::uint32_t x, std::uint32_t y,
                                                  std::uint32_t z) noexcept {
    FlagList flags{};
    while (!s.empty()) {
        if (s.front() != '.') return {}; // R2
        s.remove_prefix(1);
        if (!take_flag_node(s, flags, 0)) return {};
    }
    return make_semver(x, y, z, flags);
}
} // namespace detail

/// Der EINE dokumentierte SENTINEL-WORTLAUT ("kein bekannter Stand"). Er ist dreistellig und flaglos und
/// die EINZIGE Sentinel-Schreibweise, die eine Registry-Wache als ABSICHT durchgehen laesst -- jedes ANDERE
/// Literal, das auf den Sentinel parst, ist ein Tippfehler und muss compile-time brechen (B12).
/// FLAG-GRAMMATIK v2: das frueher fuehrende 'v' ist entfallen (R1). Damit faellt dieser Wortlaut mit dem
/// GERENDERTEN Sentinel zusammen -- rohe und gerenderte Form sind dieselbe Zeichenfolge, und
/// abi/anatomy_stamp_entries.hpp bezieht seinen Sentinel-Wortlaut von HIER statt ein zweites Literal zu
/// fuehren.
inline constexpr std::string_view kAlgoSemVerSentinelLiteral = "0.0.0";

/// Parst die Version der FLAG-GRAMMATIK v2: "X.Y.Z" plus null bis n punkt-getrennte Flags mit optionalen,
/// rekursiven Klammer-Gruppen. Alles andere -> {0,0,0}-Sentinel: leer, MIT 'v' (die Alt-Form, R1), die
/// Kurzform "X"/"X.Y", ein Flag ohne fuehrenden Punkt (R2), ein fuehrender Punkt hinter '{' (R4), eine
/// leere Gruppe, eine unbalancierte Klammer, Grossbuchstaben, Unterstriche, jeder sonstige Rest.
/// Das Null-Tripel bleibt IMMER der reine Sentinel (K-5). Rein constexpr.
///
/// ES GIBT NUR DIESEN EINEN PARSER. parse_dotted_semver (die frueher getrennte gerenderte Form) ist mit dem
/// Wegfall des 'v'-Praefixes ersatzlos entfallen -- s. "EIN PARSER, NICHT ZWEI" im Datei-Kopf.
[[nodiscard]] constexpr AlgoSemVer parse_algo_semver(std::string_view v) noexcept {
    auto const [x, okx] = detail::take_uint(v);
    if (!okx) return {};
    // KURZFORM-RUECKBAU (Owner-Q3 "Die Kurzform ist verboten ... immer 3-Stellig"): nach der ersten Zahl
    // MUSS ein '.' folgen. "1"/"1.c" enden damit im Sentinel statt in {1,0,0}.
    if (v.empty() || v.front() != '.') return {};
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "X.Y" verboten (auch "X.Y.c")
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz) return {};
    return detail::take_flag_tail(v, x, y, z);
}

// == DER RENDERER =================================================================================

/// Obergrenze des gerenderten FLAG-SCHWANZES, AUS DEN DECKELN ABGELEITET statt geraten: je Knoten
/// hoechstens 1 Trennzeichen ('.' oder '{') + das Token + 1 schliessende Klammer, plus die am Ende noch
/// offenen Klammern (hoechstens kMaxFlagDepth). Bewusst eine OBERgrenze -- sie muss nicht scharf sein,
/// sie muss halten.
inline constexpr std::size_t kMaxRenderedFlagTailLen = kMaxFlagNodes * (kMaxFlagTokenLen + 2u) + kMaxFlagDepth;

/// Obergrenze der gerenderten Voll-Form: 3 Komponenten a hoechstens kMaxSemVerComponentDigits Ziffern +
/// 2 Punkte + der Flag-Schwanz.
inline constexpr std::size_t kMaxRenderedAlgoSemVerLen = 3u * kMaxSemVerComponentDigits + 2u + kMaxRenderedFlagTailLen;

/// Das Ergebnis des Flag-Schwanz-Renderers: fester Puffer + Laenge, EINSCHLIESSLICH des fuehrenden Punktes
/// (leer, wenn die Version keine Flags traegt).
/// WOZU ER EINZELN EXISTIERT: die POD-Kodierung in abi/anatomy_stamp_entries.hpp hasht nach Owner-F-3 die
/// NORMALISIERTE FLAG-ZEICHENKETTE. Sie muss genau die Zeichen sehen, die der Renderer emittiert -- wuerde
/// sie den Schwanz selbst zusammensetzen oder aus der Voll-Form herausschneiden, gaebe es eine zweite
/// Wahrheit ueber die kanonische Schreibweise.
struct RenderedFlagTail {
    std::array<char, kMaxRenderedFlagTailLen + 1u> text{};
    std::size_t                                    len = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view{text.data(), len}; }
};

/// Rendert den Flag-Schwanz einer Flag-Liste (mit fuehrendem Punkt).
///
/// DIE UMKEHRUNG IST VERLUSTFREI, und zwar aus der Pre-Order-Darstellung heraus:
///   erster Knoten -> '.'  (R2: der Punkt steht auch vor dem ERSTEN Flag)
///   Tiefe steigt  -> '{'  (R3: ohne Punkt zwischen Basis und Klammer)
///   Tiefe gleich  -> '.'
///   Tiefe faellt  -> so viele '}' wie Ebenen, danach '.'
///   Ende          -> die noch offenen '}' schliessen
[[nodiscard]] constexpr RenderedFlagTail render_flag_tail(FlagList const& flags) noexcept {
    RenderedFlagTail out{};
    auto const       put          = [&out](char c) constexpr noexcept { out.text[out.len++] = c; };
    std::uint8_t     vorige_tiefe = 0;
    for (std::size_t i = 0; i < flags.count; ++i) {
        FlagToken const& n = flags.nodes[i];
        if (i == 0u) {
            put('.');
        } else if (n.depth > vorige_tiefe) {
            put('{');
        } else if (n.depth == vorige_tiefe) {
            put('.');
        } else {
            for (std::uint8_t k = n.depth; k < vorige_tiefe; ++k) put('}');
            put('.');
        }
        for (std::size_t k = 0; k < n.len; ++k) put(n.text[k]);
        vorige_tiefe = n.depth;
    }
    for (std::uint8_t k = 0; k < vorige_tiefe; ++k) put('}');
    out.text[out.len] = '\0';
    return out;
}

/// Das Ergebnis des Voll-Renderers: fester Puffer + Laenge. KEIN std::string -- der consteval-Zwilling in
/// builder/ceb_version_stamp.hpp rendert damit dieselbe Zeichenfolge wie der Laufzeit-Weg, ohne einen
/// zweiten Renderer und ohne eine zweite Laengen-Rechnung (das war vor der v2 die Drift-Naht).
struct RenderedAlgoSemVer {
    std::array<char, kMaxRenderedAlgoSemVerLen + 1u> text{};
    std::size_t                                      len = 0;

    [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view{text.data(), len}; }
};

/// Rendert eine AlgoSemVer in die kanonische Form "X.Y.Z[.flag]*".
/// Der Sentinel rendert immer nackt "0.0.0" (er traegt nie Flags, K-5).
[[nodiscard]] constexpr RenderedAlgoSemVer render_algo_semver(AlgoSemVer const& v) noexcept {
    RenderedAlgoSemVer out{};
    auto const         put      = [&out](char c) constexpr noexcept { out.text[out.len++] = c; };
    auto const         put_uint = [&put](std::uint32_t n) constexpr noexcept {
        char        tmp[kMaxSemVerComponentDigits + 4u]{};
        std::size_t d = 0;
        do {
            tmp[d++] = static_cast<char>('0' + (n % 10u));
            n /= 10u;
        } while (n != 0u);
        for (std::size_t k = 0; k < d; ++k) put(tmp[d - 1u - k]);
    };
    put_uint(v.x);
    put('.');
    put_uint(v.y);
    put('.');
    put_uint(v.z);
    // Der Puffer wird BENANNT und nicht als Temporary durchgereicht: .view() zeigt in ihn hinein. Die
    // Lebenszeit-Verlaengerung im range-for gaebe es zwar seit C++23 (P2718R0), aber sie haengt am
    // Compiler-Stand -- und dieser Header uebersetzt in der GCC-, der clang- UND der MSVC-Bahn.
    RenderedFlagTail const schwanz = render_flag_tail(v.flags);
    for (char const c : schwanz.view()) put(c);
    out.text[out.len] = '\0';
    return out;
}

/// Die kanonische Form als std::string -- NUR fuer neue Stempel-Zeilen/Planer/Registry (NIE die .algos-Sig,
/// die serialisiert das ROHE Literal verbatim). Reine Bequemlichkeit ueber render_algo_semver.
/// FLAG-GRAMMATIK v2: da das 'v'-Praefix entfallen ist, ist diese Funktion fuer wohlgeformte Eingaben die
/// IDENTITAET auf der Zeichenfolge -- das ist keine Redundanz, sondern der Beweis, dass rohe und gerenderte
/// Form zusammengefallen sind (Roundtrip-Batterie unten).
[[nodiscard]] inline std::string algo_semver_string(std::string_view algo_version) {
    return std::string{render_algo_semver(parse_algo_semver(algo_version)).view()};
}

// == S2-KATALOG-ANDOCKSTELLE (GEBAUT 07.08.2026) ==================================================
//
// GREP-ANKER: S2-KATALOG-ANDOCKSTELLE
//
// WAS HIER STEHT: die Katalog-Wache -- die Pruefung, ob ein Token ueberhaupt ein zulaessiges Flag ist
// UND ob es an SEINER Stelle stehen darf ('p'/'e' nur unter 'c', 'sse2' nur unter 'x128'). Der Parser
// prueft die FORM, diese Wache den KATALOG.
//
// WARUM SIE ERST JETZT KAM: eine Wache gegen einen GERATENEN Katalog waere schlimmer als keine gewesen --
// sie haette gueltige Flags abgelehnt und dabei ausgesehen wie eine Zusage. Und eine Attrappe, die heute
// `true` liefert, aber so heisst, als pruefte sie etwas, ist die gefaehrlichere Form von "nicht gebaut".
// Der Vorgaenger hat deshalb ausdruecklich KEINE gebaut, sondern nur die Andockstelle benannt.
//
// DAS MATERIAL LAG SEIT DEM 07.08.2026 VOR (SIMD-Recherche, Owner-Auftrag F-5/F-6, 6 Lenses, 108 Token).
// Der Katalog selbst steht in measurement/flag_grammar_catalog.hpp -- in der Nachbarschaft von
// measurement/simd_feature_flag.hpp, dessen Muster (cpuinfo-Id und Compiler-Schalter als EIGENE
// Tabellenfelder, nie per String-Heuristik ineinander umgerechnet) er um den DRITTEN Namensraum
// fortschreibt. 62 zugelassene Eintraege, 14 begruendet abgelehnte (Reserve), beides compile-time.
//
// -- DIE DREI NAMENSRAEUME (der wichtigste Befund fuer S2) -----------------------------------------
// Ein SIMD-Flag hat DREI verschiedene Namen, und keine zwei sind durcheinander ableitbar:
//     Faehigkeit          cpuinfo-Id (Kernel)     Compiler-Schalter      UNSER Grammatik-Token
//     SSE3                pni                     -msse3                 sse3
//     SSE4.1              sse4_1                  -msse4.1               sse41
//     AVX-512 VBMI2       avx512_vbmi2            -mavx512vbmi2          vbmi2   (unter x512)
//     PCLMULQDQ           pclmulqdq               -mpclmul               pclmulqdq
//     3DNow!-Prefetch     3dnowprefetch           -mprfchw               3dnowprefetch
// Die ersten beiden Spalten fuehrt measurement/simd_feature_flag.hpp bereits GETRENNT und ausdruecklich
// "NIE per String-Heuristik ineinander umgerechnet" (dort der Kopf-Kommentar zur Unterstrich-Falle). S2
// bleibt bei DIESEM Muster und erfindet kein zweites: der Katalog-Eintrag bekommt ein drittes Feld
// (Grammatik-Token) neben cpuinfo und gpp -- er leitet es NICHT ab.
//
// WELCHEN NAMENSRAUM DIESE GRAMMATIK FUEHRT: den DRITTEN, und zwar zwingend.
//   * Der Compiler-Schalter scheidet aus: "-msse4.1" traegt einen PUNKT, und der Punkt IST unser Trenner.
//   * Die cpuinfo-Id scheidet aus: "avx512_vbmi2" traegt einen UNTERSTRICH, und Owner-F-7 sagt verbatim
//     "Ja wir verwenden NUR den Punkt".
// Unsere Token sind also AUSDRUECKLICH NICHT die Compiler-Schalter und NICHT die Kernel-Namen, sondern
// eine eigene, punkt- und unterstrichfreie Schreibweise. Das ist keine Bequemlichkeit, sondern die
// einzige Form, die in einer punkt-getrennten Grammatik eindeutig bleibt. Wer eine Abbildung braucht,
// baut sie als TABELLE (Muster simd_feature_flag.hpp), nie als String-Umformung.
//
// -- DIE VIER STRUKTURFAELLE, die der Katalog verlangt (alle vier traegt die Form, s. Beweise unten) --
//   (1) BASIS MIT SUB-LISTE:        x512{f.vl.bw.dq}      -- Basis + Klammer
//   (2) COMPANION OHNE BASIS:       gfni                  -- Breite FOLGT der Basis, eigenes CPUID-Bit
//   (3) SKALAR OHNE JEDE BREITE:    popcnt, bmi2, abm     -- GPR-Instruktionen, nie in einer Klammer
//   (4) EIGENE BASIS, ANDERE DATEI: m64{mmx.mmxext.3dnow.3dnowext} -- MM0-MM7, x87-aliasiert; eine
//       ANDERE Registerdatei als x128/x256/x512, deshalb eine EIGENE Basis (MedienBasis) statt einer
//       vierten Breite derselben Achse.
// (2) und (3) sehen in der FORM gleich aus -- ein Token auf Tiefe 0 ohne Klammer -- und sind trotzdem
// zwei verschiedene Sachen. Genau das ist der Grund, warum die Unterscheidung in den KATALOG gehoert
// und nicht in den Parser: die Form kann sie nicht sehen, und sie muss es auch nicht. (4) sah bis zum
// 07.08.2026 ebenso aus (blosses Token); der Parser HAELT diese alte Form weiterhin fuer wohlgeformt
// (er kennt keinen Katalog), aber die Katalog-Wache laesst sie seit dem Entscheid NICHT mehr durch --
// s. (m4) unten fuer den Beweis, dass die alte Schreibweise jetzt LAUT bricht.
//
// FALL (4) IST ENTSCHIEDEN (07.08.2026): die MMX-/3DNow!-Familie bekommt die eigene Basis `m64`
// ("1.0.0.c.m64{mmx.mmxext.3dnow}"), NICHT `x64` (Namens-Entscheid, Begruendung in
// flag_grammar_catalog.hpp am Kopf) und NICHT mehr das blosse Tiefe-0-Token. Primaerquelle: AMD APM
// Vol. 3, Anhang D, SS D.1, Punkt 4 -- MMX und 3DNow! sind EINE Instruktionsgruppe auf EINEM
// Registersatz. Die vollstaendige Recherche (Primaerquellen im Volltext, Gegenrede eingeschlossen)
// steht in docs/plaene/20260807-RECHERCHE-mmx-3dnow-basis-oder-token.md. Der `entscheid_offen`-Zaehler
// in flag_grammar_catalog.hpp steht jetzt auf NULL (vorher sechs); wer ihn wieder anhebt, eroeffnet
// einen NEUEN Entscheid und muss ihn begruenden.
//
// WO DIE WACHE ANDOCKT -- drei Stellen, alle gebaut:
//   (1) `flag_catalog_is_satisfied(AlgoSemVer const&)` unmittelbar unter dieser Andockstelle. Sie laeuft
//       ueber for_each_flag_node() und prueft je Knoten Token UND Elternteil.
//   (2) ce_owned_version_is_wellformed() traegt sie als weiteren Konjunktions-Term (ungated).
//   (3) Die CT-Batterie am Dateiende hat den Abschnitt (m) mit den Katalog-Fehlformen.
// Die STRUKTUR dafuer war fertig: for_each_flag_node() liefert Token, Tiefe und ELTERN-Token je Knoten --
// die Wache tokenisiert die Zeile nicht erneut, und sie stellt die Eltern-Bedingung ('p' nur unter 'c')
// ohne einen zweiten Parser.

/// Der Index des ELTERN-Knotens von nodes[i] -- oder kNoFlagParent, wenn nodes[i] auf Tiefe 0 steht.
/// Aus der Pre-Order-Darstellung ABGELEITET (der naechste Knoten links mit Tiefe depth-1), nicht gespeichert:
/// ein zusaetzliches Eltern-Feld waere eine zweite Wahrheit ueber dieselbe Information und koennte driften.
inline constexpr std::size_t kNoFlagParent = static_cast<std::size_t>(-1);

[[nodiscard]] constexpr std::size_t parent_flag_node_index(FlagList const& f, std::size_t i) noexcept {
    if (i >= f.count || f.nodes[i].depth == 0u) return kNoFlagParent;
    std::uint8_t const eltern_tiefe = static_cast<std::uint8_t>(f.nodes[i].depth - 1u);
    for (std::size_t k = i; k-- > 0;)
        if (f.nodes[k].depth == eltern_tiefe) return k;
    return kNoFlagParent; // unerreichbar fuer eine geparste Liste (Tiefe 0 steht immer vorne)
}

/// Laeuft die Flag-Liste in TEXT-Reihenfolge ab und ruft fn(token, tiefe, eltern_token) je Knoten; das
/// Eltern-Token ist leer, wenn der Knoten auf Tiefe 0 steht. Die Traversal-Primitive der
/// S2-KATALOG-ANDOCKSTELLE -- und zugleich die Form, in der jede kuenftige Auswertung ueber die Flags
/// laufen soll, statt die Zeichenfolge ein zweites Mal zu zerlegen.
template <class Fn>
constexpr void for_each_flag_node(AlgoSemVer const& v, Fn&& fn) {
    for (std::size_t i = 0; i < v.flags.count; ++i) {
        std::size_t const      p      = parent_flag_node_index(v.flags, i);
        std::string_view const eltern = (p == kNoFlagParent) ? std::string_view{} : v.flags.nodes[p].view();
        fn(v.flags.nodes[i].view(), v.flags.nodes[i].depth, eltern);
    }
}

/// DIE KATALOG-WACHE. Steht JEDES Flag-Token der Version im Katalog, und steht es UNTER SEINER BASIS?
///
/// Sie ist der zweite Halbsatz der Grammatik: der Parser sagt "die Zeichen sind richtig gesetzt", diese
/// Wache sagt "die Token bedeuten etwas, und sie bedeuten es an dieser Stelle". Ohne sie ist
/// "1.0.0.x512{quatsch}" ein gueltiger Stempel, und ein Stempel ist Identitaet.
///
/// WARUM DAS ELTERN-TOKEN UND NICHT NUR DAS TOKEN -- der Katalog belegt es an drei Stellen selbst:
///   * 'sse2' IST ein Katalog-Token, gehoert aber unter 'x128'. "x512{sse2}" waere eine Behauptung ueber
///     eine 512-bit-Form, die es unter dem sse2-Bit nicht gibt.
///   * 'vnni' und 'ifma' existieren je ZWEIMAL -- als avx_vnni/avx_ifma unter x256 und als
///     avx512_vnni/avx512ifma unter x512. Verschiedene CPUID-Bits, verschiedene Compiler-Schalter. Ohne
///     das Elternteil waeren sie nicht unterscheidbar, und der Stempel bezoege sich auf zwei Sachen.
///   * 'f' ist unter 'x512' das AVX-512-Foundation-Subset und auf Tiefe 0 die Ziel-Hardware FPGA.
/// Die TIEFE braucht die Wache dagegen NICHT: die Eltern-Kette begrenzt sich selbst. Um ein Token auf
/// Tiefe 2 zu erreichen, muss sein Elternteil auf Tiefe 1 zugelassen sein -- und jedes Tiefe-1-Token des
/// Katalogs hat als Elternteil eine Basis, die ihrerseits nur auf Tiefe 0 steht. "c{c{p}}" faellt damit
/// schon am inneren 'c' (s. Batterie (m)).
///
/// KEIN FRUEHER AUSSTIEG: for_each_flag_node laeuft die Liste vollstaendig ab. Bei hoechstens
/// kMaxFlagNodes == 96 Knoten ist das im constexpr-Pfad kostenlos, und es haelt die Wache an der EINEN
/// Traversal-Primitive statt eine zweite Schleifenform daneben zu stellen.
[[nodiscard]] constexpr bool flag_catalog_is_satisfied(AlgoSemVer const& v) noexcept {
    bool erfuellt = true;
    for_each_flag_node(
        v, [&erfuellt](std::string_view token, std::uint8_t /*tiefe*/, std::string_view eltern) constexpr noexcept {
            if (!flag_token_is_admitted_under(token, eltern)) erfuellt = false;
        });
    return erfuellt;
}

/// Dieselbe Frage an ein ROHES Literal -- fuer Aufrufer, die keine AlgoSemVer in der Hand haben.
/// ACHTUNG, die Bedeutung fuer unparsbare Eingaben: die parst auf den Sentinel, und der Sentinel traegt
/// NIE Flags (K-5) -- er erfuellt den Katalog also LEER. Das ist Absicht und keine Luecke: die
/// Parsbarkeit ist die Aufgabe von version_is_parsable_or_documented_sentinel(), und eine Wache, die
/// zwei verschiedene Fehlerklassen in EINEM Verdikt vermischt, sagt am Ende ueber keine von beiden
/// etwas. ce_owned_version_is_wellformed() stellt deshalb beide Fragen nacheinander.
[[nodiscard]] constexpr bool version_flags_are_in_catalog(std::string_view raw) noexcept {
    return flag_catalog_is_satisfied(parse_algo_semver(raw));
}

// == DIE POLITIK-WACHEN (B12) =====================================================================

/// PFLICHT-WACHE (Owner-Q3: "Wir produzieren nur CPU code"; in der v2 als F-10 praezisiert: "ce-eigene
/// Achsen tragen mindestens 'c'"). Praedikat GETRENNT vom Parser: der Parser toleriert die flaglose Form
/// weiter (sonst waeren die static_assert-Batterien dieses Headers define-abhaengig und ein Header haette
/// zwei Bedeutungen); die POLITIK lebt hier und wird hinter COMDARE_VERSION_HW_FLAG_ENFORCE scharf
/// geschaltet. IMMER kompiliert und CT-bewiesen (Batterie unten) -- das Define schaltet nur die ANWENDUNG
/// auf jede Registry-Variante, nie die Existenz der Wache.
///
/// WAS SICH GEGENUEBER DER Q3-FASSUNG GEAENDERT HAT -- und was NICHT:
/// Q3 fragte "das (genau eine) Hardware-Flag IST 'c'". v2 fragt "'c' ist UNTER den Flags" (F-10). Der
/// Unterschied wird erst durch die neue Kardinalitaet (1 -> n) ueberhaupt sichtbar: fuer JEDE Form, die
/// unter Q3 existieren konnte, liefern beide Fassungen dasselbe Verdikt -- "1.0.0.g" faellt hier wie dort
/// durch, "1.0.0.c" besteht hier wie dort. NEU entscheidbar sind nur Formen, die es unter Q3 gar nicht
/// gab, etwa "1.0.0.c.g" (CPU UND GPU): die besteht jetzt. Das ist keine Lockerung an einer bestehenden
/// Form, sondern eine Aussage ueber eine neue.
[[nodiscard]] constexpr bool version_satisfies_cpu_only_policy(std::string_view raw) noexcept {
    AlgoSemVer const v = parse_algo_semver(raw);
    return !v.is_sentinel() && v.has_top_level_flag("c");
}

/// B12 (a): PARSBAR-PFLICHT. Ein Literal, das auf den Sentinel parst, ist entweder ABSICHT (dann steht es
/// exakt als kAlgoSemVerSentinelLiteral da) oder ein Tippfehler. Ohne diese Wache fiel jedes junk-Literal
/// ("1.0", "v1.0.0", "X", "") still auf @0.0.0 -- die Registry haette eine Version behauptet und der
/// Stempel haette einen Nicht-Stand gerendert, ohne dass irgendetwas bricht.
[[nodiscard]] constexpr bool version_is_parsable_or_documented_sentinel(std::string_view raw) noexcept {
    return !parse_algo_semver(raw).is_sentinel() || raw == kAlgoSemVerSentinelLiteral;
}

/// B12: die VOLLE Wohlgeformtheits-Politik einer ce-EIGENEN Version -- die EINE Stelle, an der die UNGATED
/// Pflichten zusammenstehen, damit sie nicht je Registry einzeln (und luecken-verschieden) nachgebaut
/// werden. Genutzt von der Organ-Registry (axis_variant_version_table.hpp) und den Nicht-Organ-Registries
/// (System-Achsen-Code, Mess-Tooling, Mess-Framework, Toolchain, Pruefdock):
///   (a) PARSBAR (oder exakt der dokumentierte Sentinel)      -- kein stiller @0.0.0-Fall,
///   (b) wenn ueberhaupt ein Flag da ist, dann ist 'c' darunter (Owner-F-10),
///   (c) JEDES Flag-Token steht im KATALOG und unter SEINER Basis (S2, 07.08.2026).
/// Die Owner-PFLICHT "ein Flag MUSS da sein" ist bewusst NICHT hier, sondern im gated Zwilling unten -- so
/// bleibt dieser ungated Teil auch fuer Alt-/Fremd-Literale nutzbar.
///
/// WARUM (c) UNGATED IST UND NICHT HINTER COMDARE_VERSION_HW_FLAG_ENFORCE: das Define schaltet eine
/// POLITIK ("ce-eigene Achsen MUESSEN 'c' tragen"), also eine Aussage darueber, WAS deklariert sein muss.
/// Der Katalog ist keine Politik, sondern eine Aussage darueber, ob das Deklarierte ueberhaupt existiert.
/// "1.0.0.c{quatsch}" ist unter JEDER Politik falsch. Eine Wache dafuer hinter ein Define zu haengen,
/// hiesse einem Header zwei Bedeutungen zu geben -- genau das, wogegen die Zweiteilung hier gebaut ist.
///
/// ENTFALLEN GEGENUEBER DER Q3-FASSUNG: der frueher dritte Term "NIE experimentell". Er hat mit der v2
/// keinen Gegenstand mehr -- 'e' bedeutet EFFICIENCY CORE und ist ein legitimes Flag (R7). Die alte
/// B12-Wache "ce-Registry traegt NIE 'e'" ist damit nicht abgeschwaecht, sondern GEGENSTANDSLOS; ihre
/// Aufgabe uebernimmt die Pflicht "traegt mindestens 'c'" (Owner F-10 verbatim: "Der vorschlag trifft ins
/// Schwarze. Genau so.").
[[nodiscard]] constexpr bool ce_owned_version_is_wellformed(std::string_view raw) noexcept {
    if (!version_is_parsable_or_documented_sentinel(raw)) return false;
    AlgoSemVer const v = parse_algo_semver(raw);
    if (v.has_flags() && !v.has_top_level_flag("c")) return false;
    return flag_catalog_is_satisfied(v);
}

/// B12 (c): der GATED Zwilling (COMDARE_VERSION_HW_FLAG_ENFORCE). Er verlangt das CPU-Flag, waehrend der
/// ungated Teil oben es nur PRUEFT, falls eines da ist. Die Zweiteilung ist Absicht: nur so bleibt der
/// ungated Teil fuer Alt-/Fremd-Literale nutzbar, ohne dass ein Header zwei Bedeutungen bekommt.
[[nodiscard]] constexpr bool ce_owned_version_satisfies_cpu_enforce(std::string_view raw) noexcept {
    return version_satisfies_cpu_only_policy(raw);
}

// == COMPILE-ZEIT-BATTERIE ========================================================================
// Sie ist der Beweis, nicht die Behauptung. Aufbau: (a) das Zahlen-Tripel, (b) die acht Regeln R1..R8 je
// EINZELN, (c) die Sentinel-Bedingungen je EINZELN, (d) Roundtrip, (e) die Politik-Wachen.

// -- (a) Das Zahlen-Tripel (unveraendert aus der Q3-Welt) ------------------------------------------
static_assert(parse_algo_semver("1.0.0") == AlgoSemVer{1, 0, 0});
static_assert(parse_algo_semver("0.0.0") == AlgoSemVer{0, 0, 0}); // Sentinel
static_assert(parse_algo_semver("2.3.4") == AlgoSemVer{2, 3, 4});
static_assert(parse_algo_semver("10.0.1") == AlgoSemVer{10, 0, 1});
static_assert(parse_algo_semver("1.2") == AlgoSemVer{}); // Kurzform verboten
static_assert(parse_algo_semver("1") == AlgoSemVer{});   // Kurzform verboten
static_assert(parse_algo_semver("") == AlgoSemVer{});    // leer
static_assert(parse_algo_semver("x") == AlgoSemVer{});   // nicht-numerisch
static_assert(!parse_algo_semver("1.0.0").is_sentinel());
static_assert(parse_algo_semver("0.0.0").is_sentinel());
// ALIAS-WACHE der Token-Regel: eine vierstellige Version ist NICHT zugleich eine dreistellige mit einem
// Ziffern-Flag. Ein reines Ziffern-Token gibt es nicht (s. take_flag_token).
static_assert(parse_algo_semver("1.2.3.4") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0.512") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0.0") == AlgoSemVer{});
static_assert(!parse_algo_semver("1.0.0.x512").is_sentinel()); // ... mit Buchstabe darin sehr wohl
static_assert(!parse_algo_semver("1.0.0.f16c").is_sentinel());

// -- B11: UEBERLAUF-/LEADING-ZERO-WACHE der Komponenten (Codex-Review 02.08.2026) -------------------
// Die Wache existiert gegen ALIAS-IDENTITAETEN: zwei roh verschiedene Literale duerfen nie dasselbe
// Stempel-Segment erzeugen (der .algos-Sidecar-Pfad serialisiert die rohen Literale VERBATIM).
static_assert(parse_algo_semver("4294967297.0.0.c") == AlgoSemVer{});                    // Modulo-2^32-Umlauf
static_assert(parse_algo_semver("4294967296.0.0") == AlgoSemVer{});                      // 2^32 selbst
static_assert(parse_algo_semver("1.0.4294967297") == AlgoSemVer{});                      // auch in der Z-Komponente
static_assert(!(parse_algo_semver("4294967297.0.0.c") == parse_algo_semver("1.0.0.c"))); // kein Alias mehr
static_assert(!parse_algo_semver("999999.0.0.c").is_sentinel());                         // 6 Ziffern tragen
static_assert(parse_algo_semver("1000000.0.0.c") == AlgoSemVer{});                       // 7 Ziffern nicht
static_assert(parse_algo_semver("1.1000000.0") == AlgoSemVer{});
static_assert(parse_algo_semver("0.0.1000000") == AlgoSemVer{});
static_assert(kMaxSemVerComponentDigits == 6);
static_assert(parse_algo_semver("01.0.0.c") == AlgoSemVer{}); // fuehrende Null
static_assert(parse_algo_semver("1.00.0") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.007") == AlgoSemVer{});
static_assert(parse_algo_semver("0.1.0") == AlgoSemVer{0, 1, 0}); // "0" als ECHTE Komponente
static_assert(parse_algo_semver("0.0.1") == AlgoSemVer{0, 0, 1});

namespace detail {
/// Test-Helfer der Batterie: baut eine erwartete FlagList aus (token, tiefe)-Paaren. Sie steht in detail,
/// weil sie NUR die Beweise unten bedient -- der Parser selbst benutzt sie nicht.
template <std::size_t N>
[[nodiscard]] constexpr FlagList flag_list_of(std::pair<std::string_view, std::uint8_t> const (&paare)[N]) noexcept {
    FlagList f{};
    for (std::size_t i = 0; i < N; ++i) (void)f.append(paare[i].first, paare[i].second);
    return f;
}
} // namespace detail

// -- (b) DIE ACHT REGELN, JEDE EINZELN ------------------------------------------------------------

// R1: KEIN 'v'-PRAEFIX MEHR. Die Alt-Form ist Sentinel -- keine Uebergangs-Toleranz.
//
//     OWNER-DIREKTIVE 07.08.2026, verbatim: "Wir WOLLEN den gesamten Bestand invalidieren, weil uns das
//     spaeter das Leben erleichtert. Die Entscheidung fuer den Umbau steht."
//     Das ist die Begruendung dafuer, dass hier KEINE Doppel-Akzeptanz steht. Daraus folgt aber auch die
//     Pflicht, unter der dieser Header seither gebaut ist: der Bruch muss LAUT sein. Ein Alt-Literal darf
//     nie still zum Sentinel werden und weiterlaufen -- es muss an einer Wache compile-time anschlagen.
//     Die Wachen, die das leisten, sind ce_owned_version_is_wellformed / _satisfies_cpu_enforce (hier),
//     measurement::axis_version_entries_are_wellformed (an den Stempel-STELLEN) und
//     abi::dotted_version_is_wellformed (auf der Ruecklese-Seite).
//
//     WAS EIN LESER DES ALT-LITERALS WISSEN MUSS -- die Bedeutung von 'e' HAT GEWECHSELT:
//       Q3 (bis 06.08.2026):  "v1.0.0ce" == CPU + EXPERIMENTELL (Pruefling-Markierung, Owner-E2)
//       v2  (ab 07.08.2026):  'e' == EFFICIENCY CORE, ein Hardware-Filter wie jeder andere
//     Ein Alt-Literal "v1.0.0ce" bedeutet also NICHT "1.0.0.c{e}". Wer es migriert, uebersetzt es auf
//     "1.0.0.c" -- das 'e' faellt weg, weil das Konzept dahinter ersatzlos deprecated ist. Im ce-Bestand
//     ist dieser Fall nie aufgetreten (Haeufigkeit NULL, per grep erhoben); die Regel steht hier fuer
//     Fremd-Literale und fuer die Lesbarkeit der git-Historie.
static_assert(parse_algo_semver("v1.0.0") == AlgoSemVer{});
static_assert(parse_algo_semver("v1.0.0c") == AlgoSemVer{});  // die alte Q3-Bestandsform
static_assert(parse_algo_semver("v1.0.0ce") == AlgoSemVer{}); // die alte 'ce'-Form (CPU + experimentell)
static_assert(parse_algo_semver("v0.0.0") == AlgoSemVer{});   // auch der alte Sentinel-Wortlaut
static_assert(parse_algo_semver("v1.0.0.c") == AlgoSemVer{}); // 'v' + neue Notation ist ebenfalls falsch
static_assert(kAlgoSemVerSentinelLiteral == "0.0.0");

// R2: PUNKT VOR JEDEM FLAG, auch dem ersten.
static_assert(!parse_algo_semver("1.0.0.c").is_sentinel()); // mit Punkt: gueltig
static_assert(parse_algo_semver("1.0.0c") == AlgoSemVer{}); // ohne Punkt: Sentinel (die Q3-Schreibweise)
static_assert(parse_algo_semver("1.0.0.c.p") != AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0.cp") != parse_algo_semver("1.0.0.c.p")); // 'cp' ist EIN Token
static_assert(parse_algo_semver("1.0.0..c") == AlgoSemVer{});                   // doppelter Punkt
static_assert(parse_algo_semver("1.0.0.") == AlgoSemVer{});                     // Punkt ohne Flag

// R3: BASIS DIREKT AN IHRER KLAMMER, ohne Punkt dazwischen.
static_assert(!parse_algo_semver("1.0.0.c{p.e}").is_sentinel());
static_assert(parse_algo_semver("1.0.0.c.{p.e}") == AlgoSemVer{}); // Punkt vor der Klammer: Sentinel
static_assert(parse_algo_semver("1.0.0.{p.e}") == AlgoSemVer{});   // Klammer ohne Basis: Sentinel

// R4: HINTER '{' NIE EIN FUEHRENDER PUNKT -- und nie eine leere Gruppe.
static_assert(parse_algo_semver("1.0.0.c{.p.e}") == AlgoSemVer{}); // fuehrender Punkt
static_assert(parse_algo_semver("1.0.0.c{}") == AlgoSemVer{});     // leere Gruppe
static_assert(parse_algo_semver("1.0.0.c{p..e}") == AlgoSemVer{}); // Doppelpunkt INNEN
static_assert(parse_algo_semver("1.0.0.c{p.}") == AlgoSemVer{});   // Punkt vor der schliessenden Klammer

// R5: NUR DER PUNKT TRENNT -- Unterstriche sind kein Trenner und kein Token-Zeichen.
static_assert(parse_algo_semver("1.0.0.avx512_vbmi2") == AlgoSemVer{}); // die Katalog-Schreibweise ist ungueltig
static_assert(!parse_algo_semver("1.0.0.x512{vbmi2}").is_sentinel());   // ... so heisst sie in der Notation
static_assert(parse_algo_semver("1.0.0.c_p") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0.c-p") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0,c") == AlgoSemVer{});

// R6: KARDINALITAET 1 -> n, und die Reihenfolge bleibt formal erhalten (der Parser sortiert NICHT).
static_assert(parse_algo_semver("1.0.0.c.g.f.n").flags.count == 4);
static_assert(parse_algo_semver("1.0.0.c.g").flags.nodes[0].view() == "c");
static_assert(parse_algo_semver("1.0.0.c.g").flags.nodes[1].view() == "g");
static_assert(!(parse_algo_semver("1.0.0.c.g") == parse_algo_semver("1.0.0.g.c"))); // Ordnung ist Identitaet
static_assert(parse_algo_semver("1.0.0.c.g").has_top_level_flag("c"));
static_assert(parse_algo_semver("1.0.0.c.g").has_top_level_flag("g"));
static_assert(!parse_algo_semver("1.0.0.c.g").has_top_level_flag("f"));

// R7: 'e' IST EFFICIENCY CORE -- ein Flag wie jedes andere. Es gibt KEIN experimental-Konzept mehr.
//     Beweis-Form: 'e' ist nur noch als Sub unter 'c' bzw. als Token ueberhaupt sichtbar, und es gibt
//     KEINE Frage mehr, die den Struct danach fragen koennte (das Feld existiert nicht).
static_assert(parse_algo_semver("1.0.0.c{e}").flags.count == 2);
static_assert(parse_algo_semver("1.0.0.c{e}").flags.nodes[1].view() == "e");
static_assert(parse_algo_semver("1.0.0.c{e}").flags.nodes[1].depth == 1);
static_assert(!parse_algo_semver("1.0.0.c{e}").is_sentinel());
// Die frueher als "experimentell" gelesene Q3-Form ist heute schlicht die ALTE SCHREIBWEISE -> Sentinel.
static_assert(parse_algo_semver("1.0.0ce") == AlgoSemVer{});

// R8: 'p' und 'e' sind SUB-Flags unter der Basis 'c'; "{p}" ist der Default.
//     ACHTUNG, ausdruecklich: die Default-Aussage ist SEMANTISCH. Der Parser faltet "c" NICHT auf "c{p}";
//     die beiden Formen sind hier ZWEI Werte (s. "WARUM NICHT SORTIERT UND NICHT NORMALISIERT" im Kopf).
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.count == 3);
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[0].view() == "c");
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[0].depth == 0);
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[1].view() == "p");
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[1].depth == 1);
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[2].view() == "e");
static_assert(parse_algo_semver("1.0.0.c{p.e}").flags.nodes[2].depth == 1);
static_assert(!(parse_algo_semver("1.0.0.c") == parse_algo_semver("1.0.0.c{p}"))); // NICHT normalisiert
static_assert(parse_algo_semver("1.0.0.c{p}").has_top_level_flag("c"));
static_assert(!parse_algo_semver("1.0.0.c{p}").has_top_level_flag("p")); // 'p' steht auf Tiefe 1

// -- MEHRZEICHIGE BASEN: der Parser raet NICHT zeichenweise ----------------------------------------
// Ohne diese Eigenschaft haette 'gfni' als 'g' + Rest 'fni' gelesen und 'x512' als 'x' + '512'.
static_assert(parse_algo_semver("1.0.0.x512{f.vl.bw.dq}").flags.nodes[0].view() == "x512");
static_assert(parse_algo_semver("1.0.0.x512{f.vl.bw.dq}").flags.count == 5);
static_assert(parse_algo_semver("1.0.0.x256{f}").flags.nodes[0].view() == "x256");
static_assert(parse_algo_semver("1.0.0.x128{f}").flags.nodes[0].view() == "x128");
static_assert(parse_algo_semver("1.0.0.gfni").flags.count == 1);
static_assert(parse_algo_semver("1.0.0.gfni").flags.nodes[0].view() == "gfni"); // NICHT 'g' + 'fni'
static_assert(!(parse_algo_semver("1.0.0.gfni") == parse_algo_semver("1.0.0.g")));
static_assert(parse_algo_semver("1.0.0.vpclmulqdq").flags.nodes[0].view() == "vpclmulqdq");
static_assert(parse_algo_semver("1.0.0.x512{vbmi2}").flags.nodes[1].view() == "vbmi2");           // Ziffer IM Token
static_assert(parse_algo_semver("1.0.0.3dnowprefetch").flags.nodes[0].view() == "3dnowprefetch"); // Ziffer VORN

// -- DIE VIER STRUKTURFAELLE DES ECHTEN KATALOGS (SIMD-Recherche 07.08.2026) ------------------------
// Der Auftrag war ausdruecklich: wenn die Repraesentation einen dieser vier Faelle nicht abbilden kann,
// ist sie falsch gewaehlt. Hier steht der Beweis, dass sie alle vier traegt -- an REALEN Token, nicht an
// Platzhaltern. Es ist KEINE Katalog-Wache: die Form sagt nur, dass sich diese Gestalten SCHREIBEN
// lassen, nicht dass die Token gueltig sind.
// (1) Basis mit Sub-Liste.
static_assert(parse_algo_semver("1.0.0.x512{f.vl.bw.dq}").flags.count == 5);
static_assert(parse_algo_semver("1.0.0.x128{sse.sse2.sse3.ssse3.sse41.sse42.sse4a}").flags.count == 8);
static_assert(parse_algo_semver("1.0.0.x256{avx.avx2.fma.f16c.vnni.vnniint8.vnniint16}").flags.count == 8);
// (2) Companion OHNE Basis -- Breite folgt der Basis (gfni/vaes/vpclmulqdq).
static_assert(parse_algo_semver("1.0.0.x512{f.vl}.gfni.vaes.vpclmulqdq").flags.count == 6);
static_assert(parse_algo_semver("1.0.0.x512{f.vl}.gfni").has_top_level_flag("gfni"));
// (3) Skalar OHNE JEDE Breite -- reine GPR-Instruktionen.
static_assert(parse_algo_semver("1.0.0.c.popcnt.bmi1.bmi2.abm.movbe.adx.rdrand.rdseed").flags.count == 9);
// (4) Medien-Basis m64 (MMX/3DNow!, x87-aliasierte MM-Register auf einer ANDEREN Registerdatei als
//     x128/x256/x512). Die FORM traegt beide Gestalten -- der Parser kennt keinen Katalog und haelt
//     auch die seit dem 07.08.2026 KATALOG-UNGUELTIGE bare-Token-Form fuer syntaktisch wohlgeformt.
//     Das ist beabsichtigt (Trennung Form/Katalog, s. Kopf) und wird in (m4) unten katalogseitig
//     scharf gestellt: dort bricht die bare-Token-Form, hier nicht.
static_assert(parse_algo_semver("1.0.0.c.mmx.mmxext.3dnow.3dnowext.3dnowprefetch").flags.count == 6);
static_assert(parse_algo_semver("1.0.0.c.m64{mmx.mmxext.3dnow}").flags.count == 5);
static_assert(parse_algo_semver("1.0.0.c.m64{mmx.mmxext.3dnow}").flags.nodes[1].view() == "m64");
// DER VOLLAUSBAU: alles zugleich. Er belegt den Knoten-Deckel an der Sache statt an einer Schaetzung.
namespace detail {
inline constexpr std::string_view kVollausbau =
    "1.0.0.c{p.e}"
    ".x128{sse.sse2.sse3.ssse3.sse41.sse42.sse4a.aes.pclmulqdq.sha}"
    ".x256{avx.avx2.fma.f16c.vnni.ifma.vnniint8.vnniint16.neconvert.sha512.sm3.sm4}"
    ".x512{f.cd.vl.dq.bw.ifma.vbmi.vbmi2.vnni.bitalg.vpopcntdq.vp2intersect.bf16.fp16}"
    ".gfni.vaes.vpclmulqdq.popcnt.bmi1.bmi2.abm.movbe.adx.rdrand.rdseed"
    ".m64{mmx.mmxext.3dnow.3dnowext}.3dnowprefetch";
} // namespace detail
static_assert(parse_algo_semver(detail::kVollausbau).flags.count == 59,
              "der am echten Katalog gerechnete Vollausbau -- er ist die Bemessung von kMaxFlagNodes. "
              "58 -> 59 seit dem m64-Entscheid: die Basis selbst ist jetzt ein EIGENER Knoten (vorher "
              "waren mmx/mmxext/3dnow/3dnowext vier bare Blaetter ohne Basis-Knoten).");
static_assert(!parse_algo_semver(detail::kVollausbau).is_sentinel(),
              "der Vollausbau MUSS parsen. Tut er es nicht, ist ein Deckel zu klein -- und ein Deckel, der "
              "eine legitime Eingabe verwirft, ist ein Defekt und keine Wache.");
static_assert(kMaxFlagNodes > 59, "Luft ueber dem heutigen Vollausbau, nicht knapp daneben.");
// Die DOTTED-Katalognamen (avx10.1/avx10.2) brechen LAUT statt still in zwei Knoten zu zerfallen -- das
// ist Loch 2 der Token-Regel, hier als Beweis und nicht als Behauptung.
static_assert(parse_algo_semver("1.0.0.x512{avx10.1}") == AlgoSemVer{});
static_assert(parse_algo_semver("1.0.0.avx10.2") == AlgoSemVer{});
static_assert(!parse_algo_semver("1.0.0.x512{avx10}").is_sentinel()); // ... ohne die Version sehr wohl
// Und die beiden ANDEREN Namensraeume sind ebenfalls keine gueltigen Token (s. DREI-NAMENSRAEUME):
static_assert(parse_algo_semver("1.0.0.x128{sse4.1}") == AlgoSemVer{}); // Compiler-Schalter-Form (PUNKT)
static_assert(parse_algo_semver("1.0.0.x128{sse4_1}") == AlgoSemVer{}); // cpuinfo-Form (UNTERSTRICH)
static_assert(parse_algo_semver("1.0.0.avx512_vbmi2") == AlgoSemVer{}); // cpuinfo-Form der AVX-512-Familie
static_assert(!parse_algo_semver("1.0.0.x128{sse41}").is_sentinel());   // ... unsere Form traegt
static_assert(!parse_algo_semver("1.0.0.x512{vbmi2}").is_sentinel());

// -- DAS OWNER-BEISPIEL, vollstaendig ---------------------------------------------------------------
// memory_layout=SoaMemoryLayout@1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni  -- hier der Versions-Anteil.
namespace detail {
inline constexpr std::pair<std::string_view, std::uint8_t> kOwnerBeispielKnoten[] = {
    {"c", 0}, {"p", 1}, {"e", 1}, {"x512", 0}, {"f", 1}, {"vl", 1}, {"bw", 1}, {"dq", 1}, {"gfni", 0}};
inline constexpr std::string_view kOwnerBeispiel = "1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni";
} // namespace detail
static_assert(parse_algo_semver(detail::kOwnerBeispiel) ==
              AlgoSemVer{1, 0, 0, detail::flag_list_of(detail::kOwnerBeispielKnoten)});
static_assert(parse_algo_semver(detail::kOwnerBeispiel).flags.count == 9);
static_assert(parse_algo_semver(detail::kOwnerBeispiel).has_top_level_flag("c"));
static_assert(parse_algo_semver(detail::kOwnerBeispiel).has_top_level_flag("x512"));
static_assert(parse_algo_semver(detail::kOwnerBeispiel).has_top_level_flag("gfni"));
static_assert(!parse_algo_semver(detail::kOwnerBeispiel).has_top_level_flag("vl"));

// -- REKURSION > 1: die Struktur traegt sie HEUTE, ohne zweiten Umbau -------------------------------
// Kein Bestands-Literal uebt das aus; die Grammatik erlaubt es ("p und e sind je ein Komposit-Flag").
static_assert(parse_algo_semver("1.0.0.c{p{x}}").flags.count == 3);
static_assert(parse_algo_semver("1.0.0.c{p{x}}").flags.nodes[2].depth == 2);
static_assert(parse_algo_semver("1.0.0.c{p{x}.e}").flags.count == 4);
static_assert(parse_algo_semver("1.0.0.c{p{x}.e}").flags.nodes[3].view() == "e");
static_assert(parse_algo_semver("1.0.0.c{p{x}.e}").flags.nodes[3].depth == 1); // zurueck auf Ebene 1
static_assert(parse_algo_semver("1.0.0.a{b{c{d}}}").flags.nodes[3].depth == 3);
static_assert(parse_algo_semver("1.0.0.a{b{c{d{e}}}}").flags.nodes[4].depth == 4); // == kMaxFlagDepth
static_assert(parse_algo_semver("1.0.0.a{b{c{d{e{f}}}}}") == AlgoSemVer{});        // > kMaxFlagDepth
static_assert(kMaxFlagDepth == 4);
// Eltern-Ableitung (die S2-Andockstelle stuetzt sich darauf).
static_assert(parent_flag_node_index(parse_algo_semver("1.0.0.c{p.e}").flags, 0) == kNoFlagParent);
static_assert(parent_flag_node_index(parse_algo_semver("1.0.0.c{p.e}").flags, 1) == 0);
static_assert(parent_flag_node_index(parse_algo_semver("1.0.0.c{p.e}").flags, 2) == 0);
static_assert(parent_flag_node_index(parse_algo_semver("1.0.0.c{p{x}}").flags, 2) == 1);
static_assert(parent_flag_node_index(parse_algo_semver("1.0.0.c{p.e}.gfni").flags, 3) == kNoFlagParent);

// -- (c) SENTINEL-BEDINGUNGEN, jede EINZELN --------------------------------------------------------
static_assert(parse_algo_semver("1.0.0.c{") == AlgoSemVer{});    // unbalanciert: '{' offen
static_assert(parse_algo_semver("1.0.0.c{p") == AlgoSemVer{});   // unbalanciert: Gruppe offen
static_assert(parse_algo_semver("1.0.0.c}") == AlgoSemVer{});    // schliessende Klammer ohne oeffnende
static_assert(parse_algo_semver("1.0.0.c{p}}") == AlgoSemVer{}); // eine zu viel
static_assert(parse_algo_semver("1.0.0.C") == AlgoSemVer{});     // Grossbuchstabe
static_assert(parse_algo_semver("1.0.0.c{P}") == AlgoSemVer{});  // Grossbuchstabe im Sub
static_assert(parse_algo_semver("1.0.0.c ") == AlgoSemVer{});    // Leerzeichen-Rest
static_assert(parse_algo_semver(" 1.0.0.c") == AlgoSemVer{});    // fuehrendes Leerzeichen
static_assert(parse_algo_semver("1.0.0.{}") == AlgoSemVer{});    // leere Gruppe ohne Basis
// Token-Laengen-Deckel: exakt kMaxFlagTokenLen traegt, ein Zeichen mehr nicht (nie stilles Kuerzen).
static_assert(parse_algo_semver("1.0.0.abcdefghijklmnop").flags.nodes[0].len == kMaxFlagTokenLen);
static_assert(parse_algo_semver("1.0.0.abcdefghijklmnopq") == AlgoSemVer{});
// K-5-SENTINEL-BATTERIE: das Null-Tripel ist der REINE Sentinel -- auch mit Flags.
static_assert(parse_algo_semver("0.0.0.c") == AlgoSemVer{});
static_assert(parse_algo_semver("0.0.0.c{p.e}") == AlgoSemVer{});
static_assert(!parse_algo_semver("0.0.0.c").has_flags()); // KEIN "CPU-Sentinel"
static_assert(parse_algo_semver("0.0.0.c").is_sentinel());
static_assert(!parse_algo_semver("1.0.0.c").is_sentinel());
// UNTERSCHEIDBARKEIT (Fingerprint-/Lager-Wirkung): jede Flag-Form ist ein anderes Stempel-Segment.
static_assert(!(parse_algo_semver("1.0.0.c") == parse_algo_semver("1.0.0")));
static_assert(!(parse_algo_semver("1.0.0.c") == parse_algo_semver("1.0.0.g")));
static_assert(!(parse_algo_semver("1.0.0.c") == parse_algo_semver("1.0.0.c{e}")));
static_assert(!(parse_algo_semver("1.0.0.c{p}") == parse_algo_semver("1.0.0.c{e}")));
static_assert(!(parse_algo_semver("1.0.0.x512{f}") == parse_algo_semver("1.0.0.x512{f.vl}")));

// -- (d) ROUNDTRIP: parsen -> rendern -> parsen ergibt dasselbe ------------------------------------
namespace detail {
/// Der Roundtrip-Beweis in EINER Form, damit die Batterie unten je Literal eine Zeile ist statt drei.
/// Er prueft BEIDES: die Zeichenfolge kommt unveraendert zurueck UND der Wert ist stabil.
[[nodiscard]] constexpr bool roundtrip_ist_treu(std::string_view lit) noexcept {
    AlgoSemVer const         v = parse_algo_semver(lit);
    RenderedAlgoSemVer const r = render_algo_semver(v);
    return r.view() == lit && parse_algo_semver(r.view()) == v;
}
} // namespace detail
static_assert(detail::roundtrip_ist_treu("1.0.0"));
static_assert(detail::roundtrip_ist_treu("1.0.0.c"));
static_assert(detail::roundtrip_ist_treu("1.0.2.c"));
static_assert(detail::roundtrip_ist_treu("2.3.4.c{p.e}"));
static_assert(detail::roundtrip_ist_treu("1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni"));
static_assert(detail::roundtrip_ist_treu("1.0.0.x512{f}"));
static_assert(detail::roundtrip_ist_treu("1.0.0.c.g.f.n"));
static_assert(detail::roundtrip_ist_treu("1.0.0.c{p{x}.e}"));
static_assert(detail::roundtrip_ist_treu("1.0.0.a{b{c{d}}}"));
static_assert(detail::roundtrip_ist_treu("10.0.1.c"));
static_assert(detail::roundtrip_ist_treu("999999.999999.999999.c"));
static_assert(detail::roundtrip_ist_treu(kAlgoSemVerSentinelLiteral));
// Der am echten SIMD-Katalog gerechnete VOLLAUSBAU (58 Knoten) kommt ebenfalls verbatim zurueck --
// der teuerste Fall ist damit nicht nur parsbar, sondern auch verlustfrei renderbar.
static_assert(detail::roundtrip_ist_treu(detail::kVollausbau));
// Der Sentinel rendert IMMER nackt -- auch wenn das Quell-Literal Flags trug (K-5).
static_assert(render_algo_semver(parse_algo_semver("0.0.0.c{p}")).view() == "0.0.0");
// Und ein UNPARSBARES Literal rendert den Sentinel, statt zu raten.
static_assert(render_algo_semver(parse_algo_semver("v1.0.0c")).view() == "0.0.0");
static_assert(render_algo_semver(parse_algo_semver("quatsch")).view() == "0.0.0");
// Der Puffer-Deckel haelt fuer den groessten konstruierbaren Fall (Deckel-Ableitung, nicht geraten).
static_assert(kMaxRenderedAlgoSemVerLen > 3u * kMaxSemVerComponentDigits + 2u);
static_assert(render_algo_semver(parse_algo_semver(detail::kOwnerBeispiel)).len <= kMaxRenderedAlgoSemVerLen);
// Der Flag-Schwanz-Renderer ist der SELBE Weg wie die Voll-Form (keine zweite Wahrheit): die Voll-Form ist
// exakt "X.Y.Z" + Schwanz. Das ist die Zusage, auf der die POD-Kodierung (Owner-F-3) aufsetzt.
static_assert(render_flag_tail(parse_algo_semver("1.0.0.c{p.e}").flags).view() == ".c{p.e}");
static_assert(render_flag_tail(parse_algo_semver(detail::kOwnerBeispiel).flags).view() ==
              ".c{p.e}.x512{f.vl.bw.dq}.gfni");
static_assert(render_flag_tail(parse_algo_semver("1.0.0").flags).view() == "");
static_assert(render_flag_tail(parse_algo_semver("0.0.0.c").flags).view() == ""); // Sentinel traegt nie Flags
static_assert(render_flag_tail(FlagList{}).len == 0);

// -- (e) DIE POLITIK-WACHEN ------------------------------------------------------------------------
// Owner-F-10: "ce-eigene Achsen tragen mindestens 'c'".
static_assert(version_satisfies_cpu_only_policy("1.0.0.c"));
static_assert(version_satisfies_cpu_only_policy("1.0.2.c"));
static_assert(version_satisfies_cpu_only_policy("1.0.0.c{p.e}"));
static_assert(version_satisfies_cpu_only_policy("1.0.0.c{p.e}.x512{f}"));
static_assert(version_satisfies_cpu_only_policy("1.0.0.c.g")); // NEU entscheidbar (Kardinalitaet 1 -> n)
static_assert(!version_satisfies_cpu_only_policy("1.0.0"));    // flaglos
static_assert(!version_satisfies_cpu_only_policy("1.0.0.g"));  // reserviert, nicht produziert
static_assert(!version_satisfies_cpu_only_policy("1.0.0.f"));
static_assert(!version_satisfies_cpu_only_policy("1.0.0.n"));
static_assert(!version_satisfies_cpu_only_policy("1.0.0.x512{f}")); // SIMD ohne CPU-Basis
static_assert(!version_satisfies_cpu_only_policy("1.0.0.c{p"));     // unparsbar erfuellt nie eine Politik
static_assert(!version_satisfies_cpu_only_policy("0.0.0.c"));       // Sentinel erfuellt nie eine Politik
static_assert(!version_satisfies_cpu_only_policy("v1.0.0c"));       // die Alt-Form ebenfalls nicht
static_assert(!version_satisfies_cpu_only_policy(""));

// B12 (a): PARSBAR-PFLICHT -- nur der dokumentierte Sentinel-Wortlaut darf auf den Sentinel parsen.
static_assert(version_is_parsable_or_documented_sentinel("1.0.0"));
static_assert(version_is_parsable_or_documented_sentinel("1.0.0.c"));
static_assert(version_is_parsable_or_documented_sentinel(kAlgoSemVerSentinelLiteral)); // ABSICHT
static_assert(!version_is_parsable_or_documented_sentinel("0.0.0.c"));  // Sentinel-WERT, falsches Literal
static_assert(!version_is_parsable_or_documented_sentinel("1.0"));      // Kurzform -> junk
static_assert(!version_is_parsable_or_documented_sentinel("v1.0.0c"));  // Alt-Form -> junk
static_assert(!version_is_parsable_or_documented_sentinel("01.0.0.c")); // B11-Leading-Zero -> junk
static_assert(!version_is_parsable_or_documented_sentinel("1.0.0.c{")); // unbalanciert -> junk
static_assert(!version_is_parsable_or_documented_sentinel(""));

// B12 (b): ce-EIGENE Versionen -- parsbar UND (wenn Flags da sind) mit 'c' darunter.
static_assert(ce_owned_version_is_wellformed("1.0.0"));   // flaglos bleibt GRAMMATISCH wohlgeformt ...
static_assert(ce_owned_version_is_wellformed("1.0.0.c")); // ... die Ziel-Form ist diese
static_assert(ce_owned_version_is_wellformed("1.0.2.c"));
static_assert(ce_owned_version_is_wellformed("1.0.0.c{p.e}"));
static_assert(ce_owned_version_is_wellformed(kAlgoSemVerSentinelLiteral));
static_assert(!ce_owned_version_is_wellformed("1.0.0.g")); // falsches Hardware-Flag
static_assert(!ce_owned_version_is_wellformed("1.0.0.f"));
static_assert(!ce_owned_version_is_wellformed("1.0.0.n"));
static_assert(!ce_owned_version_is_wellformed("1.0.0.x512{f}")); // SIMD ohne CPU-Basis
static_assert(!ce_owned_version_is_wellformed("1.0"));           // unparsbar
static_assert(!ce_owned_version_is_wellformed("0.0.0.c"));       // Sentinel-Wert, undokumentiertes Literal
static_assert(!ce_owned_version_is_wellformed("4294967297.0.0.c"));
static_assert(!ce_owned_version_is_wellformed("v1.0.0c")); // die Alt-Form ist ab jetzt junk
static_assert(!ce_owned_version_is_wellformed(""));
// R7-Gegenprobe: ein 'e' ist HIER kein Ablehnungsgrund mehr -- es ist ein legitimes Flag.
static_assert(ce_owned_version_is_wellformed("1.0.0.c{e}"));
static_assert(ce_owned_version_is_wellformed("1.0.0.c{p.e}"));

// B12 (c): GATED -- das CPU-Flag ist Pflicht.
static_assert(ce_owned_version_satisfies_cpu_enforce("1.0.0.c"));
static_assert(ce_owned_version_satisfies_cpu_enforce("1.0.0.c{p.e}"));
static_assert(!ce_owned_version_satisfies_cpu_enforce("1.0.0")); // flaglos ist hart
static_assert(!ce_owned_version_satisfies_cpu_enforce("1.0.0.g"));
static_assert(!ce_owned_version_satisfies_cpu_enforce(kAlgoSemVerSentinelLiteral));
static_assert(!ce_owned_version_satisfies_cpu_enforce(""));
// Der gated Zwilling ist NIE schwaecher als der ungated Teil (Ordnungs-Gegenprobe).
static_assert(ce_owned_version_satisfies_cpu_enforce("1.0.0.c") && ce_owned_version_is_wellformed("1.0.0.c"));
static_assert(!ce_owned_version_satisfies_cpu_enforce("1.0.0") && ce_owned_version_is_wellformed("1.0.0"));

// -- (m) DIE KATALOG-WACHE (S2, 07.08.2026) --------------------------------------------------------
// Der Beweis, dass die Wache BEISST -- und dass sie den Bestand nicht bricht.

namespace detail {
/// JEDES Katalog-Token muss ein GRAMMATISCH ERREICHBARES Token sein: take_flag_token() muss es
/// VOLLSTAENDIG und UNVERAENDERT konsumieren. Das ist die schaerfste der Tabellen-Wachen, weil sie die
/// EINE Fehlerklasse faengt, die eine Tabelle sonst still traegt: ein Eintrag, den die Grammatik gar
/// nicht schreiben kann. Wer "avx512_vbmi2" (Unterstrich), "sse4.1" (Punkt), "SSE2" (Grossbuchstabe)
/// oder ein 17 Zeichen langes Token in die token-Spalte setzt, legt eine Zulassung an, die NIE greift --
/// die Tabelle behauptete dann eine Wache, die es nicht gibt.
/// Sie steht HIER und nicht in flag_grammar_catalog.hpp, weil sie den PARSER braucht: der Katalog kennt
/// die Grammatik nicht (und darf sie nicht kennen -- sonst haetten wir wieder zwei Wahrheiten ueber die
/// Token-Regel, und der Include liefe im Kreis).
[[nodiscard]] constexpr bool token_ist_grammatisch_erreichbar(std::string_view t) noexcept {
    std::string_view       rest = t;
    std::string_view const tok  = take_flag_token(rest);
    return !tok.empty() && tok == t && rest.empty();
}
[[nodiscard]] constexpr bool alle_katalog_token_sind_erreichbar() noexcept {
    for (FlagCatalogEntry const& e : kFlagGrammarCatalog) {
        if (!token_ist_grammatisch_erreichbar(e.token)) return false;
        if (!e.eltern.empty() && !token_ist_grammatisch_erreichbar(e.eltern)) return false;
        if (!e.eltern_alternativ.empty() && !token_ist_grammatisch_erreichbar(e.eltern_alternativ)) return false;
    }
    for (FlagReserveEntry const& r : kFlagGrammarReserve)
        if (!token_ist_grammatisch_erreichbar(r.token)) return false;
    return true;
}
} // namespace detail
static_assert(detail::alle_katalog_token_sind_erreichbar(),
              "Ein Katalog-Token, das die Grammatik nicht schreiben kann (Unterstrich, Punkt, "
              "Grossbuchstabe, zu lang), ist eine Zulassung, die nie greift -- also keine Wache.");
// Gegenprobe der Wache selbst: die drei Fehlformen, gegen die sie gebaut ist, faellt sie auch wirklich.
static_assert(!detail::token_ist_grammatisch_erreichbar("avx512_vbmi2"));      // cpuinfo-Namensraum
static_assert(!detail::token_ist_grammatisch_erreichbar("sse4.1"));            // Compiler-Schalter-Namensraum
static_assert(!detail::token_ist_grammatisch_erreichbar("SSE2"));              // Grossbuchstabe
static_assert(!detail::token_ist_grammatisch_erreichbar("abcdefghijklmnopq")); // > kMaxFlagTokenLen
static_assert(detail::token_ist_grammatisch_erreichbar("3dnowprefetch"));      // das laengste echte Token

// (m1) UNBEKANNTES TOKEN bricht -- auf JEDER Tiefe.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{quatsch}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.quatsch")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c{quatsch}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.zzz")));
// ... und der Parser haelt sie weiterhin fuer WOHLGEFORMT. Genau diese Arbeitsteilung ist der Punkt:
// eine Zeile, die formal richtig gesetzt ist und trotzdem nichts bedeutet, faellt jetzt auf.
static_assert(!parse_algo_semver("1.0.0.x512{quatsch}").is_sentinel());

// (m2) BEKANNTES TOKEN UNTER FALSCHER BASIS bricht -- die eigentliche Leistung dieser Wache.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{sse2}"))); // sse2 gehoert unter x128
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{sse2}")));  // ... dort traegt es
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{vl}")));   // vl gehoert unter x512
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x256{vl}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{avx2}"))); // avx2 gehoert unter x256
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c{vl}")));      // und schon gar nicht unter c
// 'p'/'e' NUR unter 'c' (Owner-R8) -- die Bedingung, die die Andockstelle namentlich genannt hat.
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c{p.e}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.p")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.e")));  // 'e' auf Tiefe 0
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.g{p}"))); // 'p' unter der GPU
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{p}")));
// Die Tiefe braucht die Wache nicht: die Eltern-Kette begrenzt sich selbst (s. flag_catalog_is_satisfied).
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c{c{p}}"))); // schon das innere 'c' faellt
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c{p{e}}"))); // 'e' ist kein Sub von 'p'
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{f{vl}}")));

// (m3) COMPANION AN BASIS-STELLE bricht -- und das ist eine HARDWARE-Aussage, keine Stil-Regel.
// Es gibt KEIN separates avx512-gfni-Bit: gfni deckt mit EINEM CPUID-Bit Legacy-SSE, VEX.128/256 und
// (mit avx512f bzw. avx512vl) EVEX ab. Ein "x512{gfni}" behauptete ein Bit, das die ISA nicht kennt --
// waehrend die Owner-Form "...x512{f.vl.bw.dq}.gfni" genau richtig ist: der Companion steht NEBEN der
// Basis, nicht in ihr, weil seine Breite der Basis FOLGT.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{gfni}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x256{vaes}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{vpclmulqdq}")));
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{f.vl}.gfni.vaes.vpclmulqdq")));
// Dieselbe Regel fuer die Skalare, aus dem ENTGEGENGESETZTEN Grund (sie haben gar keine Breite).
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{popcnt}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{bmi2}")));
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.popcnt.bmi1.bmi2.abm.movbe.adx.rdrand.rdseed")));

// (m4) FALL (4) IST ENTSCHIEDEN: NUR die m64-Basis-Form geht noch durch. Die ALTE bare-Token-Form
// (Gestalt (a), gestern noch gleichberechtigt) bricht jetzt LAUT -- genau die Falle, die beim ersten
// Grammatik-Commit erst am Schadensort auffiel: hier ist die WACHE selbst der Bissbeweis.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.mmx.mmxext.3dnow.3dnowext")));     // Gestalt (a)
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.m64{mmx.mmxext.3dnow.3dnowext}"))); // (b)
// Der alte Name "x64" ist NICHT mehr im Katalog -- weder als Basis noch als Elternteil. Eine Komposition,
// die ihn benutzt, faellt auf ZWEI Arten zugleich (x64 selbst unbekannt, mmx darunter also erst recht):
// die alte Schreibweise liefert kein stilles @0.0.0, sie liefert compile-time FALSE an dieser Wache.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.x64{mmx.mmxext.3dnow}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.x64{mmx.mmxext.3dnow.3dnowext}")));
// ... und die vier Subsets stehen NICHT unter einer der drei echten Breiten-Basen. Das ist die
// Hardware-Aussage hinter Fall (4): MM-Register sind auf den x87-Stack aliasiert und eine ANDERE
// Registerdatei als XMM/YMM/ZMM -- m64 ist keine vierte Breite derselben Achse.
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{mmx}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x256{mmxext}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x512{3dnow}")));
static_assert(!flag_catalog_is_satisfied(parse_algo_semver("1.0.0.x128{3dnowext}")));

// (m5) DER VOLLAUSBAU GEHT VOLLSTAENDIG DURCH. Er ist das schaerfste Positiv-Kriterium dieser Datei: 59
// Knoten, jedes einzelne Token ein echtes Katalog-Token an seiner richtigen Stelle. Faellt er, ist eine
// Katalog-Zeile falsch -- und ein Katalog, der eine legitime Vollform verwirft, ist ein Defekt und keine
// Wache (dasselbe Argument, das schon kMaxFlagNodes von 32 auf 96 gehoben hat).
static_assert(flag_catalog_is_satisfied(parse_algo_semver(detail::kVollausbau)),
              "der am echten SIMD-Katalog gerechnete Vollausbau MUSS den Katalog erfuellen.");
static_assert(ce_owned_version_is_wellformed(detail::kVollausbau));
static_assert(ce_owned_version_satisfies_cpu_enforce(detail::kVollausbau));
// Und das Owner-Beispiel ebenso -- Zeichen fuer Zeichen die Form aus dem Design-Doc.
static_assert(flag_catalog_is_satisfied(parse_algo_semver(detail::kOwnerBeispiel)));
static_assert(ce_owned_version_is_wellformed(detail::kOwnerBeispiel));
// Die drei Basen einzeln, in ihrer vollen Sub-Liste.
static_assert(flag_catalog_is_satisfied(
    parse_algo_semver("1.0.0.x128{sse.sse2.sse3.ssse3.sse41.sse42.sse4a.aes.pclmulqdq.sha}")));
static_assert(flag_catalog_is_satisfied(
    parse_algo_semver("1.0.0.x256{avx.avx2.fma.f16c.vnni.ifma.vnniint8.vnniint16.neconvert.sha512.sm3.sm4}")));
static_assert(flag_catalog_is_satisfied(
    parse_algo_semver("1.0.0.x512{f.cd.vl.dq.bw.ifma.vbmi.vbmi2.vnni.bitalg.vpopcntdq.vp2intersect.bf16.fp16}")));

// (m6) DER BESTAND BLEIBT GRUEN. Alle ce-eigenen Literale tragen heute 'c' bzw. 'c{p.e}'-Formen; die
// Wache darf sie nicht brechen. Das ist keine Vermutung: die Registry-Wachen an den Stempel-STELLEN
// laufen ueber genau diese Literale, und der ctest-Lauf ist der Vollzugsbeleg.
static_assert(ce_owned_version_is_wellformed("1.0.0.c"));
static_assert(ce_owned_version_is_wellformed("1.0.1.c"));
static_assert(ce_owned_version_is_wellformed("1.0.2.c"));
static_assert(ce_owned_version_is_wellformed("1.0.0.c{p}"));
static_assert(ce_owned_version_is_wellformed("1.0.0.c{e}"));
static_assert(ce_owned_version_is_wellformed("1.0.0.c{p.e}"));
static_assert(ce_owned_version_is_wellformed("1.0.0")); // flaglos: der Katalog ist LEER erfuellt
static_assert(ce_owned_version_is_wellformed("0.0.0")); // Sentinel: traegt nie Flags (K-5)
static_assert(flag_catalog_is_satisfied(AlgoSemVer{})); // die leere Liste erfuellt ihn vakuum-weise
// ... und die Katalog-Fehlform faellt jetzt AUCH an der B12-Wache, nicht nur am Einzelpraedikat.
static_assert(!ce_owned_version_is_wellformed("1.0.0.c{quatsch}"));
static_assert(!ce_owned_version_is_wellformed("1.0.0.c.x512{sse2}"));
static_assert(!ce_owned_version_is_wellformed("1.0.0.c.gfni.quatsch"));
static_assert(!ce_owned_version_is_wellformed("1.0.0.c{p.x}"));
// GEGENPROBE, damit (c) nicht von (b) verdeckt wird: "1.0.0.g{p}" muesste schon an der 'c'-Pflicht
// scheitern -- "1.0.0.c.g{p}" traegt 'c' und faellt trotzdem, also greift wirklich der Katalog-Term.
static_assert(!ce_owned_version_is_wellformed("1.0.0.c.g{p}"));
static_assert(version_satisfies_cpu_only_policy("1.0.0.c.g{p}")); // ... die Politik allein liesse es durch

// (m7) DIE ROHE FORM version_flags_are_in_catalog: ihre Semantik fuer UNPARSBARES ist offen gelegt.
// Sie sagt NICHTS ueber die Parsbarkeit -- ein unparsbares Literal parst auf den Sentinel, der Sentinel
// traegt nie Flags, und der leere Katalog ist erfuellt. Wer beide Fragen braucht, nimmt
// ce_owned_version_is_wellformed().
static_assert(version_flags_are_in_catalog("1.0.0.c{p.e}"));
static_assert(!version_flags_are_in_catalog("1.0.0.x512{sse2}"));
static_assert(version_flags_are_in_catalog("v1.0.0c"));    // unparsbar -> Sentinel -> LEER erfuellt
static_assert(!ce_owned_version_is_wellformed("v1.0.0c")); // ... aber die Voll-Wache faellt trotzdem
static_assert(version_flags_are_in_catalog("quatsch"));
static_assert(!ce_owned_version_is_wellformed("quatsch"));

// (m8) WAS DIESE WACHE AUSDRUECKLICH NICHT PRUEFT -- benannt, damit niemand es fuer geprueft haelt:
//   * DOPPELUNGEN. "1.0.0.c.c" und "1.0.0.x512{f.f}" gehen durch. Jedes Token steht im Katalog und an
//     seiner Stelle; dass es zweimal dasteht, ist eine andere Fehlerklasse (Redundanz, nicht Bedeutung)
//     und braucht eine eigene Wache. Sie ist NICHT gebaut.
//   * EINE BREITEN-BASIS OHNE SUB-LISTE. "1.0.0.c.x512" geht durch. Ob eine nackte Basis eine sinnvolle
//     Aussage ist ("gebaut fuer 512-bit, ohne zu sagen welches Subset"), ist eine Sach-Frage, die keine
//     der Quellen beantwortet -- und strenger zu sein als die Quellen waere Raten.
//   * ABHAENGIGKEITEN ZWISCHEN FLAGS. "1.0.0.c.x512{vl}" ohne 'f' geht durch, obwohl alle AVX-512-Subsets
//     auf avx512f gegated sind; ebenso "x256{vaes}" ohne aes. Die Recherche liefert diese Ketten
//     (aes->vaes, pclmulqdq->vpclmulqdq, avx512f->alles), aber eine Voraussetzungs-Wache ist ein eigener
//     Schritt mit eigener Semantik-Frage (fordert das Flag die Voraussetzung, oder impliziert es sie?).
//   * OB DIE MASCHINE DAS FLAG HAT. Das ist die Aufgabe von machine_simd_signature.hpp und des
//     Bau-Gates -- diese Wache prueft das VOKABULAR, nicht die Hardware vor Ort.
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.c")));        // Doppelung: NICHT geprueft
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.x512")));     // nackte Basis: NICHT geprueft
static_assert(flag_catalog_is_satisfied(parse_algo_semver("1.0.0.c.x512{vl}"))); // ohne 'f': NICHT geprueft

} // namespace comdare::cache_engine::measurement
