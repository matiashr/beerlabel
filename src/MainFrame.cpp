#include "MainFrame.h"
#include "LabelCanvas.h"
#include "SvgIO.h"
#include "LabelPrintout.h"

#include <wx/printdlg.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/filename.h>
#include <wx/artprov.h>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_NEW, MainFrame::OnNew)
    EVT_MENU(wxID_OPEN, MainFrame::OnOpen)
    EVT_MENU(wxID_SAVE, MainFrame::OnSave)
    EVT_MENU(wxID_SAVEAS, MainFrame::OnSaveAs)
    EVT_MENU(wxID_PRINT, MainFrame::OnPrint)
    EVT_MENU(ID_PrintPrev, MainFrame::OnPrintPreview)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(ID_AddText, MainFrame::OnAddText)
    EVT_MENU(ID_AddImage, MainFrame::OnAddImage)
    EVT_MENU(ID_DeleteItem, MainFrame::OnDelete)

    EVT_SPINCTRLDOUBLE(ID_LabW, MainFrame::OnLabelSpin)
    EVT_SPINCTRLDOUBLE(ID_LabH, MainFrame::OnLabelSpin)
    EVT_CHOICE(ID_Shape, MainFrame::OnLabelPropChanged)
    EVT_CHOICE(ID_Bottle, MainFrame::OnLabelPropChanged)
    EVT_COLOURPICKER_CHANGED(ID_BgColour, MainFrame::OnColour)
    EVT_SLIDER(ID_Zoom, MainFrame::OnZoom)

    EVT_TEXT(ID_Text, MainFrame::OnItemChanged)
    EVT_SPINCTRLDOUBLE(ID_FontSize, MainFrame::OnItemSpin)
    EVT_CHECKBOX(ID_Bold, MainFrame::OnItemChanged)
    EVT_CHECKBOX(ID_Italic, MainFrame::OnItemChanged)
    EVT_SPINCTRLDOUBLE(ID_Rot, MainFrame::OnItemSpin)
    EVT_COLOURPICKER_CHANGED(ID_TextColour, MainFrame::OnColour)
    EVT_SPINCTRLDOUBLE(ID_ImgW, MainFrame::OnItemSpin)
    EVT_SPINCTRLDOUBLE(ID_ImgH, MainFrame::OnItemSpin)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Beer Label Designer",
              wxDefaultPosition, wxSize(1100, 760)) {
    m_label = std::make_unique<Label>();

    BuildMenu();
    BuildToolBar();

    auto* split = new wxBoxSizer(wxHORIZONTAL);
    m_canvas = new LabelCanvas(this, m_label.get());
    m_canvas->onSelectionChanged = [this](LabelItem* i) { OnSelectionChanged(i); };
    m_canvas->onModelChanged = [this] { SetModified(true); SyncInspectorFromModel(); };

    wxWindow* inspector = BuildInspector(this);

    split->Add(m_canvas, 1, wxEXPAND);
    split->Add(inspector, 0, wxEXPAND);
    SetSizer(split);

    CreateStatusBar();
    SetStatusText("Add text/images, drag to position, then Save (SVG) or Print.");

    SyncInspectorFromModel();
    UpdateTitle();
}

void MainFrame::BuildMenu() {
    auto* file = new wxMenu;
    file->Append(wxID_NEW, "&New\tCtrl-N");
    file->Append(wxID_OPEN, "&Open...\tCtrl-O");
    file->Append(wxID_SAVE, "&Save\tCtrl-S");
    file->Append(wxID_SAVEAS, "Save &As...");
    file->AppendSeparator();
    file->Append(ID_PrintPrev, "Print Pre&view...");
    file->Append(wxID_PRINT, "&Print...\tCtrl-P");
    file->AppendSeparator();
    file->Append(wxID_EXIT, "E&xit\tCtrl-Q");

    auto* edit = new wxMenu;
    edit->Append(ID_AddText, "Add &Text\tCtrl-T");
    edit->Append(ID_AddImage, "Add &Image (PNG)...\tCtrl-I");
    edit->Append(ID_DeleteItem, "&Delete Selected\tDel");

    auto* help = new wxMenu;
    help->Append(wxID_ABOUT, "&About");

    auto* bar = new wxMenuBar;
    bar->Append(file, "&File");
    bar->Append(edit, "&Edit");
    bar->Append(help, "&Help");
    SetMenuBar(bar);
}

