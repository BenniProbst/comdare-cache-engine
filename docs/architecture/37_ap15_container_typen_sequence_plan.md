# 37 — AP-15 Container-Typen-Plan: Sequence + weitere std-Container (Planung)

**Status:** PLANUNG (kein Code-Increment). Grundlage für #29-Folgearbeit.
**Bezug:** #29 (comdare::container Kopf-Framework, Schritt 1 vollzogen: `libs/cache_engine/anatomy/container_framework.hpp`), User-Plan AP-15 (2026-07-07), Memory `project_ap15_container_gattung_comdare_container_sequence_plan`.
**Invarianz:** rein additiv, golden/ABI-neutral. Dieses Dokument nimmt KEINE Realisierung vorweg; es legt die Taxonomie-Optionen offen und empfiehlt. Anatomie-Enum/GenusBindingTraits/golden_fullpilot_320/permutation_axes bleiben unberührt, bis ein konkreter Realisierungs-Increment mit eigener Kadenz (+ ggf. User-GO bei ABI-Berührung) folgt.

## 1. Ist-Taxonomie (3-Ebenen-Modell, `anatomy_base.hpp`)

- **Ebene 1 — AnatomyGattung (3):** `SearchAlgorithm` | `Container` | `Graph`. Außen-Interface / Prüf-Dock.
- **Ebene 2 — AnatomyGenus (Tier-Unterklasse):** unter `Container` liegen `Set` (Vogel) / `Sequence` (Reptil) / `Adapter` (Wirbelloses) / `View` (Pflanze). `SearchAlgorithm` ist eigene Gattung mit einer Tier-Unterklasse (19 Achsen, Säugetier).
- **Ebene 3 — Achsen:** je Genus ein fester Achsen-Satz (Adapter 13 / Set 15 / Sequence 11 / View 7 Slots).

`comdare::container` (Schritt 1) präsentiert die 4 Container-Genus als compile-time-`type_list` unter EINEM Außen-Interface (`type_count == 4`), re-exportiert die bestehenden `GenusBindingTraits<G>` je Typ — ohne Umbau.

Tier-Tabelle (aus `anatomy_base.hpp`, autoritativ):

| Tier | Genus (Ebene 2) | Gattung | std::-Realisierungen (Ebene-3-Kandidaten) |
|------|-----------------|---------|-------------------------------------------|
| Säugetier | SearchAlgorithm | SearchAlgorithm | `map`, `multimap`, `unordered_map` |
| Vogel | Set | Container | `set`, `multiset`, `unordered_set`, `unordered_multiset` |
| Reptil | **Sequence** | Container | **`vector`, `list`, `deque`, `array`, `forward_list`** |
| Wirbelloses | Adapter | Container | `stack`, `queue`, `priority_queue` |
| Pflanze | View | Container | `span`, `mdspan`, `string_view` |

## 2. AP-15 (2) — Sequence: Genus, nicht eigene Gattung (KLARSTELLUNG)

Der User-Plan: „Gattung Sequence NUR in der Planung anlegen; der TYP Sequence (äquivalent zu `std::vector`) real UNTER den Containern."

**Befund:** Diese Klarstellung ist im Ist-Modell bereits erfüllt und bleibt so:
- `Sequence` ist eine **Tier-Unterklasse (Genus, Ebene 2) UNTER der Container-Gattung** — **keine** eigene Ebene-1-Gattung. Eine „Gattung Sequence" wird bewusst NICHT angelegt (verhindert Gattungs-Inflation; nur `SearchAlgorithm`/`Container`/`Graph` sind Gattungen).
- Der **vector-äquivalente Typ ist real** über `ISequenceTier` (`sequence_tier.hpp`: `push_back(value)` / `at(index)→value`, V-indexiert, eigener `SequenceObserverSnapshotV1`-POD). Das ist die „std::vector-äquivalente" Realisierung des Sequence-Genus.

⇒ AP-15 (2) ist **taxonomisch abgeschlossen** (Sequence = Genus unter Container, vector-äquivalent real). Kein weiterer Code nötig.

## 3. AP-15 (3) — Weitere std-Container-Typen: der Taxonomie-Fork

