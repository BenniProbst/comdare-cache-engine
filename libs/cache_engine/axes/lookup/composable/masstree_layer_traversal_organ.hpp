#pragma once
// V41 (Task #42-Folge) — MasstreeLayerTraversal-Concept + MasstreeLayerTraversalOrgan (B+Baum-of-Tries-Descent).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @paper P03 Masstree (Mao/Kohler/Morris EuroSys 2012)
//
// Das span-parametrisierte Masstree-Descent-Organ (is_original=false Re-Impl). Single-Thread (Concurrency weg).
//
// **MODELL (Voll-Dekomposition, blueprint-endossiert „der Rest ist immer in weitere Slices zerlegbar"):**
// Der uint64-Key wird in MaxLayers = 8/SliceBytes Slices zerlegt (Layer 0 = hoechstwertiger SliceBytes-Block ->
// numerische == lexikografische Ordnung == std::map-Ordnung, KEIN byteswap noetig). Jeder Layer ist ein
// EIGENER B+Baum (kpermuter-Leaf-Knoten + Internode-Branch), gekeyt mit slice[layer]. Ein Leaf-Slot bei
// layer < MaxLayers-1 ist ein LAYER-Zeiger (keylenx=128 -> Sub-Layer-B+Baum-Wurzel); bei layer == MaxLayers-1
// haelt er den INLINE-Wert (keylenx=8). So entstehen echte Multi-Layer-Tries (make_new_layer real erreichbar,
// nicht toter Code wie bei SliceBytes=8) — die Masstree-DISTINKTION ggue. einem flachen B+Baum.
//
// INVARIANTEN: I1 (SliceBytes=8 == 1 Layer == reiner B+Baum-Degenerationsanker, std::map-test-gleich);
// I2 (MSB-Block-zuerst-Slicing == numerische uint64-Ordnung); I3 (kpermuter-Slots: Insert schiebt nur Nibbles,
// Split rebuildet via make_sorted — Masstree-Praezedenz); I4 (Erase ohne Leerknoten-Kollaps = Folge-Refinement,
// wie ART/START); I5 (Free-List-Slot-Stabilitaet). Wert IMMER am tiefsten Layer (ksuf/Inline-Early-Out = Ink.2).

#include "axis_bound_scratch.hpp" // A8-S5-01b: Pfad-Stack ueber die Allokator-Achse
#include "masstree_layer_pool_concept.hpp"
#include "masstree_layer_pool_store.hpp" // fuer den Selbstbeweis am Dateiende

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace comdare::cache_engine::lookup::composable {

/// MASSTREE-LAYER-TRAVERSAL-Concept: statische insert_into/lookup_in/erase_from auf einem MasstreeLayerNodePool.
template <class T, class Pool>
concept MasstreeLayerTraversal =
    MasstreeLayerNodePool<Pool> &&
    requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
    };

/// B+Baum-of-Tries-Descent-Organ. SliceBytes = Slice-Breite je Layer (1/2/4/8); Default 2 (echte Mehr-Layer).
template <unsigned SliceBytes = 2>
struct MasstreeLayerTraversalOrgan {
    static_assert(SliceBytes >= 1 && SliceBytes <= 8 && (8 % SliceBytes == 0), "SliceBytes in {1,2,4,8}");
    static constexpr unsigned MaxLayers = 8U / SliceBytes;

    /// A8-S5-01b: der Abstiegs-Pfad-Stack des Layer-B+Baums. Seine Tiefe ist die BAUMHOEHE und damit
    /// datenabhaengig -- eine Compile-Time-Kappe gibt es nicht, Form A scheidet hier ehrlich aus. Er laeuft
    /// deshalb ueber das Allokator-Achsen-Interface (Form B). Der Puffer besitzt seine Strategie selbst und
    /// lebt nur fuer die Dauer EINER Einfuege-Operation -- die Organ-Groesse bleibt unveraendert.
    template <class Pool>
    using path_stack_t = AxisBoundBuffer<composition_allocator_t<Pool>, std::pair<std::size_t, int>>;

    [[nodiscard]] static constexpr std::uint64_t slice_mask() noexcept {
        return (SliceBytes == 8U) ? ~std::uint64_t{0} : ((std::uint64_t{1} << (SliceBytes * 8U)) - 1);
    }
    /// slice[layer] = der (MaxLayers-1-layer)-te SliceBytes-Block von k, MSB-Block zuerst (I2).
    [[nodiscard]] static constexpr std::uint64_t slice_at(std::uint64_t k, unsigned layer) noexcept {
        unsigned const shift = (MaxLayers - 1U - layer) * SliceBytes * 8U;
        return (k >> shift) & slice_mask();
    }

