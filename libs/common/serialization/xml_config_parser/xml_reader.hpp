#pragma once
// xml_reader.hpp — Minimaler, self-contained XML-DOM-Reader (KF-1, 2026-06-02)
//
// Kein tinyxml2 (Offline-Build-Restriktion, FortiGate blockt GitHub; vgl. Boost.MP11-
// Vendoring). Deckt wohlgeformtes UTF-8-XML ab, wie es die Konfig-/Profil-Dateien nutzen:
//   - Elemente mit Attributen, verschachtelte Kinder, Textinhalt
//   - Kommentare <!-- -->, XML-Deklaration <?xml ?>, DOCTYPE <! >, self-closing <tag/>
//   - Basis-Entities (&lt; &gt; &amp; &quot; &apos;)
// BEWUSST NICHT: Namespaces, DTD-Validierung, CDATA, numerische Entities — fuer die
// kontrollierten, selbst erzeugten Profile nicht noetig. Robust genug + ohne Abhaengigkeit.

#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::common::xml {

struct XmlNode {
    std::string tag;
    std::map<std::string, std::string> attrs;
    std::vector<XmlNode> children;
    std::string text;  // direkter Textinhalt (getrimmt)

    [[nodiscard]] XmlNode const* child(std::string_view name) const {
        for (auto const& c : children) if (c.tag == name) return &c;
        return nullptr;
    }
    [[nodiscard]] std::vector<XmlNode const*> children_named(std::string_view name) const {
        std::vector<XmlNode const*> out;
        for (auto const& c : children) if (c.tag == name) out.push_back(&c);
        return out;
    }
    [[nodiscard]] std::string attr(std::string_view key, std::string_view def = "") const {
        auto it = attrs.find(std::string{key});
        return (it == attrs.end()) ? std::string{def} : it->second;
    }
    [[nodiscard]] bool has_attr(std::string_view key) const {
        return attrs.find(std::string{key}) != attrs.end();
    }
    // Whitespace-getrennte Tokens des Textinhalts (z.B. "A B C D E F" / "1 2 4").
    [[nodiscard]] std::vector<std::string> text_tokens() const {
        std::vector<std::string> out;
        std::size_t i = 0;
        while (i < text.size()) {
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            std::size_t start = i;
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            if (i > start) out.push_back(text.substr(start, i - start));
        }
        return out;
    }
};

namespace detail {

inline constexpr auto npos = std::string_view::npos;

inline void decode_entities(std::string& s) {
    struct E { std::string_view from; char to; };
    static constexpr E kEnt[] = {
        {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}, {"&amp;", '&'}};
    for (auto const& e : kEnt) {
        std::size_t pos = 0;
        while ((pos = s.find(e.from, pos)) != std::string::npos) {
            s.replace(pos, e.from.size(), 1, e.to);
            pos += 1;
        }
    }
}

inline std::string trimmed(std::string_view s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return std::string{s.substr(a, b - a)};
}

struct Cursor { std::string_view s; std::size_t i = 0; };

inline bool starts_with(Cursor const& c, std::string_view p) {
    return c.s.substr(c.i, p.size()) == p;
}
inline void skip_ws(Cursor& c) {
    while (c.i < c.s.size() && std::isspace(static_cast<unsigned char>(c.s[c.i]))) ++c.i;
}
// Kommentare / XML-Deklaration / DOCTYPE ueberspringen.
inline void skip_misc(Cursor& c) {
    for (;;) {
        skip_ws(c);
        if (starts_with(c, "<!--")) { auto e = c.s.find("-->", c.i); c.i = (e == npos) ? c.s.size() : e + 3; }
        else if (starts_with(c, "<?")) { auto e = c.s.find("?>", c.i); c.i = (e == npos) ? c.s.size() : e + 2; }
        else if (starts_with(c, "<!")) { auto e = c.s.find('>', c.i); c.i = (e == npos) ? c.s.size() : e + 1; }
        else break;
    }
}
inline std::string read_name(Cursor& c) {
    std::size_t start = c.i;
    while (c.i < c.s.size()) {
        char ch = c.s[c.i];
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == ':' || ch == '.') ++c.i;
        else break;
    }
    return std::string{c.s.substr(start, c.i - start)};
}

// Parst EIN Element ab '<'. Gibt false bei Wohlgeformtheits-Fehler.
inline bool parse_element(Cursor& c, XmlNode& out) {
    skip_misc(c);
    if (c.i >= c.s.size() || c.s[c.i] != '<') return false;
    ++c.i;  // '<'
    out.tag = read_name(c);
    if (out.tag.empty()) return false;

    // Attribute lesen bis '>' oder '/>'.
    for (;;) {
        skip_ws(c);
        if (c.i >= c.s.size()) return false;
        char ch = c.s[c.i];
        if (ch == '/') { if (starts_with(c, "/>")) { c.i += 2; return true; } return false; }
        if (ch == '>') { ++c.i; break; }
        std::string an = read_name(c);
        if (an.empty()) return false;
        skip_ws(c);
        if (c.i >= c.s.size() || c.s[c.i] != '=') return false;
        ++c.i;  // '='
        skip_ws(c);
        if (c.i >= c.s.size()) return false;
        char q = c.s[c.i];
        if (q != '"' && q != '\'') return false;
        ++c.i;
        std::size_t vstart = c.i;
        while (c.i < c.s.size() && c.s[c.i] != q) ++c.i;
        std::string av{c.s.substr(vstart, c.i - vstart)};
        if (c.i < c.s.size()) ++c.i;  // schliessendes Quote
        decode_entities(av);
        out.attrs[an] = std::move(av);
    }

    // Inhalt: Text + Kind-Elemente bis </tag>.
    std::string text;
    for (;;) {
        std::size_t tstart = c.i;
        while (c.i < c.s.size() && c.s[c.i] != '<') ++c.i;
        text.append(c.s.substr(tstart, c.i - tstart));
        if (c.i >= c.s.size()) break;
        if (starts_with(c, "<!--")) { auto e = c.s.find("-->", c.i); c.i = (e == npos) ? c.s.size() : e + 3; continue; }
        if (starts_with(c, "</")) {
            c.i += 2;
            read_name(c);  // close-Name (tolerant; Wohlgeformtheit vorausgesetzt)
            skip_ws(c);
            if (c.i < c.s.size() && c.s[c.i] == '>') ++c.i;
            break;
        }
        XmlNode child;
        if (!parse_element(c, child)) return false;
        out.children.push_back(std::move(child));
    }
    decode_entities(text);
    out.text = trimmed(text);
    return true;
}

}  // namespace detail

// Parst ein XML-Dokument und gibt das Wurzelelement zurueck (nullopt bei Fehler).
[[nodiscard]] inline std::optional<XmlNode> parse_document(std::string_view xml) {
    detail::Cursor c{xml, 0};
    detail::skip_misc(c);
    XmlNode root;
    if (!detail::parse_element(c, root)) return std::nullopt;
    return root;
}

}  // namespace comdare::common::xml
