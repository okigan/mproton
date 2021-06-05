package mproton

/*
#cgo darwin CFLAGS: -x objective-c
#cgo darwin CFLAGS: -Wobjc-property-no-attribute
#cgo darwin LDFLAGS: -framework Cocoa
#cgo darwin LDFLAGS: -framework Foundation
#cgo darwin LDFLAGS: -framework WebKit

//sudo apt-get -y install libgtk-3-dev libwebkit2gtk-4.0-dev libappindicator3-dev

#cgo linux openbsd freebsd CXXFLAGS: -DWEBVIEW_GTK -std=c++11
#cgo linux openbsd freebsd pkg-config: gtk+-3.0 webkit2gtk-4.0 appindicator3-0.1


#cgo windows CXXFLAGS: -std=c++11
#cgo windows CXXFLAGS: -I./Microsoft.Web.WebView2.Sdk/build/native/include
#cgo windows CXXFLAGS: -I./Windows/include
#cgo windows LDFLAGS: -L./Microsoft.Web.WebView2.Sdk/build/native/x64/
#cgo windows LDFLAGS: -lole32 -lWebView2Loader

// sudo apt-get install -y electric-fence
// #cgo linux LDFLAGS: -lefence

#include "mproton.h"


#include <stdio.h>
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"sync"
	"unsafe"
)

var callbackMapMutex sync.RWMutex
var callbackMap = make(map[string]func(v string) (string, error))

func registerCallback(name string, callback func(v string) (string, error)) int {
	callbackMapMutex.Lock()
	callbackMap[name] = callback
	callbackMapMutex.Unlock()
	return 0
}

//export prtn_goTrampoline
func prtn_goTrampoline(param1 *C.char, param2 *C.char) (*C.char, *C.char) {
	p1 := C.GoString(param1)
	p2 := C.GoString(param2)

	println("[golang]", p1, p2)
	callbackMapMutex.RLock()
	callback, ok := callbackMap[p1]
	callbackMapMutex.RUnlock()

	if !ok {
		msg := fmt.Sprintf("No callback registered for: %s", p1)
		return nil, C.CString(msg)
	}

	r1, err := callback(p2)

	if err != nil {
		return C.CString(r1), C.CString(err.Error())
	} else {
		return C.CString(r1), nil
	}

}

type mProtonApp interface {
	Run()
	// Destroy()
	SetTitle(path string)
	SetContentPath(path string)
	Bind(name string, callback func(string) (string, error))
	SetMenuExtraText(name string)
	AddMenuExtra(name string)
	//	SetTitle(title string)
}

type mprotonHandle struct {
}

func New() mProtonApp {
	h := &mprotonHandle{}
	C.prtn_initialize()
	return h
}

func (handle *mprotonHandle) Run() {
	C.xmain()
}

func (handle *mprotonHandle) SetTitle(title string) {
	c_title := C.CString(title)
	defer C.free(unsafe.Pointer(c_title))

	C.prtn_set_title(c_title)
}

func (handle *mprotonHandle) SetMenuExtraText(text string) {
	c_text := C.CString(text)
	defer C.free(unsafe.Pointer(c_text))

	C.prtn_set_menu_extra_text(c_text)
}

func (handle *mprotonHandle) AddMenuExtra(text string) {
	c_text := C.CString(text)
	defer C.free(unsafe.Pointer(c_text))

	C.prtn_add_menu_extra_item(c_text)
}

func (handle *mprotonHandle) SetContentPath(path string) {
	c_path := C.CString(path)
	defer C.free(unsafe.Pointer(c_path))

	C.prtn_add_content_path(c_path)
}

func (handle *mprotonHandle) Bind(name string, callback func(string) (string, error)) {
	c_name := C.CString(name)
	defer C.free(unsafe.Pointer(c_name))

	registerCallback(name, callback)
	C.prtn_add_script_message_handler(c_name)
}
