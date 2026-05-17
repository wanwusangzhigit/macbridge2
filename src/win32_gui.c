#include "win32_gui.h"
#include "app_bundle.h"
#include "dmg.h"
#include "platform.h"
#include "macho.h"
#include "dyld.h"
#include "syscall.h"
#include "vfs.h"
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <winnls.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

static HINSTANCE hInst;

void SetButtonIcon(HWND hBtn, int iconIndex) {
    HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
    SendMessage(hBtn, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
}

void CreateMainButtons(HWND hwnd, HINSTANCE hInstance) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    
    int btnWidth = 280;
    int btnHeight = 50;
    int spacing = 20;
    
    int startX = (clientWidth - btnWidth) / 2;
    int startY = (clientHeight - (btnHeight * 5 + spacing * 4)) / 2 + 60;
    
    HWND hTitle = CreateWindowW(L"STATIC", L"WinDarling", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 20, clientWidth, 40, hwnd, NULL, hInstance, NULL);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    
    HWND hSubtitle = CreateWindowW(L"STATIC", L"在 Windows 上运行 macOS 应用", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 65, clientWidth, 20, hwnd, NULL, hInstance, NULL);
    
    HWND hBtnInstall = CreateWindowW(L"BUTTON", L"📦 安装 DMG 应用",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        startX, startY, btnWidth, btnHeight, hwnd, (HMENU)IDC_INSTALL_BTN, hInstance, NULL);
    
    HWND hBtnList = CreateWindowW(L"BUTTON", L"📋 查看已安装应用",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        startX, startY + btnHeight + spacing, btnWidth, btnHeight, hwnd, (HMENU)IDC_LIST_BTN, hInstance, NULL);
    
    HWND hBtnUninstall = CreateWindowW(L"BUTTON", L"🗑️  卸载应用",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        startX, startY + (btnHeight + spacing) * 2, btnWidth, btnHeight, hwnd, (HMENU)IDC_UNINSTALL_BTN, hInstance, NULL);
    
    HWND hBtnDMGInfo = CreateWindowW(L"BUTTON", L"ℹ️  DMG 文件信息",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        startX, startY + (btnHeight + spacing) * 3, btnWidth, btnHeight, hwnd, (HMENU)IDC_DMGINFO_BTN, hInstance, NULL);
    
    HWND hBtnMacho = CreateWindowW(L"BUTTON", L"🔍 测试 Mach-O 文件",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        startX, startY + (btnHeight + spacing) * 4, btnWidth, btnHeight, hwnd, (HMENU)IDC_MACHOTEST_BTN, hInstance, NULL);
}

LRESULT CALLBACK WinDarlingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE:
            hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            CreateMainButtons(hwnd, hInst);
            break;
            
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_INSTALL_BTN:
                    DialogBox(hInst, MAKEINTRESOURCE(101), hwnd, InstallDlgProc);
                    break;
                case IDC_LIST_BTN:
                    DialogBox(hInst, MAKEINTRESOURCE(102), hwnd, AppListDlgProc);
                    break;
                case IDC_UNINSTALL_BTN:
                    DialogBox(hInst, MAKEINTRESOURCE(103), hwnd, UninstallDlgProc);
                    break;
                case IDC_DMGINFO_BTN:
                    DialogBox(hInst, MAKEINTRESOURCE(104), hwnd, DMGInfoDlgProc);
                    break;
                case IDC_MACHOTEST_BTN:
                    DialogBox(hInst, MAKEINTRESOURCE(105), hwnd, MachOTestDlgProc);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static char g_selectedDmgPath[MAX_PATH] = {0};

