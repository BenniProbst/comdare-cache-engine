# MODIFICATIONS.md — paper_p07_wormhole

Keine direkten Modifikationen am Original-Code. Luecken-Fueller:
- clear: kein Bulk-Clear im Wormhole-Paper — Re-Impl als Drain-Loop (alle Keys via wh_iter_skip enumeriert + wh_del), is_original_clear()=false
