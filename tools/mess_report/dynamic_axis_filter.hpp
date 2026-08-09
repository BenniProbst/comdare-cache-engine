#pragma once
// tools/mess_report/dynamic_axis_filter.hpp -- A9-S4: die "CoR-Filterkette fuer dynamische
// Variablen" (A9-Doc Abschnitt 4.3), die entscheidet, welche Unter-Achsen-Spalten in den
// Dateinamen wandern und welche als Meta-Eintrag im INFO-Blatt landen.
//
// Doktrin (A9-Doc Abschnitt 5): "Name = NUR Datum + Uhrzeit + dynamische Unter-Achsen-Variablen;
// ... nie sich aendernde Variablen werden WEGGELASSEN (Meta-Eintrag im INFO-Blatt)." Und: "wert :=
// [a-z0-9._-]+ (sanitisiert; 'sweep' wenn die Variable im File selbst ueber Sheets laeuft)".
//
// Drei Ausgaenge, drei Handler -- cold-path virtueller Dispatch, Haus-Stil wie
// builder/experiment_tree/selection_filter_chain.hpp::FilterHandler (dieselbe
// dispatch()/set_next()-Form, dieselbe Pass-to-next-Semantik):
//   1. FehlendWertHandler: die Spalte ist im Kandidaten-Satz vorhanden, aber in JEDER Zeile leer
//      -> Konstant("-") (nichts Sinnvolles zu zeigen, aber die Spalte EXISTIERT -- ehrliches "-"
//      statt eines stillen Verschwindens, "n/a statt Null"-Doktrin).
//   2. EinWertHandler: genau EIN nicht-leerer Wert kommt in der gesamten Gruppe vor -> Konstant mit
//      diesem Wert (WEGGELASSEN aus dem Dateinamen NUR wenn der Aufrufer entscheidet, s.u.; die
//      Kette selbst liefert den Wert, die Weglass-Politik ist Aufrufer-Sache je Achse).
//   3. MehrWerteHandler: mehr als ein nicht-leerer Wert -> Dynamisch ("sweep" im Dateinamen).
//
// Die Kandidaten-LISTE (welche Spalten ueberhaupt durch die Kette laufen) ist NICHT Teil dieser
// Datei -- das waere eine Achsen-Registry-Klassifikation, die A9-S4 nicht besitzt. Der Aufrufer
// (mess_report_render.hpp) uebergibt die Spalten, die er als Unter-Achsen-Kandidaten kennt.

#include <string>
#include <vector>

namespace comdare::cache_engine::tools::mess_report {

struct DynamikVerdikt {
    enum class Entscheidung { Konstant, Dynamisch, WeiterInDerKette };
    Entscheidung entscheidung = Entscheidung::WeiterInDerKette;
    std::string  wert; // bei Konstant: der EINE Wert (oder "-" wenn ueberall leer)
    std::string  begruendung;
};

/// EIN Handler der Dynamik-Kette.
class DynamikHandler {
public:
    DynamikHandler()                                 = default;
    DynamikHandler(DynamikHandler const&)            = default;
    DynamikHandler& operator=(DynamikHandler const&) = default;
    virtual ~DynamikHandler()                        = default;

    void set_next(DynamikHandler const* n) noexcept { next_ = n; }

    [[nodiscard]] DynamikVerdikt dispatch(std::string const& spalte, std::vector<std::string> const& werte) const {
        DynamikVerdikt const v = handle(spalte, werte);
        if (v.entscheidung == DynamikVerdikt::Entscheidung::WeiterInDerKette && next_ != nullptr)
            return next_->dispatch(spalte, werte);
        return v;
    }

protected:
    [[nodiscard]] virtual DynamikVerdikt handle(std::string const&              spalte,
                                                std::vector<std::string> const& werte) const = 0;

private:
    DynamikHandler const* next_ = nullptr;
};

namespace detail {
/// Zaehlt die DISTINKTEN nicht-leeren Werte in `werte` (Reihenfolge irrelevant, Kappe bei 2 genuegt
/// den drei Handlern -- keiner braucht mehr als "0 / genau 1 / mehr als 1").
[[nodiscard]] inline std::vector<std::string> distinkte_nichtleere_werte(std::vector<std::string> const& werte) {
    std::vector<std::string> out;
    for (std::string const& w : werte) {
        if (w.empty() || w == "-") continue;
        bool schon_da = false;
        for (std::string const& o : out)
            if (o == w) {
                schon_da = true;
                break;
            }
        if (!schon_da) {
            out.push_back(w);
            if (out.size() > 1) return out; // frueher Ausstieg: mehr als 1 reicht schon
        }
    }
    return out;
}
} // namespace detail

class FehlendWertHandler final : public DynamikHandler {
protected:
    [[nodiscard]] DynamikVerdikt handle(std::string const&, std::vector<std::string> const& werte) const override {
        if (detail::distinkte_nichtleere_werte(werte).empty())
            return {DynamikVerdikt::Entscheidung::Konstant, "-", "Spalte ist in jeder Zeile leer"};
        return {DynamikVerdikt::Entscheidung::WeiterInDerKette, {}, {}};
    }
};

class EinWertHandler final : public DynamikHandler {
protected:
    [[nodiscard]] DynamikVerdikt handle(std::string const&, std::vector<std::string> const& werte) const override {
        auto const distinkt = detail::distinkte_nichtleere_werte(werte);
        if (distinkt.size() == 1)
            return {DynamikVerdikt::Entscheidung::Konstant, distinkt.front(), "genau ein Wert in der Gruppe"};
        return {DynamikVerdikt::Entscheidung::WeiterInDerKette, {}, {}};
    }
};

class MehrWerteHandler final : public DynamikHandler {
protected:
    [[nodiscard]] DynamikVerdikt handle(std::string const&, std::vector<std::string> const&) const override {
        // Letztes Glied der Kette: alles, was die vorigen zwei Handler nicht abschliessend
        // entschieden haben, hat per Konstruktion mehr als einen distinkten Wert.
        return {DynamikVerdikt::Entscheidung::Dynamisch, "sweep", "mehr als ein Wert in der Gruppe"};
    }
};

/// Baut die Standard-Kette (Reihenfolge bindend: leer -> ein Wert -> mehrere Werte) und verdrahtet
/// sie. `halter` haelt die drei Handler-Objekte am Leben, solange die zurueckgegebene Referenz auf
/// den Kettenkopf benutzt wird (RAII-Kette, Muster wie run_selection_filter_chain's ordered-Vektor).
struct DynamikKette {
    FehlendWertHandler leer;
    EinWertHandler     ein_wert;
    MehrWerteHandler   mehr_werte;

    DynamikKette() {
        leer.set_next(&ein_wert);
        ein_wert.set_next(&mehr_werte);
        // mehr_werte ist das Kettenende (kein set_next noetig, next_ bleibt nullptr).
    }

    [[nodiscard]] DynamikVerdikt bewerte(std::string const& spalte, std::vector<std::string> const& werte) const {
        return leer.dispatch(spalte, werte);
    }
};

} // namespace comdare::cache_engine::tools::mess_report
