LRESULT APIENTRY MainWndProc(HWND,UINT,UINT,LONG);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);

int RunDll(LPSTR szDllname,LPIMAGEPROCSTR pInfo);