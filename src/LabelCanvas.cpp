#include "LabelCanvas.h"
#include <wx/dcbuffer.h>

LabelCanvas::LabelCanvas(wxWindow* parent, Label* label)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE),
      m_label(label) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(70, 70, 75));
    Bind(wxEVT_PAINT, &LabelCanvas::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &LabelCanvas::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &LabelCanvas::OnLeftUp, this);
    Bind(wxEVT_MOTION, &LabelCanvas::OnMotion, this);
    Bind(wxEVT_SIZE, &LabelCanvas::OnSize, this);
}

void LabelCanvas::SetLabel(Label* label) {
    m_label = label;
    m_selected = nullptr;
    m_dragging = false;
    if (onSelectionChanged) onSelectionChanged(nullptr);
    Refresh();
}

void LabelCanvas::SetBottleGuide(const Label::BottlePreset* preset) {
    m_guide = preset;
    Refresh();
}

void LabelCanvas::SetZoom(double pxPerMM) {
    m_pxPerMM = pxPerMM;
    Refresh();
}

void LabelCanvas::Refresh2() { Refresh(); }

void LabelCanvas::SelectItem(LabelItem* item) {
    m_label->ClearSelection();
    m_selected = item;
    if (item) item->selected = true;
    if (onSelectionChanged) onSelectionChanged(item);
    Refresh();
}

wxPoint LabelCanvas::Origin() const {
    // Centre the label within the available client area.
    wxSize cs = GetClientSize();
    int labW = static_cast<int>(m_label->widthMM * m_pxPerMM);
    int labH = static_cast<int>(m_label->heightMM * m_pxPerMM);
    int ox = std::max(m_marginPx, (cs.x - labW) / 2);
    int oy = std::max(m_marginPx, (cs.y - labH) / 2);
    return wxPoint(ox, oy);
}

wxPoint2DDouble LabelCanvas::ToMM(const wxPoint& p) const {
    wxPoint o = Origin();
    return wxPoint2DDouble((p.x - o.x) / m_pxPerMM, (p.y - o.y) / m_pxPerMM);
}

void LabelCanvas::OnSize(wxSizeEvent& e) { Refresh(); e.Skip(); }

void LabelCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    wxPoint o = Origin();

    // Bottle fit guide (dashed) drawn behind the label, sized to the
    // recommended maximum footprint for the selected bottle.
    if (m_guide) {
        int gw = static_cast<int>(m_guide->maxW * m_pxPerMM);
        int gh = static_cast<int>(m_guide->maxH * m_pxPerMM);
        wxPen pen(wxColour(255, 200, 80), 1, wxPENSTYLE_LONG_DASH);
        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(o.x, o.y, gw, gh);

        bool fits = m_label->widthMM <= m_guide->maxW &&
                    m_label->heightMM <= m_guide->maxH;
        dc.SetTextForeground(fits ? wxColour(140, 230, 140)
                                  : wxColour(255, 120, 120));
        dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                          wxFONTWEIGHT_NORMAL));
        wxString msg = wxString::Format(
            "%s guide: max %.0f x %.0f mm  -  label %.0f x %.0f mm  [%s]",
            m_guide->name, m_guide->maxW, m_guide->maxH,
            m_label->widthMM, m_label->heightMM, fits ? "FITS" : "TOO BIG");
        dc.DrawText(msg, m_marginPx, 6);
    }

    // Label background + outline.
    m_label->DrawBackground(dc, o, m_pxPerMM);

    // Items.
    for (const auto& it : m_label->items)
        it->Draw(dc, o, m_pxPerMM);

    // Selection handles.
    if (m_selected) {
        wxRect2DDouble b = m_selected->BoundsMM();
        int x = o.x + static_cast<int>(b.m_x * m_pxPerMM);
        int y = o.y + static_cast<int>(b.m_y * m_pxPerMM);
        int w = static_cast<int>(b.m_width * m_pxPerMM);
        int h = static_cast<int>(b.m_height * m_pxPerMM);
        dc.SetPen(wxPen(wxColour(60, 140, 255), 1, wxPENSTYLE_SHORT_DASH));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(x - 2, y - 2, w + 4, h + 4);
    }
}

void LabelCanvas::OnLeftDown(wxMouseEvent& e) {
    wxPoint2DDouble mm = ToMM(e.GetPosition());
    m_label->ClearSelection();
    m_selected = m_label->HitTest(mm.m_x, mm.m_y);
    if (m_selected) {
        m_selected->selected = true;
        m_dragging = true;
        m_dragOffsetMM = wxPoint2DDouble(mm.m_x - m_selected->x,
                                         mm.m_y - m_selected->y);
        CaptureMouse();
    }
    if (onSelectionChanged) onSelectionChanged(m_selected);
    Refresh();
}

void LabelCanvas::OnMotion(wxMouseEvent& e) {
    if (m_dragging && m_selected && e.Dragging() && e.LeftIsDown()) {
        wxPoint2DDouble mm = ToMM(e.GetPosition());
        m_selected->x = mm.m_x - m_dragOffsetMM.m_x;
        m_selected->y = mm.m_y - m_dragOffsetMM.m_y;
        if (onModelChanged) onModelChanged();
        Refresh();
    }
}

void LabelCanvas::OnLeftUp(wxMouseEvent&) {
    if (m_dragging) {
        m_dragging = false;
        if (HasCapture()) ReleaseMouse();
    }
}
