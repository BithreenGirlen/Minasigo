﻿
#include <Windows.h>
#include <CommCtrl.h>

#include "main_window.h"
#include "file_dialogues.h"
#include "file_system_utility.h"
#include "json_serialisation.h"
#include "minasigo.h"

#pragma comment(lib, "Comctl32.lib")

CMainWindow::CMainWindow()
{

}

CMainWindow::~CMainWindow()
{

}

bool CMainWindow::Create(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
    wcex.lpszClassName = m_class_name.c_str();

    if (::RegisterClassExW(&wcex))
    {
        m_hInstance = hInstance;

        m_hWnd = ::CreateWindowW(m_class_name.c_str(), m_window_name.c_str(), WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 450, nullptr, nullptr, hInstance, this);
        if (m_hWnd != nullptr)
        {
            return true;
        }
        else
        {
            std::wstring wstrMessage = L"CreateWindowExW failed; code: " + std::to_wstring(::GetLastError());
            ::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
        }
    }
    else
    {
        std::wstring wstrMessage = L"RegisterClassW failed; code: " + std::to_wstring(::GetLastError());
        ::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
    }

	return false;
}

int CMainWindow::MessageLoop()
{
    MSG msg;

    for (;;)
    {
        BOOL bRet = ::GetMessageW(&msg, 0, 0, 0);
        if (bRet > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        else if (bRet == 0)
        {
            /*ループ終了*/
            return static_cast<int>(msg.wParam);
        }
        else
        {
            /*ループ異常*/
            std::wstring wstrMessage = L"GetMessageW failed; code: " + std::to_wstring(::GetLastError());
            ::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
            return -1;
        }
    }
    return 0;
}
/*C CALLBACK*/
LRESULT CMainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMainWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE)
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = reinterpret_cast<CMainWindow*>(pCreateStruct->lpCreateParams);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }

    pThis = reinterpret_cast<CMainWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pThis != nullptr)
    {
        return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
    }

    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*メッセージ処理*/
LRESULT CMainWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        return OnCreate(hWnd);
    case WM_DESTROY:
        return OnDestroy();
    case WM_CLOSE:
        return OnClose();
    case WM_PAINT:
        return OnPaint();
    case WM_SIZE:
        return OnSize();
    case WM_COMMAND:
        return OnCommand(wParam);
    }

    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*WM_CREATE*/
LRESULT CMainWindow::OnCreate(HWND hWnd)
{
    m_hWnd = hWnd;

    InitialiseMenuBar();

    m_hrComInit = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(m_hrComInit))
    {
        std::wstring wstrMessage = L"CoInitializeEx failed; code: " + std::to_wstring(::GetLastError());
        ::MessageBoxW(m_hWnd, wstrMessage.c_str(), L"Error", MB_ICONERROR);
    }

    InitialiseWebViewEnvironment();

    SetupMinasigoAuthorisation();

    return 0;
}
/*WM_DESTROY*/
LRESULT CMainWindow::OnDestroy()
{
    if (SUCCEEDED(m_hrComInit))
    {
        ::CoUninitialize();
    }

    ::PostQuitMessage(0);

    return 0;
}
/*WM_CLOSE*/
LRESULT CMainWindow::OnClose()
{
    ::DestroyWindow(m_hWnd);
    ::UnregisterClassW(m_class_name.c_str(), m_hInstance);

    return 0;
}
/*WM_PAINT*/
LRESULT CMainWindow::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(m_hWnd, &ps);

    ::EndPaint(m_hWnd, &ps);

    return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::OnSize()
{
    ResizeWebViewBound();

    return 0;
}
/*WM_COMMAND*/
LRESULT CMainWindow::OnCommand(WPARAM wParam)
{
    int wmKind = HIWORD(wParam);
    int wmId = LOWORD(wParam);
    if (wmKind == 0)
    {
        /*Menus*/
        switch (wmId)
        {
        case Menu::kOpenFile:
            MenuOnOpenFile();
            break;
        case Menu::kDecryptManifest:
            MenuOnDecryptManifest();
            break;
        case Menu::kDecryptPayload:
            MenuOnDecryptPayload();
            break;
        case Menu::kDecryptResponse:
            MenuOnDecryptResponse();
            break;
        case Menu::kDownloadStoryMasterData:
            MenuOnDownloadStoryMasterData();
            break;
        case Menu::kDownloadScenarioPaths:
            MenuOnDownloadScenarioPaths();
            break;
        case Menu::kDownloadScenarios:
            MenuOnDownloadScenarios();
            break;
        case Menu::kDownloadScenarioResourcePath:
            MenuOnDownloadScenarioResourcePaths();
            break;
        case Menu::kDownloadScenarioResources:
            MenuOnDownloadScenarioResources();
            break;
        }
    }
    if (wmKind > 1)
    {
        /*Controls*/
    }

    return 0;
}