void MainFrame::BuildToolBar() {
    auto* tb = CreateToolBar();
    tb->AddTool(ID_AddText, "Text",
                wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR),
                "Add text");
    tb->AddTool(ID_AddImage, "Image",
                wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR),
                "Add PNG image");
    tb->AddTool(ID_DeleteItem, "Delete",
                wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR),
                "Delete selected");
    tb->AddSeparator();
    tb->AddTool(wxID_SAVE, "Save",
                wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR),
                "Save as SVG");
    tb->AddTool(wxID_PRINT, "Print",
                wxArtProvider::GetBitmap(wxART_PRINT, wxART_TOOLBAR),
                "Print labels on A4");
    tb->Realize();
}

static wxSpinCtrlDouble* MakeSpin(wxWindow* p, int id, double min, double max,
                                  double val, double inc) {
    auto* s = new wxSpinCtrlDouble(p, id, "", wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, min, max, val, inc);
    s->SetDigits(1);
    return s;
}

wxWindow* MainFrame::BuildInspector(wxWindow* parent) {
    auto* panel = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition,
                                       wxSize(280, -1));
    panel->SetScrollRate(0, 10);
    m_inspector = panel;
    auto* root = new wxBoxSizer(wxVERTICAL);

    // ---- Label properties ------------------------------------------------
    auto* labBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Label");
    auto* grid = new wxFlexGridSizer(2, 4, 4);
    grid->AddGrowableCol(1, 1);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Width (mm)"), 0,
              wxALIGN_CENTER_VERTICAL);
    m_labW = MakeSpin(panel, ID_LabW, 10, 300, m_label->widthMM, 1);
    grid->Add(m_labW, 1, wxEXPAND);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Height (mm)"), 0,
              wxALIGN_CENTER_VERTICAL);
    m_labH = MakeSpin(panel, ID_LabH, 10, 300, m_label->heightMM, 1);
    grid->Add(m_labH, 1, wxEXPAND);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Shape"), 0,
              wxALIGN_CENTER_VERTICAL);
    m_shape = new wxChoice(panel, ID_Shape);
    m_shape->Append("Rectangle");
    m_shape->Append("Rounded");
    m_shape->Append("Ellipse");
    m_shape->SetSelection((int)m_label->shape);
    grid->Add(m_shape, 1, wxEXPAND);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Background"), 0,
              wxALIGN_CENTER_VERTICAL);
    m_bgColour = new wxColourPickerCtrl(panel, ID_BgColour, m_label->background);
    grid->Add(m_bgColour, 1, wxEXPAND);

    grid->Add(new wxStaticText(panel, wxID_ANY, "Bottle guide"), 0,
              wxALIGN_CENTER_VERTICAL);
    m_bottle = new wxChoice(panel, ID_Bottle);
    m_bottle->Append("33 cl");
    m_bottle->Append("50 cl");
    m_bottle->Append("None");
    m_bottle->SetSelection(0);
    grid->Add(m_bottle, 1, wxEXPAND);

    labBox->Add(grid, 0, wxEXPAND | wxALL, 4);
    root->Add(labBox, 0, wxEXPAND | wxALL, 6);

    // ---- Zoom ------------------------------------------------------------
    auto* zoomBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Zoom (px/mm)");
    m_zoom = new wxSlider(panel, ID_Zoom, 4, 1, 12);
    zoomBox->Add(m_zoom, 1, wxEXPAND | wxALL, 4);
    root->Add(zoomBox, 0, wxEXPAND | wxALL, 6);

    // ---- Text item -------------------------------------------------------
    m_textBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Text");
    m_text = new wxTextCtrl(panel, ID_Text, "", wxDefaultPosition,
                            wxSize(-1, 60), wxTE_MULTILINE);
    m_textBox->Add(m_text, 0, wxEXPAND | wxALL, 4);

    auto* tgrid = new wxFlexGridSizer(2, 4, 4);
    tgrid->AddGrowableCol(1, 1);
    tgrid->Add(new wxStaticText(panel, wxID_ANY, "Font size (pt)"), 0,
               wxALIGN_CENTER_VERTICAL);
    m_fontSize = MakeSpin(panel, ID_FontSize, 4, 200, 18, 1);
    tgrid->Add(m_fontSize, 1, wxEXPAND);
    tgrid->Add(new wxStaticText(panel, wxID_ANY, "Rotation (deg)"), 0,
               wxALIGN_CENTER_VERTICAL);
    m_rot = MakeSpin(panel, ID_Rot, -180, 180, 0, 5);
    tgrid->Add(m_rot, 1, wxEXPAND);
    tgrid->Add(new wxStaticText(panel, wxID_ANY, "Colour"), 0,
               wxALIGN_CENTER_VERTICAL);
    m_textColour = new wxColourPickerCtrl(panel, ID_TextColour, *wxBLACK);
    tgrid->Add(m_textColour, 1, wxEXPAND);
    m_textBox->Add(tgrid, 0, wxEXPAND | wxALL, 4);

    auto* checks = new wxBoxSizer(wxHORIZONTAL);
    m_bold = new wxCheckBox(panel, ID_Bold, "Bold");
    m_italic = new wxCheckBox(panel, ID_Italic, "Italic");
    checks->Add(m_bold, 0, wxRIGHT, 8);
    checks->Add(m_italic, 0);
    m_textBox->Add(checks, 0, wxALL, 4);
    root->Add(m_textBox, 0, wxEXPAND | wxALL, 6);

    // ---- Image item ------------------------------------------------------
    m_imageBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Image");
    auto* igrid = new wxFlexGridSizer(2, 4, 4);
    igrid->AddGrowableCol(1, 1);
    igrid->Add(new wxStaticText(panel, wxID_ANY, "Width (mm)"), 0,
               wxALIGN_CENTER_VERTICAL);
    m_imgW = MakeSpin(panel, ID_ImgW, 2, 300, 30, 1);
    igrid->Add(m_imgW, 1, wxEXPAND);
    igrid->Add(new wxStaticText(panel, wxID_ANY, "Height (mm)"), 0,
               wxALIGN_CENTER_VERTICAL);
    m_imgH = MakeSpin(panel, ID_ImgH, 2, 300, 30, 1);
    igrid->Add(m_imgH, 1, wxEXPAND);
    m_imageBox->Add(igrid, 0, wxEXPAND | wxALL, 4);
    root->Add(m_imageBox, 0, wxEXPAND | wxALL, 6);

    panel->SetSizer(root);
    return panel;
}

