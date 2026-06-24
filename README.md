# Beer Label Designer

A wxWidgets desktop application for designing beer bottle labels.

## Features

- **Free placement** — add text and PNG images, drag them anywhere on the label.
- **Text styling** — per-item font size (points), bold, italic, colour, and rotation.
- **Label shapes** — rectangle, rounded rectangle, or ellipse, with a custom
  background colour and editable size in millimetres.
- **Bottle fit guide** — overlays the recommended maximum footprint for a
  **33 cl** or **50 cl** bottle and tells you whether the current label *fits*
  or is *too big*.
- **Save as SVG** — files are valid SVG (open in any viewer / browser) and also
  carry the full editable model in a `<metadata>` block, so they reopen without
  loss. Embedded PNGs are stored as base64 data URIs.
- **Print on A4** — tiles as many copies of the label as fit on an A4 sheet
  (with margin and gap), at the printer's native resolution. Print preview
  included.

Geometry is stored in millimetres throughout, and fonts are sized in pixels
derived from the target resolution, so the on-screen canvas, the SVG export and
the printout all match.

## Build

Requires wxWidgets 3.x, a C++17 compiler and CMake.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/beerlabel
```

## Usage

1. Set the label size, shape and background in the **Label** panel (right).
2. Pick a **Bottle guide** (33 cl / 50 cl) to check the label fits.
3. **Add Text** (Ctrl-T) or **Add Image (PNG)** (Ctrl-I), then drag items on the
   canvas to position them. Edit the selected item in the inspector.
4. **Save** (Ctrl-S) to write an `.svg`, or **Open** (Ctrl-O) to reload one.
5. **Print** (Ctrl-P) to lay out multiple labels on an A4 page.

## Notes / possible next steps

- Bottle-guide dimensions are approximate envelopes (33 cl: 95×95 mm,
  50 cl: 105×120 mm) defined in `src/LabelModel.cpp`; adjust to your bottles.
- Item resize is done numerically in the inspector (drag-resize handles and
  text-on-a-curve are natural follow-ups).