LRESULT CALLBACK InstallDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwnd, IDC_FILE_PATH, "");
            EnableWindow(GetDlgItem(hwnd, IDC_INSTALL_BTN), FALSE);
            return TRUE;
        }
        
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_BROWSE_BTN: {
                    OPENFILENAME ofn;
                    char szFile[MAX_PATH] = {0};
                    
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"DMG 文件 (*.dmg)\0*.dmg\0ISO 文件 (*.iso)\0*.iso\0所有文件 (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    
                    if(GetOpenFileNameW(&ofn) == TRUE) {
                        strncpy(g_selectedDmgPath, szFile, MAX_PATH - 1);
                        SetDlgItemText(hwnd, IDC_FILE_PATH, szFile);
                        EnableWindow(GetDlgItem(hwnd, IDC_INSTALL_BTN), TRUE);
                    }
                    break;
                }
                
                case IDC_INSTALL_BTN: {
                    if(g_selectedDmgPath[0] == 0) {
                        MessageBoxW(hwnd, L"请先选择一个 DMG 文件！", L"提示", MB_OK | MB_ICONINFORMATION);
                        return TRUE;
                    }
                    
                    SetDlgItemTextW(hwnd, IDC_INSTALL_STATUS, L"正在打开 DMG 文件...");
                    SendDlgItemMessage(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, 20, 0);
                    UpdateWindow(hwnd);
                    
                    dmg_context* ctx = NULL;
                    if(!dmg_open(g_selectedDmgPath, &ctx)) {
                        MessageBoxW(hwnd, L"无法打开 DMG 文件！", L"错误", MB_OK | MB_ICONERROR);
                        SetDlgItemTextW(hwnd, IDC_INSTALL_STATUS, L"安装失败");
                        SendDlgItemMessage(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, 0, 0);
                        return TRUE;
                    }
                    
                    SetDlgItemTextW(hwnd, IDC_INSTALL_STATUS, L"正在提取应用...");
                    SendDlgItemMessage(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, 50, 0);
                    UpdateWindow(hwnd);
                    
                    if(ctx->is_raw_hfs) {
                        dmg_extract_from_raw_hfs(ctx, "./extracted");
                    } else {
                        dmg_find_and_extract_app(ctx, "*.app", "./extracted");
                    }
                    
                    SetDlgItemTextW(hwnd, IDC_INSTALL_STATUS, L"正在完成安装...");
                    SendDlgItemMessage(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, 80, 0);
                    UpdateWindow(hwnd);
                    
                    dmg_close(ctx);
                    
                    SendDlgItemMessage(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, 100, 0);
                    SetDlgItemTextW(hwnd, IDC_INSTALL_STATUS, L"安装完成！");
                    UpdateWindow(hwnd);
                    
                    MessageBoxW(hwnd, L"应用安装成功！

应用已提取到：./extracted", L"成功", MB_OK | MB_ICONINFORMATION);
                    
                    EndDialog(hwnd, IDOK);
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK AppListDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            HWND hList = GetDlgItem(hwnd, IDC_APP_LIST);
            
            LVCOLUMN col;
            ZeroMemory(&col, sizeof(LVCOLUMN));
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.cx = 150;
            col.pszText = "应用名称";
            ListView_InsertColumn(hList, 0, &col);
            
            col.cx = 200;
            col.pszText = "Bundle ID";
            ListView_InsertColumn(hList, 1, &col);
            
            col.cx = 80;
            col.pszText = "版本";
            ListView_InsertColumn(hList, 2, &col);
            
            col.cx = 250;
            col.pszText = "路径";
            ListView_InsertColumn(hList, 3, &col);
            
            if(app_manager_init(NULL)) {
                app_bundle_list_installed();
                app_manager_cleanup();
            }
            
            return TRUE;
        }
        
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_REFRESH_BTN: {
                    HWND hList = GetDlgItem(hwnd, IDC_APP_LIST);
                    ListView_DeleteAllItems(hList);
                    
                    if(app_manager_init(NULL)) {
                        app_bundle_list_installed();
                        app_manager_cleanup();
                    }
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK UninstallDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            HWND hList = GetDlgItem(hwnd, IDC_UNINSTALL_LIST);
            
            LVCOLUMN col;
            ZeroMemory(&col, sizeof(LVCOLUMN));
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.cx = 200;
            col.pszText = "应用名称";
            ListView_InsertColumn(hList, 0, &col);
            
            col.cx = 300;
            col.pszText = "Bundle ID";
            ListView_InsertColumn(hList, 1, &col);
            
            if(app_manager_init(NULL)) {
                app_bundle_list_installed();
                app_manager_cleanup();
            }
            
            return TRUE;
        }
        
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDOK: {
                    HWND hList = GetDlgItem(hwnd, IDC_UNINSTALL_LIST);
                    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    
                    if(sel == -1) {
                        MessageBoxW(hwnd, L"请先选择一个应用！", L"提示", MB_OK | MB_ICONINFORMATION);
                        return TRUE;
                    }
                    
                    LVITEM item;
                    char bundleId[MAX_PATH] = {0};
                    char appName[MAX_PATH] = {0};
                    
                    ZeroMemory(&item, sizeof(LVITEM));
                    item.mask = LVIF_TEXT;
                    item.iItem = sel;
                    item.iSubItem = 0;
                    item.pszText = appName;
                    item.cchTextMax = MAX_PATH;
                    ListView_GetItem(hList, &item);
                    
                    item.iSubItem = 1;
                    item.pszText = bundleId;
                    ListView_GetItem(hList, &item);
                    
                    char msg[512];
                    sprintf(msg, "确定要卸载 \"%s\" 吗？\n\n此操作不可恢复！", appName);
                    
                    if(MessageBoxW(hwnd, L"确定要卸载该应用吗？\n\n此操作不可恢复！", L"确认卸载", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        if(app_manager_init(NULL)) {
                            if(app_bundle_uninstall(bundleId)) {
                                MessageBoxW(hwnd, L"应用卸载成功！", L"成功", MB_OK | MB_ICONINFORMATION);
                                app_manager_cleanup();
                                EndDialog(hwnd, IDOK);
                            } else {
                                MessageBoxW(hwnd, L"应用卸载失败！", L"错误", MB_OK | MB_ICONERROR);
                                app_manager_cleanup();
                            }
                        }
                    }
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

static char g_dmgInfoPath[MAX_PATH] = {0};

LRESULT CALLBACK DMGInfoDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwnd, IDC_FILE_PATH, "");
            EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            return TRUE;
        }
        
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_BROWSE_BTN: {
                    OPENFILENAME ofn;
                    char szFile[MAX_PATH] = {0};
                    
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"DMG 文件 (*.dmg)\0*.dmg\0ISO 文件 (*.iso)\0*.iso\0所有文件 (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    
                    if(GetOpenFileNameW(&ofn) == TRUE) {
                        strncpy(g_dmgInfoPath, szFile, MAX_PATH - 1);
                        SetDlgItemText(hwnd, IDC_FILE_PATH, szFile);
                        EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
                    }
                    break;
                }
                
                case IDOK: {
                    if(g_dmgInfoPath[0] == 0) {
                        MessageBoxW(hwnd, L"请先选择一个 DMG 文件！", L"提示", MB_OK | MB_ICONINFORMATION);
                        return TRUE;
                    }
                    
                    dmg_context* ctx = NULL;
                    if(!dmg_open(g_dmgInfoPath, &ctx)) {
                        MessageBoxW(hwnd, L"无法打开 DMG 文件！", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    
                    char info[4096] = {0};
                    dmg_print_info(ctx);
                    
                    dmg_close(ctx);
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

static char g_machoPath[MAX_PATH] = {0};

LRESULT CALLBACK MachOTestDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_INITDIALOG: {
            SetDlgItemText(hwnd, IDC_FILE_PATH, "");
            EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            return TRUE;
        }
        
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_BROWSE_BTN: {
                    OPENFILENAME ofn;
                    char szFile[MAX_PATH] = {0};
                    
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "应用程序 (*.app)\0*.app\0动态库 (*.dylib)\0*.dylib\0所有文件 (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    
                    if(GetOpenFileNameW(&ofn) == TRUE) {
                        strncpy(g_machoPath, szFile, MAX_PATH - 1);
                        SetDlgItemText(hwnd, IDC_FILE_PATH, szFile);
                        EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
                    }
                    break;
                }
                
                case IDOK: {
                    if(g_machoPath[0] == 0) {
                        MessageBoxW(hwnd, L"请先选择一个文件！", L"提示", MB_OK | MB_ICONINFORMATION);
                        return TRUE;
                    }
                    
                    init_syscall_table();
                    vfs_init();
                    dyld_init();
                    
                    struct mach_header* header = NULL;
                    bool is_64bit = false;
                    
                    if(!parse_macho_header(g_machoPath, &header, &is_64bit)) {
                        MessageBoxW(hwnd, L"无法解析 Mach-O 文件！", L"错误", MB_OK | MB_ICONERROR);
                        dyld_cleanup();
                        vfs_cleanup();
                        return TRUE;
                    }
                    
                    void* base_address = NULL;
                    if(!map_macho_segments(g_machoPath, header, is_64bit, &base_address)) {
                        MessageBoxW(hwnd, L"无法映射 Mach-O 段！", L"错误", MB_OK | MB_ICONERROR);
                        free(header);
                        dyld_cleanup();
                        vfs_cleanup();
                        return TRUE;
                    }
                    
                    find_main_function(g_machoPath, header, is_64bit, base_address);
                    
                    unmap_macho_segments(base_address, header, is_64bit);
                    free(header);
                    dyld_cleanup();
                    vfs_cleanup();
                    
                    MessageBoxW(hwnd, L"测试完成！", L"成功", MB_OK | MB_ICONINFORMATION);
                    EndDialog(hwnd, IDOK);
                    break;
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
            
        default:
            return FALSE;
    }
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetConsoleOutputCP(CP_UTF8);
    hInst = hInstance;
    
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WinDarlingWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"WinDarlingClass";
    
    if(!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"窗口注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"WinDarlingClass",
        L"WinDarling - macOS 应用管理器",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 550,
        NULL, NULL, hInstance, NULL);
    
    if(hwnd == NULL) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while(GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return msg.wParam;
}

#endif