/*操作欄作成*/
void CMainWindow::InitialiseMenuBar()
{
    HMENU hMenuFile = nullptr;
    HMENU hMenuDecrypt = nullptr;
    HMENU hMenuDownload = nullptr;
    HMENU hMenuBar = nullptr;
    BOOL iRet = FALSE;

    if (m_hMenuBar != nullptr)return;

    hMenuFile = ::CreateMenu();
    if (hMenuFile == nullptr)goto failed;

    iRet = ::AppendMenuA(hMenuFile, MF_STRING, Menu::kOpenFile, "Open");
    if (iRet == 0)goto failed;

    hMenuDecrypt = ::CreateMenu();
    if (hMenuDecrypt == nullptr)goto failed;

    iRet = ::AppendMenuA(hMenuDecrypt, MF_STRING, Menu::kDecryptManifest, "Manifest");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDecrypt, MF_STRING, Menu::kDecryptPayload, "Payload");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDecrypt, MF_STRING, Menu::kDecryptResponse, "Response");
    if (iRet == 0)goto failed;

    hMenuDownload = ::CreateMenu();
    if (hMenuDownload == nullptr)goto failed;
    iRet = ::AppendMenuA(hMenuDownload, MF_STRING, Menu::kDownloadStoryMasterData, "Story MasterData");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDownload, MF_STRING, Menu::kDownloadScenarioPaths, "Scenario paths");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDownload, MF_STRING, Menu::kDownloadScenarios, "Scenarios");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDownload, MF_STRING, Menu::kDownloadScenarioResourcePath, "Scenario resource paths");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuDownload, MF_STRING, Menu::kDownloadScenarioResources, "Scenario resources");
    if (iRet == 0)goto failed;

    hMenuBar = ::CreateMenu();
    if (hMenuBar == nullptr) goto failed;

    iRet = ::AppendMenuA(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuFile), "File");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuDecrypt), "Decrypt");
    if (iRet == 0)goto failed;
    iRet = ::AppendMenuA(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuDownload), "Download");
    if (iRet == 0)goto failed;

    iRet = ::SetMenu(m_hWnd, hMenuBar);
    if (iRet == 0)goto failed;

    m_hMenuBar = hMenuBar;

    /*正常終了*/
    return;

