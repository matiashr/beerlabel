#include "SvgIO.h"
#include <wx/xml/xml.h>
#include <wx/base64.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <functional>

namespace {

const char* kNs = "https://beerlabel.local/ns";

wxString ColourHex(const wxColour& c) {
    return wxString::Format("#%02X%02X%02X", c.Red(), c.Green(), c.Blue());
}

wxColour ParseColour(const wxString& s) {
    wxColour c;
    c.Set(s);
    return c.IsOk() ? c : *wxBLACK;
}

wxString EncodePng(const std::vector<unsigned char>& bytes) {
    if (bytes.empty()) return "";
    return wxBase64Encode(bytes.data(), bytes.size());
}

std::vector<unsigned char> DecodePng(const wxString& b64) {
    wxMemoryBuffer buf = wxBase64Decode(b64);
    const unsigned char* p = static_cast<const unsigned char*>(buf.GetData());
    return std::vector<unsigned char>(p, p + buf.GetDataLen());
}

void AddAttr(wxXmlNode* n, const wxString& k, const wxString& v) {
    n->AddAttribute(k, v);
}

} // namespace

bool SvgIO::Save(const Label& label, const wxString& path) {
    wxXmlDocument doc;
    auto* svg = new wxXmlNode(wxXML_ELEMENT_NODE, "svg");
    doc.SetRoot(svg);
    AddAttr(svg, "xmlns", "http://www.w3.org/2000/svg");
    AddAttr(svg, "xmlns:xlink", "http://www.w3.org/1999/xlink");
    AddAttr(svg, "xmlns:bl", kNs);
    AddAttr(svg, "width", wxString::Format("%gmm", label.widthMM));
    AddAttr(svg, "height", wxString::Format("%gmm", label.heightMM));
    AddAttr(svg, "viewBox",
            wxString::Format("0 0 %g %g", label.widthMM, label.heightMM));

    // ---- editable model in <metadata> -----------------------------------
    auto* metadata = new wxXmlNode(wxXML_ELEMENT_NODE, "metadata");
    auto* mlabel = new wxXmlNode(wxXML_ELEMENT_NODE, "bl:label");
    AddAttr(mlabel, "width", wxString::Format("%g", label.widthMM));
    AddAttr(mlabel, "height", wxString::Format("%g", label.heightMM));
    AddAttr(mlabel, "shape", wxString::Format("%d", (int)label.shape));
    AddAttr(mlabel, "background", ColourHex(label.background));
    for (const auto& it : label.items) {
        if (it->Type() == ItemType::Text) {
            auto* t = static_cast<TextItem*>(it.get());
            auto* n = new wxXmlNode(wxXML_ELEMENT_NODE, "bl:text");
            AddAttr(n, "x", wxString::Format("%g", t->x));
            AddAttr(n, "y", wxString::Format("%g", t->y));
            AddAttr(n, "size", wxString::Format("%g", t->fontSizePt));
            AddAttr(n, "bold", t->bold ? "1" : "0");
            AddAttr(n, "italic", t->italic ? "1" : "0");
            AddAttr(n, "rot", wxString::Format("%g", t->rotationDeg));
            AddAttr(n, "colour", ColourHex(t->colour));
            AddAttr(n, "face", t->faceName);
            n->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", t->text));
            mlabel->AddChild(n);
        } else {
            auto* im = static_cast<ImageItem*>(it.get());
            auto* n = new wxXmlNode(wxXML_ELEMENT_NODE, "bl:image");
            AddAttr(n, "x", wxString::Format("%g", im->x));
            AddAttr(n, "y", wxString::Format("%g", im->y));
            AddAttr(n, "w", wxString::Format("%g", im->widthMM));
            AddAttr(n, "h", wxString::Format("%g", im->heightMM));
            AddAttr(n, "data", EncodePng(im->pngBytes));
            mlabel->AddChild(n);
        }
    }
    metadata->AddChild(mlabel);
    svg->AddChild(metadata);

    // ---- visible SVG rendering -------------------------------------------
    auto* bg = new wxXmlNode(wxXML_ELEMENT_NODE,
                             label.shape == LabelShape::Ellipse ? "ellipse"
                                                                : "rect");
    if (label.shape == LabelShape::Ellipse) {
        AddAttr(bg, "cx", wxString::Format("%g", label.widthMM / 2));
        AddAttr(bg, "cy", wxString::Format("%g", label.heightMM / 2));
        AddAttr(bg, "rx", wxString::Format("%g", label.widthMM / 2));
        AddAttr(bg, "ry", wxString::Format("%g", label.heightMM / 2));
    } else {
        AddAttr(bg, "x", "0");
        AddAttr(bg, "y", "0");
        AddAttr(bg, "width", wxString::Format("%g", label.widthMM));
        AddAttr(bg, "height", wxString::Format("%g", label.heightMM));
        if (label.shape == LabelShape::RoundedRect) {
            AddAttr(bg, "rx", "6");
            AddAttr(bg, "ry", "6");
        }
    }
    AddAttr(bg, "fill", ColourHex(label.background));
    AddAttr(bg, "stroke", "#787878");
    AddAttr(bg, "stroke-width", "0.2");
    svg->AddChild(bg);

    for (const auto& it : label.items) {
        if (it->Type() == ItemType::Text) {
            auto* t = static_cast<TextItem*>(it.get());
            double sizeMM = t->fontSizePt / 72.0 * 25.4;
            auto* n = new wxXmlNode(wxXML_ELEMENT_NODE, "text");
            AddAttr(n, "x", wxString::Format("%g", t->x));
            // SVG y is the baseline; approximate ascent as 0.8 of size.
            AddAttr(n, "y", wxString::Format("%g", t->y + sizeMM * 0.8));
            AddAttr(n, "font-size", wxString::Format("%g", sizeMM));
            AddAttr(n, "fill", ColourHex(t->colour));
            if (t->bold) AddAttr(n, "font-weight", "bold");
            if (t->italic) AddAttr(n, "font-style", "italic");
            if (!t->faceName.IsEmpty())
                AddAttr(n, "font-family", t->faceName);
            if (t->rotationDeg != 0.0)
                AddAttr(n, "transform",
                        wxString::Format("rotate(%g %g %g)",
                                         -t->rotationDeg, t->x, t->y));
            // Multi-line via tspans.
            wxArrayString lines = wxStringTokenize(t->text, "\n",
                                                   wxTOKEN_RET_EMPTY_ALL);
            for (size_t i = 0; i < lines.size(); ++i) {
                auto* span = new wxXmlNode(wxXML_ELEMENT_NODE, "tspan");
                AddAttr(span, "x", wxString::Format("%g", t->x));
                if (i > 0)
                    AddAttr(span, "dy", wxString::Format("%g", sizeMM * 1.2));
                span->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", lines[i]));
                n->AddChild(span);
            }
            svg->AddChild(n);
        } else {
            auto* im = static_cast<ImageItem*>(it.get());
            auto* n = new wxXmlNode(wxXML_ELEMENT_NODE, "image");
            AddAttr(n, "x", wxString::Format("%g", im->x));
            AddAttr(n, "y", wxString::Format("%g", im->y));
            AddAttr(n, "width", wxString::Format("%g", im->widthMM));
            AddAttr(n, "height", wxString::Format("%g", im->heightMM));
            AddAttr(n, "xlink:href",
                    "data:image/png;base64," + EncodePng(im->pngBytes));
            svg->AddChild(n);
        }
    }

    return doc.Save(path);
}

