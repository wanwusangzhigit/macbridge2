#include <wx/app.h>
#include "gui.h"

class WinDarlingApp : public wxApp {
public:
    virtual bool OnInit() override;
};

wxIMPLEMENT_APP(WinDarlingApp);

bool WinDarlingApp::OnInit() {
    wxInitAllImageHandlers();
    
    WinDarlingFrame* frame = new WinDarlingFrame(
        "WinDarling - macOS 应用管理器",
        wxDefaultPosition,
        wxSize(900, 650)
    );
    frame->Show(true);
    return true;
}

int main(int argc, char** argv) {
    return wxEntry(argc, argv);
}
