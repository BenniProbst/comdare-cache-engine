#pragma once
// -----------------------------------------------------------------------------
// planer_block_value.hpp -- G4b-2 / E4: der WERT der Planer-Vorreservierung, an einer Stelle.
//
// Ein planer_block meldet dem Lager, dass DIESER Planer gleich eine CEB-Compile-Strecke anstoesst --
// damit ein zweiter Planer auf einer anderen Maschine nicht dieselbe Strecke doppelt reserviert.
// Er blockiert die Strecke als GANZES: slice_begin/slice_count sind 0/0, im Gegensatz zu
// BatchTyp::tier, der das 4096er-Korn traegt.
//
// WARUM DIESE DATEI (E4): der Wert wurde bisher in experiment_plan_director.hpp:1263 gebildet, in
// einem Header, der den ganzen Planer-Katalog mitbringt. Damit war er (a) aus einer Test-TU mit dem
// builder-Include-Satz nicht erreichbar und (b) fuer den Emissions-Pfad nur ueber einen schweren
// Include zu holen. Der Wert wandert deshalb hierher -- in einen leichten Header unter
// builder/bestandslog/, der ausser reservation_lifecycle.hpp nichts zieht. Der Director-Helfer
// bleibt bestehen und DELEGIERT hierher: seine gepinnten Tests
// (test_experiment_plan_director.cpp:1502/:1517-1519) bleiben unveraendert gruen, es entsteht kein
// toter Code, und die Doktrin "ce liefert den WERT, der Host fuehrt den Effekt aus" ist gewahrt --
// bl:: IST ce.
//
// EXPLIZITE id (E2): die id kommt als Argument herein, sie wird hier NICHT zusammengesetzt. Grund:
// es gibt zwei legitime Bildungsregeln fuer dieselbe Reservierungs-Art.
//   Director-Pfad: owner_uuid + "/" + seq      (die gepinnte Alt-Form, unveraendert)
//   Emissions-Pfad: owner_uuid + "/planer"     (E2: eine Sperre je Lauf, nicht je Sequenz)
// Die Eindeutigkeit je Lauf tragen NICHT die Sequenz, sondern der lauf-eindeutige owner (CI:
// CI_JOB_ID@host, lokal: pid@host). Ein neuer Lauf hat einen neuen owner und damit eine neue id ->
// die Merge-Monotonie von pick_reservierung (bestandslog_lock.hpp) ist harmlos, ein Re-Open einer
// terminalen id findet nie statt. `maschine` bleibt ein EIGENES Feld und wird NICHT aus der id
// zurueckgelesen (die id ist Schluessel, der owner ist Identitaet).
//
// UHRFREI: beide Zeitstempel kommen vom Aufrufer -- "Zeit-Formatierung ist nicht Sache dieser
// Zustandsmaschine" (reservation_lifecycle.hpp:54). Zwei Aufrufe mit denselben Argumenten liefern
// denselben Wert, die Funktion ist damit exakt testbar und hat keinen versteckten now()-Zugriff.
// Der Aufrufer bildet die 30-min-Frist nach dem Vorbild von make_slice_reservation
// (builder_registration.hpp:193-200): beide Stempel aus DEMSELBEN now_epoch_s, die Frist ueber
// utc_iso_from_epoch(pro_forma_deadline_epoch_s(now_epoch_s)).
//
// CEB-BINDUNG (E3, gelandet mit Impl-LockB, ce 0cb1902a): `ceb_legende` (die [a,b,c]-Mess-Achsen-
// Klammer der emittierenden CEB-Strecke, plan_legend.hpp:74 canonical_combo) und `ceb_key_sha512`
// (der 128-hex-Binary-Fingerprint der CEB, ceb_version_stamp.hpp:98 kCebFingerprint) sind ZWEI
// GETRENNTE Felder -- sie beantworten "welche Version" auf verschiedenen Ebenen und bleiben deshalb
// einzeln lesbar und filterbar. Beide sind OPTIONAL: leer == "nicht gemeldet", und der Emitter
// schreibt das Attribut dann gar nicht (bestandslog_document.hpp:360/364) -- ein v2-Leser versteht
// das Dokument also unveraendert. tier-Batches lassen beide leer; die Bindung ist Sache des
// planer_block. Sie stehen am ENDE der Feld-Folge, weil die Attribut-Reihenfolge des Emitters Teil
// der Byte-Stabilitaet ist.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + reservation_lifecycle.hpp.
// -----------------------------------------------------------------------------

#include "reservation_lifecycle.hpp" // BatchReservierung / BatchTyp / make_pro_forma_reservation

#include <string>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

[[nodiscard]] inline BatchReservierung make_planer_block_reservation_value(std::string id, std::string maschine,
                                                                           unsigned threads, std::string ceb_legende,
                                                                           std::string ceb_key_sha512,
                                                                           std::string reserviert_utc,
                                                                           std::string pro_forma_bis_utc) {
    BatchReservierung r = make_pro_forma_reservation(std::move(id), BatchTyp::planer_block, std::move(maschine),
                                                     threads, /*slice_begin=*/0, /*slice_count=*/0,
                                                     std::move(reserviert_utc), std::move(pro_forma_bis_utc));
    r.ceb_legende       = std::move(ceb_legende);
    r.ceb_key_sha512    = std::move(ceb_key_sha512);
    return r;
}

} // namespace comdare::cache_engine::builder::bestandslog
