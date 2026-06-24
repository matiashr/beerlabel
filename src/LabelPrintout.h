#pragma once

#include <wx/print.h>
#include "LabelModel.h"

// Tiles as many copies of the label as fit onto an A4 sheet, with a
// user-defined gap and page margin.
class LabelPrintout : public wxPrintout {
public:
    LabelPrintout(const Label* label, double gapMM = 4.0, double marginMM = 8.0);

    bool OnPrintPage(int page) override;
    bool HasPage(int page) override { return page == 1; }
    void GetPageInfo(int* minPage, int* maxPage,
                     int* selFrom, int* selTo) override;

private:
    const Label* m_label;
    double m_gapMM;
    double m_marginMM;
};
