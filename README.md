# Qt Resource Dumper

An LD_PRELOAD library that dumps the Qt resource filesystem of any Qt application at runtime.

## Purpose

Qt applications can embed resources (QML files, images, configs, etc.) directly into the executable or shared libraries via `qt_add_resources()` or `qt_add_qml_module()`. This tool inspects what resources are actually embedded by hooking into the Qt application startup and dumping the resource tree.

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/lib/cmake
cmake --build build
```

This produces `build/libqt_resource_dumper.so`.

## Usage

### Basic dump (entire resource filesystem)

```bash
LD_PRELOAD=/path/to/libqt_resource_dumper.so QT_DUMP_RESOURCES=1 ./your_qt_app
```

### Dump with content inline

Prints the full content of text files (.qml, .json, .qss, .xml, .txt, .js, .qmldir, .conf, .ini):

```bash
LD_PRELOAD=/path/to/libqt_resource_dumper.so \
  QT_DUMP_RESOURCES=1 \
  QT_DUMP_RESOURCES_CONTENT=1 \
  ./your_qt_app
```

### Filter to a specific resource path

```bash
LD_PRELOAD=/path/to/libqt_resource_dumper.so \
  QT_DUMP_RESOURCES=1 \
  QT_DUMP_RESOURCE_PATH=":/qt/qml/Com/Example" \
  ./your_qt_app
```

Accepts multiple path formats:
- `:/qt/qml/My.Module`
- `qrc:/qt/qml/My.Module`
- `qt/qml/My.Module` (auto-normalized to `:/qt/qml/My.Module`)

### Headless/offscreen applications

For Qt apps that need an offscreen platform:

```bash
LD_PRELOAD=/path/to/libqt_resource_dumper.so \
  QT_DUMP_RESOURCES=1 \
  QT_QPA_PLATFORM=offscreen \
  ./your_qt_app
```

## Environment Variables

| Variable | Values | Behavior |
|----------|--------|----------|
| `QT_DUMP_RESOURCES` | `1` (default) | Dump on first event loop tick (fires after `engine.loadFromModule()` completes, so catches all dynamically-loaded QML modules) |
| | `startup` | Dump at `QCoreApplication` construction (early; only sees statically-linked resources) |
| | `exit` | Dump at `QCoreApplication::aboutToQuit` (requires clean Qt shutdown; SIGKILL will not trigger) |
| `QT_DUMP_RESOURCES_CONTENT` | `1` | Also print contents of text files (.qml, .qmldir, .json, .js, .qss, .xml, .txt, .conf, .ini) |
| `QT_DUMP_RESOURCE_PATH` | resource path | Filter dump to only this path and its children (e.g., `:/qt/qml/Com/Example`) |

## How It Works

The library uses `Q_COREAPP_STARTUP_FUNCTION` to register a callback that fires immediately after `QCoreApplication` is constructed. This hook then either:

1. **Immediately dumps** (mode: `startup`) — useful for debugging statically-linked resources
2. **Schedules a dump** via `QTimer::singleShot(0)` (mode: `1`) — waits for the first event loop tick, after all synchronous QML engine initialization is complete
3. **Connects to aboutToQuit** (mode: `exit`) — dumps just before clean shutdown

The default mode (`singleShot(0)`) is ideal because Qt applications typically call `engine.loadFromModule()` or `engine.load()` **synchronously** in `main()` before entering the event loop. This means all QML module libraries (which embed QML resources) are loaded by the time the timer fires, unlike the `startup` mode which runs too early.

## Example Output

```
╔════════════════════════════════════════════════════════╗
║              Qt Resource Filesystem Dump               ║
║  Timing: first event loop tick (all QML modules loaded)║
║  Path: :/qt/qml/Com/Example                            ║
╚════════════════════════════════════════════════════════╝
[DIR]  :/qt/qml/Com/Example/App
  [FILE] :/qt/qml/Com/Example/App/Main.qml  (469 bytes)
  [FILE] :/qt/qml/Com/Example/App/qmldir  (120 bytes)
[DIR]  :/qt/qml/Com/Example/Tasks
  [FILE] :/qt/qml/Com/Example/Tasks/TaskDelegate.qml  (2784 bytes)
  [FILE] :/qt/qml/Com/Example/Tasks/TaskListView.qml  (1137 bytes)
  [FILE] :/qt/qml/Com/Example/Tasks/qmldir  (260 bytes)
──────────────────────────────────────────────────
```

## Requirements

- Qt 6.0 or later
- CMake 3.21+
- Linux (LD_PRELOAD mechanism)

## License

GPLv3 (GPL-3.0-only) — see [LICENSE.txt](LICENSE.txt) or [LICENSES/GPL-3.0-only.txt](LICENSES/GPL-3.0-only.txt)
