# prt_art/legacy_reimpl — Re-Implementations-Skelette (Phase 4.B)
#
# REV 7.6 V8.2 (User-Direktive 2026-05-13/14):
#   "Derzeit gibt es faelschlicherweise einen PRT-ART Ordner direkt in der
#    CacheEngine, aber der PRT-ART sollte mit neuen Layered Algorithmus-
#    Baustein-Bestandteilen und einer separaten eigenen Konfiguration im
#    PRT-ART repo als Pruefling dargestellt werden."
#
# DEPRECATED — gestaffelte Migration:
#   1. (heute V8.2) Default OFF via COMDARE_BUILD_LEGACY_REIMPL — verhindert,
#      dass diese Re-Implementations-Skelette versehentlich gebaut werden.
#   2. (Folgesession) Skelette werden nach comdare-prt-art/prt_art/legacy_reimpl/
#      verschoben.
#   3. (Folge-Folge) prt_art/-Ordner aus cache-engine entfernt.

if(COMDARE_BUILD_LEGACY_REIMPL)
    message(STATUS "[DEPRECATED] Building prt_art/legacy_reimpl/ — wird in Folgesession nach prt-art Repo verschoben.")
    add_subdirectory(P11-CSS-tree)
    add_subdirectory(P12-CSB-tree)
    add_subdirectory(P13-Hankins)
    add_subdirectory(P14-Samuel)
    add_subdirectory(P16-Bender-TreeLayout)
    add_subdirectory(P17-Bender-CacheOblivious)
    add_subdirectory(P18-Saikkonen-MultiLevel)
    add_subdirectory(P19-Saikkonen-LayoutInvariant)
    add_subdirectory(P21-Chen-PrefetchBPlus)
    add_subdirectory(P22-Chen-Fractal)
    add_subdirectory(P23-Khan-AdaptivePrefetch)
    add_subdirectory(P24-NaderanTahan)
    add_subdirectory(P26-Zhang-FGCS)
    add_subdirectory(P27-Zhang-ASPLOS-Hierarchical)
else()
    message(STATUS "[V8.2] prt_art/legacy_reimpl/ skipped (COMDARE_BUILD_LEGACY_REIMPL=OFF).")
endif()
