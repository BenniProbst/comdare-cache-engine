// best_binary_selector — Implementierung. Siehe Header für Zweck + Pattern-Begründung.
#include "best_binary_selector.hpp"

#include <charconv>
#include <cmath>
#include <unordered_map>

namespace comdare::cache_engine::best_binary {

namespace fs = std::filesystem;

namespace {

// Spalten-Split einer ';'-Zeile (kein Quoting im WIDE-Schema — die Felder sind alnum/_/=/./: ).
[[nodiscard]] std::vector<std::string> split_semicolon(std::string const& line) {
    std::vector<std::string> out;
    std::string              cur;
    for (char c : line) {
        if (c == ';') {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

// (REV-DATA-04, WP-5 2026-07-16) STRIKTER Zahl-Parser: std::from_chars (locale-unabhängig, portabel für
// Dezimal- + Exponentialform), voller Token-Verbrauch PFLICHT ("12junk" ⇒ false statt still 12) und
// std::isfinite PFLICHT (nan/inf/-inf ⇒ false — NaN passierte zuvor den v<=0-Filter und verletzte die
// strikte schwache Ordnung von std::sort ⇒ nichtdeterministisches Ranking). Ersetzt das frühere rohe
// `std::stod` + catch(...)-auf-0-Mapping.
[[nodiscard]] bool parse_double_strict(std::string const& s, double& out) {
    if (s.empty()) return false;
    double      v      = 0.0;
    char const* first  = s.data();
    char const* last   = s.data() + s.size();
    auto const [p, ec] = std::from_chars(first, last, v);
    if (ec != std::errc{} || p != last) return false;
    if (!std::isfinite(v)) return false;
    out = v;
    return true;
}

} // namespace

int parse_measurement_csv(fs::path const& in, std::vector<MeasurementRow>& out_rows,
                          std::vector<std::string>* reject_diags) {
    std::ifstream f{in};
    if (!f) return -1;

    std::string header_line;
    if (!std::getline(f, header_line)) return -1;
    std::vector<std::string> const headers = split_semicolon(header_line);

    // Header-getriebene Spaltenindex-Auflösung (Reihenfolge-/Breite-agnostisch).
    std::unordered_map<std::string, std::size_t> col;
    for (std::size_t i = 0; i < headers.size(); ++i) col[headers[i]] = i;

    auto need = [&](char const* name, std::size_t& idx) -> bool {
        auto it = col.find(name);
        if (it == col.end()) return false;
        idx = it->second;
        return true;
    };

    std::size_t i_bid = 0, i_nsop = 0, i_valid = 0;
    std::size_t i_ins = 0, i_lk = 0, i_er = 0, i_sc = 0, i_rmw = 0;
    if (!need("binary_id", i_bid)) return -1;
    if (!need("ns_per_op", i_nsop)) return -1;
    if (!need("two_phase_valid", i_valid)) return -1;
    // Interface-p50-Spalten optional (header-getrieben): fehlt eine, bleibt der Wert 0 (= n/a).
    bool const has_ins = need("op_insert_p50_ns", i_ins);
    bool const has_lk  = need("op_lookup_p50_ns", i_lk);
    bool const has_er  = need("op_erase_p50_ns", i_er);
    bool const has_sc  = need("op_scan_p50_ns", i_sc);
    bool const has_rmw = need("op_rmw_p50_ns", i_rmw);

    // (REV-DATA-04) Diagnose-Helfer: verworfene Zeile protokollieren (Zeilennummer 1-basiert inkl. Header).
    std::size_t line_no = 1; // Header war Zeile 1
    auto        reject  = [&](char const* col, std::string const& val) {
        if (reject_diags != nullptr)
            reject_diags->push_back("Zeile " + std::to_string(line_no) + ": Feld '" + col + "' ungueltig ('" + val +
                                    "') -> Zeile verworfen");
    };

    int         count = 0;
    std::string line;
    while (std::getline(f, line)) {
        ++line_no;
        if (line.empty()) continue;
        std::vector<std::string> const fields = split_semicolon(line);
        if (fields.size() <= i_valid || fields.size() <= i_nsop || fields.size() <= i_bid) {
            if (reject_diags != nullptr)
                reject_diags->push_back("Zeile " + std::to_string(line_no) + ": zu wenige Felder -> Zeile verworfen");
            continue;
        }

        MeasurementRow r;
        r.binary_id = fields[i_bid];
        // Pflichtwert ns_per_op: STRIKT (leer/partiell/nan/inf ⇒ Zeile verworfen, REV-DATA-04).
        if (!parse_double_strict(fields[i_nsop], r.ns_per_op)) {
            reject("ns_per_op", fields[i_nsop]);
            continue;
        }
        r.two_phase_valid = (fields[i_valid] == "1");
        // Optionale op_*-Felder: leer ⇒ 0 (= n/a, Bestands-Semantik); nicht-leer ⇒ STRIKT.
        auto opt_field = [&](bool has, std::size_t idx, char const* col, double& dst) -> bool {
            if (!has || fields.size() <= idx || fields[idx].empty()) return true; // fehlend/leer ⇒ 0 (n/a)
            if (parse_double_strict(fields[idx], dst)) return true;
            reject(col, fields[idx]);
            return false;
        };
        if (!opt_field(has_ins, i_ins, "op_insert_p50_ns", r.op_insert_p50)) continue;
        if (!opt_field(has_lk, i_lk, "op_lookup_p50_ns", r.op_lookup_p50)) continue;
        if (!opt_field(has_er, i_er, "op_erase_p50_ns", r.op_erase_p50)) continue;
        if (!opt_field(has_sc, i_sc, "op_scan_p50_ns", r.op_scan_p50)) continue;
        if (!opt_field(has_rmw, i_rmw, "op_rmw_p50_ns", r.op_rmw_p50)) continue;
        out_rows.push_back(std::move(r));
        ++count;
    }
    return count;
}

std::vector<RankedBinary> rank_binaries(std::vector<MeasurementRow> const& rows, RankingCriterion const& crit) {
    // Gruppiere Kriteriums-Werte je binary_id (nur two_phase_valid + Wert>0 = vom Profil ausgeübt).
    std::map<std::string, std::vector<double>> by_id;
    for (auto const& r : rows) {
        if (!r.two_phase_valid) continue;
        double const v = crit.value_of(r);
        if (v <= 0.0) continue; // 0 = Metrik nicht ausgeübt → nicht werten
        by_id[r.binary_id].push_back(v);
    }

    std::vector<RankedBinary> out;
    out.reserve(by_id.size());
    for (auto& [id, vals] : by_id) {
        std::sort(vals.begin(), vals.end());
        // Median (nearest-rank, untere Mitte) — identisch zum csv_to_latex-Aggregat.
        double const median = vals.empty() ? 0.0 : vals[(vals.size() - 1) / 2];
        out.push_back(RankedBinary{id, median, vals.size()});
    }
    // Aufsteigend nach Median (kleiner = besser); Tie-Break: mehr Samples zuerst, dann binary_id.
    std::sort(out.begin(), out.end(), [](RankedBinary const& a, RankedBinary const& b) {
        if (a.median_value != b.median_value) return a.median_value < b.median_value;
        if (a.samples != b.samples) return a.samples > b.samples;
        return a.binary_id < b.binary_id;
    });
    return out;
}

std::optional<fs::path> TiereDllRepository::resolve_dir(std::string const& binary_id) const {
    std::error_code ec;
    if (!fs::is_directory(tiere_dir_, ec)) return std::nullopt;

    std::string const san = orch_sanitize(binary_id);
    if (san.size() <= kStemMax) {
        // Kurze ID: exakter sanitisierter Verzeichnisname (rückwärtskompatibel, orch_make_stem-Zweig 1).
        fs::path const d = tiere_dir_ / san;
        if (fs::is_directory(d, ec) && fs::exists(d / "perm.dll", ec)) return d;
        return std::nullopt;
    }

    // Lange ID: orch_make_stem hängt `_<index>_<fnv1a-hex>` an → eindeutiger Suffix ist der Hash.
    std::string const suffix = "_" + orch_fnv1a_hex(binary_id);
    for (auto const& e : fs::directory_iterator(tiere_dir_, ec)) {
        if (!e.is_directory()) continue;
        std::string const name = e.path().filename().string();
        if (name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
            if (fs::exists(e.path() / "perm.dll", ec)) return e.path();
        }
    }
    return std::nullopt;
}

std::optional<ShippedArtifact> ShippedArtifactBuilder::build(RankedBinary const& winner, Metric metric,
                                                             fs::path const& source_dir, std::string& error) const {
    std::error_code ec;

    // (REV-DATA-05, WP-5 2026-07-16) Schritt 0a: Namens-Härtung VOR jedem Schreib-Effekt. Nur ein einfacher
    // Allowlist-Stamm ([A-Za-z0-9_-], keine Separatoren/dot/dotdot/reservierte Windows-Namen) ist zulässig —
    // `--name ../../x` konnte zuvor mit overwrite_existing Dateien AUSSERHALB des out_dir überschreiben.
    if (!valid_artifact_stem(artifact_name_)) {
        error = "unzulaessiger Artefaktname (erlaubt: [A-Za-z0-9_-], 1.." + std::to_string(kStemMax) +
                " Zeichen, keine reservierten Windows-Namen): '" + artifact_name_ + "'";
        return std::nullopt;
    }

    // Schritt 1: out_dir anlegen.
    fs::create_directories(out_dir_, ec);
    if (ec) {
        error = "out_dir konnte nicht angelegt werden: " + ec.message();
        return std::nullopt;
    }

    // (REV-DATA-05) Schritt 0b: Containment-Wache (defense-in-depth zur Allowlist): der kanonisierte Zielpfad
    // MUSS direkt unter dem kanonisierten out_dir liegen — sonst Abbruch ohne Schreib-Effekt.
    fs::path const out_canon = fs::weakly_canonical(out_dir_, ec);
    if (ec || out_canon.empty()) {
        error = "out_dir nicht kanonisierbar: " + ec.message();
        return std::nullopt;
    }
    fs::path const target_probe = fs::weakly_canonical(out_canon / (artifact_name_ + ".dll"), ec);
    if (ec || target_probe.parent_path() != out_canon) {
        error = "Zielpfad verlaesst out_dir (Containment-Verletzung): " + target_probe.string();
        return std::nullopt;
    }

    ShippedArtifact art;
    art.binary_id    = winner.binary_id;
    art.metric       = metric_name(metric);
    art.median_value = winner.median_value;
    art.samples      = winner.samples;
    art.source_dll   = source_dir / "perm.dll";

    if (!fs::exists(art.source_dll, ec)) {
        error = "Quell-perm.dll fehlt: " + art.source_dll.string();
        return std::nullopt;
    }

    // (REV-DATA-06, WP-5 2026-07-16) ATOMARER PUBLISH: das komplette Artefaktset (DLL + .version-Sidecar +
    // Manifest) wird ZUERST vollständig in ein temporäres Sibling-Verzeichnis geschrieben und verifiziert;
    // erst danach wird es per fs::rename (atomar je Datei, Manifest ZULETZT) in den out_dir gehoben. Vorher
    // überschrieb Schritt 2 die produktive DLL DIREKT (overwrite_existing) und ein Sidecar-Schreibfehler
    // wurde ignoriert — jeder Fehler nach der DLL-Kopie hinterließ einen Mischstand (neue DLL + altes/leeres
    // Sidecar/Manifest), den die Methode sogar als Erfolg meldete. Jetzt gilt: Fehler VOR dem ersten rename
    // ⇒ vorherige Generation vollständig unverändert; die rename-Reihenfolge DLL → Version → Manifest stellt
    // sicher, dass NIE ein neues Manifest auf eine alte DLL zeigt (Manifest = Commit-Marke).
    fs::path const tmp_dir = out_dir_ / (".tmp_publish_" + artifact_name_);
    fs::remove_all(tmp_dir, ec); // Reste eines früheren abgebrochenen Laufs entfernen (eigener Namensraum)
    fs::create_directories(tmp_dir, ec);
    if (ec) {
        error = "temporaeres Publish-Verzeichnis konnte nicht angelegt werden: " + ec.message();
        return std::nullopt;
    }
    // Best-effort-Aufräumen des tmp-Verzeichnisses auf JEDEM Fehlerpfad.
    auto fail = [&](std::string msg) {
        std::error_code ig;
        fs::remove_all(tmp_dir, ig);
        error = std::move(msg);
        return std::nullopt;
    };

    // Schritt 2 (tmp): DLL-Kopie als benanntes Versand-Artefakt + Größen-Verifikation gegen die Quelle.
    fs::path const tmp_dll = tmp_dir / (artifact_name_ + ".dll");
    fs::copy_file(art.source_dll, tmp_dll, fs::copy_options::overwrite_existing, ec);
    if (ec) return fail("DLL-Kopie fehlgeschlagen: " + ec.message());
    auto const src_size = fs::file_size(art.source_dll, ec);
    if (ec) return fail("Quell-DLL-Groesse nicht lesbar: " + ec.message());
    auto const dst_size = fs::file_size(tmp_dll, ec);
    if (ec || dst_size != src_size)
        return fail("DLL-Kopie unvollstaendig (" + std::to_string(dst_size) + " != " + std::to_string(src_size) +
                    " Bytes)");

    // Schritt 3 (tmp): .version-Sidecar mitliefern (Build-Version der Quelle, z.B. cowfix-v1) — Schreiberfolg
    // wird jetzt GEPRÜFT (vorher wurde ein Sidecar-Fehler still verschluckt und Erfolg gemeldet).
    fs::path const tmp_version = tmp_dir / (artifact_name_ + ".dll.version");
    {
        std::ifstream vf{source_dir / "perm.dll.version", std::ios::binary};
        if (vf) {
            std::ostringstream ss;
            ss << vf.rdbuf();
            art.dll_build_version = ss.str();
        }
        std::ofstream vout{tmp_version, std::ios::binary | std::ios::trunc};
        if (!vout) return fail("Version-Sidecar konnte nicht geoeffnet werden: " + tmp_version.string());
        vout << art.dll_build_version;
        vout.flush();
        if (!vout.good()) return fail("Version-Sidecar-Schreiben fehlgeschlagen: " + tmp_version.string());
    }

    // Zielpfade (kanonischer out_dir-Bestand) — erst NACH vollständigem tmp-Set befüllt.
    art.shipped_dll   = out_dir_ / (artifact_name_ + ".dll");
    art.manifest_path = out_dir_ / (artifact_name_ + ".manifest.txt");

    // Schritt 4 (tmp): Manifest (selbst-beschreibend; ABI-Major/Minor/Magic + Gewinner-Metrik + Herkunft).
    fs::path const tmp_manifest = tmp_dir / (artifact_name_ + ".manifest.txt");
    {
        std::ofstream mf{tmp_manifest, std::ios::trunc};
        if (!mf) return fail("Manifest konnte nicht geschrieben werden");
        mf << "# COMDARE best-binary artifact manifest\n";
        mf << "artifact_name=" << artifact_name_ << "\n";
        mf << "shipped_dll=" << art.shipped_dll.filename().string() << "\n";
        mf << "binary_id=" << art.binary_id << "\n";
        mf << "ranking_metric=" << art.metric << "\n";
        mf << "median_ns=" << art.median_value << "\n";
        mf << "samples=" << art.samples << "\n";
        mf << "source_dir=" << source_dir.string() << "\n";
        mf << "dll_build_version=" << art.dll_build_version << "\n";
        mf << "abi_major=" << kAbiMajor << "\n";
        mf << "abi_minor=" << kAbiMinor << "\n";
        mf << "abi_magic=0x" << std::hex << kAbiMagic << std::dec << "\n";
        mf.flush();
        if (!mf.good()) return fail("Manifest-Flush fehlgeschlagen");
    }

    // Schritt 5 (Commit): atomare renames in den out_dir — DLL → Version → MANIFEST ZULETZT (Commit-Marke:
    // ein sichtbares neues Manifest garantiert, dass DLL + Sidecar bereits die neue Generation sind).
    fs::rename(tmp_dll, art.shipped_dll, ec);
    if (ec) return fail("Publish-rename der DLL fehlgeschlagen: " + ec.message());
    fs::rename(tmp_version, out_dir_ / (artifact_name_ + ".dll.version"), ec);
    if (ec) return fail("Publish-rename des Version-Sidecars fehlgeschlagen: " + ec.message());
    fs::rename(tmp_manifest, art.manifest_path, ec);
    if (ec) return fail("Publish-rename des Manifests fehlgeschlagen: " + ec.message());

    fs::remove_all(tmp_dir, ec); // leeres tmp-Verzeichnis aufräumen (best-effort)
    return art;
}

} // namespace comdare::cache_engine::best_binary