std::unique_ptr<Label> SvgIO::Load(const wxString& path) {
    wxXmlDocument doc;
    if (!doc.Load(path)) return nullptr;
    wxXmlNode* root = doc.GetRoot();
    if (!root) return nullptr;

    // Find metadata/bl:label.
    wxXmlNode* mlabel = nullptr;
    std::function<void(wxXmlNode*)> find = [&](wxXmlNode* n) {
        for (wxXmlNode* c = n->GetChildren(); c && !mlabel; c = c->GetNext()) {
            if (c->GetName() == "bl:label") { mlabel = c; return; }
            find(c);
        }
    };
    find(root);
    if (!mlabel) return nullptr; // not one of our files

    auto label = std::make_unique<Label>();
    double d;
    if (mlabel->GetAttribute("width").ToDouble(&d)) label->widthMM = d;
    if (mlabel->GetAttribute("height").ToDouble(&d)) label->heightMM = d;
    long sh;
    if (mlabel->GetAttribute("shape").ToLong(&sh))
        label->shape = static_cast<LabelShape>(sh);
    label->background = ParseColour(mlabel->GetAttribute("background", "#FFFFFF"));

    for (wxXmlNode* c = mlabel->GetChildren(); c; c = c->GetNext()) {
        if (c->GetName() == "bl:text") {
            auto t = std::make_unique<TextItem>();
            c->GetAttribute("x").ToDouble(&t->x);
            c->GetAttribute("y").ToDouble(&t->y);
            c->GetAttribute("size").ToDouble(&t->fontSizePt);
            t->bold = c->GetAttribute("bold", "0") == "1";
            t->italic = c->GetAttribute("italic", "0") == "1";
            c->GetAttribute("rot", "0").ToDouble(&t->rotationDeg);
            t->colour = ParseColour(c->GetAttribute("colour", "#000000"));
            t->faceName = c->GetAttribute("face", "");
            t->text = c->GetNodeContent();
            label->items.push_back(std::move(t));
        } else if (c->GetName() == "bl:image") {
            auto im = std::make_unique<ImageItem>();
            c->GetAttribute("x").ToDouble(&im->x);
            c->GetAttribute("y").ToDouble(&im->y);
            c->GetAttribute("w").ToDouble(&im->widthMM);
            c->GetAttribute("h").ToDouble(&im->heightMM);
            im->pngBytes = DecodePng(c->GetAttribute("data", ""));
            if (!im->pngBytes.empty()) {
                wxMemoryInputStream s(im->pngBytes.data(), im->pngBytes.size());
                im->image.LoadFile(s, wxBITMAP_TYPE_PNG);
            }
            label->items.push_back(std::move(im));
        }
    }
    return label;
}
