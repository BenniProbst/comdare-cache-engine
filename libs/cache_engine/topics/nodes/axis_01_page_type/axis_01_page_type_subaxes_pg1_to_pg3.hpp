#pragma once
// V41.F.6.1 F.6 axis_01_page_type Subaxes-Tags

namespace comdare::cache_engine::nodes::axis_01_page_type::subaxes {

// PG1: Struktur-Rolle (branch / leaf / redirect-collapse)
struct structure_role_tag {};

// PG2: Dichte-Klasse (dense / sparse)
struct density_class_tag {};

// PG3: Pfad-Kollaps (none / single-path-collapse)
struct path_collapse_tag {};

}  // namespace
