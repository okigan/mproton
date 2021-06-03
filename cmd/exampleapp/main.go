package main

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/okigan/mproton"
)

func mycallback1(param string) (string, error) {
	println("go lang in", "mycallback1", param)
	return "Done", nil
}

func main() {
	if len(os.Args) == 2 && os.Args[1] == "--self-test" {
		print("self test complete.\n")
		return
	}

	path, err := filepath.Abs(filepath.Dir(os.Args[0]))
	// path, err := os.Getwd()
	if err != nil {
		println(err)
	}

	m := mproton.New()
	m.SetTitle("mProton example 🌯")
	m.Bind("mycallback1", mycallback1)
	m.SetMenuExtraText("🌯🌯")
	m.AddMenuExtra("Extra menu item 🌯!")
	m.SetContentPath(fmt.Sprintf("file://%s/protonappui/dist/index.html", path))

	m.Run()
}
