# Strategy Suite Example Plugin

Build with:

```bash
./build.sh
```

This produces `build/libibtrade_strategy_suite_plugin.so`.

To install it into the host runtime folder during development:

```bash
mkdir -p ../../runtime/strategy_suite_plugin/build
cp plugin.json ../../runtime/strategy_suite_plugin/
cp build/libibtrade_strategy_suite_plugin.so ../../runtime/strategy_suite_plugin/build/
```

Restart the host application, then open `View > Plugins` to verify the package was loaded.