// ---------------------------------------------------------------------------
// Selection / inspector sync
// ---------------------------------------------------------------------------
void MainFrame::OnSelectionChanged(LabelItem*) { SyncInspectorFromModel(); }

void MainFrame::SyncInspectorFromModel() {
    m_labW->SetValue(m_label->widthMM);
    m_labH->SetValue(m_label->heightMM);
    m_shape->SetSelection((int)m_label->shape);
    m_bgColour->SetColour(m_label->background);

    LabelItem* sel = m_canvas ? m_canvas->Selected() : nullptr;
    bool isText = sel && sel->Type() == ItemType::Text;
    bool isImage = sel && sel->Type() == ItemType::Image;

    if (isText) {
        auto* t = static_cast<TextItem*>(sel);
        if (m_text->GetValue() != t->text) m_text->ChangeValue(t->text);
        m_fontSize->SetValue(t->fontSizePt);
        m_rot->SetValue(t->rotationDeg);
        m_bold->SetValue(t->bold);
        m_italic->SetValue(t->italic);
        m_textColour->SetColour(t->colour);
    }
    if (isImage) {
        auto* im = static_cast<ImageItem*>(sel);
        m_imgW->SetValue(im->widthMM);
        m_imgH->SetValue(im->heightMM);
    }

    m_inspector->GetSizer()->Show(m_textBox, isText, true);
    m_inspector->GetSizer()->Show(m_imageBox, isImage, true);
    m_inspector->Layout();
}

// ---------------------------------------------------------------------------
// Label property handlers
// ---------------------------------------------------------------------------
void MainFrame::OnLabelSpin(wxSpinDoubleEvent&) {
    m_label->widthMM = m_labW->GetValue();
    m_label->heightMM = m_labH->GetValue();
    SetModified(true);
    m_canvas->Refresh();
}

