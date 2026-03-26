# Strategy Plugin Template

This directory is the minimal starting point for an external IbTrade plugin package.

## Files

- `plugin.cpp`
  - C ABI implementation.
- `plugin.json.template`
  - Manifest template. Rename to `plugin.json`.
- `build.sh`
  - Minimal Linux build helper.

## Start a New Plugin

1. Copy this directory.
2. Rename `plugin.json.template` to `plugin.json`.
3. Replace IDs, names, vendor, and library file names.
4. Implement `create_extension`, `initialize_instance`, and `invoke_json`.
5. Run `./build.sh`.
6. Copy the package directory into `plugins/runtime/<your_package>/`.

## Expected Export

Your shared library must export:

```cpp
IBTRADE_PLUGIN_EXPORT const ibtrade_plugin_api_v1* ibtrade_get_plugin_api_v1(void)
```

## Reminder

Keep the external contract free of Qt types. The host will wrap your plugin in Qt-native adapters internally.
