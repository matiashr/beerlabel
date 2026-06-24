#include "LabelPrintout.h"

LabelPrintout::LabelPrintout(const Label* label, double gapMM, double marginMM)
    : wxPrintout("Beer labels"),
      m_label(label), m_gapMM(gapMM), m_marginMM(marginMM) {}

void LabelPrintout::GetPageInfo(int* minPage, int* maxPage,
                                int* selFrom, int* selTo) {
    *minPage = 1; *maxPage = 1; *selFrom = 1; *selTo = 1;
}

bool LabelPrintout::OnPrintPage(int) {
    wxDC* dc = GetDC();
    if (!dc) return false;

    // Draw in a fixed logical unit (10 units = 1 mm) and let wxWidgets scale
    // the whole A4 sheet onto the physical paper. This keeps the mapping
    // correct for both real printing and the (scaled-down) preview DC; drawing
    // at the raw printer DPI overshoots the preview bitmap and the label runs
    // off the page.
    const double pxPerMM = 10.0;
    const double pageW = 210.0, pageH = 297.0;     // A4 mm
    FitThisSizeToPaper(wxSize(static_cast<int>(pageW * pxPerMM),
                              static_cast<int>(pageH * pxPerMM)));

    // A4 usable area minus margins.
    double usableW = pageW - 2 * m_marginMM;
    double usableH = pageH - 2 * m_marginMM;

    double cellW = m_label->widthMM + m_gapMM;
    double cellH = m_label->heightMM + m_gapMM;
    int cols = std::max(1, static_cast<int>((usableW + m_gapMM) / cellW));
    int rows = std::max(1, static_cast<int>((usableH + m_gapMM) / cellH));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            double mmX = m_marginMM + c * cellW;
            double mmY = m_marginMM + r * cellH;
            wxPoint origin(static_cast<int>(mmX * pxPerMM + 0.5),
                           static_cast<int>(mmY * pxPerMM + 0.5));
            m_label->DrawBackground(*dc, origin, pxPerMM);
            for (const auto& it : m_label->items)
                it->Draw(*dc, origin, pxPerMM);
        }
    }
    return true;
}