void MainFrame::OnLabelPropChanged(wxCommandEvent& e) {
    if (e.GetId() == ID_Shape) {
        m_label->shape = static_cast<LabelShape>(m_shape->GetSelection());
        SetModified(true);
    } else if (e.GetId() == ID_Bottle) {
        switch (m_bottle->GetSelection()) {
            case 0: m_canvas->SetBottleGuide(&Label::Bottle33cl); break;
            case 1: m_canvas->SetBottleGuide(&Label::Bottle50cl); break;
            default: m_canvas->SetBottleGuide(nullptr); break;
        }
    }
    m_canvas->Refresh();
}

void MainFrame::OnZoom(wxCommandEvent&) {
    m_canvas->SetZoom(m_zoom->GetValue());
}

// ---------------------------------------------------------------------------
// Item property handlers
// ---------------------------------------------------------------------------
void MainFrame::OnItemChanged(wxCommandEvent&) {
    LabelItem* sel = m_canvas->Selected();
    if (!sel) return;
    if (sel->Type() == ItemType::Text) {
        auto* t = static_cast<TextItem*>(sel);
        t->text = m_text->GetValue();
        t->bold = m_bold->GetValue();
        t->italic = m_italic->GetValue();
    }
    SetModified(true);
    m_canvas->Refresh();
}

void MainFrame::OnItemSpin(wxSpinDoubleEvent&) {
    LabelItem* sel = m_canvas->Selected();
    if (!sel) return;
    if (sel->Type() == ItemType::Text) {
        auto* t = static_cast<TextItem*>(sel);
        t->fontSizePt = m_fontSize->GetValue();
        t->rotationDeg = m_rot->GetValue();
    } else {
        auto* im = static_cast<ImageItem*>(sel);
        im->widthMM = m_imgW->GetValue();
        im->heightMM = m_imgH->GetValue();
    }
    SetModified(true);
    m_canvas->Refresh();
}

void MainFrame::OnColour(wxColourPickerEvent& e) {
    if (e.GetId() == ID_BgColour) {
        m_label->background = m_bgColour->GetColour();
    } else if (e.GetId() == ID_TextColour) {
        LabelItem* sel = m_canvas->Selected();
        if (sel && sel->Type() == ItemType::Text)
            static_cast<TextItem*>(sel)->colour = m_textColour->GetColour();
    }
    SetModified(true);
    m_canvas->Refresh();
}

// ---------------------------------------------------------------------------
// Item add/delete
// ---------------------------------------------------------------------------
void MainFrame::OnAddText(wxCommandEvent&) {
    auto t = std::make_unique<TextItem>();
    t->text = "Beer";
    t->x = m_label->widthMM / 2 - 15;
    t->y = m_label->heightMM / 2 - 5;
    LabelItem* raw = t.get();
    m_label->items.push_back(std::move(t));
    m_canvas->SelectItem(raw);
    SetModified(true);
    SyncInspectorFromModel();
}

