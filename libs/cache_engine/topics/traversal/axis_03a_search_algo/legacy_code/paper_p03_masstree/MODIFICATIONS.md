# MODIFICATIONS.md — paper_p03_masstree

Keine direkten Modifikationen am Original-Code. Template-Adapter-Pattern via
`Masstree::basic_table<table_params>` nach rotaki/masstree-wrapper Gold-Standard:

- table_params extends `Masstree::nodeparams<15, 15>` mit value_type=u64,
  value_print_type, threadinfo_type, key_unparse_type
- Cursor-API: tcursor<P>::find_insert / find_locked, unlocked_tcursor<P>::find_unlocked
- threadinfo pro Thread, lazy thread_local Initialisierung
- Wrapper-Methods rufen finish(+1/-1/0, ti) fuer commit/release

Bei Linker-Build (s4): masstree_glue.cpp erfordert globale Symbole
(active_epoch, globalepoch, recovering) — ein global initialisiertes File pro
Binary.
