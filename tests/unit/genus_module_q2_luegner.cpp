// Review #15 Fix 1 (A2.5, 18.08.2026) -- DAS LUEGNER-MODUL: der Koeder, der den Identitaets-Riegel
// (status_identity_mismatch, Schloss 11) am ECHTEN dlopen-Ladeweg AUSLOEST.
//
// WOZU ES EXISTIEREN MUSS (K13): der namensgebende Riegel des Q2/V-06-Bruchs wurde bis zu diesem
// Fixture von KEINEM Test ausgeloest -- ein invertierter oder toter Riegel waere bei allen Tests
// gruen geblieben. Dieses Modul ist die reproduzierbare Quelle des Widerspruchs, den der Riegel
// abweisen soll: das Symbol comdare_anatomy_genus behauptet SEQUENCE, die eigene Factory liefert
// eine SET-Instanz. Beide Antworten sind je fuer sich gueltig (bekannte, ABI-sichtbare Genera,
// Gattung konsistent zur Behauptung abgeleitet) -- NUR der Widerspruch zwischen ihnen ist der
// Defekt. Genau ein Schloss faellt, alle anderen halten (Mutations-Ehrlichkeit wie bei den
// Alt-Fixtures: echte Magic, echte Version, echter SetAbiAdapter, kein nullptr-Stub).
//
// DIE ZAEHLER (comdare_q2_testonly_*_count): der Fehlerpfad des Loaders MUSS die schon gebaute
// Instanz via comdare_destroy_anatomy freigeben, BEVOR er das Modul entlaedt (destroy-vor-dlclose,
// ErwerbsGuard A-F4). Der Test haelt das Modul ueber ein eigenes dlopen resident und liest die
// Zaehler NACH dem Loader-Lauf: create==1 und destroy==1 belegen den Vollzug prozess-lokal --
// unter ASan/LSan zusaetzlich dadurch gedeckt, dass eine vergessene Freigabe als Leak schluege.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Export-Makro + Magic/Major/Minor + IAnatomyBase
#include <anatomy/set_abi_adapter.hpp>                     // SetAbiAdapter
#include <anatomy/set_anatomy.hpp>                         // SetAnatomy / SetComposition
#include <anatomy/set_default_organ.hpp>                   // SortedArrayKeySet

#include <cstdint>
#include <new>

namespace {

/// Die BEHAUPTETE Identitaet -- Sequence. Bekannt (genus_bekannt), ABI-sichtbar
/// (ist_abi_sichtbares_genus), Gattung unten konsistent daraus abgeleitet: die Behauptung passiert
/// jede Wertklassen-Wache und faellt erst am Konsistenz-Riegel gegen die Instanz.
inline constexpr auto kBehaupteterGenus = ::comdare::cache_engine::anatomy::AnatomyGenus::Sequence;

using LuegnerComposition =
    ::comdare::cache_engine::anatomy::SetComposition<::comdare::cache_engine::anatomy::SortedArrayKeySet, int, int, int,
                                                     int, int, int, int, int, int, int, int, int>;

std::uint64_t create_count  = 0; // wie oft die Factory WIRKLICH lief
std::uint64_t destroy_count = 0; // wie oft destroy WIRKLICH lief (destroy-vor-dlclose-Beleg)

} // namespace

/// HEUTIGE Version + HEUTIGE Magic: dieses Modul soll Schloss 1-4 PASSIEREN und erst am Riegel
/// fallen -- sonst pruefte die Probe irgendein Schloss, nur nicht das gemeinte.
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {
    return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |
           static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {
    return COMDARE_ANATOMY_ABI_MAGIC;
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*
comdare_create_anatomy() noexcept {
    ++create_count;
    using AnatomyType = ::comdare::cache_engine::anatomy::SetAnatomy<LuegnerComposition>;
    return new (::std::nothrow)::comdare::cache_engine::anatomy::SetAbiAdapter<AnatomyType>{}; // genus() == Set
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT void
comdare_destroy_anatomy(::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {
    if (ptr != nullptr) ++destroy_count;
    delete ptr;
}

/// DIE LUEGE: das Symbol behauptet Sequence, die Factory oben liefert Set. Die Gattung wird
/// KONSISTENT zur Behauptung abgeleitet (gattung_of(Sequence) == Container) -- damit faellt das
/// Modul ausschliesslich an der Instanz-gegen-Symbol-Haelfte des Riegels, nicht an der
/// Gattungs-Ableitung und nicht an einer Wertklassen-Wache.
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_gattung() noexcept {
    return static_cast<std::uint8_t>(::comdare::cache_engine::anatomy::gattung_of(kBehaupteterGenus));
}
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_genus() noexcept {
    return static_cast<std::uint8_t>(kBehaupteterGenus);
}

/// Test-Sonden (KEINE ABI-Pflicht-Symbole; der Loader kennt sie nicht): der Test liest sie ueber
/// sein eigenes dlopen-Handle, waehrend das Modul resident bleibt.
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_q2_testonly_create_count() noexcept { return create_count; }
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_q2_testonly_destroy_count() noexcept {
    return destroy_count;
}
