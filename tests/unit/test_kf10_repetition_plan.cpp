// test_kf10_repetition_plan — KF-10 (2026-06-02)
// Belegt: jede Experiment-Einstellung wird N-mal (Default 3) SEPARAT ausgeführt + dokumentiert, NIE interpoliert.
// repetition_index je Wiederholung; CSV = eine Zeile je Wiederholung (kein Mittelwert).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf10_repetition_plan.cpp

#include "builder/experiment_tree/repetition_plan.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}
static std::size_t count_substr(std::string const& h, std::string const& n) {
    std::size_t c = 0, p = 0;
    while ((p = h.find(n, p)) != std::string::npos) {
        ++c;
        p += n.size();
    }
    return c;
}

int main() {
    std::cout << "KF-10: Validierungs-Wiederholungen (Default 3, separat, nie interpoliert):\n";

    // Default = 3 Wiederholungen
    ex::RepetitionPlan plan{};
    check_eq("Default-Wiederholungen", plan.repetitions(), std::uint32_t{3});

    // Jede Wiederholung liefert ein DISTINKTES rohes Ergebnis (10,11,12) — NICHT gemittelt.
    auto recs = plan.run("traversal=ART/node=N4/concurrency.thread_count=2",
                         [](std::uint32_t r) { return std::pair<double, std::uint64_t>{10.0 + r, 1000 + r}; });
    check_eq("3 separate Datensätze", recs.size(), std::size_t{3});
    check_eq("repetition_index[0]", recs[0].repetition_index, std::uint32_t{0});
    check_eq("repetition_index[2]", recs[2].repetition_index, std::uint32_t{2});
    check_true("rohe Werte DISTINKT (10/11/12, NICHT auf 11 gemittelt)",
               recs[0].micros_per_op == 10.0 && recs[1].micros_per_op == 11.0 && recs[2].micros_per_op == 12.0);
    check_true("setting_id je Datensatz erhalten",
               recs[0].setting_id == "traversal=ART/node=N4/concurrency.thread_count=2");

    // CSV: repetition_index-Spalte + EINE Zeile je Wiederholung (separat).
    std::string csv = ex::RepetitionPlan::to_csv(recs);
    check_true("CSV-Header hat repetition_index-Spalte", csv.find("repetition_index") != std::string::npos);
    check_eq("CSV: 3 Datenzeilen (je Wiederholung eine)", count_substr(csv, "traversal=ART"), std::size_t{3});
    check_true("CSV enthält rohen Wert 10.000000 (nicht interpoliert)", csv.find("10.000000") != std::string::npos);
    check_true("CSV enthält rohen Wert 12.000000 (nicht interpoliert)", csv.find("12.000000") != std::string::npos);

    // Konfigurierbar
    check_eq("konfigurierbar: 5 Wiederholungen", ex::RepetitionPlan{5}.repetitions(), std::uint32_t{5});
    check_eq("0 → auf 1 normalisiert (mind. ein Lauf)", ex::RepetitionPlan{0}.repetitions(), std::uint32_t{1});
    check_eq("5 Wiederholungen → 5 Datensätze",
             ex::RepetitionPlan{5}
                 .run("s", [](std::uint32_t r) { return std::pair<double, std::uint64_t>{double(r), r}; })
                 .size(),
             std::size_t{5});

    std::cout << "\n==== KF-10 Wiederholungen separat + nie interpoliert: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
