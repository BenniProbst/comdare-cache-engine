#!/bin/sh
# =============================================================================
#  KOEDER-BATTERIE: CWG 1430 in container_framework.hpp
#  scripts/koeder_clang_cwg1430.sh
# =============================================================================
#
# WELCHEN DEFEKT SIE NACHWEIST
# anatomy/container_framework.hpp reichte in type_traits::CompositionFor einen
# Pack T... DIREKT an das Alias-Template Binding::CompositionFor weiter. Die
# realen Bindungen deklarieren dort aber keinen Pack, sondern feste Parameter
# (genus_binding_traits.hpp: T0..T9 + Inner). clang 22.1 lehnt das ab
# ("pack expansion used as argument for non-pack parameter of alias template"),
# g++ 15.3 akzeptiert es. Ursache: CWG 1430 -- ein Alias-Template wird SOFORT
# ersetzt, ein Klassen-Template erst bei der Instanziierung.
#   Heilung:  ce development 2cb0ea34  (Bau-Commit f7535751)
#             container_framework_cwg1430_detail::KompositionFuer
#   Doku:     ce development e2a4cb45  (Bau-Commit 9817a101) -- warum AnatomyFor
#             NICHT mitgezogen wurde; Beleg in koeder_clang_cwg1430_anatomyfor.cpp
#
# WARUM ER SO LANGE LAG -- ZWEI UNABHAENGIGE BLINDHEITEN
#  (a) Der Selbstbeweis DIESES Headers konnte ihn strukturell nie finden: die
#      Mock-Bindung traegt SELBST einen Pack (CompositionFor = void ueber
#      class... T). Pack auf Pack passt. Es braucht eine ECHTE Bindung mit
#      festen Parametern, damit der Fehler ueberhaupt entsteht.
#  (b) g++ meldet ihn nicht.
# Deshalb prueft KC1 unten NICHT den Header allein, sondern baut eine Bindung mit
# festen Parametern nach -- genau die Konstellation aus container_type_traits.hpp.
#
# WIE MAN SIE FAEHRT
#     sh scripts/koeder_clang_cwg1430.sh
# Kein Bau noetig, keine Argumente. Braucht /usr/bin/g++ und clang++ im PATH,
# arbeitet auf Kopien -- der Arbeitsbaum wird NICHT veraendert.
#
# ERWARTETE AUSGABE (Stand 08.08.2026)
#   TEIL 1 (AnatomyFor-Probe):
#     A_PACK_DIREKT     g++ OK      clang FEHLER
#     B_PACK_INDIREKT   g++ OK      clang OK
#     C_MOCK_DIREKT     g++ FEHLER  clang FEHLER
#     D_MOCK_INDIREKT   g++ OK      clang OK
#   TEIL 2:
#     K0       beide BRECHEN mit "K0 WERKZEUG-PROBE"
#     NENNER   beide BAUEN DURCH
#     KC1      g++ BAUT DURCH, clang BRICHT      <-- kein Mangel, s.u.
#     KC2      beide BRECHEN mit "KC2: die Indirektion reicht den Typ ..."
#     KA1      beide BRECHEN mit "pack expansion argument for non-pack parameter"
#
# DREI DINGE, DIE MAN BEIM LESEN WISSEN MUSS
#  * K0 IST DIE WERKZEUG-PROBE und steht bewusst vor allem anderen: sie belegt,
#    dass dieser Pruefaufruf ueberhaupt Fehler SIEHT.
#  * BEI KC1 IST "g++ BAUT DURCH" KEIN MANGEL DES KOEDERS, SONDERN DER BEFUND
#    SELBST. Genau diese Asymmetrie hielt den Defekt latent.
#  * DIE ERSTE FASSUNG DIESER BATTERIE WAR UNTAUGLICH, und das ist die Lehre,
#    die den Nenner rechtfertigt: die Extraktion des Alias aus der echten Datei
#    zog nur die `using`-Zeile und verlor die `template <class... T>`-Kopfzeile.
#    Folge: der NENNER brach an "T was not declared in this scope" -- und damit
#    brach auch K0 aus dem FALSCHEN Grund, sah aber richtig aus. Ein Koeder, der
#    aus dem falschen Grund beisst, ist schlimmer als keiner. Deshalb liest die
#    Extraktion unten BEIDE Zeilen (awk mit prev), und deshalb steht der Nenner
#    fest in der Batterie: er ist die einzige Zeile, die einen kaputten
#    Pruefaufbau von einem echten Befund unterscheidet.
#
# POSIX-sh, ASCII-only.
# =============================================================================
set -u

