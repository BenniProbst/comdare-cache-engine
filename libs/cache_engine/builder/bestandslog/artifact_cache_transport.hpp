#pragma once
// artifact_cache_transport.hpp -- G3 / #46b Lagerhaltung, Folge-A (T1-Binder).
//
// Die EINE Kante bestandslog -> artifact_transport. Sie bindet die vier ABSTRAKTEN Verben der
// Transport-Naht (BestandTransport, bestandslog_lock.hpp) an die vier KONKRETEN Objekt-per-Key-Verben
// des ArtifactCache (object_fetch/store/remove/stat).
//
// DEPENDENCY INVERSION (benanntes Muster): das Bestandslog haengt an seiner eigenen Abstraktion, nicht
// am Transport-Client -- alle B1-B4/I1-Schichten kennen nur BestandTransport. Der Transport-Client
// kennt das Bestandslog seinerseits NICHT (artifact_cache.hpp inkludiert nichts aus bestandslog/).
// Beide Seiten sieht ausschliesslich diese Datei. Praktische Folge: wer sie nicht inkludiert, zieht die
// artifact_transport-Abhaengigkeit auch nicht -- die Fake-Transporte der Tests bleiben dep-frei und
// deterministisch (kein mc, kein minio, kein Netz).
//
// INERT-NEUTRAL: ist der ArtifactCache ohne Ebene B (minio_enabled()==false), melden alle vier Verben
// ehrlich "nichts geschehen" (nullopt bzw. false). Der LagerRunState sieht dann ein leeres Lager,
// klassifiziert alles als frisch und meldet beim flush() einen Store-Fehler statt eines Phantom-
// Erfolgs. Der Bau selbst bleibt davon unberuehrt (das Lager INFORMIERT, es filtert nicht).
//
// LEBENSDAUER: der zurueckgegebene Transport haelt eine REFERENZ auf den Cache (Muster wie die
// CachePushFn-/CachePullFn-Closures im Iterator, die ebenfalls auf die Host-Instanz zeigen). Der Host
// MUSS die ArtifactCache-Instanz laenger am Leben halten als den Transport und alles, was daraus
// gebunden wurde. Die Instanz ist nach from_env() unveraendert und alle vier Verben sind const ->
// die Referenz ist const und die Nutzung aus mehreren Threads unproblematisch.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), stdlib. Kein Runtime-Switch, kein variant.

#include "../artifact_transport/artifact_cache.hpp" // ArtifactCache + ObjectMeta (die konkrete Seite)
#include "bestandslog_lock.hpp"                     // BestandTransport + ObjectStat (die abstrakte Seite)

#include <optional>
#include <string>

namespace comdare::cache_engine::builder::bestandslog {

// Bindet die vier Verben. Der Cache wird per const& gehalten (s. LEBENSDAUER im Kopf).
[[nodiscard]] inline BestandTransport make_bestand_transport(artifact_transport::ArtifactCache const& cache) {
    BestandTransport t;
    t.fetch = [&cache](std::string const& key) -> std::optional<std::string> { return cache.object_fetch(key); };
    t.store = [&cache](std::string const& key, std::string const& content) -> bool {
        return cache.object_store(key, content);
    };
    t.remove = [&cache](std::string const& key) -> bool { return cache.object_remove(key); };
    t.stat   = [&cache](std::string const& key) -> std::optional<ObjectStat> {
        auto const meta = cache.object_stat(key);
        if (!meta) return std::nullopt;
        // Feld-fuer-Feld statt Reinterpretation: die beiden PODs gehoeren verschiedenen Schichten und
        // duerfen unabhaengig wachsen. mtime_epoch_s reist als 0 == unbekannt durch (s. ObjectMeta) --
        // hier wird KEIN Ersatzwert erfunden.
        return ObjectStat{.size = meta->size, .mtime_epoch_s = meta->mtime_epoch_s};
    };
    return t;
}

} // namespace comdare::cache_engine::builder::bestandslog
