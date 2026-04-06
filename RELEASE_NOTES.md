# ASAP Fork Release Notes

## Annotation Hotkeys

| Key | Action | Description |
|-----|--------|-------------|
| **H** | Toggle visibility | Show/hide all annotations on the current slide. Press again to restore. |
| **E** | Delete selected | Delete all currently selected annotations. Select annotations by clicking them (Ctrl+click for multi-select). |
| **C** | Change color | Opens a color picker dialog to change the color of all selected annotations. |

> **Note:** The Delete key still removes the last point while drawing a polygon, and Shift+Delete cancels the current annotation being drawn. The `E` key replaces the previous `D` key for deleting finished annotations to avoid conflicts during drawing.

## Annotation Tool Options

New settings have been added to the annotation options dialog (**Set options for annotation tools**):

| Setting | Default | Description |
|---------|---------|-------------|
| **Simplify polygons on finish** | Enabled | When enabled, freehand polygon and spline annotations are automatically simplified using the Douglas-Peucker algorithm when finished. This reduces unnecessary points on straight segments while preserving curve detail. |
| **Simplification epsilon** | 2.0 | Controls how aggressively points are simplified, in image pixels. Lower values retain more detail (more points), higher values produce simpler shapes (fewer points). Range: 0.5 - 50.0. |

### How freehand simplification works

When drawing a polygon or spline by dragging the mouse:
1. Points are collected at short intervals (5px on screen) for accurate curve capture
2. On finish, the Douglas-Peucker algorithm removes redundant points on straight segments
3. The result is a clean annotation with few points on straight edges and dense points along curves

This behavior only applies to **Polygon** and **Spline** annotation types. Rectangle, Dot, Pointset, and Measurement annotations are unaffected.

To disable simplification entirely, uncheck **Simplify polygons on finish** in the options dialog.

## Annotation Undo/Redo

Undo and redo support has been added for annotation operations. Use **Ctrl+Z** to undo and **Ctrl+Shift+Z** to redo.

### Supported operations

| Operation | Undo | Description |
|-----------|------|-------------|
| **Move vertex** | Yes | Dragging a control point to a new position. Consecutive drags on the same point are merged into a single undo step. |
| **Insert vertex** | Yes | Double-clicking on an edge to insert a new control point. |
| **Remove vertex** | Yes | Deleting a control point from a finished annotation (Del key). |
| **Create annotation** | Yes | Creating a new annotation is undoable — the annotation is removed from the list and hidden. |
| **Delete annotation** | Yes | Deleting an annotation (E key) is undoable — the annotation is restored with its full state. |

### Notes

- Undo history is cleared when loading a new image or pressing the Clear button.
- The undo stack limit is 100 operations.
- Vertex operations during annotation creation (before finishing) are not undoable — use Del to remove the last point or Esc to cancel the entire annotation.
- Undo/Redo shortcuts can be customized in **Settings > General** tab of the annotation options dialog.
