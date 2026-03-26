# Momentum Alpha Example Plugin

This example package implements an external momentum alpha block that behaves close to the built-in `MomentumAlphaBlock`.

Build with:

```bash
./build.sh
```

To install it into the local runtime folder during development:

```bash
mkdir -p ../../runtime/momentum_alpha_plugin/build
cp plugin.json ../../runtime/momentum_alpha_plugin/
cp build/libibtrade_momentum_alpha_plugin.so ../../runtime/momentum_alpha_plugin/build/
```

Restart the host application, then open `View > Plugins` to confirm the package was loaded.
