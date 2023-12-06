
#include <atlbase.h>
#include <shobjidl.h>

#include "file_dialogues.h"

wchar_t* SelectOpenFile(const wchar_t* pszFile, const wchar_t* pszSpec, HWND hParentWnd)
{
	CComPtr<IFileOpenDialog> pFileDialog;
	HRESULT hr = pFileDialog.CoCreateInstance(CLSID_FileOpenDialog);

	if (SUCCEEDED(hr)) {
		COMDLG_FILTERSPEC filter[1]{};
		filter[0].pszName = pszFile;
		filter[0].pszSpec = pszSpec;
		hr = pFileDialog->SetFileTypes(1, filter);
		if (SUCCEEDED(hr))
		{
			FILEOPENDIALOGOPTIONS opt{};
			pFileDialog->GetOptions(&opt);
			pFileDialog->SetOptions(opt | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);

			if (SUCCEEDED(pFileDialog->Show(hParentWnd)))
			{
				CComPtr<IShellItem> pSelectedItem;
				pFileDialog->GetResult(&pSelectedItem);

				wchar_t* pPath;
				pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);

				return pPath;
			}
		}
	}

	return nullptr;
}

/*展開HTMLファイル選択*/
wchar_t* SelectHtmlFileToOpen(HWND hParentWnd)
{
	return SelectOpenFile(L"HTML files", L"*.htm;*.html;", hParentWnd);
}
/*展開JSONファイル選択*/
wchar_t* SelectJsonFileToOpen(HWND hParentWnd)
{
	return SelectOpenFile(L"JSON files", L"*.json;", hParentWnd);
}
/*展開TXTファイル選択*/
wchar_t* SelectTextFileToOpen(HWND hParentWnd)
{
	return SelectOpenFile(L"Text files", L"*.txt;", hParentWnd);
}
/*フォルダ選択ダイアログ*/
wchar_t* SelectWorkFolder(HWND hParentWnd)
{
	CComPtr<IFileOpenDialog> pFolderDlg;
	HRESULT hr = pFolderDlg.CoCreateInstance(CLSID_FileOpenDialog);

	if (SUCCEEDED(hr)) {
		FILEOPENDIALOGOPTIONS opt{};
		pFolderDlg->GetOptions(&opt);
		pFolderDlg->SetOptions(opt | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);

		if (SUCCEEDED(pFolderDlg->Show(hParentWnd)))
		{
			CComPtr<IShellItem> pSelectedItem;
			pFolderDlg->GetResult(&pSelectedItem);

			wchar_t* pPath;
			pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);

			return pPath;
		}
	}

	return nullptr;
}

/*保存先選択ダイアログ*/
wchar_t* SelectFileToSave(const wchar_t* pszFile, const wchar_t* pszSpec, const wchar_t* pszDefaultFileName, HWND hParentWnd)
{
	CComPtr<IFileSaveDialog> pFileDialog;
	HRESULT hr = pFileDialog.CoCreateInstance(CLSID_FileSaveDialog);

	if (SUCCEEDED(hr))
	{
		COMDLG_FILTERSPEC filter[1]{};
		filter[0].pszName = pszFile;
		filter[0].pszSpec = pszSpec;
		hr = pFileDialog->SetFileTypes(1, filter);
		if (SUCCEEDED(hr))
		{
			pFileDialog->SetFileName(pszDefaultFileName);

			FILEOPENDIALOGOPTIONS opt{};
			pFileDialog->GetOptions(&opt);
			pFileDialog->SetOptions(opt | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);

			if (SUCCEEDED(pFileDialog->Show(nullptr)))
			{
				CComPtr<IShellItem> pSelectedItem;
				pFileDialog->GetResult(&pSelectedItem);

				wchar_t* pPath;
				pSelectedItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);

				return pPath;
			}
		}
	}

	return nullptr;
}
/*保存JSONファイル選択*/
wchar_t* SelectJsonFileToSave(const wchar_t* pwzFileName, HWND hParentWnd)
{
	return SelectFileToSave(L"JSON files", L"*.json;", pwzFileName, hParentWnd);
}
/*保存TXTファイル選択*/
wchar_t* SelectTextFileToSave(const wchar_t* pwzFileName, HWND hParentWnd)
{
	return SelectFileToSave(L"Text files", L"*.text;", pwzFileName, hParentWnd);
}
