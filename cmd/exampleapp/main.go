package main

import (
	"fmt"
	"log"
	"os"

	"github.com/okigan/mproton"
)

type User struct {
	Name string
	Age  int
}

func mycallback1(param1 string, param2 string, param3 User) (string, error) {
	log.Print("[golang] in mycallback1: ", param1, param2, param3)
	return "Done", nil
}

func main() {
	log.SetFlags(log.LstdFlags)

	// minimal self test to make sure the executable can bootstrap and
	// find all dependent libraries
	if len(os.Args) == 2 && os.Args[1] == "--self-test" {
		println("self test complete.")
		return
	}

	// figure out where to load the content files from.
	// this is not reliable as `go run` creates/runs files from /tmp location
	// path, err := filepath.Abs(filepath.Dir(os.Args[0]))
	path, err := os.Getwd()
	if err != nil {
		println(err)
	}

	m := mproton.New()
	m.SetTitle("mProton example ☄️")
	m.Bind("mycallback1", mycallback1)
	m.Bind(mproton.MenuItemCallbackName, func(param string) (string, error) {
		log.Print("in mycallback2: ", param)

		// notify UI by executing the script
		m.ExecuteScript("console.log(\"hello from golang\")")

		return "Done", nil
	})

	// add Menu Bar Extra(s), more context at:
	// https://developer.apple.com/design/human-interface-guidelines/macos/extensions/menu-bar-extras/
	m.SetMenuBarExtraText("☄️")
	m.AddMenuBarExtra("Extra menu item ☄️!", 42)

	// load the rest of the UI content
	m.SetContentPath(fmt.Sprintf("file://%s/protonappui/dist/index.html", path))

	m.Run()
}
