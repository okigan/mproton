package main

import (
	"embed"
	"log"
	"os"

	"github.com/okigan/mproton"
)

//go:embed protonappui/dist/index.html
var content embed.FS

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

	// // figure out where to load the content files from.
	// // this is not reliable as `go run` creates/runs files from /tmp location
	// // path, err := filepath.Abs(filepath.Dir(os.Args[0]))
	// path, err := os.Getwd()
	// if err != nil {
	// 	println(err)
	// }

	m := mproton.New()
	m.SetTitle("mProton example ☄️")
	m.Bind("mycallback1", mycallback1)

	m.Bind(mproton.MenuItemCallbackName, func(param int) (string, error) {
		log.Print("in lambda callback: ", param)

		go func() {
			log.Print("calling script from go coroutine: ", param)

			m.ExecuteScript("console.log(\"hello from go coroutine\")")
		}()
		return "Done", nil
	})

	// add Menu Bar Extra(s), more context at:
	// https://developer.apple.com/design/human-interface-guidelines/macos/extensions/menu-bar-extras/
	m.SetMenuBarExtraText("☄️")
	m.AddMenuBarExtra("Extra menu item ☄️!", 42)

	data, ok := content.ReadFile("protonappui/dist/index.html")
	if ok != nil {
		println("could not load the contents")
	}

	// load the rest of the UI content
	m.SetContent(string(data))
	// m.SetContentPath(fmt.Sprintf("file://%s/protonappui/dist/index.html", path))

	m.Run()
}
