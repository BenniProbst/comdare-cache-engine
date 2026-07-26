#pragma once
// STRUKT-R ORG-18 axis_persistence_target Subaxis-Tags PT1-PT2
// @topic io @achse persistence_target
//
// Vorbild: axes/io_dispatch/axis_io_subaxes_io1_to_io3.hpp (Tag je Sub-Dimension; jeder Baustein waehlt
// die Dimension, auf der er variiert). Bewusst 2 Tags = 2 Bausteine (Session-Doc §5: memory_only +
// disk_writeback, "spaeter erweiterbar" -> weitere Tags additiv).
//
// ABGRENZUNG zu io_dispatch (dieselbe Topic-Familie io, disjunkte Frage): io_dispatch fragt WIE der
// Zugriff vermittelt wird (Page-Cache / O_DIRECT / mmap = Dispatch-Profil), persistence_target fragt OB
// ueberhaupt zurueckgeschrieben werden MUSS (Verweildauer-Ziel des Index). Ein Algorithmus kann
// io_in_memory_only dispatchen und dennoch disk_writeback-pflichtig sein - die Achsen sind orthogonal.

namespace comdare::cache_engine::persistence_target::subaxes {

// PT1: Residency -- wo der Index dauerhaft verweilt (nur RAM / auch Platte)
struct residency_tag {};

// PT2: Writeback-Pflicht -- ob der Algorithmus einen Rueckschreib-Pfad bedienen muss
struct writeback_tag {};

} // namespace comdare::cache_engine::persistence_target::subaxes
