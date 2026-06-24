#pragma once

#include <wx/string.h>
#include <memory>
#include "LabelModel.h"

// SVG save/load. The file is valid SVG (rendered by any viewer) and also
// carries the full editable model inside a <metadata> block so it reopens
// without loss. PNGs are embedded as base64 data URIs.
namespace SvgIO {
    bool Save(const Label& label, const wxString& path);
    std::unique_ptr<Label> Load(const wxString& path);
}