failed:
    std::wstring wstrMessage = L"Failed to create menu; code: " + std::to_wstring(::GetLastError());
    ::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
    /*SetMenu成功後はウィンドウ破棄時に破棄されるが、今は紐づけ前なのでここで破棄する。*/
    if (hMenuFile != nullptr)
    {
        ::DestroyMenu(hMenuFile);
    }
    if (hMenuDecrypt != nullptr)
    {
        ::DestroyMenu(hMenuDecrypt);
    }
    if (hMenuDownload != nullptr)
    {
        ::DestroyMenu(hMenuDownload);
    }
    if (hMenuBar != nullptr)
    {
        ::DestroyMenu(hMenuBar);
    }

}
/*HTML選択*/
void CMainWindow::MenuOnOpenFile()
{
    wchar_t* pBuffer = SelectHtmlFileToOpen(m_hWnd);
    if (pBuffer != nullptr)
    {
        if (m_pWebView != nullptr)
        {
            m_pWebView->Navigate(pBuffer);
        }

        ::CoTaskMemFree(pBuffer);
    }
}
/*資源一覧ファイル復号*/
void CMainWindow::MenuOnDecryptManifest()
{
    wil::unique_cotaskmem_string pwBuffer(SelectJsonFileToOpen(m_hWnd));
    if (pwBuffer != nullptr)
    {
        std::wstring wstrText = LoadFileAsString(pwBuffer.get());
        if (!wstrText.empty())
        {
            std::wstring wstrResult = ExecuteDecryptManifestFunctionOnWebPage(wstrText);
            if (!wstrResult.empty())
            {
                wil::unique_cotaskmem_string pwBuffer2(SelectJsonFileToSave(GetFileNameFromFilePath(pwBuffer.get()).c_str(), m_hWnd));
                if (pwBuffer2 != nullptr)
                {
                    WriteStringToFile(wstrResult, pwBuffer2.get());
                }
            }
            else
            {
                ::MessageBoxW(nullptr, L"Failed to decrypt the text.", L"Error", MB_ICONERROR);
            }
        }
    }
}
/*送信ペイロード復号*/
void CMainWindow::MenuOnDecryptPayload()
{
    wil::unique_cotaskmem_string pwBuffer(SelectTextFileToOpen(m_hWnd));
    if (pwBuffer != nullptr)
    {
        std::wstring wstrText = LoadFileAsString(pwBuffer.get());
        if (!wstrText.empty())
        {
            std::wstring wstrResult = ExecuteDecryptDataFunctionOnWebPage(wstrText, minasigo::GetToken());
            if (!wstrResult.empty())
            {
                wil::unique_cotaskmem_string pwBuffer2(SelectTextFileToSave(GetFileNameFromFilePath(pwBuffer.get()).c_str(), m_hWnd));
                if (pwBuffer2 != nullptr)
                {
                    WriteStringToFile(wstrResult, pwBuffer2.get());
                }
            }
            else
            {
                ::MessageBoxW(nullptr, L"Failed to decrypt the text.", L"Error", MB_ICONERROR);
            }
        }
    }
}
/*受信伝文復号*/
void CMainWindow::MenuOnDecryptResponse()
{
    wil::unique_cotaskmem_string pwBuffer(SelectTextFileToOpen(m_hWnd));
    if (pwBuffer != nullptr)
    {
        std::wstring wstrText = LoadFileAsString(pwBuffer.get());
        if (wstrText.size() > 2)
        {
            std::wstring wstrResult = ExecuteDecryptDataFunctionOnWebPage(wstrText.substr(1, wstrText.size() - 2), minasigo::GetToken());
            if (!wstrResult.empty())
            {
                wil::unique_cotaskmem_string pwBuffer2(SelectTextFileToSave(GetFileNameFromFilePath(pwBuffer.get()).c_str(), m_hWnd));
                if (pwBuffer2 != nullptr)
                {
                    WriteStringToFile(wstrResult, pwBuffer2.get());
                }
            }
            else
            {
                ::MessageBoxW(nullptr, L"Failed to decrypt the text.", L"Error", MB_ICONERROR);
            }
        }
    }
}
/*Menu::kDownloadStoryMasterData*/
void CMainWindow::MenuOnDownloadStoryMasterData()
{
    minasigo::GetStoryMasterData();
}
/*Menu::kDownloadScenarioPaths*/
void CMainWindow::MenuOnDownloadScenarioPaths()
{
    minasigo::GetScenarioPaths();
}
/*Menu::kDownloadScenarios*/
void CMainWindow::MenuOnDownloadScenarios()
{
    minasigo::DownloadScenarios();
}
/*Menu::kDownloadScenarioResourcePath*/
void CMainWindow::MenuOnDownloadScenarioResourcePaths()
{
    minasigo::GetScenarioResourcePaths();
}
/*Menu::kDownloadScenarioResources*/
void CMainWindow::MenuOnDownloadScenarioResources()
{
    minasigo::DownloadScenarioResources();
}
/*WebView2環境初期設定*/
void CMainWindow::InitialiseWebViewEnvironment()
{
    HRESULT hr = ::CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &CMainWindow::OnCreateEnvironmentCompleted).Get()
    );
}
/*ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler Callback*/
HRESULT CMainWindow::OnCreateEnvironmentCompleted(HRESULT hResult, ICoreWebView2Environment* pWebViewEnvironment)
{
    if (SUCCEEDED(hResult) && pWebViewEnvironment != nullptr)
    {
        m_pWebView2Environment = pWebViewEnvironment;

        HRESULT hr = m_pWebView2Environment->CreateCoreWebView2Controller(
            m_hWnd,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(this, &CMainWindow::OnCreateWebViewControllerCompleted).Get()
        );
    }

    return S_OK;
}
/*ICoreWebView2CreateCoreWebView2ControllerCompletedHandler Callback*/
HRESULT CMainWindow::OnCreateWebViewControllerCompleted(HRESULT hResult, ICoreWebView2Controller* pWebViewController)
{
    if (SUCCEEDED(hResult) && pWebViewController != nullptr)
    {
        m_pWebViewController = pWebViewController;

        HRESULT hr = m_pWebViewController->get_CoreWebView2(&m_pWebView);
        if (SUCCEEDED(hr))
        {
            ResizeWebViewBound();

            m_pWebView->Navigate(CreateWorkFolder(nullptr).c_str());
        }
    }

    return S_OK;
}
/*WebView2表示位置・寸法指定*/
void CMainWindow::ResizeWebViewBound()
{
    if (m_pWebViewController != nullptr)
    {
        RECT srcRect{};
        ::GetClientRect(m_hWnd, &srcRect);
        m_pWebViewController->put_Bounds(srcRect);
    }
}

