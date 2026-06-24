#pragma once

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/clrpicker.h>
#include <memory>
#include "LabelModel.h"

class LabelCanvas;

class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    void BuildMenu();
    void BuildToolBar();
    wxWindow* BuildInspector(wxWindow* parent);

    // File
    void OnNew(wxCommandEvent&);
    void OnOpen(wxCommandEvent&);
    void OnSave(wxCommandEvent&);
    void OnSaveAs(wxCommandEvent&);
    void OnPrint(wxCommandEvent&);
    void OnPrintPreview(wxCommandEvent&);
    void OnExit(wxCommandEvent&);
    void OnAbout(wxCommandEvent&);

    // Items
    void OnAddText(wxCommandEvent&);
    void OnAddImage(wxCommandEvent&);
    void OnDelete(wxCommandEvent&);

    // Inspector callbacks
    void OnSelectionChanged(LabelItem* item);
    void SyncInspectorFromModel();
    void OnLabelPropChanged(wxCommandEvent&);
    void OnLabelSpin(wxSpinDoubleEvent&);
    void OnItemChanged(wxCommandEvent&);
    void OnItemSpin(wxSpinDoubleEvent&);
    void OnColour(wxColourPickerEvent&);
    void OnZoom(wxCommandEvent&);

    bool DoSave(const wxString& path);
    void SetModified(bool m);
    void UpdateTitle();

    std::unique_ptr<Label> m_label;
    LabelCanvas* m_canvas = nullptr;
    wxWindow* m_inspector = nullptr;
    wxString m_path;
    bool m_modified = false;

    // Label inspector controls
    wxSpinCtrlDouble* m_labW = nullptr;
    wxSpinCtrlDouble* m_labH = nullptr;
    wxChoice* m_shape = nullptr;
    wxColourPickerCtrl* m_bgColour = nullptr;
    wxChoice* m_bottle = nullptr;
    wxSlider* m_zoom = nullptr;

    // Item inspector controls
    wxStaticBoxSizer* m_textBox = nullptr;
    wxStaticBoxSizer* m_imageBox = nullptr;
    wxTextCtrl* m_text = nullptr;
    wxSpinCtrlDouble* m_fontSize = nullptr;
    wxCheckBox* m_bold = nullptr;
    wxCheckBox* m_italic = nullptr;
    wxSpinCtrlDouble* m_rot = nullptr;
    wxColourPickerCtrl* m_textColour = nullptr;
    wxSpinCtrlDouble* m_imgW = nullptr;
    wxSpinCtrlDouble* m_imgH = nullptr;

    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_AddText = wxID_HIGHEST + 1,
    ID_AddImage,
    ID_DeleteItem,
    ID_PrintPrev,
    ID_LabW, ID_LabH, ID_Shape, ID_BgColour, ID_Bottle, ID_Zoom,
    ID_Text, ID_FontSize, ID_Bold, ID_Italic, ID_Rot, ID_TextColour,
    ID_ImgW, ID_ImgH
};
