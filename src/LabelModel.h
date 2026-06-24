#pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <memory>
#include <vector>

// All geometry is stored in millimetres relative to the top-left of the label.
// Rendering converts mm -> device pixels via a caller supplied pixels-per-mm
// scale so that the on-screen canvas, the SVG export and the printer output
// all share the same coordinate model.

enum class ItemType { Text, Image };

// Outline shape of the label itself.
enum class LabelShape { Rectangle, RoundedRect, Ellipse };

class LabelItem {
public:
    virtual ~LabelItem() = default;
    virtual ItemType Type() const = 0;
    virtual std::unique_ptr<LabelItem> Clone() const = 0;

    // Bounding box of the item in millimetres.
    virtual wxRect2DDouble BoundsMM() const = 0;

    // Draw onto a device context. originPx is the device pixel position of the
    // label's (0,0) corner; pxPerMM converts millimetres to device pixels.
    virtual void Draw(wxDC& dc, const wxPoint& originPx, double pxPerMM) const = 0;

    double x = 0.0;   // mm
    double y = 0.0;   // mm
    double rotationDeg = 0.0;
    bool selected = false;
};

class TextItem : public LabelItem {
public:
    ItemType Type() const override { return ItemType::Text; }
    std::unique_ptr<LabelItem> Clone() const override;
    wxRect2DDouble BoundsMM() const override;
    void Draw(wxDC& dc, const wxPoint& originPx, double pxPerMM) const override;

    // Build the wxFont sized for a given pixels-per-mm scale. Point sizes are
    // converted to a pixel height so output is resolution independent.
    wxFont MakeFont(double pxPerMM) const;

    wxString text = "Beer";
    double fontSizePt = 18.0;
    bool bold = false;
    bool italic = false;
    wxColour colour = *wxBLACK;
    wxString faceName;   // empty => default family

private:
    // Approximate text extent in mm (x=width, y=height), via a memory DC.
    wxRealPoint MeasureMM(double pxPerMM) const;
};

class ImageItem : public LabelItem {
public:
    ItemType Type() const override { return ItemType::Image; }
    std::unique_ptr<LabelItem> Clone() const override;
    wxRect2DDouble BoundsMM() const override;
    void Draw(wxDC& dc, const wxPoint& originPx, double pxPerMM) const override;

    bool LoadFromFile(const wxString& path);

    wxImage image;             // source image (RGB/RGBA)
    std::vector<unsigned char> pngBytes;  // original PNG bytes, for SVG embedding
    double widthMM = 30.0;
    double heightMM = 30.0;
};

class Label {
public:
    // Recommended maximum label footprint for common bottle sizes (mm).
    // Approximate front/wrap label envelopes; editable by the user.
    struct BottlePreset { const char* name; double maxW; double maxH; };
    static const BottlePreset Bottle33cl;
    static const BottlePreset Bottle50cl;

    double widthMM = 90.0;
    double heightMM = 90.0;
    LabelShape shape = LabelShape::RoundedRect;
    wxColour background = *wxWHITE;
    std::vector<std::unique_ptr<LabelItem>> items;

    Label() = default;
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    std::unique_ptr<Label> Clone() const;

    // Draw the label outline/background. Item drawing is done by the caller.
    void DrawBackground(wxDC& dc, const wxPoint& originPx, double pxPerMM) const;

    LabelItem* HitTest(double mmX, double mmY) const;
    void ClearSelection();
};
