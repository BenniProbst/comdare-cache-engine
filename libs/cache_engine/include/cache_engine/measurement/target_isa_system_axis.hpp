// WEGWEISER (S-18/#16, Home-Prinzip KON27-01): dieser Header ist in das Kategorie-Home umgezogen.
//   NEU:   libs/cache_engine/system_axes/target_isa_system_axis.hpp
//   FORM:  #include <system_axes/target_isa_system_axis.hpp>
//   WARUM: je Achsen-Kategorie EIN Verzeichnis-Home + EIN Waechter (tools/axis_version_lock);
//          der Overlay-Schnitt (builder/overlay_source_set.hpp) zeigt auf das Home.
//   WER:   die includierende Datei (der Compiler nennt sie in der Fehlerkette) zieht auf die
//          neue Include-Form nach. Wegweiser loeschbar in der W7/#88-Triage.
// cppcheck-suppress preprocessorErrorDirective
#error "S-18/#16: umgezogen -- neue Include-Form <system_axes/target_isa_system_axis.hpp> (KON27-01)"
