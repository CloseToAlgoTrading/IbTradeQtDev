# IbTradeQt Plugin System

## Purpose

IbTradeQt now supports an external plugin system for pipeline blocks and future extension points. The design goal is to let the Qt host remain in control of UI, lifecycle, persistence, and runtime wiring while external plugins stay sellable, framework-neutral packages.

This system is additive. It does not replace the existing pipeline architecture or strategy runtime. It extends them through a stable host-managed boundary.

## Design Summary

The plugin system uses two layers:

- External boundary: versioned C ABI with no Qt types.
- Internal host layer: Qt-native wrappers that adapt external plugins into the existing block runtime.

That gives us:

- sellable external packages without Qt API coupling
- existing signal/slot behavior inside the host
- unchanged `pipelineConfig` persistence shape
- unchanged `CPipelineStrategyAdapter`, `PipelineFactory`, and `StrategyPipelineRunner` core behavior

## Current V1 Scope

Version 1 supports strategy extension points:

- `pipeline.selection`
- `pipeline.alpha`
- `pipeline.rebalance`
- `pipeline.risk`
- `pipeline.execution`

The registry and package model are generic so future non-strategy extension points can be added without redesigning the platform.

## Repository Layout

There are now two documentation roots for plugins:

- `docs/PLUGIN_SYSTEM.md`
  - repository-level architecture overview
- `plugins/docs/README.md`
  - plugin authoring and SDK details

Main source locations:

- `Plugin/`
  - host-side discovery, manifest parsing, runtime loading, extension registry, wrappers
- `plugins/api/`
  - public C ABI
- `plugins/sdk/`
  - helper utilities for plugin authors
- `plugins/examples/`
  - buildable example packages, including a strategy suite package and a momentum alpha package
- `plugins/templates/`
  - starter template for a new plugin
- `plugins/runtime/`
  - runtime package discovery folder

## Package Model

Each external plugin is a package directory containing:

- `plugin.json`
- shared library
- optional documentation, license files, and resources

Example:

```text
plugins/runtime/my_plugin/
  plugin.json
  build/
    libmy_plugin.so
```

The host discovers package directories at startup and validates the manifest before activating any extension.

## Manifest Fields

Important manifest fields:

- `manifest_version`
- `plugin_id`
- `name`
- `version`
- `vendor`
- `host_api_version`
- `library`
- `extensions`

Per-extension fields:

- `extension_point_id`
- `extension_id`
- `name`
- `description`
- `scope`
- `default_config`
- optional `supports_async_semantic`

Recommended naming:

- plugin id: reverse-DNS style, for example `com.vendor.plugin`
- extension id: `plugin_id/local-name`

## Host Runtime Flow

At startup the host:

1. computes plugin search directories
2. scans package folders
3. parses `plugin.json`
4. validates manifest and host API compatibility
5. loads the shared library
6. validates the ABI export and required function table entries
7. registers extensions in the generic extension registry
8. maps supported strategy extensions into `Pipeline::BlockRegistry`

If a package fails:

- the app keeps running
- the package is skipped
- the failure is recorded for diagnostics

## ABI Boundary

The external ABI is defined in:

- `plugins/api/ibtrade_plugin_api.h`

Key rules:

- exported entrypoint must be `ibtrade_get_plugin_api_v1`
- ABI version must match the host
- `struct_size` must be valid
- required function pointers must be present

The ABI intentionally avoids:

- `QObject`
- `QString`
- `QJsonObject`
- host private classes

## Qt Host Wrappers

External plugins do not speak Qt directly. The host wraps them in Qt-native adapters that implement the existing pipeline interfaces.

Current wrappers cover:

- selection blocks
- alpha blocks
- rebalance blocks
- risk blocks
- execution blocks

Inside the host, plugin-backed blocks behave like normal blocks from the pipeline’s perspective.

## Persistence and Runtime Compatibility

The plugin system preserves the existing strategy persistence model:

- plugin-backed blocks still live inside `pipelineConfig`
- storage remains `blockId` + `config`
- built-in blocks and plugin blocks coexist in the same registry model

Unknown non-empty plugin block IDs fail closed in the factory. They do not silently fall back to built-in behavior.

## Plugin Manager

The GUI now includes a read-only plugin manager dialog.

Open it from:

- `View > Plugins`
- the toolbar `Plugins` action

It shows:

- loaded plugin packages
- vendor and version
- package root path
- registered extensions
- plugin search directories
- load failures and skip reasons

The manager is intentionally read-only in v1. Discovery happens at startup, so adding or replacing packages requires a restart.

## Building Plugins

Plugin build and packaging instructions live in:

- `plugins/docs/README.md`

That SDK guide covers:

- public ABI rules
- host services
- manifest format
- build commands
- example package build
- runtime installation steps

## Tests

Dedicated coverage exists for:

- manifest validation
- duplicate plugin and extension IDs
- malformed ABI tables
- missing exported entrypoint
- wrapper integration
- pipeline graph creation with plugin-backed blocks
- end-to-end adapter execution
- late async callback safety after destruction

Primary suite:

- `tests/plugin/tst_plugin_runtime.h`

## Limitations in V1

- no hot reload / live enable-disable UI
- no plugin-owned widgets
- no direct plugin-to-plugin Qt communication
- startup-time discovery only

These are deliberate constraints to keep the platform stable and safe while preserving the current application architecture.
