// +build windows

#include <atomic>
#include <functional>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Hstring.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <tchar.h>

#include "WebView2.h"

void FatalError(LPCTSTR format, ...) {
  MessageBox(nullptr, format, TEXT("webview2-mingw"), MB_OK | MB_ICONSTOP);
  ExitProcess(1);
}

class Handler
    : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
      public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
      public ICoreWebView2WebMessageReceivedEventHandler,
      public ICoreWebView2PermissionRequestedEventHandler {
public:
  Handler() {}
  virtual ~Handler() {}

  std::function<HRESULT(HRESULT result,
                        ICoreWebView2Environment *created_environment)>
      EnvironmentCompleted;
  std::function<HRESULT(HRESULT result, ICoreWebView2Controller *controller)>
      ControllerCompleted;

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) {
    FatalError(TEXT("Call to QueryInterface failed!"));

    return E_FAIL;
  }
  ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }
  ULONG STDMETHODCALLTYPE Release(void) { return 1; }

  HRESULT STDMETHODCALLTYPE
  Invoke(HRESULT errorCode, ICoreWebView2Environment *createdEnvironment) {
    return EnvironmentCompleted(errorCode, createdEnvironment);
  }

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode,
                                   ICoreWebView2Controller *createdController) {
    return ControllerCompleted(errorCode, createdController);
  }

  HRESULT STDMETHODCALLTYPE Invoke(
      /* [in] */ ICoreWebView2 *sender,
      /* [in] */ ICoreWebView2WebMessageReceivedEventArgs *args) {
    FatalError(
        TEXT("Call to ICoreWebView2WebMessageReceivedEventHandler:Invoke"));
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Invoke(
      /* [in] */ ICoreWebView2 *sender,
      /* [in] */ ICoreWebView2PermissionRequestedEventArgs *args) {
    FatalError(
        TEXT("Call to ICoreWebView2PermissionRequestedEventHandler:Invoke"));
    return S_OK;
  }
};

HWND hMainWnd = NULL;
Handler handler;

ICoreWebView2Controller *controller;
ICoreWebView2 *webview2;
std::atomic<bool> flag(false);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HRESULT result = S_OK;
  switch (msg) {
  case WM_CREATE: {
  } break;

  case WM_SIZE:
    if (controller != NULL) {
      RECT bounds;
      GetClientRect(hWnd, &bounds);
      controller->put_Bounds(bounds);
    }
    break;
  case WM_CLOSE:
    DestroyWindow(hWnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

extern "C" {
int xmain(void) {
  std::cout << "Hello World!" << std::endl;

  ShowWindow(hMainWnd, SW_SHOW);
  UpdateWindow(hMainWnd);
  SetFocus(hMainWnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
}

#ifdef STANDALONEPROG
int main(int argc, char **argv) { return xmain(); }
#endif

extern "C" {
int initialize(void) {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  HANDLE himage =
      LoadImage(hInstance, IDI_APPLICATION, IMAGE_ICON,
                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
  HICON hicon = reinterpret_cast<HICON>(himage);

  WNDCLASSEX wc{};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.hInstance = hInstance;
  wc.lpszClassName = TEXT("webview");
  wc.hIcon = hicon;
  wc.hIconSm = hicon;
  wc.lpfnWndProc = WndProc;

  RegisterClassEx(&wc);

  handler.EnvironmentCompleted =
      [&](HRESULT result, ICoreWebView2Environment *created_environment) {
        std::cout << "EnvironmentCompleted" << std::endl;
        if (FAILED(result)) {
          FatalError(TEXT("Failed to create environment?"));
        }

        std::cout << "hMainWnd" << hMainWnd << std::endl;
        created_environment->CreateCoreWebView2Controller(hMainWnd, &handler);
        return S_OK;
      };

  handler.ControllerCompleted =
      [&](HRESULT result, ICoreWebView2Controller *new_controller) mutable {
        std::cout << "ControllerCompletede" << std::endl;

        if (FAILED(result)) {
          FatalError(TEXT("Failed to create controller?"));
        }
        controller = new_controller;
        controller->AddRef();
        controller->get_CoreWebView2(&webview2);
        controller->put_ParentWindow(hMainWnd);
        RECT bounds;
        GetClientRect(hMainWnd, &bounds);
        controller->put_Bounds(bounds);

        ICoreWebView2Settings *settings;
        webview2->get_Settings(&settings);

        EventRegistrationToken token;
        webview2->add_WebMessageReceived(&handler, &token);
        webview2->add_PermissionRequested(&handler, &token);

        settings->put_IsScriptEnabled(TRUE);
        settings->put_AreDefaultScriptDialogsEnabled(TRUE);
        settings->put_IsWebMessageEnabled(TRUE);
        
        webview2->AddScriptToExecuteOnDocumentCreated(L"window.proton={invoke:s=>window.chrome.webview.postMessage(s)}", nullptr);

        webview2->AddRef();
        flag = true;
        std::cout << "switched flag" << std::endl;
        // return S_OK;
        return webview2->Navigate(L"https://apple.com");
      };

  hMainWnd = CreateWindowEx(0, TEXT("webview"), TEXT("MinGW WebView2 Demo"),
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                            CW_USEDEFAULT, 1024, 768, nullptr, nullptr,
                            hInstance, nullptr);
  std::cout << "hMainWnd: " << hMainWnd << std::endl;

  HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  result = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
                                                    &handler);

  if (FAILED(result)) {
    FatalError(
        TEXT("Call to CreateCoreWebView2EnvironmentWithOptions failed!"));
  }
  // pump the memssage pump till initialization is complete
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    std::cout << "got message" << std::endl;
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (flag) {
      std::cout << "done with pump" << std::endl;
      break;
    }
  }

  return 0;
}

int set_title(const char *title) { return SetWindowTextA(hMainWnd, title); }

int set_menu_extra_text(const char *text) { return 0; }

int add_menu_extra_item(const char *text) { return 0; }
LPWSTR to_lpwstr(const std::string s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
  wchar_t *ws = new wchar_t[n];
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws, n);
  return ws;
}
int add_content_path(const char *path) {
  std::cout << "about to navigate" << std::endl;

  if (webview2 != NULL) {
    webview2->Navigate(to_lpwstr(path));

    std::cout << "done navigate" << std::endl;
  }
  return 0;
}

int add_script_message_handler(const char *name) { return 0; }
}