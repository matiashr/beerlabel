#include <wx/wx.h>
#include <wx/image.h>
#include "MainFrame.h"

class BeerLabelApp : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers();
        auto* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(BeerLabelApp);
