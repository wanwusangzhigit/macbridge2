#ifndef WINDARLING_GUI_H
#define WINDARLING_GUI_H

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/messagebox.h>
#include <wx/gauge.h>
#include <wx/dialog.h>
#include <wx/treectrl.h>
#include <wx/artprov.h>
#include <wx/icon.h>

class WinDarlingFrame : public wxFrame {
public:
    WinDarlingFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    
private:
    void OnQuit(wxCommandEvent& event);
    void OnInstallDMG(wxCommandEvent& event);
    void OnListApps(wxCommandEvent& event);
    void OnUninstallApp(wxCommandEvent& event);
    void OnDMGInfo(wxCommandEvent& event);
    void OnTestMacho(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void CreateMenuBar();
    
    wxPanel* panel;
    wxBoxSizer* mainSizer;
    
    wxDECLARE_EVENT_TABLE();
};

class InstallDialog : public wxDialog {
public:
    InstallDialog(wxWindow* parent, const wxString& title);
    ~InstallDialog();
    
    wxString GetSelectedFile() const { return selectedFile; }
    
private:
    void OnBrowse(wxCommandEvent& event);
    void OnInstall(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void UpdateInstallButton();
    
    wxTextCtrl* filePathCtrl;
    wxButton* browseButton;
    wxButton* installButton;
    wxButton* cancelButton;
    wxStaticText* statusText;
    wxGauge* progressGauge;
    wxString selectedFile;
    
    wxDECLARE_EVENT_TABLE();
};

class AppListDialog : public wxDialog {
public:
    AppListDialog(wxWindow* parent, const wxString& title);
    ~AppListDialog();
    
private:
    void PopulateAppList();
    void OnRefresh(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    
    wxListCtrl* appListCtrl;
    wxButton* refreshButton;
    wxButton* closeButton;
    
    wxDECLARE_EVENT_TABLE();
};

class UninstallDialog : public wxDialog {
public:
    UninstallDialog(wxWindow* parent, const wxString& title);
    ~UninstallDialog();
    
private:
    void OnUninstall(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSelectApp(wxCommandEvent& event);
    
    wxListCtrl* appListCtrl;
    wxButton* uninstallButton;
    wxButton* cancelButton;
    wxStaticText* infoText;
    
    wxDECLARE_EVENT_TABLE();
};

class DMGInfoDialog : public wxDialog {
public:
    DMGInfoDialog(wxWindow* parent, const wxString& title);
    ~DMGInfoDialog();
    
private:
    void OnBrowse(wxCommandEvent& event);
    void OnAnalyze(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    
    wxTextCtrl* filePathCtrl;
    wxButton* browseButton;
    wxButton* analyzeButton;
    wxButton* closeButton;
    wxTextCtrl* infoTextCtrl;
    wxStaticText* fileInfoText;
    
    wxDECLARE_EVENT_TABLE();
};

class MachOToolDialog : public wxDialog {
public:
    MachOToolDialog(wxWindow* parent, const wxString& title);
    ~MachOToolDialog();
    
private:
    void OnBrowse(wxCommandEvent& event);
    void OnTest(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    
    wxTextCtrl* filePathCtrl;
    wxButton* browseButton;
    wxButton* testButton;
    wxButton* closeButton;
    wxTextCtrl* resultTextCtrl;
    wxStaticText* fileInfoText;
    
    wxDECLARE_EVENT_TABLE();
};

#endif
