# MODIFICATIONS.md — paper_p10_surf

Keine direkten Modifikationen am Original-Code. Luecken-Fueller (SuRF ist Read-Only):
- insert: kein incremental Insert — Re-Impl als Rebuild-Trigger, is_original_insert()=false
- erase:  kein Erase im Read-Only-Index — Re-Impl als Tombstone-Set, is_original_erase()=false
- clear:  kein Clear — Re-Impl als Bitmap-Reset, is_original_clear()=false