    // ── In-Leaf-Binaersuche ueber die sortierte Permutation (ksearch) ──
    template <class Pool>
    [[nodiscard]] static int leaf_lower_bound(Pool const& p, std::size_t leaf, std::uint64_t slice, int sz) {
        int lo = 0, hi = sz;
        while (lo < hi) {
            int const m = (lo + hi) >> 1;
            if (p.leaf_slice_at(leaf, p.leaf_perm_at(leaf, m)) < slice)
                lo = m + 1;
            else
                hi = m;
        }
        return lo;
    }

    // ── Innerhalb EINES Layer-B+Baums zum Leaf absteigen (ohne Stack, fuer lookup/erase) ──
    template <class Pool>
    [[nodiscard]] static std::size_t descend_leaf(Pool const& p, std::size_t root, std::uint64_t slice) {
        std::size_t node = root;
        while (!p.is_leaf(node)) {
            int const n = p.inode_n(node);
            int       i = 0;
            while (i < n && slice >= p.inode_slice_at(node, i)) ++i;
            node = p.inode_child_at(node, i);
        }
        return node;
    }

    // ── FIND-OR-INSERT slice im Layer-B+Baum. Setzt bei Neu nur den SLICE (Caller setzt keylenx/value/layer).
    //    out_leaf/out_phys = finale Slot-Position; was_new; new_root = (evtl. neue) Layer-Wurzel nach Split.
    template <class Pool>
    static void bplus_find_or_insert(Pool& p, std::size_t root, std::uint64_t slice, std::size_t& out_leaf,
                                     int& out_phys, bool& was_new, std::size_t& new_root) {
        new_root = root;
        // A8-S5-01b: der Pfad-Stack ist UNBOUNDED (Hoehe des Layer-B+Baums, datenabhaengig) -> Form B
        // ueber die Allokator-ACHSE. Er wird jetzt per REFERENZ durchgereicht statt per Wert + std::move:
        // die Helfer verbrauchten ihn ohnehin, und der besitzende Puffer bleibt so an GENAU EINER Stelle.
        path_stack_t<Pool> stack{}; // (internode, child-index) auf dem Pfad
        std::size_t        node = root;
        while (!p.is_leaf(node)) {
            int const n = p.inode_n(node);
            int       i = 0;
            while (i < n && slice >= p.inode_slice_at(node, i)) ++i;
            stack.emplace_back(node, i);
            node = p.inode_child_at(node, i);
        }
        std::size_t const lf  = node;
        int const         sz  = p.leaf_size(lf);
        int const         pos = leaf_lower_bound(p, lf, slice, sz);
        if (pos < sz && p.leaf_slice_at(lf, p.leaf_perm_at(lf, pos)) == slice) {
            was_new  = false;
            out_leaf = lf;
            out_phys = p.leaf_perm_at(lf, pos);
            return;
        }
        was_new = true;
        if (sz < Pool::kWidth) { // Platz: nur Nibble-Shift im Permutationswort
            int const ph = p.leaf_alloc_slot(lf);
            p.leaf_set_slice_at(lf, ph, slice);
            p.leaf_perm_insert(lf, pos);
            out_leaf = lf;
            out_phys = ph;
            return;
        }
        // `root` wird hier NICHT mehr weitergereicht: es war bis in propagate_split hinunter tot
        // (Warnungs-Review Runde 2b). Die Zusicherung "new_root ist gesetzt" wird eine Zeile ueber
        // diesem Aufruf gegeben (`new_root = root;` am Kopf von bplus_find_or_insert) und nicht
        // unterwegs durchgereicht -- eine Kette weitergereichter, nie gelesener Parameter macht
        // genau das unauffindbar.
        split_leaf_and_insert(p, lf, slice, stack, out_leaf, out_phys, new_root);
    }

private:
    struct LEntry {
        std::uint64_t slice;
        std::uint8_t  keylenx;
        std::uint64_t value;
        std::size_t   layer;
    };

    template <class Pool>
    static void leaf_load(Pool& p, std::size_t leaf, LEntry const* e, int n) {
        for (int j = 0; j < n; ++j) {
            p.leaf_set_slice_at(leaf, j, e[j].slice);
            p.leaf_set_keylenx_at(leaf, j, e[j].keylenx);
            p.leaf_set_value_at(leaf, j, e[j].value);
            p.leaf_set_layer_at(leaf, j, e[j].layer);
        }
        p.leaf_set_sorted_size(leaf, n); // perm = make_sorted(n): phys j == logical j
    }

