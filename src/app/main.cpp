#include <windows.h>

#include "App.h"

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    App app;
    return app.Run(hInstance, nCmdShow);
}