Der User-Plan: „linked list + übrige std-Container = Container-TYPEN, KEINE eigenen Gattungen, da sie Elemente speichern und freigeben können (sehr ähnlich → gleiches Interface)."

Der Begriff „Container-TYP" ist präzisierungsbedürftig, weil zwei Ebenen infrage kommen. **Fork (echter Architektur-Entscheid, NICHT geraten):**

- **Option A — Ebene-3-Realisierungen unter den bestehenden 4 Genus** *(anatomy_base.hpp-Tabelle)*: `list`/`deque`/`array`/`forward_list` sind Realisierungen UNTER dem `Sequence`-Genus (per backing-structure-Achse oder je eigene Composition); `unordered_set` unter `Set`; usw. → `comdare::container::type_count` bleibt **4** (Ebene 2 unverändert), golden/ABI unberührt; Unterschiede (z.B. `vector` O(1)-`at` vs. `list` O(n)) werden Achsen-/Composition-Varianten.
- **Option B — je eigener Ebene-2-Genus** *(container_framework.hpp Z.48 „als Genus gebunden")*: `list` etc. werden neue Tier-Unterklassen → `type_count` wächst additiv. Erfordert je neuen Genus eigene `GenusBindingTraits`-Bindung; ABI/golden nur berührt, falls sie in Permutation/binary_id eingehen (dann User-GO).

**Interface-Nuance (entscheidungsrelevant):** `ISequenceTier` setzt `at(index)→value` voraus. `vector`/`deque`/`array` erfüllen indizierten O(1)/amortisiert-Zugriff; `list`/`forward_list` NICHT effizient (O(n)) und `forward_list` hat kein `size()`. Ein gemeinsames Sequence-Interface für alle wäre also entweder (a) auf die gemeinsame Teilmenge (`push`/iterate) reduziert oder (b) je Realisierung ein Achsen-Flag „random_access: yes/no".

**Empfehlung:** **Option A** — Ebene-3-Realisierungen unter den bestehenden Genus, mit einer `backing_structure`-Achse im Sequence-Genus (`vector`|`deque`|`list`|`forward_list`|`array`) + einem `random_access`-Merkmal. Begründung: (i) deckt sich mit der autoritativen `anatomy_base.hpp`-Tabelle; (ii) hält „gleiches Interface" (User-Wort) wörtlich — gleiches Genus = gleiches Außen-Interface; (iii) vermeidet Gattungs-/Genus-Inflation (Memory-Warnung); (iv) golden/ABI-neutral, solange die neue Achse default-OFF/additiv am Ende eingehängt wird (S22/A24-Präzedenz). Option B bleibt möglich, falls der User die std-Container als eigenständige Tier-Unterklassen erforschen will (dann eigener ABI/golden-GO).

## 4. Roadmap (je eigener Kadenz-Increment, nach User-Wahl A/B)

1. **Sequence-Realisierungen** (nach Option A): `backing_structure`-Achse additiv am Sequence-Genus (default `vector` = bit-identisch zum Ist) → dann `list`/`deque`/`array`/`forward_list` als Achsen-Werte + `random_access`-Merkmal; je Realisierung ein Unit-Test nach `test_ap7*`-Vorlage.
2. **Set-Realisierungen:** `multiset`/`unordered_set`/`unordered_multiset` analog unter `Set`-Genus.
3. **Adapter/View:** `stack`/`queue`/`priority_queue` bzw. `span`/`mdspan`/`string_view` — die Adapter sind Wrapper über ein Inner-Substrat (Sequence/Set), Views sind non-owning (kein Freigabe-Organ → Achsen-Satz kleiner).
4. Je Increment: golden-Neutralität empirisch (default-Pfad bit-identisch), `type_count`-Invariante prüfen, conformance-Oracle unberührt.

## 5. Tabus (unverändert)

Kein neuer Ebene-1-Gattungs-Eintrag außer via expliziter User-GO. Anatomie-Enum-Reihenfolge, `GenusBindingTraits`, `golden_fullpilot_320_binary_ids.txt`, `permutation_axes.xml`, POD sizeof==1416, conformance-Oracle bleiben unberührt, bis ein Realisierungs-Increment sie nachweislich golden-neutral (default-OFF-End-Append) berührt. Metaprogrammierung compile-time (Concepts/`if constexpr`/mp11), kein runtime-switch.
