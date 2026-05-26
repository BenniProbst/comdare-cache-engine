# MODIFICATIONS.md — paper_q01_concurrentqueue

Keine direkten Modifikationen am Original-Code. Lücken-Fueller fuer cache-engine
Buffer-Interface (BufferOriginalCodeMixin braucht 6 Methods, concurrentqueue hat 2):

- emplace:    cache-engine eigene Re-Impl via `enqueue(std::move(T{args...}))`,
              is_original_emplace()=false
- peek_front: cache-engine eigene Re-Impl (concurrentqueue ist async-only kein peek),
              is_original_peek_front()=false
- peek_back:  analog peek_front, is_original_peek_back()=false
- clear:      cache-engine eigene Re-Impl via Drain-Loop, is_original_clear()=false

Original-Source (ext/queuing/Q01-concurrentqueue/concurrentqueue.h) bleibt unangetastet.
