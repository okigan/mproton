package main

import (
	"fmt"
	"os"

	"github.com/okigan/mproton"
)

func mycallback1(param string) (string, error) {
	println("go lang in", "mycallback1", param)
	return "Done", nil
}

func main() {
	path, err := os.Getwd()
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
