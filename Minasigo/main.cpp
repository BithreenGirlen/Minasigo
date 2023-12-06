

#include "framework.h"
#include "main_window.h"

int main()
{
    HINSTANCE hInstance = ::GetModuleHandle(NULL);
    int iRet = 0;
    CMainWindow* pMainWindow = new CMainWindow();
    if (pMainWindow != nullptr)
    {
        bool bRet = pMainWindow->Create(hInstance);
        if (bRet)
        {
            ::ShowWindow(pMainWindow->GetHwnd(), SW_SHOWNORMAL);

            iRet = pMainWindow->MessageLoop();
        }

        delete pMainWindow;
    }

    return iRet;
}
