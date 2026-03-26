# IbTrade External Plugin SDK

This tree contains the external plugin surface for IbTrade. The goal is to let us ship and sell plugins independently while keeping the host application in control of Qt UI, lifecycle, persistence, and runtime wiring.

## Architecture

The plugin system has two layers:

- External plugin boundary: plain C ABI, no Qt types, no host private headers.
- Host integration layer: Qt-native wrappers inside the app that adapt external plugins into the existing pipeline runtime.

This means:

- External plugins do not link against Qt just to satisfy the host contract.
- The host still communicates internally through Qt objects, signals, and slots.
- Existing pipeline JSON, `CPipelineStrategyAdapter`, `PipelineFactory`, and block registry behavior stay intact.

## Directory Layout

- `api/`
  - Stable, versioned C ABI headers.
- `sdk/`
  - Tiny helper utilities for plugin authors.
- `examples/`
  - Buildable example packages.
- `templates/`
  - Starter files for a new plugin package.
- `docs/`
  - This documentation.
- `runtime/`
  - Local runtime install folder used by the host during discovery.

## Supported Extension Points in V1

Version 1 supports strategy pipeline extensions first:

- `pipeline.selection`
- `pipeline.alpha`
- `pipeline.rebalance`
- `pipeline.risk`
- `pipeline.execution`

The registry and manifest model are generic, so later non-strategy extension points can reuse the same platform without redesigning the package format.

## Package Format

Each plugin is a self-contained package directory.

Example:

```text
plugins/runtime/my_plugin/
  plugin.json
  build/
    libmy_plugin.so
  LICENSE.txt
  README.md
```

The host loads a package directory, not just a bare shared library.

## Manifest Format

The package must contain `plugin.json`.

Required top-level fields:

- `manifest_version`
- `plugin_id`
- `name`
- `version`
- `description`
- `vendor`
- `host_api_version`
- `library`
- `extensions`

Example:

```json
{
  "manifest_version": 1,
  "plugin_id": "com.vendor.myplugin",
  "name": "My Strategy Plugin",
  "version": "1.0.0",
  "description": "External strategy package",
  "vendor": "Vendor",
  "host_api_version": 1,
  "library": "build/libmy_plugin.so",
  "extensions": [
    {
      "extension_point_id": "pipeline.alpha",
      "extension_id": "com.vendor.myplugin/alpha",
      "name": "My Alpha",
      "description": "Example alpha block",
      "scope": "Strategy",
      "default_config": {
        "fixed_probability": 0.7,
        "fixed_amount": 10.0
      },
      "supports_async_semantic": false
    }
  ]
}
```

Notes:

- `plugin_id` should be globally unique, preferably reverse-DNS style.
- `extension_id` should also be globally unique. The recommended format is `plugin_id/local-name`.
- `scope` currently accepts `Strategy` or `Portfolio`.
- `default_config` is what the host surfaces in block creation/edit flows.

## ABI Contract

The public ABI is defined in `api/ibtrade_plugin_api.h`.

Important rules:

- The exported symbol must be `ibtrade_get_plugin_api_v1`.
- `abi_version` must match the host.
- `struct_size` must match the published table layout.
- Required function pointers must be non-null:
  - `create_extension`
  - `destroy_extension`
  - `invoke_json`

Optional functions are supported by the host when present:

- `initialize`
- `shutdown`
- `set_config_json`
- `get_state_json`

## Host Services Available to Plugins

The host exposes a narrow service table through `ibtrade_host_services_v1`.

Current services:

- current time lookup
- market data lookup
- historical bar lookup
- holdings lookup
- subscription request submission
- order placement
- cancel all pending orders
- structured logging
- event emission back to the host wrapper

Plugins must not cache host internals beyond the published service table.

## Build a Plugin

### Quick Start from the template

Copy:

```text
plugins/templates/strategy_plugin_template
```

Rename:

- `plugin.json.template` to `plugin.json`
- library output names inside `build.sh`
- IDs and metadata inside `plugin.json`

Then implement the C ABI in `plugin.cpp`.

### Linux build example

The simplest build path uses `g++`.

```bash
cd plugins/templates/strategy_plugin_template
./build.sh
```