    // Voller Leaf (kWidth Eintraege) + neuer slice -> Split (mid = kWidth/2+1), neuer Eintrag landet links/rechts.
    // 09.08.2026 (Warnungs-Runde 2, clang -Wunused-parameter; Klasse MUTANT): hier und in
    // propagate_split wurde `root` durch drei Ebenen gereicht und NIRGENDS gelesen -- ob der Split
    // die Wurzel erreicht, entscheidet allein der leere Pfad-Stack. Der tote Parameter suggerierte
    // eine wurzel-relative Logik, die es nie gab; wer ihn sah, suchte sie. Er faellt weg.
    template <class Pool>
    static void split_leaf_and_insert(Pool& p, std::size_t lf, std::uint64_t slice, path_stack_t<Pool>& stack,
                                      std::size_t& out_leaf, int& out_phys, std::size_t& new_root) {
        constexpr int             W = Pool::kWidth;
        std::array<LEntry, W + 1> tmp{};
        for (int i = 0; i < W; ++i) {
            int const ph = p.leaf_perm_at(lf, i); // sortierte Reihenfolge
            tmp[i]       = LEntry{p.leaf_slice_at(lf, ph), static_cast<std::uint8_t>(p.leaf_keylenx_at(lf, ph)),
                                  p.leaf_value_at(lf, ph), p.leaf_layer_at(lf, ph)};
        }
        // Einfuege-Position fuer den neuen slice (Platzhalter keylenx/value/layer; Caller ueberschreibt).
        int ins = 0;
        while (ins < W && tmp[ins].slice < slice) ++ins;
        for (int i = W; i > ins; --i) tmp[i] = tmp[i - 1];
        tmp[ins] = LEntry{slice, 0, 0, Pool::kNil};

        int const           total       = W + 1;
        int const           mid         = W / 2 + 1; // linke Haelfte behaelt `mid` Eintraege
        std::uint64_t const split_slice = tmp[mid].slice;
        std::size_t const   rt          = p.new_leaf();
        leaf_load(p, lf, tmp.data(), mid);
        leaf_load(p, rt, tmp.data() + mid, total - mid);

        // B-link einfuegen (rt zwischen lf und lf.next)
        std::size_t const old_next = p.leaf_next(lf);
        p.leaf_set_next(rt, old_next);
        p.leaf_set_prev(rt, lf);
        if (old_next != Pool::kNil) p.leaf_set_prev(old_next, rt);
        p.leaf_set_next(lf, rt);

        // finale Slot-Position des neuen Eintrags
        if (ins < mid) {
            out_leaf = lf;
            out_phys = ins;
        } else {
            out_leaf = rt;
            out_phys = ins - mid;
        }

        propagate_split(p, stack, split_slice, lf, rt, new_root);
    }

