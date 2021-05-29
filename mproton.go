package mproton

/*
#cgo darwin CFLAGS: -x objective-c
#cgo darwin CFLAGS: -Wobjc-property-no-attribute
#cgo darwin LDFLAGS: -framework Cocoa
#cgo darwin LDFLAGS: -framework Foundation
#cgo darwin LDFLAGS: -framework WebKit

#include "mproton.h"

#include <stdio.h>
#include <stdlib.h>
*/
import "C"
import "fmt"

var callbackMap = make(map[string]func(v string) (string, error))

func registerCallback(name string, callback func(v string) (string, error)) int {
	callbackMap[name] = callback
	return 0
}

//export goTrampoline
func goTrampoline(a int, param1 *C.char, param2 *C.char) (*C.char, *C.char) {
	p1 := C.GoString(param1)
	p2 := C.GoString(param2)

	println("[golang]", a, p1, p2)
	callback := callbackMap[p1]

	if callback == nil {
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

type CgoExpApp interface {
	Run()
	// Destroy()
	SetTitle(path string)
	SetContentPath(path string)
	Bind(name string, callback func(string) (string, error))
	SetMenuExtraText(name string)
	AddMenuExtra(name string)
	//	SetTitle(title string)
}

type cgoexpHandle struct {
	ContentPath string
	Title       string
}

func New() CgoExpApp {
	h := &cgoexpHandle{}
	C.initialize()
	return h
}

func (handle *cgoexpHandle) Run() {
	C.xmain()
}

func (handle *cgoexpHandle) SetTitle(path string) {
	C.set_title(C.CString(path)) // TODO: cleanup mem
}

func (handle *cgoexpHandle) SetMenuExtraText(text string) {
	C.set_menu_extra_text(C.CString(text))
}

func (handle *cgoexpHandle) AddMenuExtra(text string) {
	C.add_menu_extra_item(C.CString(text))
}

func (handle *cgoexpHandle) SetContentPath(path string) {
	C.add_content_path(C.CString(path))
}

func (handle *cgoexpHandle) Bind(name string, callback func(string) (string, error)) {
	registerCallback(name, callback)
}