That script currently compiles:

```bash
g++ -std=c++17 -fPIC -shared \
  plugin.cpp \
  -I../../api \
  -I../../sdk \
  -o build/libyour_strategy_plugin.so
```

### Build the bundled example

```bash
cd plugins/examples/strategy_suite_plugin
./build.sh
```

Expected output:

```text
plugins/examples/strategy_suite_plugin/build/libibtrade_strategy_suite_plugin.so
```

Momentum-only example:

```bash
cd plugins/examples/momentum_alpha_plugin
./build.sh
```

Expected output:

```text
plugins/examples/momentum_alpha_plugin/build/libibtrade_momentum_alpha_plugin.so
```

## Install a Plugin for the Host

The application searches these runtime locations at startup:

- `<app dir>/plugins/runtime`
- `<app dir>/../plugins/runtime`
- `<current working dir>/plugins/runtime`

The easiest development workflow in this repository is:

```bash
mkdir -p plugins/runtime/strategy_suite_plugin
cp plugins/examples/strategy_suite_plugin/plugin.json plugins/runtime/strategy_suite_plugin/
mkdir -p plugins/runtime/strategy_suite_plugin/build
cp plugins/examples/strategy_suite_plugin/build/libibtrade_strategy_suite_plugin.so \
  plugins/runtime/strategy_suite_plugin/build/
```

Then restart the application.

## How the Host Uses Plugin Blocks

When a package loads successfully:

- the manifest is parsed and validated
- ABI compatibility is checked
- extensions are registered in the generic extension registry
- strategy extensions are mapped into the existing `BlockRegistry`
- plugin-backed blocks appear in the same block selection flows as built-ins

Persistence stays unchanged:

- the same `pipelineConfig` JSON structure is used
- plugin-backed blocks are stored through `blockId` + `config`

## Plugin Manager in the App

The host now includes a read-only plugin manager dialog.

Open it from:

- `View > Plugins`
- the left toolbar `Plugins` action

The dialog shows:

- loaded plugin packages
- package vendor/version/root directory
- extensions inside each loaded package
- configured plugin search directories
- load failures with the reason a package was skipped

Current behavior:

- discovery happens at startup
- the manager is an inspection tool, not a live hot-reload controller
- after adding or replacing plugin packages, restart the host

## Validation and Failure Behavior

The loader currently rejects:

- missing manifest fields
- unsupported manifest version
- unsupported host API version
- duplicate plugin IDs
- duplicate extension IDs
- duplicate block IDs already present in the host registry
- missing exported entrypoint
- malformed/truncated ABI tables
- missing required ABI function pointers

If a plugin fails validation:

- the package is skipped
- the host keeps running
- the failure is visible in logs and in the plugin manager

## Important Runtime Semantics

- Unknown non-empty plugin block IDs fail closed in `PipelineFactory`; they do not silently fall back to built-in blocks.
- Plugin libraries are kept resident for safety in v1.
- Late callbacks after instance destruction are ignored safely by the host wrapper.
- Plugins communicate with the host through callbacks and JSON payloads; the host translates these into Qt-native events internally.

## Troubleshooting

If your plugin does not appear in the host:

1. Confirm the package contains `plugin.json`.
2. Confirm `library` in the manifest points to a real shared library path relative to the package root.
3. Confirm the library exports `ibtrade_get_plugin_api_v1`.
4. Confirm `host_api_version` matches `IBTRADE_PLUGIN_ABI_VERSION_V1`.
5. Open `View > Plugins` and inspect the failure list.
6. Check the application log for `plugin.loader` and `plugin.wrappers` messages.

If your plugin loads but does not execute:

1. Confirm the `extension_id` used in `pipelineConfig` exactly matches the manifest.
2. Confirm the extension point matches the block type you are adding.
3. Confirm your `invoke_json` implementation handles the operation names expected by the wrapper.

## Repository References

- Public ABI: `plugins/api/ibtrade_plugin_api.h`
- SDK helpers: `plugins/sdk/PluginSdk.hpp`
- Example package: `plugins/examples/strategy_suite_plugin`
- Starter template: `plugins/templates/strategy_plugin_template`
