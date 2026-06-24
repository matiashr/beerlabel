#pragma once

#include <wx/wx.h>
#include "LabelModel.h"

// Editing surface. Renders the label centred with a margin, draws a dashed
// "bottle fit" guide, and lets the user select / drag items with the mouse.
class LabelCanvas : public wxPanel {
public:
    LabelCanvas(wxWindow* parent, Label* label);

    void SetBottleGuide(const Label::BottlePreset* preset);
    const Label::BottlePreset* BottleGuide() const { return m_guide; }

    void SetZoom(double pxPerMM);
    double Zoom() const { return m_pxPerMM; }

    // Called by the frame when the model changes externally.
    void Refresh2();

    // Notify the frame that selection changed so it can update the inspector.
    std::function<void(LabelItem*)> onSelectionChanged;
    // Notify the frame that the model changed (for title "modified" marker).
    std::function<void()> onModelChanged;

    LabelItem* Selected() const { return m_selected; }
    void SelectItem(LabelItem* item);   // programmatic selection

private:
    void OnPaint(wxPaintEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnMotion(wxMouseEvent&);
    void OnSize(wxSizeEvent&);

    wxPoint Origin() const;             // device pixel pos of label (0,0)
    wxPoint2DDouble ToMM(const wxPoint& devicePt) const;

    Label* m_label;
    double m_pxPerMM = 4.0;             // zoom
    int m_marginPx = 30;

    LabelItem* m_selected = nullptr;
    bool m_dragging = false;
    wxPoint2DDouble m_dragOffsetMM;     // pointer -> item origin offset

    const Label::BottlePreset* m_guide = &Label::Bottle33cl;
};