    // (up_slice, right_node) in den Eltern-Internode einfuegen; bei voll: Internode-Split (Median hoch).
    //
    // TOTER PARAMETER `root` ENTFERNT (Warnungs-Review Runde 2b, 09.08.2026). clang meldete:
    //   masstree_layer_traversal_organ.hpp:197:96: warning: unused parameter 'root' [-Wunused-parameter]
    // Er suggerierte, diese Funktion setze new_root aus root -- tatsaechlich tut das
    // bplus_find_or_insert() ganz oben (`new_root = root;`, eine Ebene ueber split_leaf_and_insert),
    // und zwar VOR jedem Pfad, der hierher fuehrt. Ein Parameter, der eine Zusicherung nur
    // ANDEUTET, ist schlimmer als keiner: er laedt dazu ein, die Vorbedingung hier zu suchen.
    // VERHALTENS-NEUTRAL: private static, ein einziger Aufrufer (split_leaf_and_insert), kein ABI.
    template <class Pool>
    static void propagate_split(Pool& p, path_stack_t<Pool>& stack, std::uint64_t up_slice, std::size_t left_node,
                                std::size_t right_node, std::size_t& new_root) {
        constexpr int W = Pool::kWidth;
        // NACHBEDINGUNG: verlaesst diese Funktion ueber den "es ist Platz"-Zweig, bleibt die Layer-Wurzel
        // unveraendert -- new_root behaelt dann den Wert, den die Deklarationsstelle ihm gegeben hat
        // (`std::size_t new_root = layer_root;`). Die Zusicherung haengt damit an einer INITIALISIERUNG und
        // nicht mehr an einer spaeteren Zuweisung irgendwo im Aufrufpfad.
        //
        // ZWEI WEGE, EINE STELLE (Merge-Befund 09.08.2026): warn-libs wollte `root` als Parameter behalten
        // und hier `new_root = root;` schreiben; warn-tests hat `root` aus der ganzen Kette
        // (bplus_find_or_insert -> split_leaf_and_insert -> propagate_split) entfernt und stattdessen
        // new_root schon bei der Deklaration belegt. Beides heilt dieselbe -Wunused-parameter-Meldung, aber
        // NUR EINES von beidem darf im Baum stehen. Der Automerge nahm die Signaturen des einen und die
        // Rumpfzeile des anderen -- `new_root = root;` ohne `root` im Sichtbarkeitsbereich, ein harter
        // Uebersetzungsfehler, der erst im VOLLBAU auffiel (kein Konfliktmarker, die Stellen liegen 80
        // Zeilen auseinander). Genommen ist warn-tests' Weg: er entfernt einen wirklich toten Parameter aus
        // drei Signaturen UND beseitigt nebenbei eine uninitialisierte Variable (vorher `std::size_t
        // new_root;` ohne Wert). warn-libs' Zuweisung waere an dieser Stelle ein reines No-Op gewesen.
        //
        // GEPRUEFT, ob hier stattdessen ein Vergleich VERGESSEN wurde (z.B. `if (inode == root)`): NEIN.
        // Der Wurzel-Split wird ueber `stack.empty()` erkannt, und der Stack traegt genau den Pfad
        // Wurzel->Blatt; ist er leer, ist die Wurzel ueberschritten. Ein Knoten-Vergleich gegen `root`
        // waere dazu redundant, nicht ergaenzend.
        for (;;) {
            if (stack.empty()) { // Wurzel-Split -> neue Wurzel-Internode
                std::size_t const nr = p.new_internode();
                p.inode_set_n(nr, 1);
                p.inode_set_slice_at(nr, 0, up_slice);
                p.inode_set_child_at(nr, 0, left_node);
                p.inode_set_child_at(nr, 1, right_node);
                new_root = nr;
                return;
            }
            auto const [inode, ci] = stack.back();
            stack.pop_back();
            int const n = p.inode_n(inode);
            if (n < W) { // Platz: einschieben
                for (int j = n; j > ci; --j) p.inode_set_slice_at(inode, j, p.inode_slice_at(inode, j - 1));
                for (int j = n + 1; j > ci + 1; --j) p.inode_set_child_at(inode, j, p.inode_child_at(inode, j - 1));
                p.inode_set_slice_at(inode, ci, up_slice);
                p.inode_set_child_at(inode, ci + 1, right_node);
                p.inode_set_n(inode, n + 1);
                return; // new_root unveraendert -- gesetzt in bplus_find_or_insert (`new_root = root;`)
            }
            // Internode voll: temp mit (n+1) Slices + (n+2) Kindern aufbauen, dann am Median splitten.
            std::array<std::uint64_t, W + 1> sl{};
            std::array<std::size_t, W + 2>   ch{};
            for (int j = 0; j < n; ++j) sl[j] = p.inode_slice_at(inode, j);
            for (int j = 0; j <= n; ++j) ch[j] = p.inode_child_at(inode, j);
            for (int j = n; j > ci; --j) sl[j] = sl[j - 1];
            for (int j = n + 1; j > ci + 1; --j) ch[j] = ch[j - 1];
            sl[ci]                        = up_slice;
            ch[ci + 1]                    = right_node;
            int const           tn        = n + 1;  // == W+1 Slices, W+2 Kinder
            int const           medi      = tn / 2; // Median-Slice-Index
            std::uint64_t const med_slice = sl[medi];
            // linke Haelfte bleibt in `inode`: Slices [0..medi-1], Kinder [0..medi]
            p.inode_set_n(inode, medi);
            for (int j = 0; j < medi; ++j) {
                p.inode_set_slice_at(inode, j, sl[j]);
                p.inode_set_child_at(inode, j, ch[j]);
            }
            p.inode_set_child_at(inode, medi, ch[medi]);
            // rechte Haelfte in neuen Internode: Slices [medi+1..tn-1], Kinder [medi+1..tn]
            std::size_t const ni = p.new_internode();
            int const         rn = tn - medi - 1;
            p.inode_set_n(ni, rn);
            for (int j = 0; j < rn; ++j) {
                p.inode_set_slice_at(ni, j, sl[medi + 1 + j]);
                p.inode_set_child_at(ni, j, ch[medi + 1 + j]);
            }
            p.inode_set_child_at(ni, rn, ch[tn]);
            up_slice   = med_slice;
            left_node  = inode;
            right_node = ni; // weiter nach oben propagieren
        }
    }

public:
    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) p.set_root(p.new_leaf());
        std::size_t layer_root          = p.root();
        bool        holder_is_pool_root = true;
        std::size_t holder_leaf         = NIL;
        int         holder_phys         = 0;

        for (unsigned layer = 0; layer < MaxLayers; ++layer) {
            std::uint64_t const slice = slice_at(k, layer);
            std::size_t         leaf;
            int                 phys;
            bool                was_new;
            // VORBELEGT statt uninitialisiert (Warnungs-Review Runde 2b, 09.08.2026). Der Wert wird von
            // bplus_find_or_insert in seiner ersten Zeile ohnehin ueberschrieben -- aber solange er das
            // NUR dort tut, haengt die Wohldefiniertheit dieser Variablen an der Disziplin einer fremden
            // Funktion. Der Vergleich `new_root != layer_root` zwei Zeilen weiter wuerde sonst auf Muell
            // laufen und die Layer-Wurzel auf einen erfundenen Knoten-Index setzen. layer_root ist die
            // richtige Vorbelegung: "keine neue Wurzel" ist genau der Fall, in dem sie unveraendert bleibt.
            std::size_t new_root = layer_root;
            bplus_find_or_insert(p, layer_root, slice, leaf, phys, was_new, new_root);
            if (new_root != layer_root) {
                if (holder_is_pool_root)
                    p.set_root(new_root);
                else
                    p.leaf_set_layer_at(holder_leaf, holder_phys, new_root);
                layer_root = new_root;
            }
            if (was_new) {
                if (layer + 1U == MaxLayers) {
                    p.leaf_set_keylenx_at(leaf, phys, Pool::kInlineKeylenx);
                    p.leaf_set_value_at(leaf, phys, v);
                    p.leaf_set_layer_at(leaf, phys, NIL);
                    p.inc_size();
                    return;
                }
                std::size_t const sub = p.new_leaf();
                p.leaf_set_keylenx_at(leaf, phys, Pool::kLayerKeylenx);
                p.leaf_set_value_at(leaf, phys, 0);
                p.leaf_set_layer_at(leaf, phys, sub);
                holder_is_pool_root = false;
                holder_leaf         = leaf;
                holder_phys         = phys;
                layer_root          = sub;
            } else {
                if (layer + 1U == MaxLayers) {
                    p.leaf_set_value_at(leaf, phys, v);
                    return;
                } // Update
                layer_root          = p.leaf_layer_at(leaf, phys);
                holder_is_pool_root = false;
                holder_leaf         = leaf;
                holder_phys         = phys;
            }
        }
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) return std::nullopt;
        std::size_t layer_root = p.root();
        for (unsigned layer = 0; layer < MaxLayers; ++layer) {
            std::uint64_t const slice = slice_at(k, layer);
            std::size_t const   leaf  = descend_leaf(p, layer_root, slice);
            int const           sz    = p.leaf_size(leaf);
            int const           pos   = leaf_lower_bound(p, leaf, slice, sz);
            if (!(pos < sz && p.leaf_slice_at(leaf, p.leaf_perm_at(leaf, pos)) == slice)) return std::nullopt;
            int const phys = p.leaf_perm_at(leaf, pos);
            if (layer + 1U == MaxLayers) return p.leaf_value_at(leaf, phys);
            layer_root = p.leaf_layer_at(leaf, phys);
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) return false;
        std::size_t layer_root = p.root();
        for (unsigned layer = 0; layer < MaxLayers; ++layer) {
            std::uint64_t const slice = slice_at(k, layer);
            std::size_t const   leaf  = descend_leaf(p, layer_root, slice);
            int const           sz    = p.leaf_size(leaf);
            int const           pos   = leaf_lower_bound(p, leaf, slice, sz);
            if (!(pos < sz && p.leaf_slice_at(leaf, p.leaf_perm_at(leaf, pos)) == slice)) return false;
            int const phys = p.leaf_perm_at(leaf, pos);
            if (layer + 1U == MaxLayers) {
                p.leaf_perm_remove(leaf, pos);
                p.dec_size();
                return true;
            } // I4: kein Kollaps
            layer_root = p.leaf_layer_at(leaf, phys);
        }
        return false;
    }
};

// Selbstbeweis: das Organ erfuellt das Concept fuer SliceBytes=2 (Mehr-Layer-Default) und SliceBytes=8 (Anker).
static_assert(MasstreeLayerTraversal<MasstreeLayerTraversalOrgan<2>, MasstreeLayerNodePoolStore<>>);
static_assert(MasstreeLayerTraversal<MasstreeLayerTraversalOrgan<8>, MasstreeLayerNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
