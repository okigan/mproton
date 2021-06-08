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

//TODO: review https://github.com/aquasecurity/tracee/pull/649#issuecomment-812615069
type ReplyFunc func(*string, error)

// type CallbackFunc func(reply ReplyFunc, params ...interface{})
type CallbackFunc interface{}

var callbackMapMutex sync.Mutex
var callbackWithReplyMap = make(map[string]CallbackFunc)

func registerCallbackWithReply(name string, callback CallbackFunc) int {
	callbackMapMutex.Lock()
	callbackWithReplyMap[name] = callback
	callbackMapMutex.Unlock()
	return 0
}

func jump_back_to_host(promise_id string, result *string, err error) {
	c_promise_id := C.CString(promise_id)
	defer C.free(unsafe.Pointer(c_promise_id))

	if err != nil {
		c_error_string := C.CString(err.Error())
		defer C.free(unsafe.Pointer(c_error_string))
		C.prtn_resolve(c_promise_id, nil, c_error_string)
	} else {
		c_result := C.CString(*result)
		defer C.free(unsafe.Pointer(c_result))
		C.prtn_resolve(C.CString(promise_id), C.CString(*result), nil)
	}
}

//export _prtn_call_into_go_with_reply
func _prtn_call_into_go_with_reply(obj *C.char) (*C.char, *C.char) {
	log.Print("[golang] in _prtn_call_into_go_with_reply")

	json1 := C.GoString(obj)
	param := gjson.Get(json1, "param").Array()
	name := gjson.Get(json1, "name").Str
	promise_id := gjson.Get(json1, "promiseId").Raw

	callbackMapMutex.Lock()
	callback, ok := callbackWithReplyMap[name]
	callbackMapMutex.Unlock()

	if !ok {
		msg := fmt.Sprintf("No callback registered for: %s", name)
		log.Print(msg)
	}

	// this is what following code will try to do through reflection
	// callback(func(result *string, error_string *string) {
	// 	jump_back_to_host(promise_id, result, error_string)
	// }, param)

	valueOfCallback := reflect.ValueOf(callback)

	if len(param)+1 != valueOfCallback.Type().NumIn() {
		return nil, C.CString("Improper number of parameters passed")
	}

	args := []reflect.Value{}
	args = append(args, reflect.ValueOf(func(result *string, error_string error) {
		jump_back_to_host(promise_id, result, error_string)
	}))

	offset := len(args)
	for i := offset; i < valueOfCallback.Type().NumIn(); i++ {
		arg := reflect.New(valueOfCallback.Type().In(i))
		err := json.Unmarshal([]byte(param[i-offset].Raw), arg.Interface())
		if err != nil {
			return nil, C.CString("Could not unmarshal input parameters")
		}
		args = append(args, arg.Elem())
	}

	_ = valueOfCallback.Call(args)

	// return C.CString(res[0].Interface().(string)), nil
	return C.CString("not used1"), C.CString("not used2")
}

type ProtonApp interface {
	Run()
	// Destroy()
	SetTitle(path string)
	SetContent(path string)
	SetContentPath(path string)
	BindWithReply(name string, callback CallbackFunc)
	SetMenuBarExtraText(name string)
	AddMenuBarExtra(name string, tag int)
	ExecuteScript(script string)
	//	SetTitle(title string)
}

type mprotonHandle struct {
}

func New() ProtonApp {
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

func (handle *mprotonHandle) BindWithReply(name string, callbackwithreply CallbackFunc) {
	c_name := C.CString(name)
	defer C.free(unsafe.Pointer(c_name))

	registerCallbackWithReply(name, callbackwithreply)
	C.prtn_add_script_message_handler(c_name)
}

func (hanle *mprotonHandle) ExecuteScript(script string) {
	c_script := C.CString(script)
	defer C.free(unsafe.Pointer(c_script))

	C.prtn_execute_script(c_script)
}
