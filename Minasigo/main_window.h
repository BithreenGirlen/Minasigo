#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>
#include <wrl.h>
#include <wil/com.h>

#include <string>

#include "WebView2.h"

class CMainWindow
{
public:
	CMainWindow();
	~CMainWindow();
	bool Create(HINSTANCE hInstance);
	int MessageLoop();
	HWND GetHwnd()const { return m_hWnd;}
private:
	std::wstring m_class_name = L"Minasigo Win32 WebView2";
	std::wstring m_window_name = L"Minasigo";
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnDestroy();
	LRESULT OnClose();
	LRESULT OnPaint();
	LRESULT OnSize();
	LRESULT OnCommand(WPARAM wParam);

	enum Menu
	{
		kOpenFile = 1,
		kDecryptManifest, kDecryptPayload, kDecryptResponse,
		kDownloadStoryMasterData, kDownloadScenarioPaths, kDownloadScenarios, kDownloadScenarioResourcePath, kDownloadScenarioResources,
	};
	enum MenuBar{kFile, kDecrypt, kDownload};

	HMENU m_hMenuBar = nullptr;

	void InitialiseMenuBar();

	void MenuOnOpenFile();
	void MenuOnDecryptManifest();
	void MenuOnDecryptPayload();
	void MenuOnDecryptResponse();
	void MenuOnDownloadStoryMasterData();
	void MenuOnDownloadScenarioPaths();
	void MenuOnDownloadScenarios();
	void MenuOnDownloadScenarioResourcePaths();
	void MenuOnDownloadScenarioResources();

	HRESULT m_hrComInit = E_FAIL;
	wil::com_ptr<ICoreWebView2> m_pWebView;
	wil::com_ptr<ICoreWebView2Controller> m_pWebViewController;
	wil::com_ptr<ICoreWebView2Environment> m_pWebView2Environment;

	void InitialiseWebViewEnvironment();
	HRESULT OnCreateEnvironmentCompleted(HRESULT hResult, ICoreWebView2Environment* pWebViewEnvironment);
	HRESULT OnCreateWebViewControllerCompleted(HRESULT hResult, ICoreWebView2Controller* pWebViewController);

	void ResizeWebViewBound();

public:
	std::wstring ExecuteDecryptManifestFunctionOnWebPage(const std::wstring& wstrText);
	std::wstring ExecuteEncryptDataFunctionOnWebPage(const std::wstring& wstrText, const std::wstring& wstrToken);
	std::wstring ExecuteDecryptDataFunctionOnWebPage(const std::wstring& wstrText, const std::wstring& wstrToken);
private:
	std::wstring ExecuteScriptOnWebPage(const std::wstring& wstrScript);

	void SetupMinasigoAuthorisation();

};

#endif //MAIN_WINDOW_H_