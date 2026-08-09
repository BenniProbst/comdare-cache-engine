// ck1_naht_voll.cpp -- BELEG-UEBERSETZUNGSEINHEIT A: VOLLE Mess-Konfiguration.
//
// Diese Datei und ck1_naht_leer.cpp unterscheiden sich in GENAU EINER ZEILE (der using-Zeile unten).
// Der Symbolvergleich der beiden Objekte ist damit eine Aussage ueber die KONFIGURATION und nicht
// ueber den Quelltext. ASCII-only.

#include "ck1_naht_koerper.hpp"

namespace {
using DieseKonfiguration = ::ck1::mk::Voll; // <== DIE EINE ZEILE
} // namespace

// Nicht static, nicht inline: der Aufruf MUSS als Symbol im Objekt stehen, sonst prueft der
// Vergleich nichts.
std::uint64_t ck1_fahren_voll(std::uint64_t saat) noexcept { return ::ck1::koerper_fahren<DieseKonfiguration>(saat); }
