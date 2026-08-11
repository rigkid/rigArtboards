# Surface

![preview](img/preview.png)

A4 artboard, grid, paths, selection gizmo. Wheel zoom, middle-drag pan.

Surface docks into the central dockspace on first run (`setDockLayoutBuilder`).
Layout uses style padding / frame metrics, `uiPx` / `uiHitSize`, and `GetColorU32`
theme colors. Selection bounds use `plot::boundsOfPath` over `selectedRefs()`.

```bash
cmake -S packs/rigArtboards/examples/surface -B packs/rigArtboards/examples/surface/build
cmake --build packs/rigArtboards/examples/surface/build
```