void MainFrame::OnAddImage(wxCommandEvent&) {
    wxFileDialog dlg(this, "Add PNG image", "", "",
                     "PNG files (*.png)|*.png",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    auto im = std::make_unique<ImageItem>();
    if (!im->LoadFromFile(dlg.GetPath())) {
        wxMessageBox("Could not load PNG.", "Error", wxICON_ERROR);
        return;
    }
    im->x = m_label->widthMM / 2 - im->widthMM / 2;
    im->y = m_label->heightMM / 2 - im->heightMM / 2;
    LabelItem* raw = im.get();
    m_label->items.push_back(std::move(im));
    m_canvas->SelectItem(raw);
    SetModified(true);
    SyncInspectorFromModel();
}

void MainFrame::OnDelete(wxCommandEvent&) {
    LabelItem* sel = m_canvas->Selected();
    if (!sel) return;
    auto& v = m_label->items;
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (it->get() == sel) { v.erase(it); break; }
    }
    SetModified(true);
    OnSelectionChanged(nullptr);
    m_canvas->Refresh();
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------
void MainFrame::OnNew(wxCommandEvent&) {
    m_label = std::make_unique<Label>();
    m_path.Clear();
    m_canvas = nullptr; // recreated below
    // Rebind canvas to the new label.
    // (Simplest: replace the canvas' label pointer.)
    // We kept the same canvas object; update its label instead:
    // -- but canvas stored a raw pointer; recreate cleanly:
    GetSizer()->Clear();
    m_canvas = new LabelCanvas(this, m_label.get());
    m_canvas->onSelectionChanged = [this](LabelItem* i) { OnSelectionChanged(i); };
    m_canvas->onModelChanged = [this] { SetModified(true); SyncInspectorFromModel(); };
    GetSizer()->Add(m_canvas, 1, wxEXPAND);
    GetSizer()->Add(m_inspector, 0, wxEXPAND);
    Layout();
    SetModified(false);
    SyncInspectorFromModel();
}

void MainFrame::OnOpen(wxCommandEvent&) {
    wxFileDialog dlg(this, "Open label", "", "",
                     "SVG label (*.svg)|*.svg",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    auto loaded = SvgIO::Load(dlg.GetPath());
    if (!loaded) {
        wxMessageBox("Not a Beer Label Designer SVG (missing metadata).",
                     "Open", wxICON_ERROR);
        return;
    }
    m_label = std::move(loaded);
    m_path = dlg.GetPath();
    GetSizer()->Clear();
    m_canvas = new LabelCanvas(this, m_label.get());
    m_canvas->onSelectionChanged = [this](LabelItem* i) { OnSelectionChanged(i); };
    m_canvas->onModelChanged = [this] { SetModified(true); SyncInspectorFromModel(); };
    GetSizer()->Add(m_canvas, 1, wxEXPAND);
    GetSizer()->Add(m_inspector, 0, wxEXPAND);
    Layout();
    SetModified(false);
    SyncInspectorFromModel();
}

bool MainFrame::DoSave(const wxString& path) {
    if (!SvgIO::Save(*m_label, path)) {
        wxMessageBox("Failed to save file.", "Save", wxICON_ERROR);
        return false;
    }
    m_path = path;
    SetModified(false);
    return true;
}

void MainFrame::OnSave(wxCommandEvent& e) {
    if (m_path.IsEmpty()) { OnSaveAs(e); return; }
    DoSave(m_path);
}

void MainFrame::OnSaveAs(wxCommandEvent&) {
    wxFileDialog dlg(this, "Save label as SVG", "", "label.svg",
                     "SVG label (*.svg)|*.svg",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    DoSave(dlg.GetPath());
}

void MainFrame::OnPrint(wxCommandEvent&) {
    wxPrintDialogData data;
    wxPrinter printer(&data);
    LabelPrintout printout(m_label.get());
    if (!printer.Print(this, &printout, true)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox("There was a problem printing.", "Print",
                         wxICON_ERROR);
    }
}

void MainFrame::OnPrintPreview(wxCommandEvent&) {
    auto* preview = new wxPrintPreview(new LabelPrintout(m_label.get()),
                                       new LabelPrintout(m_label.get()));
    if (!preview->IsOk()) {
        delete preview;
        wxMessageBox("Print preview unavailable.", "Preview", wxICON_ERROR);
        return;
    }
    auto* frame = new wxPreviewFrame(preview, this, "Print Preview");
    frame->Centre();
    frame->Initialize();
    frame->Show();
}

void MainFrame::OnExit(wxCommandEvent&) { Close(true); }

void MainFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox(
        "Beer Label Designer\n\n"
        "Design beer bottle labels with text and PNG images, check fit\n"
        "against 33cl / 50cl bottle guides, save as SVG and print multiple\n"
        "labels per A4 sheet.\n\nBuilt with wxWidgets.",
        "About", wxOK | wxICON_INFORMATION);
}

// ---------------------------------------------------------------------------
void MainFrame::SetModified(bool m) { m_modified = m; UpdateTitle(); }

void MainFrame::UpdateTitle() {
    wxString name = m_path.IsEmpty() ? "Untitled"
                                     : wxFileName(m_path).GetFullName();
    SetTitle(wxString::Format("%s%s - Beer Label Designer",
                              m_modified ? "*" : "", name));
}
