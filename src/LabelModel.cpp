#include "LabelModel.h"
#include <wx/dcmemory.h>
#include <wx/mstream.h>
#include <wx/file.h>

const Label::BottlePreset Label::Bottle33cl{"33 cl", 95.0, 95.0};
const Label::BottlePreset Label::Bottle50cl{"50 cl", 105.0, 120.0};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static double MMtoPx(double mm, double pxPerMM) { return mm * pxPerMM; }

// ---------------------------------------------------------------------------
// TextItem
// ---------------------------------------------------------------------------
std::unique_ptr<LabelItem> TextItem::Clone() const {
    return std::make_unique<TextItem>(*this);
}

wxFont TextItem::MakeFont(double pxPerMM) const {
    // Convert points (1pt = 1/72 inch) to a pixel height for the given scale.
    double pixelHeight = fontSizePt / 72.0 * 25.4 * pxPerMM;
    if (pixelHeight < 1) pixelHeight = 1;
    wxFont font(wxSize(0, static_cast<int>(pixelHeight + 0.5)),
                wxFONTFAMILY_DEFAULT,
                italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
                bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    if (!faceName.IsEmpty())
        font.SetFaceName(faceName);
    return font;
}

wxRealPoint TextItem::MeasureMM(double pxPerMM) const {
    wxBitmap bmp(4, 4);
    wxMemoryDC dc(bmp);
    dc.SetFont(MakeFont(pxPerMM));
    wxSize ext = dc.GetMultiLineTextExtent(text);
    return wxRealPoint(ext.x / pxPerMM, ext.y / pxPerMM);
}

wxRect2DDouble TextItem::BoundsMM() const {
    wxRealPoint s = MeasureMM(4.0); // 4 px/mm is plenty for measuring
    return wxRect2DDouble(x, y, s.x, s.y);
}

void TextItem::Draw(wxDC& dc, const wxPoint& originPx, double pxPerMM) const {
    dc.SetFont(MakeFont(pxPerMM));
    dc.SetTextForeground(colour);
    dc.SetTextBackground(wxNullColour);
    dc.SetBackgroundMode(wxTRANSPARENT);
    int px = originPx.x + static_cast<int>(MMtoPx(x, pxPerMM) + 0.5);
    int py = originPx.y + static_cast<int>(MMtoPx(y, pxPerMM) + 0.5);
    if (rotationDeg != 0.0)
        dc.DrawRotatedText(text, px, py, rotationDeg);
    else
        dc.DrawText(text, px, py);
}

// ---------------------------------------------------------------------------
// ImageItem
// ---------------------------------------------------------------------------
std::unique_ptr<LabelItem> ImageItem::Clone() const {
    return std::make_unique<ImageItem>(*this);
}

bool ImageItem::LoadFromFile(const wxString& path) {
    wxImage img;
    if (!img.LoadFile(path, wxBITMAP_TYPE_PNG))
        return false;
    image = img;

    wxFile f(path, wxFile::read);
    if (f.IsOpened()) {
        wxFileOffset len = f.Length();
        pngBytes.resize(static_cast<size_t>(len));
        f.Read(pngBytes.data(), len);
    }

    // Default the display size to ~30mm wide preserving aspect ratio.
    if (img.GetWidth() > 0) {
        double aspect = double(img.GetHeight()) / double(img.GetWidth());
        widthMM = 30.0;
        heightMM = 30.0 * aspect;
    }
    return true;
}

wxRect2DDouble ImageItem::BoundsMM() const {
    return wxRect2DDouble(x, y, widthMM, heightMM);
}

void ImageItem::Draw(wxDC& dc, const wxPoint& originPx, double pxPerMM) const {
    if (!image.IsOk()) return;
    int wpx = std::max(1, static_cast<int>(MMtoPx(widthMM, pxPerMM) + 0.5));
    int hpx = std::max(1, static_cast<int>(MMtoPx(heightMM, pxPerMM) + 0.5));
    wxImage scaled = image.Scale(wpx, hpx, wxIMAGE_QUALITY_HIGH);
    int px = originPx.x + static_cast<int>(MMtoPx(x, pxPerMM) + 0.5);
    int py = originPx.y + static_cast<int>(MMtoPx(y, pxPerMM) + 0.5);
    dc.DrawBitmap(wxBitmap(scaled), px, py, true);
}

// ---------------------------------------------------------------------------
// Label
// ---------------------------------------------------------------------------
std::unique_ptr<Label> Label::Clone() const {
    auto copy = std::make_unique<Label>();
    copy->widthMM = widthMM;
    copy->heightMM = heightMM;
    copy->shape = shape;
    copy->background = background;
    for (const auto& it : items)
        copy->items.push_back(it->Clone());
    return copy;
}

void Label::DrawBackground(wxDC& dc, const wxPoint& originPx, double pxPerMM) const {
    int wpx = static_cast<int>(MMtoPx(widthMM, pxPerMM) + 0.5);
    int hpx = static_cast<int>(MMtoPx(heightMM, pxPerMM) + 0.5);
    dc.SetBrush(wxBrush(background));
    dc.SetPen(wxPen(wxColour(120, 120, 120)));
    switch (shape) {
        case LabelShape::Rectangle:
            dc.DrawRectangle(originPx.x, originPx.y, wpx, hpx);
            break;
        case LabelShape::RoundedRect:
            dc.DrawRoundedRectangle(originPx.x, originPx.y, wpx, hpx,
                                    MMtoPx(6.0, pxPerMM));
            break;
        case LabelShape::Ellipse:
            dc.DrawEllipse(originPx.x, originPx.y, wpx, hpx);
            break;
    }
}

LabelItem* Label::HitTest(double mmX, double mmY) const {
    // Topmost item first.
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        wxRect2DDouble b = (*it)->BoundsMM();
        // Pad small text hit targets slightly.
        b.Inset(-1.0, -1.0);
        if (b.Contains(wxPoint2DDouble(mmX, mmY)))
            return it->get();
    }
    return nullptr;
}

void Label::ClearSelection() {
    for (auto& it : items)
        it->selected = false;
}
