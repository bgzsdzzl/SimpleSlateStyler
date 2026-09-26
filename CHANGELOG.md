# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2025-09-26

### Added

- `.SimpleSlateStyler` DSL parser with line and block comments, hex colors,
  and list syntax.
- Automatic discovery of `.SimpleSlateStyler` files under:
  - `<Project>/Config/SimpleSlateStyler/`
  - `<AnyPlugin>/Config/SimpleSlateStyler/`
- Texture and material brush registration into `FAppStyle`.
- Runtime material parameter API (`SetMaterialScalar`, `SetMaterialVector`).
- Scope isolation API:
  - `GetScopeNames`
  - `GetStylesInScope`
  - `RemoveScope`
  - `FindBrush` (scope-aware lookup)
- `scope` keyword for prefixing entry names at the file level.
- Type auto-detection based on the `Resource` path.
- Console commands `SimpleSlateStyler.Reload` and `SimpleSlateStyler.Dump`.
- Directory watcher for hot reload on file save.
- `.github` infrastructure: issue templates, PR template, CI checks, stale bot.
- `Scripts/package_source.py` for building a source distribution zip.