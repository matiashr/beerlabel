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

    // Printer resolution -> pixels per millimetre.
    int ppiX, ppiY;
    GetPPIPrinter(&ppiX, &ppiY);
    if (ppiX <= 0) ppiX = 600;
    double pxPerMM = ppiX / 25.4;

    // A4 usable area (210 x 297 mm) minus margins.
    const double pageW = 210.0, pageH = 297.0;
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
