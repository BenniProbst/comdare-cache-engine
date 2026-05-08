# tools/socks5_zih_delivery — SOCKS5-Tunnel-Lieferung ans ZIH (F-EXTRA-3)

**Default-Lieferpfad** fuer Cross-Compile-Binaries an ZIH (Barnard / Capella /
Grace Hopper). SSH+rsync (Variante A) ist auf ZIH gesperrt — SOCKS5 ist
einziger zulaessiger Tunnel.

## Status: Skelett (Phase 4.B)

Tooling existiert bereits in der bestehenden Infrastruktur unter:
`C:\Users\benja\OneDrive\Desktop\Projekte\Cluster\sessions\`

Phase 4.B Detail: bestehende Skripte hierher migrieren oder als Submodul einbinden.

## Erwartete Schnittstelle (zu implementieren)

```bash
./socks5_deliver.sh \
  --binary build/cache_engine_builder \
  --target zih:barnard \
  --workspace /home/s2631336/comdare-cache-engine/
```

## Konfiguration

- ZIH-Login: `s2631336`
- VPN-Cert: `Z:\Dokumente\Uni Dresden\*.ovpn` (A-Tunnel)
- ZIH-Workspace: `/home/s2631336/` oder `/projects/p_llm_compile/`
