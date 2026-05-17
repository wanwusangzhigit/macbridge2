#include "gui.h"
#include "app_bundle.h"
#include "dmg.h"
#include "platform.h"
#include "macho.h"
#include "dyld.h"
#include "syscall.h"
#include "vfs.h"
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/log.h>

wxBEGIN_EVENT_TABLE(WinDarlingFrame, wxFrame)
    EVT_MENU(wxID_EXIT, WinDarlingFrame::OnQuit)
    EVT_MENU(wxID_ANY, WinDarlingFrame::OnInstallDMG)
    EVT_BUTTON(wxID_ANY, WinDarlingFrame::OnInstallDMG)
    EVT_BUTTON(wxID_ANY, WinDarlingFrame::OnListApps)
    EVT_BUTTON(wxID_ANY, WinDarlingFrame::OnUninstallApp)
    EVT_BUTTON(wxID_ANY, WinDarlingFrame::OnDMGInfo)
    EVT_BUTTON(wxID_ANY, WinDarlingFrame::OnTestMacho)
    EVT_MENU(wxID_ABOUT, WinDarlingFrame::OnAbout)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(InstallDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, InstallDialog::OnBrowse)
    EVT_BUTTON(wxID_ANY, InstallDialog::OnInstall)
    EVT_BUTTON(wxID_CANCEL, InstallDialog::OnCancel)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(AppListDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, AppListDialog::OnRefresh)
    EVT_BUTTON(wxID_CLOSE, AppListDialog::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(UninstallDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, UninstallDialog::OnUninstall)
    EVT_BUTTON(wxID_CANCEL, UninstallDialog::OnCancel)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(DMGInfoDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, DMGInfoDialog::OnBrowse)
    EVT_BUTTON(wxID_ANY, DMGInfoDialog::OnAnalyze)
    EVT_BUTTON(wxID_CLOSE, DMGInfoDialog::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(MachOToolDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, MachOToolDialog::OnBrowse)
    EVT_BUTTON(wxID_ANY, MachOToolDialog::OnTest)
    EVT_BUTTON(wxID_CLOSE, MachOToolDialog::OnClose)
wxEND_EVENT_TABLE()

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

WinDarlingFrame::WinDarlingFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    SetMinSize(wxSize(800, 600));
    
    CreateMenuBar();
    
    panel = new wxPanel(this, wxID_ANY);
    mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* titleText = new wxStaticText(panel, wxID_ANY, 
        "WinDarling", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    titleText->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainSizer->Add(titleText, 0, wxALL | wxEXPAND, 20);
    
    wxStaticText* subtitleText = new wxStaticText(panel, wxID_ANY,
        "在 Windows 上运行 macOS 应用",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    subtitleText->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    mainSizer->Add(subtitleText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    
    wxStaticLine* line = new wxStaticLine(panel, wxID_ANY);
    mainSizer->Add(line, 0, wxLEFT | wxRIGHT | wxEXPAND, 50);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxVERTICAL);
    
    wxButton* installBtn = new wxButton(panel, wxID_ANY, "📦 安装 DMG 应用",
        wxDefaultPosition, wxSize(300, 50));
    installBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    buttonSizer->Add(installBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    wxButton* listBtn = new wxButton(panel, wxID_ANY, "📋 查看已安装应用",
        wxDefaultPosition, wxSize(300, 50));
    listBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    buttonSizer->Add(listBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    wxButton* uninstallBtn = new wxButton(panel, wxID_ANY, "🗑️  卸载应用",
        wxDefaultPosition, wxSize(300, 50));
    uninstallBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    buttonSizer->Add(uninstallBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    wxButton* dmgInfoBtn = new wxButton(panel, wxID_ANY, "ℹ️  DMG 文件信息",
        wxDefaultPosition, wxSize(300, 50));
    dmgInfoBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    buttonSizer->Add(dmgInfoBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    wxButton* machoBtn = new wxButton(panel, wxID_ANY, "🔍 测试 Mach-O 文件",
        wxDefaultPosition, wxSize(300, 50));
    machoBtn->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    buttonSizer->Add(machoBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    mainSizer->Add(buttonSizer, 1, wxALIGN_CENTER | wxALL, 20);
    
    panel->SetSizer(mainSizer);
    
    Centre();
}

void WinDarlingFrame::CreateMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar();
    
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_EXIT, "退出\tCtrl+Q", "退出程序");
    menuBar->Append(fileMenu, "文件");
    
    wxMenu* actionMenu = new wxMenu();
    actionMenu->Append(wxID_ANY, "安装 DMG 应用\tCtrl+I", "从 DMG 文件安装应用");
    actionMenu->Append(wxID_ANY, "查看已安装应用\tCtrl+L", "显示所有已安装的 macOS 应用");
    actionMenu->Append(wxID_ANY, "卸载应用\tCtrl+U", "卸载已安装的应用");
    actionMenu->AppendSeparator();
    actionMenu->Append(wxID_ANY, "DMG 文件信息\tCtrl+D", "查看 DMG 文件详细信息");
    actionMenu->Append(wxID_ANY, "测试 Mach-O 文件\tCtrl+T", "测试 Mach-O 可执行文件");
    menuBar->Append(actionMenu, "操作");
    
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "关于 WinDarling", "关于本程序");
    menuBar->Append(helpMenu, "帮助");
    
    SetMenuBar(menuBar);
    
    CreateStatusBar();
    SetStatusText("就绪");
}

void WinDarlingFrame::OnQuit(wxCommandEvent& event) {
    Close(true);
}

void WinDarlingFrame::OnInstallDMG(wxCommandEvent& event) {
    InstallDialog* dialog = new InstallDialog(this, "安装 DMG 应用");
    dialog->ShowModal();
    dialog->Destroy();
}

void WinDarlingFrame::OnListApps(wxCommandEvent& event) {
    AppListDialog* dialog = new AppListDialog(this, "已安装应用列表");
    dialog->ShowModal();
    dialog->Destroy();
}

void WinDarlingFrame::OnUninstallApp(wxCommandEvent& event) {
    UninstallDialog* dialog = new UninstallDialog(this, "卸载应用");
    dialog->ShowModal();
    dialog->Destroy();
}

void WinDarlingFrame::OnDMGInfo(wxCommandEvent& event) {
    DMGInfoDialog* dialog = new DMGInfoDialog(this, "DMG 文件信息");
    dialog->ShowModal();
    dialog->Destroy();
}

void WinDarlingFrame::OnTestMacho(wxCommandEvent& event) {
    MachOToolDialog* dialog = new MachOToolDialog(this, "Mach-O 测试工具");
    dialog->ShowModal();
    dialog->Destroy();
}

void WinDarlingFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox(
        "WinDarling v0.8.0\n\n"
        "一个让你在 Windows 上运行 macOS 应用的工具\n\n"
        "功能特点：\n"
        "• 支持 DMG 镜像安装\n"
        "• 应用包管理\n"
        "• Mach-O 二进制分析\n"
        "• 系统调用转换",
        "关于 WinDarling",
        wxOK | wxICON_INFORMATION
    );
}

InstallDialog::InstallDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(600, 300))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* instructionText = new wxStaticText(this, wxID_ANY,
        "选择一个 DMG 文件进行安装：");
    mainSizer->Add(instructionText, 0, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    
    filePathCtrl = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(400, 25), wxTE_READONLY);
    fileSizer->Add(filePathCtrl, 1, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    browseButton = new wxButton(this, wxID_ANY, "浏览...");
    fileSizer->Add(browseButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(fileSizer, 0, wxALL | wxEXPAND, 10);
    
    statusText = new wxStaticText(this, wxID_ANY, "请选择 DMG 文件");
    mainSizer->Add(statusText, 0, wxALL | wxEXPAND, 10);
    
    progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(500, 25));
    mainSizer->Add(progressGauge, 0, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    installButton = new wxButton(this, wxID_ANY, "安装");
    installButton->Disable();
    buttonSizer->Add(installButton, 0, wxALL, 5);
    
    cancelButton = new wxButton(this, wxID_CANCEL, "取消");
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    Centre();
}

InstallDialog::~InstallDialog() {}

void InstallDialog::OnBrowse(wxCommandEvent& event) {
    wxFileDialog dialog(this, "选择 DMG 文件",
        "", "", "DMG 文件 (*.dmg)|*.dmg|ISO 文件 (*.iso)|*.iso|所有文件 (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        selectedFile = dialog.GetPath();
        filePathCtrl->SetValue(selectedFile);
        statusText->SetLabel("已选择文件：" + selectedFile);
        installButton->Enable();
    }
}

void InstallDialog::OnInstall(wxCommandEvent& event) {
    if (selectedFile.IsEmpty()) {
        wxMessageBox("请先选择一个 DMG 文件！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    installButton->Disable();
    browseButton->Disable();
    statusText->SetLabel("正在打开 DMG 文件...");
    progressGauge->SetValue(20);
    
    dmg_context* ctx = NULL;
    if (!dmg_open(selectedFile.mb_str(), &ctx)) {
        wxMessageBox("无法打开 DMG 文件！\n\n可能的原因：\n"
                    "• 文件格式不支持\n"
                    "• 文件已损坏\n"
                    "• 需要管理员权限",
                    "错误", wxOK | wxICON_ERROR);
        statusText->SetLabel("安装失败");
        installButton->Enable();
        browseButton->Enable();
        progressGauge->SetValue(0);
        return;
    }
    
    progressGauge->SetValue(40);
    statusText->SetLabel("正在提取应用...");
    
    if (ctx->is_raw_hfs) {
        dmg_extract_from_raw_hfs(ctx, "./extracted");
    } else {
        dmg_find_and_extract_app(ctx, "*.app", "./extracted");
    }
    
    progressGauge->SetValue(80);
    statusText->SetLabel("正在完成安装...");
    
    dmg_close(ctx);
    
    progressGauge->SetValue(100);
    statusText->SetLabel("安装完成！");
    
    wxMessageBox("应用安装成功！\n\n应用已提取到：./extracted",
                "安装完成", wxOK | wxICON_INFORMATION);
    
    EndModal(wxID_OK);
}

void InstallDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

AppListDialog::AppListDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* headerText = new wxStaticText(this, wxID_ANY, "已安装的 macOS 应用：");
    mainSizer->Add(headerText, 0, wxALL, 10);
    
    appListCtrl = new wxListCtrl(this, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL);
    
    appListCtrl->InsertColumn(0, "应用名称", wxLIST_FORMAT_LEFT, 200);
    appListCtrl->InsertColumn(1, "Bundle ID", wxLIST_FORMAT_LEFT, 250);
    appListCtrl->InsertColumn(2, "版本", wxLIST_FORMAT_LEFT, 100);
    appListCtrl->InsertColumn(3, "安装路径", wxLIST_FORMAT_LEFT, 250);
    
    mainSizer->Add(appListCtrl, 1, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    refreshButton = new wxButton(this, wxID_ANY, "刷新");
    buttonSizer->Add(refreshButton, 0, wxALL, 5);
    
    closeButton = new wxButton(this, wxID_CLOSE, "关闭");
    buttonSizer->Add(closeButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    
    PopulateAppList();
    
    Centre();
}

AppListDialog::~AppListDialog() {}

void AppListDialog::PopulateAppList() {
    appListCtrl->DeleteAllItems();
    
    if (!app_manager_init(NULL)) {
        wxMessageBox("无法初始化应用管理器！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    app_bundle_list_installed();
    
    app_manager_cleanup();
}

void AppListDialog::OnRefresh(wxCommandEvent& event) {
    PopulateAppList();
}

void AppListDialog::OnClose(wxCommandEvent& event) {
    EndModal(wxID_CLOSE);
}

UninstallDialog::UninstallDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    infoText = new wxStaticText(this, wxID_ANY, "选择一个应用进行卸载：");
    mainSizer->Add(infoText, 0, wxALL, 10);
    
    appListCtrl = new wxListCtrl(this, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL);
    
    appListCtrl->InsertColumn(0, "应用名称", wxLIST_FORMAT_LEFT, 200);
    appListCtrl->InsertColumn(1, "Bundle ID", wxLIST_FORMAT_LEFT, 300);
    
    mainSizer->Add(appListCtrl, 1, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    uninstallButton = new wxButton(this, wxID_ANY, "卸载");
    buttonSizer->Add(uninstallButton, 0, wxALL, 5);
    
    cancelButton = new wxButton(this, wxID_CANCEL, "取消");
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    
    if (!app_manager_init(NULL)) {
        wxMessageBox("无法初始化应用管理器！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    app_bundle_list_installed();
    
    app_manager_cleanup();
    
    Centre();
}

UninstallDialog::~UninstallDialog() {}

void UninstallDialog::OnSelectApp(wxCommandEvent& event) {
    long sel = appListCtrl->GetFirstSelected();
    if (sel < 0) {
        wxMessageBox("请先选择一个应用！", "提示", wxOK | wxICON_WARNING);
        return;
    }
}

void UninstallDialog::OnUninstall(wxCommandEvent& event) {
    long sel = appListCtrl->GetFirstSelected();
    if (sel < 0) {
        wxMessageBox("请先选择一个应用！", "提示", wxOK | wxICON_WARNING);
        return;
    }
    
    wxString appName = appListCtrl->GetItemText(sel, 0);
    wxString bundleId = appListCtrl->GetItemText(sel, 1);
    
    int ret = wxMessageBox(
        "确定要卸载 " + appName + " 吗？\n\n此操作不可恢复！",
        "确认卸载",
        wxYES_NO | wxICON_QUESTION
    );
    
    if (ret == wxYES) {
        if (app_manager_init(NULL)) {
            if (app_bundle_uninstall(bundleId.mb_str())) {
                wxMessageBox("应用卸载成功！", "成功", wxOK | wxICON_INFORMATION);
                app_manager_cleanup();
                EndModal(wxID_OK);
            } else {
                wxMessageBox("应用卸载失败！", "错误", wxOK | wxICON_ERROR);
                app_manager_cleanup();
            }
        }
    }
}

void UninstallDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

DMGInfoDialog::DMGInfoDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* headerText = new wxStaticText(this, wxID_ANY, "选择 DMG 文件查看信息：");
    mainSizer->Add(headerText, 0, wxALL, 10);
    
    wxBoxSizer* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    
    filePathCtrl = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(450, 25), wxTE_READONLY);
    fileSizer->Add(filePathCtrl, 1, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    browseButton = new wxButton(this, wxID_ANY, "浏览...");
    fileSizer->Add(browseButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(fileSizer, 0, wxALL | wxEXPAND, 10);
    
    fileInfoText = new wxStaticText(this, wxID_ANY, "");
    mainSizer->Add(fileInfoText, 0, wxALL | wxEXPAND, 10);
    
    infoTextCtrl = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(650, 250),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    mainSizer->Add(infoTextCtrl, 1, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    analyzeButton = new wxButton(this, wxID_ANY, "分析");
    analyzeButton->Disable();
    buttonSizer->Add(analyzeButton, 0, wxALL, 5);
    
    closeButton = new wxButton(this, wxID_CLOSE, "关闭");
    buttonSizer->Add(closeButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    Centre();
}

DMGInfoDialog::~DMGInfoDialog() {}

void DMGInfoDialog::OnBrowse(wxCommandEvent& event) {
    wxFileDialog dialog(this, "选择 DMG 文件",
        "", "", "DMG 文件 (*.dmg)|*.dmg|ISO 文件 (*.iso)|*.iso|所有文件 (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxString selectedFile = dialog.GetPath();
        filePathCtrl->SetValue(selectedFile);
        fileInfoText->SetLabel("已选择文件：" + selectedFile);
        analyzeButton->Enable();
    }
}

void DMGInfoDialog::OnAnalyze(wxCommandEvent& event) {
    wxString selectedFile = filePathCtrl->GetValue();
    
    if (selectedFile.IsEmpty()) {
        wxMessageBox("请先选择一个 DMG 文件！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    dmg_context* ctx = NULL;
    if (!dmg_open(selectedFile.mb_str(), &ctx)) {
        wxMessageBox("无法打开 DMG 文件！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    infoTextCtrl->SetValue("正在分析 DMG 文件...\n\n");
    infoTextCtrl->AppendText("DMG 信息：\n");
    infoTextCtrl->AppendText("================================\n");
    
    dmg_print_info(ctx);
    
    dmg_close(ctx);
}

void DMGInfoDialog::OnClose(wxCommandEvent& event) {
    EndModal(wxID_CLOSE);
}

MachOToolDialog::MachOToolDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* headerText = new wxStaticText(this, wxID_ANY, "选择 Mach-O 文件进行测试：");
    mainSizer->Add(headerText, 0, wxALL, 10);
    
    wxBoxSizer* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    
    filePathCtrl = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(450, 25), wxTE_READONLY);
    fileSizer->Add(filePathCtrl, 1, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    browseButton = new wxButton(this, wxID_ANY, "浏览...");
    fileSizer->Add(browseButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(fileSizer, 0, wxALL | wxEXPAND, 10);
    
    fileInfoText = new wxStaticText(this, wxID_ANY, "");
    mainSizer->Add(fileInfoText, 0, wxALL | wxEXPAND, 10);
    
    resultTextCtrl = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxSize(650, 250),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    mainSizer->Add(resultTextCtrl, 1, wxALL | wxEXPAND, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    testButton = new wxButton(this, wxID_ANY, "测试");
    testButton->Disable();
    buttonSizer->Add(testButton, 0, wxALL, 5);
    
    closeButton = new wxButton(this, wxID_CLOSE, "关闭");
    buttonSizer->Add(closeButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    Centre();
}

MachOToolDialog::~MachOToolDialog() {}

void MachOToolDialog::OnBrowse(wxCommandEvent& event) {
    wxFileDialog dialog(this, "选择 Mach-O 文件",
        "", "",
        "Mach-O 文件 (*.app)|*.app|动态库 (*.dylib)|*.dylib|束 (*.bundle)|*.bundle|所有文件 (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxString selectedFile = dialog.GetPath();
        filePathCtrl->SetValue(selectedFile);
        fileInfoText->SetLabel("已选择文件：" + selectedFile);
        testButton->Enable();
    }
}

void MachOToolDialog::OnTest(wxCommandEvent& event) {
    wxString selectedFile = filePathCtrl->GetValue();
    
    if (selectedFile.IsEmpty()) {
        wxMessageBox("请先选择一个 Mach-O 文件！", "错误", wxOK | wxICON_ERROR);
        return;
    }
    
    resultTextCtrl->SetValue("正在测试 Mach-O 文件...\n\n");
    
    init_syscall_table();
    vfs_init();
    dyld_init();
    
    struct mach_header* header = NULL;
    bool is_64bit = false;
    
    if (!parse_macho_header(selectedFile.mb_str(), &header, &is_64bit)) {
        resultTextCtrl->AppendText("错误：无法解析 Mach-O 文件头！\n");
        dyld_cleanup();
        vfs_cleanup();
        return;
    }
    
    resultTextCtrl->AppendText("Mach-O 文件头信息：\n");
    resultTextCtrl->AppendText("================================\n");
    resultTextCtrl->AppendText(wxString::Format("Magic: 0x%x\n", header->magic));
    resultTextCtrl->AppendText(wxString::Format("CPU Type: 0x%x\n", header->cputype));
    resultTextCtrl->AppendText(wxString::Format("文件类型: 0x%x\n", header->filetype));
    resultTextCtrl->AppendText(wxString::Format("Load Commands 数量: %d\n", header->ncmds));
    resultTextCtrl->AppendText(wxString::Format("Load Commands 大小: %d\n", header->sizeofcmds));
    resultTextCtrl->AppendText(wxString::Format("64位: %s\n\n", is_64bit ? "是" : "否"));
    
    void* base_address = NULL;
    if (!map_macho_segments(selectedFile.mb_str(), header, is_64bit, &base_address)) {
        resultTextCtrl->AppendText("错误：无法映射 Mach-O 段！\n");
        free(header);
        dyld_cleanup();
        vfs_cleanup();
        return;
    }
    
    resultTextCtrl->AppendText("段映射：\n");
    resultTextCtrl->AppendText("================================\n");
    resultTextCtrl->AppendText(wxString::Format("基地址: %p\n\n", base_address));
    
    void* main_addr = find_main_function(selectedFile.mb_str(), header, is_64bit, base_address);
    resultTextCtrl->AppendText("主函数：\n");
    resultTextCtrl->AppendText("================================\n");
    if (main_addr) {
        resultTextCtrl->AppendText(wxString::Format("地址: %p\n", main_addr));
        resultTextCtrl->AppendText("状态: 找到\n");
    } else {
        resultTextCtrl->AppendText("状态: 未找到\n");
    }
    
    unmap_macho_segments(base_address, header, is_64bit);
    free(header);
    dyld_cleanup();
    vfs_cleanup();
    
    resultTextCtrl->AppendText("\n测试完成！");
}

void MachOToolDialog::OnClose(wxCommandEvent& event) {
    EndModal(wxID_CLOSE);
}
