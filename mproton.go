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


#cgo windows CXXFLAGS: -std=c++11 -DUNICODE -D_UNICODE
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
	"encoding/json"
	"fmt"
	"log"
	"reflect"
	"runtime"
	"sync"
	"unsafe"

	"github.com/tidwall/gjson"
)

const MenuItemCallbackName = "MenuItemCallback"

var callbackMapMutex sync.RWMutex
var callbackMap = make(map[string]interface{})

func registerCallback(name string, callback interface{}) int {
	callbackMapMutex.Lock()
	callbackMap[name] = callback
	callbackMapMutex.Unlock()
	return 0
}

//export _prtn_call_into_go
func _prtn_call_into_go(param1 *C.char, param2 *C.char) (*C.char, *C.char) {
	log.Print("[golang] in _prtn_call_into_go")

	json1 := C.GoString(param2)
	param := gjson.Get(json1, "param").Array()
	name := gjson.Get(json1, "name").Str

	callbackMapMutex.RLock()
	callback, ok := callbackMap[name]
	callbackMapMutex.RUnlock()

	if !ok {
		msg := fmt.Sprintf("No callback registered for: %s", name)
		log.Print(msg)
		return nil, C.CString(msg)
	}

	valueOfCallback := reflect.ValueOf(callback)

	args := []reflect.Value{}
	for i := 0; i < valueOfCallback.Type().NumIn(); i++ {
		arg := reflect.New(valueOfCallback.Type().In(i))
		json.Unmarshal([]byte(param[i].Raw), arg.Interface())
		args = append(args, arg.Elem())
	}

	res := valueOfCallback.Call(args)

	print(res)
	// r1, err := res[0].Interface().(string), res[1].Interface().(error)
	// if err != nil {
	// 	return C.CString(r1), C.CString(err.Error())
	// } else {
	// 	return C.CString(r1), nil
	// }

	return C.CString(res[0].Interface().(string)), nil
}

type mProtonApp interface {
	Run()
	// Destroy()
	SetTitle(path string)
	SetContent(path string)
	SetContentPath(path string)
	Bind(name string, callback interface{})
	SetMenuBarExtraText(name string)
	AddMenuBarExtra(name string, tag int)
	ExecuteScript(script string)
	//	SetTitle(title string)
}

type mprotonHandle struct {
}

func New() mProtonApp {
	runtime.LockOSThread()

	h := &mprotonHandle{}
	C.prtn_initialize()
	return h
}

func (handle *mprotonHandle) Run() {
	C.prtn_event_loop()
}

func (handle *mprotonHandle) SetTitle(title string) {
	c_title := C.CString(title)
	defer C.free(unsafe.Pointer(c_title))

	C.prtn_set_title(c_title)
}

func (handle *mprotonHandle) SetMenuBarExtraText(text string) {
	c_text := C.CString(text)
	defer C.free(unsafe.Pointer(c_text))

	C.prtn_set_menu_extra_text(c_text)
}

func (handle *mprotonHandle) AddMenuBarExtra(text string, tag int) {
	c_text := C.CString(text)
	defer C.free(unsafe.Pointer(c_text))

	C.prtn_add_menu_extra_item(c_text, C.int(tag))
}

func (handle *mprotonHandle) SetContent(content string) {
	c_content := C.CString(content)
	defer C.free(unsafe.Pointer(c_content))

	C.prtn_set_content(c_content)
}

func (handle *mprotonHandle) SetContentPath(path string) {
	c_path := C.CString(path)
	defer C.free(unsafe.Pointer(c_path))

	C.prtn_set_content_path(c_path)
}

func (handle *mprotonHandle) Bind(name string, callback interface{}) {
	c_name := C.CString(name)
	defer C.free(unsafe.Pointer(c_name))

	registerCallback(name, callback)
	C.prtn_add_script_message_handler(c_name)
}

func (hanle *mprotonHandle) ExecuteScript(script string) {
	c_script := C.CString(script)
	defer C.free(unsafe.Pointer(c_script))

	C.prtn_execute_script(c_script)
}
