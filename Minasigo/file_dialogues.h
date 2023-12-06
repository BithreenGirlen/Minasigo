#ifndef FILE_DIALOGUES_H_
#define FILE_DIALOGUES_H_

wchar_t* SelectHtmlFileToOpen(HWND hParentWnd);
wchar_t* SelectJsonFileToOpen(HWND hParentWnd);
wchar_t* SelectTextFileToOpen(HWND hParentWnd);
wchar_t* SelectWorkFolder(HWND hParentWnd);

wchar_t* SelectJsonFileToSave(const wchar_t* pwzFileName, HWND hParentWnd);
wchar_t* SelectTextFileToSave(const wchar_t* pwzFileName, HWND hParentWnd);

#endif //FILE_DIALOGUES_H_