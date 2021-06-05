// +build windows

#include <atomic>
#include <functional>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>

#include "WebView2.h"

#include "mproton.h"
#include "mproton_cgo_exports.h"


class CMultiByteToWideCharConverter {
 public:
  CMultiByteToWideCharConverter(LPCSTR psz) {
    m_psz = NULL;
    if (psz != NULL) {
      UINT nCodePage = CP_UTF8;
      size_t lenMB = strlen(psz) + 1;

      // TODO: optimize/cache this allocation
      int lenWC = MultiByteToWideChar(nCodePage, 0, psz, lenMB, NULL, 0);
      m_psz = new WCHAR[lenWC];
      int result = MultiByteToWideChar(nCodePage, 0, psz, lenMB, m_psz, lenWC);
      if (result == 0) {
        DWORD dwError = ::GetLastError();
        throw dwError;
      }
    }
  }
  ~CMultiByteToWideCharConverter() throw() { delete[] m_psz; }

  operator LPWSTR() const throw() { return m_psz; }

 private:
  LPWSTR m_psz;

 private:
  CMultiByteToWideCharConverter(const CMultiByteToWideCharConverter &) throw();
  CMultiByteToWideCharConverter &operator=(const CMultiByteToWideCharConverter &) throw();
};
typedef CMultiByteToWideCharConverter CA2W;

class CWideCharToMultiByteConverter {
 public:
  CWideCharToMultiByteConverter(LPCWSTR psz) {
    m_psz = NULL;
    if (psz != NULL) {
      UINT nCodePage = CP_UTF8;
      size_t lenWC = wcslen(psz) + 1;

      // TODO: optimize/cache this allocation
      int lenMB = WideCharToMultiByte(nCodePage, 0, psz, lenWC, NULL, 0, NULL, NULL);
      m_psz = new CHAR[lenMB];
      int result = WideCharToMultiByte(nCodePage, 0, psz, lenWC, m_psz, lenMB, NULL, NULL);
      if (result == 0) {
        DWORD dwError = ::GetLastError();
        throw dwError;
      }
    }
  }
  ~CWideCharToMultiByteConverter() throw() { delete[] m_psz; }

  operator LPSTR() const throw() { return m_psz; }

 private:
  LPSTR m_psz;

 private:
  CWideCharToMultiByteConverter(const CWideCharToMultiByteConverter &) throw();
  CWideCharToMultiByteConverter &operator=(const CWideCharToMultiByteConverter &) throw();
};
typedef CWideCharToMultiByteConverter CW2A;

#ifdef UNICODE
#define CA2T CA2W
#define CW2T CW2A
#endif

void FatalError(LPCTSTR format, ...) {
  MessageBox(nullptr, format, TEXT("webview2-mingw"), MB_OK | MB_ICONSTOP);
  ExitProcess(1);
}

HWND hMainWnd = NULL;
ICoreWebView2Controller *controller;
ICoreWebView2 *webview2;

std::atomic<bool> flag(false);

