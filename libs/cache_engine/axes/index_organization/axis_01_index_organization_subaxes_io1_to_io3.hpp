#pragma once
// V41.F.6.1.R7.5.h axis_01_index_organization Subaxes-Tags (Clustering)
//
// Klassifizierung gemaess Garcia-Molina/Ullman/Widom "Database Systems" +
// Oracle/SQL Server/PostgreSQL Storage-Organization-Theorie.

namespace comdare::cache_engine::index_organization::subaxes {

// IO1: Storage-Order (heap / clustered / unordered)
struct storage_order_tag {};

// IO2: Index-Count (none / single / multiple)
struct index_count_tag {};

// IO3: Data-Embedding (separate / leaf-embedded)
struct data_embedding_tag {};

}  // namespace
