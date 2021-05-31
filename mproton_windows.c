// +build windows

#define WIN32_LEAN_AND_MEAN
#include <Shlwapi.h>
#include <codecvt>
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shlwapi.lib")

#include "WebView2.h"
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

int xmain()
{
  WNDCLASSEX wc;
  ZeroMemory(&wc, sizeof(WNDCLASSEX));
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.hInstance = hInstance;
  wc.lpszClassName = "webview";
  wc.hIcon = icon;
  wc.hIconSm = icon;
  wc.lpfnWndProc =
      (WNDPROC)(+[](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> int
  {
    auto w = (win32_edge_engine *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_SIZE:
      // w->m_browser->resize(hwnd);
      break;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      // w->terminate();
      break;
    case WM_GETMINMAXINFO:
    {
      auto lpmmi = (LPMINMAXINFO)lp;
      if (w == nullptr)
      {
        return 0;
      }
      if (w->m_maxsz.x > 0 && w->m_maxsz.y > 0)
      {
        lpmmi->ptMaxSize = w->m_maxsz;
        lpmmi->ptMaxTrackSize = w->m_maxsz;
      }
      if (w->m_minsz.x > 0 && w->m_minsz.y > 0)
      {
        lpmmi->ptMinTrackSize = w->m_minsz;
      }
    }
    break;
    default:
      return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
  });
  RegisterClassEx(&wc);
  m_window = CreateWindow("webview", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                          CW_USEDEFAULT, 640, 480, nullptr, nullptr,
                          GetModuleHandle(nullptr), nullptr);
  SetWindowLongPtr(m_window, GWLP_USERDATA, (LONG_PTR)this);

  HINSTANCE hInstance = GetModuleHandle(nullptr);
  HICON icon = (HICON)LoadImage(
      hInstance, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

  CoInitializeEx(nullptr, 0);


  char currentExePath[MAX_PATH];
  GetModuleFileNameA(NULL, currentExePath, MAX_PATH);
  char *currentExeName = PathFindFileNameA(currentExePath);

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wideCharConverter;
  std::wstring userDataFolder = wideCharConverter.from_bytes(std::getenv("APPDATA"));
  std::wstring currentExeNameW = wideCharConverter.from_bytes(currentExeName);

  // HRESULT res = CreateCoreWebView2EnvironmentWithOptions(
  //     nullptr, (userDataFolder + L"/" + currentExeNameW).c_str(), nullptr,
  //     new webview2_com_handler(wnd, cb,
  //                              [&](ICoreWebView2Controller *controller)
  //                              {
  //                                m_controller = controller;
  //                                m_controller->get_CoreWebView2(&m_webview);
  //                                m_webview->AddRef();
  //                                flag.clear();
  //                              }));

  // Step 3 - Create a single WebView within the parent window
// Locate the browser and set up the environment for WebView
CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
        
            // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
            env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                if (controller != nullptr) {
                    webviewController = controller;
                    webviewController->get_CoreWebView2(&webviewWindow);
                }
                
                // Add a few settings for the webview
                // The demo step is redundant since the values are the default settings
                ICoreWebView2Settings* Settings;
                webviewWindow->get_Settings(&Settings);
                Settings->put_IsScriptEnabled(TRUE);
                Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                Settings->put_IsWebMessageEnabled(TRUE);
                
                // Resize WebView to fit the bounds of the parent window
                RECT bounds;
                GetClientRect(hWnd, &bounds);
                webviewController->put_Bounds(bounds);
                
                // Schedule an async task to navigate to Bing
                webviewWindow->Navigate(L"https://www.bing.com/");
                
                // Step 4 - Navigation events
                // Step 5 - Scripting
                // Step 6 - Communication between host and web content
                return S_OK;
            }).Get());
        return S_OK;
    }).Get());

    // Schedule an async task to add initialization script that freezes the Object object
webviewWindow->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
// Schedule an async task to get the document URL
webviewWindow->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
    [](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
        LPCWSTR URL = resultObjectAsJson;
        //doSomethingWithURL(URL);
        return S_OK;
    }).Get());

  // CoUninitialize();

  MSG msg;
  BOOL res;
  while ((res = GetMessage(&msg, nullptr, 0, 0)) != -1)
  {
    if (msg.hwnd)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      continue;
    }
    else if (msg.message == WM_QUIT)
    {
      return;
    }
  }

  return 0;
}

#ifdef STANDALONEPROG
int main(int argc,
         char **argv)
{
  return xmain();
}
#endif

int initialize(void)
{
  return 0;
}

int set_title(const char *title)
{
  return 0;
}

int set_menu_extra_text(const char *text)
{
  return 0;
}

int add_menu_extra_item(const char *text)
{

  return 0;
}

int add_content_path(const char *_Nullable path)
{
  return 0;
}

int add_script_message_handler(const char *_Nullable name)
{

  return 0;
}
