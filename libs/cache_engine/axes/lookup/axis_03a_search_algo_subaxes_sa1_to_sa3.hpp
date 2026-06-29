#pragma once
// V41.F.6.1 axis_03a_search_algo Subaxis-Tags SA1-SA3 (2026-05-26)
//
// @topic traversal @achse 03a
//
// Klassifikation der Such-Algorithmen (analog Q1-Subaxes QS1-QS6).
// Inspiriert von prt-art-Legacy Interpreter-Typen (DenseByte/Patricia/Multilevel).

namespace comdare::cache_engine::lookup::subaxes {

/// SA1 dense — direkt-adressiert, hohe Fuelldichte (ART Node256, Array256SearchAlgo)
struct dense_access_tag {};

/// SA2 sparse — Patricia-comprimiert, niedrige Fuelldichte (HOT k-constrained)
struct sparse_access_tag {};

/// SA3 multilevel — mehrbyteig, multi-discriminator (START, Cost-DP)
struct multilevel_access_tag {};

/// SA4 direct_multibyte — direkt-adressiert mit Multi-Byte (uint16) Diskriminator,
/// mittlere Fuelldichte 25-50 % (prt-art Array65535, REV 6 §5.17). Teilt das
/// Direkt-Adressierungs-Zugriffsmuster mit SA1, aber bei uint16-Granularitaet und
/// mittlerer (statt hoher) Density — daher eigene Subaxis.
struct direct_multibyte_access_tag {};

} // namespace comdare::cache_engine::lookup::subaxes
