package main

import (
	"embed"
	"log"
	"os"
	"time"

	"github.com/okigan/mproton"
)

//go:embed protonappui/dist/index.html
var content embed.FS

type User struct {
	Name string
	Age  int
}

func main() {
	log.SetFlags(log.LstdFlags | log.Lmicroseconds)

	// minimal self test to make sure the executable can bootstrap and
	// find all dependent libraries
	if len(os.Args) == 2 && os.Args[1] == "--self-test" {
		println("self test complete.")
		return
	}

	m := mproton.New()
	m.SetTitle("mProton example ☄️")

	m.BindWithReply("mycallback1", func(reply mproton.ReplyFunc, param1 string, param2 string, param3 User) {
		log.Print("in lambda callback: ", param1, param2, param3)

		go func() {
			log.Print("calling script from go coroutine: ", param1)
			time.Sleep(3 * time.Second) // emulate long processing time
			result := "42"
			reply(&result, nil)
			// or
			// reply(nil, fmt.Errorf("report an error"))
		}()
	})

	m.BindWithReply(mproton.MenuItemCallbackName, func(reply mproton.ReplyFunc, param int) (string, error) {
		log.Print("in lambda callback: ", param)

		go func() {
			log.Print("calling script from go coroutine: ", param)
			time.Sleep(3 * time.Second) // emulate long processing time
			m.ExecuteScript("console.log('menu item processing complete')")
		}()
		return "Done", nil
	})

	// add Menu Bar Extra(s), more context at:
	// https://developer.apple.com/design/human-interface-guidelines/macos/extensions/menu-bar-extras/
	m.SetMenuBarExtraText("☄️")
	m.AddMenuBarExtra("Extra menu item ☄️!", 42)

	data, ok := content.ReadFile("protonappui/dist/index.html")
	if ok != nil {
		log.Fatal("could not load the contents")
	}

	// load the rest of the UI content
	m.SetContent(string(data))
	// or
	// // figure out where to load the content files from.
	// // this is not reliable as `go run` creates/runs files from /tmp location
	// // path, err := filepath.Abs(filepath.Dir(os.Args[0]))
	// path, err := os.Getwd()
	// if err != nil {
	// 	println(err)
	// }
	// m.SetContentPath(fmt.Sprintf("file://%s/protonappui/dist/index.html", path))

	m.Run()
}