class Handler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
                public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
                public ICoreWebView2WebMessageReceivedEventHandler,
                public ICoreWebView2PermissionRequestedEventHandler,
                public ICoreWebView2ExecuteScriptCompletedHandler,
                public ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler {
 public:
  Handler() {}
  virtual ~Handler() {}

  HRESULT QueryInterface(REFIID riid, void **ppvObject) {
    FatalError(TEXT("Call to QueryInterface failed!"));

    return E_FAIL;
  }
  ULONG AddRef(void) { return 1; }
  ULONG Release(void) { return 1; }

  HRESULT Invoke(HRESULT errorCode, ICoreWebView2Environment *createdEnvironment) {
    std::cout << "EnvironmentCompleted" << std::endl;
    if (FAILED(errorCode)) {
      FatalError(TEXT("Failed to create environment?"));
    }

    std::cout << "hMainWnd" << hMainWnd << std::endl;
    createdEnvironment->CreateCoreWebView2Controller(hMainWnd, this);
    return S_OK;
  }

  HRESULT Invoke(HRESULT errorCode, ICoreWebView2Controller *createdController) {
    std::cout << "ControllerCompletede" << std::endl;

    if (FAILED(errorCode)) {
      FatalError(TEXT("Failed to create controller?"));
    }
    controller = createdController;
    controller->AddRef();

    controller->put_ParentWindow(hMainWnd);
    controller->get_CoreWebView2(&webview2);
    webview2->AddRef();

    RECT bounds;
    GetClientRect(hMainWnd, &bounds);
    controller->put_Bounds(bounds);

    EventRegistrationToken token;
    webview2->add_WebMessageReceived(this, &token);
    webview2->add_PermissionRequested(this, &token);

    ICoreWebView2Settings *settings;
    webview2->get_Settings(&settings);
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);

    // webview2->AddScriptToExecuteOnDocumentCreated(L"window.proton={invoke:s=>window.chrome.webview.postMessage(s)}",
    // nullptr);
    HRESULT result = S_OK;
    result = webview2->AddScriptToExecuteOnDocumentCreated(L"window.proton = { \"a\": \"b\"}", nullptr);
    result = webview2->AddScriptToExecuteOnDocumentCreated(
        L"window.chrome.webview.addEventListener('message', (event) => "
        L"console.log(event))",
        nullptr);

    flag = true;
    std::cout << "switched flag" << std::endl;
    return S_OK;
    // return webview2->Navigate(L"https://www.google.com");
  }

  HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) {
    LPWSTR source = NULL;
    args->get_Source(&source);
    LPWSTR message = NULL;
    args->get_WebMessageAsJson(&message);

    struct _prtn_call_into_go_return result = _prtn_call_into_go((char *)CW2A(source), (char *)CW2A(message));

    std::cout << "posting result: " << result.r1 << std::endl;
    sender->PostWebMessageAsString(CA2T(result.r1));
    return S_OK;
  }
  HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2PermissionRequestedEventArgs *args) {
    FatalError(TEXT("Call to ICoreWebView2PermissionRequestedEventHandler:Invoke"));
    return S_OK;
  }

  HRESULT Invoke(HRESULT errorCode, LPCWSTR resultObjectAsJson) {
    std::cout << "in ICoreWebView2ExecuteScriptCompletedHandler Invoke:" << errorCode << CW2A(resultObjectAsJson)
              << std::endl;
    return S_OK;
  }
};

Handler handler;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HRESULT result = S_OK;
  switch (msg) {
    case WM_CREATE:
      break;

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
int prtn_event_loop(void) {
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

int prtn_initialize(void) {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  HANDLE himage =
      LoadImage(hInstance, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
  HICON hicon = reinterpret_cast<HICON>(himage);

  WNDCLASSEX wc{};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.hInstance = hInstance;
  wc.lpszClassName = TEXT("webview");
  wc.hIcon = hicon;
  wc.hIconSm = hicon;
  wc.lpfnWndProc = WndProc;

  RegisterClassEx(&wc);

  hMainWnd = CreateWindowEx(0, TEXT("webview"), TEXT("MinGW WebView2 Demo"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, hInstance, nullptr);
  std::cout << "hMainWnd: " << hMainWnd << std::endl;

  HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  result = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, &handler);

  if (FAILED(result)) {
    FatalError(TEXT("Call to CreateCoreWebView2EnvironmentWithOptions failed!"));
  }
  // pump the memssage pump till initialization is complete
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    std::cout << "got message:" << msg.message << std::endl;
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (flag) {
      std::cout << "done with pump" << std::endl;
      break;
    }
  }

  return 0;
}

int prtn_set_title(const char *title) { return SetWindowText(hMainWnd, CA2T(title)); }

int prtn_set_menu_extra_text(const char *text) { return 0; }

int prtn_add_menu_extra_item(const char *text, int tag) { return 0; }

int prtn_add_content_path(const char *path) {
  std::cout << "about to navigate" << std::endl;

  if (webview2 != NULL) {
    webview2->Navigate(CA2T(path));

    std::cout << "done navigate" << std::endl;
  }
  return 0;
}

int prtn_add_script_message_handler(const char *name) {
  std::cout << "[c++] in prtn_add_script_message_handler" << std::endl;

  CHAR buf[4024] = "";
  snprintf(buf, sizeof(buf) / sizeof(buf[0]),
           "window.proton[\"%s\"] = { "
           "invoke:s=>window.chrome.webview.postMessage({name:\"%s\", param:s}) }",
           name, name);
  std::cout << buf << std::endl;

  // HRESULT result =
  // webview2->AddScriptToExecuteOnDocumentCreated(L"window.proton[\"a\"] =
  // \"prtn_add_script_message_handler1\"", &handler);
  //   webview2->ExecuteScript(L"window.proton[\"a\"] =
  //   \"prtn_add_script_message_handler2\"", &handler);
  webview2->AddScriptToExecuteOnDocumentCreated(CA2T(buf), &handler);
  webview2->ExecuteScript(CA2T(buf), &handler);

  // std::cout << "[c++] in prtn_add_script_message_handler with result" <<
  // result << std::endl;

  return 0;
}

int prtn_execute_script(const char * script) {
  return 0;
}

}

#ifdef STANDALONEPROG
int main(int argc, char** argv) { return prtn_event_loop(); }
#endif