/*function decryptManifest(t)*/
std::wstring CMainWindow::ExecuteDecryptManifestFunctionOnWebPage(const std::wstring& wstrText)
{
    std::wstring wstrScript = L"decryptManifest(" + SerialiseJsonString(wstrText) + L")";
    return ExecuteScriptOnWebPage(wstrScript);
}
/*function encryptData(t, e)*/
std::wstring CMainWindow::ExecuteEncryptDataFunctionOnWebPage(const std::wstring& wstrText, const std::wstring& wstrToken)
{
    std::wstring wstrScript = L"encryptData(" + SerialiseJsonString(wstrText) + L"," + SerialiseJsonString(wstrToken) + L")";
    return ExecuteScriptOnWebPage(wstrScript);
}
/*function decryptData(t, e)*/
std::wstring CMainWindow::ExecuteDecryptDataFunctionOnWebPage(const std::wstring& wstrText, const std::wstring& wstrToken)
{
    std::wstring wstrScript = L"decryptData(" + SerialiseJsonString(wstrText) + L"," + SerialiseJsonString(wstrToken) + L")";
    return ExecuteScriptOnWebPage(wstrScript);
}
/*スクリプト同期実行*/
std::wstring CMainWindow::ExecuteScriptOnWebPage(const std::wstring& wstrScript)
{
    if (m_pWebView != nullptr)
    {
        wil::unique_handle hEvent(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
        std::wstring wstrResult;
        HRESULT hr = m_pWebView->ExecuteScript(
            wstrScript.c_str(),
            Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [&hEvent, &wstrResult](HRESULT hResult, LPCWSTR pwzResultObjectAsJson)
                ->HRESULT
                {
                    if (SUCCEEDED(hResult) && pwzResultObjectAsJson != nullptr)
                    {
                        wstrResult = DeserialiseJsonString(pwzResultObjectAsJson);
                    }
                    ::SetEvent(hEvent.get());
                    return S_OK;
                }
            ).Get()
                    );
        if (SUCCEEDED(hr))
        {
            DWORD dwIndex = 0;
            hr = ::CoWaitForMultipleHandles(
                COWAIT_DISPATCH_WINDOW_MESSAGES | COWAIT_DISPATCH_CALLS | COWAIT_INPUTAVAILABLE,
                INFINITE, 1, hEvent.addressof(), &dwIndex);
            if (SUCCEEDED(hr))
            {
                return wstrResult;
            }
        }
    }

    return std::wstring();
}
/*認証情報ファイル取り込み・版情報取得*/
void CMainWindow::SetupMinasigoAuthorisation()
{
    minasigo::MnsgSetup(this);
}
