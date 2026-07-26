#pragma once
// STRUKT-R ORG-18 Topic-seitiger Andock-Header der persistence_target-Achse.
//
// ABGRENZUNG zum Nachbarn: topics/io/axis_io/axis_io_registry.hpp ist ein MIGRATIONS-Forwarding
// (V41.F.2, die Achse lag historisch unter topics/ und wurde nach axes/io_dispatch/ verschoben).
// persistence_target ist dagegen von Geburt an axen-zentrisch (axes/persistence_target/) -- dieser
// Header ist reine Topic-Andockung, damit topic_io_config_set.hpp beide Achsen des io-Topics ueber
// denselben topics/io/-Pfad einbindet (Muster topics/queuing/, das 2 Achsen so traegt).

#include <axes/persistence_target/axis_persistence_target_registry.hpp>

namespace comdare::cache_engine::io::axis_persistence_target {
using namespace comdare::cache_engine::persistence_target;
}