command -v git >/dev/null 2>&1 || { echo "ABBRUCH: git fehlt" >&2; exit 2; }
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "ABBRUCH: kein git-Repository" >&2; exit 2; }
CF="$REPO_ROOT/libs/cache_engine/anatomy/container_framework.hpp"
PROBE="$REPO_ROOT/scripts/koeder_clang_cwg1430_anatomyfor.cpp"
[ -f "$CF" ] || { echo "ABBRUCH: $CF fehlt" >&2; exit 2; }
[ -f "$PROBE" ] || { echo "ABBRUCH: $PROBE fehlt" >&2; exit 2; }
for CXX in /usr/bin/g++ clang++; do
    command -v "$CXX" >/dev/null 2>&1 || { echo "ABBRUCH: $CXX fehlt -- ein nicht gefahrener Test ist kein bestandener" >&2; exit 2; }
done

ARB=$(mktemp -d) || { echo "ABBRUCH: kein Arbeitsverzeichnis" >&2; exit 2; }
trap 'rm -rf "$ARB"' EXIT INT TERM
ROT=0

# Die Probe: bindet traits an eine Bindung mit FESTEN Parametern (wie GenusBindingTraits).
schreibe_probe() {
    cat > "$ARB/probe.cpp" <<'PEOF'
template <class A, class B, class C = int> struct Komposition {};
template <class Comp> struct Anatomie { using element_type = int; };
// Eine Bindung nach dem Muster von GenusBindingTraits: FESTE Parameter, KEIN Pack.
struct EchteBindung {
    template <class T0, class T1, class Inner = int>
    using CompositionFor = Komposition<T0, T1, Inner>;
    template <class Comp>
    using AnatomyFor = Anatomie<Comp>;
};
#include "traits.hpp"
using Erg = traits<EchteBindung>::CompositionFor<char, long>;
static_assert(sizeof(Erg) > 0, "die Komposition muss bildbar sein");
int main() { return 0; }
PEOF
}

# traits.hpp AUS DER ECHTEN DATEI GELESEN -- so kann die Probe nicht wegdriften.
# BEIDE Zeilen holen (Template-Kopf UND Alias): s. die Lehre im Kopf.
schreibe_traits_aus_der_datei() {
    {
        echo '#pragma once'
        sed -n '/^namespace container_framework_cwg1430_detail {/,/^} \/\/ namespace container_framework_cwg1430_detail/p' "$CF"
        echo 'template <class Binding> struct traits {'
        awk '/using CompositionFor = typename container_framework_cwg1430_detail/ {print prev; print; exit} {prev=$0}' "$CF"
        echo '};'
    } > "$ARB/traits.hpp"
}

lauf() { # $1 = Name
    for CXX in /usr/bin/g++ clang++; do
        AUS=$($CXX -std=c++23 -fsyntax-only -I"$ARB" "$ARB/probe.cpp" 2>&1)
        RC=$?
        KURZ=$(printf '%s' "$AUS" | grep -E 'error:' | head -1 | sed 's/.*error: //' | cut -c1-58)
        if [ $RC -eq 0 ]; then
            printf '  %-8s %-8s BAUT DURCH\n' "$1" "$(basename "$CXX")"
        else
            printf '  %-8s %-8s BRICHT      %s\n' "$1" "$(basename "$CXX")" "$KURZ"
        fi
    done
}

echo "=============================================================================="
echo " TEIL 1 -- WARUM AnatomyFor NICHT mitgeheilt wurde"
echo "          (Probe scripts/koeder_clang_cwg1430_anatomyfor.cpp)"
echo "=============================================================================="
for V in A_PACK_DIREKT B_PACK_INDIREKT C_MOCK_DIREKT D_MOCK_INDIREKT; do
    printf '  %-18s' "$V"
    for CXX in /usr/bin/g++ clang++; do
        printf ' %-8s' "$(basename "$CXX")"
        if $CXX -std=c++23 -fsyntax-only "-D$V" "$PROBE" >/dev/null 2>&1; then printf 'OK      '; else printf 'FEHLER  '; fi
    done
    echo ""
done
echo "  C bricht auf BEIDEN, D nicht mehr: die Indirektion wuerde die fruehe"
echo "  Fehlererkennung des Mock-Blocks aufgeben. Deshalb bleibt AnatomyFor direkt."

echo ""
echo "=============================================================================="
echo " TEIL 2 / K0  WERKZEUG-PROBE: sieht der Pruefaufruf ueberhaupt einen Fehler?"
echo "=============================================================================="
schreibe_probe; schreibe_traits_aus_der_datei
echo 'static_assert(false, "K0 WERKZEUG-PROBE");' >> "$ARB/traits.hpp"
lauf K0

echo ""
echo "=============================================================================="
echo " NENNER: die GEHEILTE Form, AUS DER ECHTEN DATEI GELESEN, baut mit beiden"
echo "=============================================================================="
schreibe_probe; schreibe_traits_aus_der_datei
for CXX in /usr/bin/g++ clang++; do
    if $CXX -std=c++23 -fsyntax-only -I"$ARB" "$ARB/probe.cpp" >/dev/null 2>&1; then
        printf '  %-8s %-8s BAUT DURCH\n' NENNER "$(basename "$CXX")"
    else
        printf '  %-8s %-8s BRICHT   <-- BATTERIE KAPUTT, nicht der Header\n' NENNER "$(basename "$CXX")"
        ROT=1
    fi
