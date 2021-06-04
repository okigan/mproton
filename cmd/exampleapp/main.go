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

func mycallback2(param string) (string, error) {
	println("go lang in", "mycallback2", param)
	return "Done", nil
}

func main() {
	if len(os.Args) == 2 && os.Args[1] == "--self-test" {
		print("self test complete.\n")
		return
	}

	// path, err := filepath.Abs(filepath.Dir(os.Args[0]))
	path, err := os.Getwd()
	if err != nil {
		println(err)
	}

	m := mproton.New()
	m.SetTitle("mProton example ☄️")
	m.Bind("mycallback1", mycallback1)
	m.Bind("mycallback2", mycallback2)
	m.SetMenuExtraText("☄️")
	m.AddMenuExtra("Extra menu item ☄️!")
	m.SetContentPath(fmt.Sprintf("file://%s/protonappui/dist/index.html", path))

	m.Run()
}
