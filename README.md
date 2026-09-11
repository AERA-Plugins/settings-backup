# AERA Settings Backup

Minimal Host API 2 example plugin. It proves that a new plugin ID can install
and run without adding a recovery scene. AERA renders its UI and performs the
approved settings backup or restore; the isolated worker never receives direct
storage access.

The protocol uses fixed-size `SOCK_SEQPACKET` messages on file descriptor 4.
See `source/main.c` for the complete lifecycle and operation example.

## Build

Compile `source/main.c` as a statically linked ARM64 executable named
`stage/usr/bin/aera-plugin`, then create the bounded runtime:

```sh
python3 source/pack.py stage build
```

Copy the values from `build/metadata.json` into `plugin.json`. Package an
unsigned local-test bundle with the registry tooling:

```sh
python3 ../AERA-plugin-registry/scripts/package_aerap.py \
  --manifest plugin.json \
  --payload build/runtime.xz \
  --output build/AERA-Settings-Backup-0.1.0.aerap
```

An unsigned package is deliberately shown under **Unofficial Apps** and
requires the warning confirmation. Signing and store publication are reserved
for reviewed AERA-Plugins releases.
