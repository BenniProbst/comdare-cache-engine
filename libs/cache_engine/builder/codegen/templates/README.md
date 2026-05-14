# codegen/templates/ — Sample-Module-Body-Templates (REV 7.6 V16.3)

**Anlass:** V15-Final §7 V16.4-TODO — Module-Bodies pro Permutation
benoetigen echte Implementation, nicht nur ABI-konforme Stubs.

V16.3 stellt **Vorlage-Templates** bereit, die in einer Folge-Phase
(V17+) von der `CodegenEngine` optional gegen den Skelett-Body
ausgetauscht werden koennen. Aktuell sind die Templates rein
dokumentarisch — der Codegen verwendet sie noch nicht automatisch.

---

## Vorhandene Templates

| Template | Ziel-Profil | Status | Nutzt Original-Code aus |
|----------|-------------|--------|--------------------------|
| `p01_art_body.hpp.template` | art (P01 Leis 2013) | Sample-Skelett | `ext/P01-ART/unodb` |

---

## Pattern (zukuenftige Templates folgen analog)

```cpp
struct <Algorithm>ModuleBody {
    // 1. Datenstruktur-Member (Trie/B-Tree/HashMap/etc.)
    // 2. Statistik-Counter (atomic fuer thread-safety)
    // 3. Construct/Destruct
    // 4. ABI-konforme run_workload + pull_live_counters
};

// ABI-Glue mit Body-Type-Substitution
extern "C" COMDARE_MODULE_EXPORT comdare_permutation_module_v1 const*
comdare_get_module_v1(void) {
    using Body = ...::ModuleBody;
    static comdare_permutation_module_v1 const k_module = {
        .abi_version = COMDARE_ABI_VERSION,
        .create_instance = +[](auto*) -> void* { return new Body{}; },
        .destroy_instance = +[](void* p) { delete static_cast<Body*>(p); },
        // ... etc
    };
    return &k_module;
}
```

---

## Folge-Phase V17 (TODO)

1. CodegenEngine Erweiterung: erkennt Template-Existenz pro Profile.id
2. Falls Template vorhanden: substituiere im generierten .cpp den Skelett-Body durch Template-#include
3. Folge-Profile mit Templates (Tier-1):
   - hot (P02) → ext/P02-HOT/hot
   - masstree (P03) → ext/P03-Masstree/masstree-beta
   - coco_trie (P04) → ext/P04-CoCo-trie/CoCo-trie
   - start (P05) → ext/P05-START/START
   - b2tree (P06) → ext/P06-B2tree/b2-tree-master
   - wormhole (P07) → ext/P07-Wormhole/wormhole
   - surf (P10) → ext/P10-SuRF/SuRF

---

## Querverweis
- `Diplomarbeit/docs/sessions/20260514-2130-v16-anker.md` §3
- `Diplomarbeit/thesis/chapters/04_implementation.tex` (Codegen-Pipeline)