done

echo ""
echo "=============================================================================="
echo " KC1  RUECKFALL: das Alias wieder DIREKT auf die Bindung -- der Bestandsfehler."
echo "      ACHTUNG BEIM LESEN: dass g++ hier DURCHBAUT ist KEIN Mangel des Koeders,"
echo "      sondern der Befund selbst -- genau diese Asymmetrie hielt ihn latent."
echo "=============================================================================="
schreibe_probe
{
    echo '#pragma once'
    echo 'template <class Binding> struct traits {'
    echo '    template <class... T> using CompositionFor = typename Binding::template CompositionFor<T...>;'
    echo '};'
} > "$ARB/traits.hpp"
lauf KC1

echo ""
echo "=============================================================================="
echo " KC2  DIE INDIREKTION FAELSCHEN: KompositionFuer liefert einen FREMDEN Typ."
echo "      Beweist, dass die Heilung den Typ wirklich DURCHREICHT und nicht"
echo "      irgendetwas Bildbares liefert, das nur zufaellig uebersetzt."
echo "=============================================================================="
cat > "$ARB/probe.cpp" <<'PEOF'
template <class A, class B, class C = int> struct Komposition {};
struct Fremd {};
struct EchteBindung {
    template <class T0, class T1, class Inner = int>
    using CompositionFor = Komposition<T0, T1, Inner>;
};
#include "traits.hpp"
using Erg = traits<EchteBindung>::CompositionFor<char, long>;
static_assert(__is_same(Erg, Komposition<char, long, int>),
              "KC2: die Indirektion reicht den Typ der Bindung NICHT unveraendert durch.");
int main() { return 0; }
PEOF
{
    echo '#pragma once'
    echo 'namespace d { template <class B, class... T> struct KompositionFuer { using type = Fremd; }; }'
    echo 'template <class Binding> struct traits {'
    echo '    template <class... T> using CompositionFor = typename d::KompositionFuer<Binding, T...>::type;'
    echo '};'
} > "$ARB/traits.hpp"
lauf KC2

echo ""
echo "=============================================================================="
echo " KA1  AM ECHTEN HEADER: AnatomyFor chirurgisch auf einen PACK gedreht."
echo "      Belegt, dass ein solcher Umbau NICHT still durchrutscht -- er bricht"
echo "      auf BEIDEN Compilern, und genau darauf beruht der Verzicht auf die"
echo "      Indirektion an jener Stelle."
echo "=============================================================================="
L=$(grep -n 'using AnatomyFor = typename Binding::template AnatomyFor<Comp>;' "$CF" | head -1 | cut -d: -f1)
if [ -z "$L" ]; then
    echo "  KA1  UEBERSPRUNGEN -- die AnatomyFor-Zeile wurde nicht gefunden."
    echo "       Das ist ROT: entweder wurde sie umbenannt oder sie ist bereits auf"
    echo "       die Indirektion umgestellt. In beiden Faellen gehoert dieser Koeder"
    echo "       nachgezogen, statt still zu verschwinden."
    ROT=1
else
    awk -v l="$L" 'NR==l-1 {print "    template <class... Comp>"; next} NR==l {print "    using AnatomyFor = typename Binding::template AnatomyFor<Comp...>;"; next} {print}' "$CF" > "$ARB/ka1.hpp"
    INC="-I$REPO_ROOT/libs/cache_engine -I$REPO_ROOT/libs/cache_engine/include -I$REPO_ROOT/libs/cache_engine/src -I$REPO_ROOT/libs/common -I$REPO_ROOT/cmake/third_party/boost_mp11/include"
    for CXX in /usr/bin/g++ clang++; do
        # shellcheck disable=SC2086
        AUS=$($CXX -std=c++23 -fsyntax-only -Wno-pragma-once-outside-header $INC -x c++ "$ARB/ka1.hpp" 2>&1)
        if [ $? -eq 0 ]; then
            printf '  %-8s %-8s BAUT DURCH  <-- der Pack-Umbau rutscht durch, AnatomyFor braucht die Indirektion\n' KA1 "$(basename "$CXX")"
            ROT=1
        else
            printf '  %-8s %-8s BRICHT      %s\n' KA1 "$(basename "$CXX")" "$(printf '%s' "$AUS" | grep -m1 'error:' | sed 's/.*error: //' | cut -c1-52)"
        fi
    done
fi

echo ""
if [ "$ROT" -eq 0 ]; then
    echo "KOEDER-BATTERIE: GRUEN -- K0 meldet, der Nenner baut durch, alle Koeder beissen."
    exit 0
fi
echo "KOEDER-BATTERIE: ROT -- s. die markierten Zeilen oben."
exit 1
