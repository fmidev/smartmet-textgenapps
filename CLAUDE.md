# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`textgenapps` produces the `qdtext` binary — a command-line weather forecast text generator for the SmartMet ecosystem. It reads QueryData (FMI's gridded weather data format) and a libconfig-style configuration file, then produces natural-language weather forecast text in multiple languages (Finnish, Swedish, English, and others) using the `smartmet-library-textgen` algorithms.

## Build commands

```bash
make                # Build (produces the 'qdtext' binary in project root)
make clean          # Clean build artifacts
make format         # clang-format the source (main/*.cpp only)
make test           # Run regression tests (cd test && make test)
make rpm            # Build RPM package
```

The build depends on SmartMet libraries (installed headers under `/usr/include/smartmet/`):
- `calculator` — configuration, settings, weather areas
- `textgen` — text generation engine, dictionaries, formatters
- `newbase` — QueryData format, settings, filesystem utilities
- `macgyver` — datetime, exceptions, general utilities

Plus system libraries: Boost (iostreams, locale), fmt, mysql++, GDAL.

## Testing

Tests are regression-based, not unit tests. The test runner is a Perl script:

```bash
cd test && ./test_qdtext    # Or: make -C test test
```

**How it works:** `test/test_qdtext` iterates over configuration files in `test/cnf/` and date-stamped data directories in `test/data/`. For each valid cnf+data combination, it:
1. Substitutes `<OUTDIR>`, `<FORECASTTIME>`, `<DATADIR>` placeholders in the cnf file
2. Runs `../qdtext` (local build) or system-installed `qdtext` against the configured QueryData
3. Compares output files in `tmp/` against expected results in `test/results/`

**Adding new tests:** Add a cnf file in `test/cnf/` or new data, run the tests (they will fail for new cases), then move the new results from `tmp/` to the corresponding `test/results/` directory. The Perl script also must be updated to map new cnf names to their valid data directories (the `$runtest` logic around line 65).

**WGS84 handling:** Results can have `.wgs84` suffix variants for WGS84 projection mode, detected at runtime from `/usr/include/smartmet/newbase/NFmiGlobals.h`.

Tests use file-based dictionaries (`test/dictionaries/`) rather than a database, configured via `test/smartmet.conf` which sets `textgen::filedictionaries = dictionaries`.

## Architecture

The entire application is a single source file: `main/qdtext.cpp`. There is no `source/` or `include/` directory (the Makefile supports them but they are empty/absent).

**Flow:**
1. `main()` → `run()` — reads global settings, parses command line (`NFmiCmdLine`), initializes locale and logging
2. `make_forecasts()` — loads dictionary (file, MySQL, or PostgreSQL), iterates over configured areas
3. For each area: builds a `WeatherArea` (from SVG polygon, PostGIS, or lat/lon), calls `TextGenerator::generate()` to produce a `Document`
4. `save_forecasts()` — formats the document for each configured product (language + formatter combination), collects output
5. `write_forecasts()` — writes output files, with optional encoding conversion (UTF-8 → Latin1) and `fmt::format` time substitution in filenames

**Configuration is hierarchical** (libconfig format via `NFmiSettings`):
- `qdtext::*` — application-level settings (areas, products, output directory, dictionary type, timezone)
- `textgen::*` — library-level settings (data paths, story parameters, section structure)
- Products are defined under `qdtext::product::<name>` with language, formatter, and per-area filenames

**Dictionary backends:** `file` (flat text files for testing), `mysql`/`multimysql`, `postgresql` (production, requires host/user/passwd/database settings under `textgen::*`).

## CI

CircleCI builds and tests on RHEL 8 and RHEL 10 using `fmidev/smartmet-cibase-{8,10}` Docker images with `ci-build` commands (`deps`, `rpm`, `testprep`, `test`).
