# Pinned third-party dependencies

These official release source archives are included so that local and CI builds use the same dependency contents without downloading them during CMake configuration. The SHA256 values are enforced in `CMakeLists.txt` and recorded in `dependencies.json`.

| Library | Version | License | Official release |
|---|---|---|---|
| doctest | 2.5.3 | MIT | https://github.com/doctest/doctest/releases/tag/v2.5.3 |
| Google Benchmark | 1.9.5 | Apache-2.0 | https://github.com/google/benchmark/releases/tag/v1.9.5 |

The source archives are unmodified. License texts were copied from the corresponding archives to `doctest-LICENSE.txt` and `benchmark-LICENSE.txt` for convenient inspection; the originals remain inside the archives.

To update a dependency, obtain its official release archive, review compatibility, update its hash and metadata, retain its license, and rerun the complete local verification. Do not replace an archive while leaving the old checksum or provenance in place.
