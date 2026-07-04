# Dataset Loader Schema (AP-10)

This directory contains host-side mechanics for external datasets. It does not register the concrete AP-10 dataset set.

## Dataset-Akte Manifest

A dataset akte is a sidecar manifest serialized as `key=value` lines:

```text
id=<dataset-id>
source_path=<path-used-for-the-source-file>
checksum=<fnv1a64-file-bytes-as-0xhex>
line_count=<number-of-newline-bytes>
preprocessing=<caller-described-preprocessing>
```

`checksum` is FNV-1a 64-bit over the source file bytes. `line_count` counts newline bytes (`\n`). `preprocessing` is a caller-provided descriptor and defaults to `none`.

The implementation is header-only in `include/comdare/measurement/dataset_loader/dataset_akte.hpp`:

- `DatasetAkte { id, source_path, preprocessing, checksum, line_count }`
- `compute_dataset_akte(id, path, preprocessing = "none")`
- `serialize_dataset_akte(akte)`
- `write_dataset_akte(path, akte)`

## String Corpus Loader (Option A)

`include/comdare/measurement/dataset_loader/loaders/string_corpus_loader.hpp` registers loader id `string_corpus`.

The loader reads one string per input line and emits `std::vector<comdare::workload_generator::Operation>` with:

- `op = OperationKind::Read`
- `key_id = FNV-1a 64-bit hash of the line string`
- `scan_length = 0`

This is Option A only: strings are deterministically mapped to the existing `uint64` operation model. Equal strings map to equal keys. The loader does not use time, randomness, or external hash libraries.

## Scope Boundary

The concrete "8 datasets" are externally specified in `Termin 7/Datasets_Spezifikation.txt` and are user-gated. They are not hardcoded here: no dataset files, IDs, checksums, or expected values are registered by this mechanic.

Option B (native String-Keys) is intentionally out of scope because it would change ABI-facing contracts (Doc 32:66). `tier_insert(uint64,uint64)`, `IDriveableTier`, `conformance_gate.hpp` (`std::map<uint64,uint64>`), snapshot PODs, golden IDs, and pipeline16 remain unchanged.