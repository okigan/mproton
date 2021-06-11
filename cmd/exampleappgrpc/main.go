// go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
// go get -u google.golang.org/protobuf/cmd/protoc-gen-go
// go install google.golang.org/protobuf/cmd/protoc-gen-go

// go get -u google.golang.org/grpc/cmd/protoc-gen-go-grpc
// go install google.golang.org/grpc/cmd/protoc-gen-go-grpc

//go:generate protoc --go_out=./ --go_opt=paths=source_relative --go-grpc_out=./proto  proto/service.proto

//go:generate protoc -I=./proto service.proto --js_out=import_style=commonjs:./webapp/hwmonitor/src/proto
//go:generate protoc -I=./proto service.proto --grpc-web_out=import_style=commonjs,mode=grpcwebtext:./webapp/hwmonitor/src/proto

package main

// https://itnext.io/streaming-data-with-grpc-2eb983fdee11

import (
	"fmt"
	"log"
	"net"
	"net/http"
	"time"

	"github.com/improbable-eng/grpc-web/go/grpcweb"
	"github.com/okigan/mproton"
	hardwaremonitoring "github.com/okigan/mproton/cmd/exampleappgrpc/proto"
	"google.golang.org/grpc"
)

type User struct {
	Name string
	Age  int
}

func main() {

	fmt.Println("Welcome to streaming HW monitoring")
	lis, err := net.Listen("tcp", ":7777")
	if err != nil {
		panic(err)
	}
	// Create a gRPC server
	gRPCserver := grpc.NewServer()

	// Create a server object of the type we created in server.go
	s := &Server{}
	hardwaremonitoring.RegisterHardwareMonitorServer(gRPCserver, s)

	go func() {
		log.Fatal(gRPCserver.Serve(lis))
	}()

	// We need to wrap the gRPC server with a multiplexer to enable
	// the usage of http2 over http1
	grpcWebServer := grpcweb.WrapServer(gRPCserver)

	multiplex := grpcMultiplexer{
		grpcWebServer,
	}

	// a regular http router
	r := http.NewServeMux()
	// Load our React application
	webapp := http.FileServer(http.Dir("webapp/hwmonitor/build"))
	// Host the web app at / and wrap it in a multiplexer
	r.Handle("/", multiplex.Handler(webapp))

	// create a http server with some defaults
	srv := &http.Server{
		Handler:      r,
		Addr:         "localhost:8080",
		WriteTimeout: 15 * time.Second,
		ReadTimeout:  15 * time.Second,
	}

	// host it
	go func() {
		log.Fatal(srv.ListenAndServe())
	}()

	m := mproton.New()
	m.SetTitle("Proton example ☄️")

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

	m.SetContentPath("http://localhost:8080")
	// m.SetContentPath("file:///Users/iokulist/Github/okigan/mproton/cmd/exampleappgrpc/webapp/hwmonitor/build/index.html")

	m.Run()

	gRPCserver.GracefulStop()
}

// grpcMultiplexer enables HTTP requests and gRPC requests to multiple on the same channel
// this is needed since browsers dont fully support http2 yet
type grpcMultiplexer struct {
	*grpcweb.WrappedGrpcServer
}

// Handler is used to route requests to either grpc or to regular http
func (m *grpcMultiplexer) Handler(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if m.IsGrpcWebRequest(r) {
			m.ServeHTTP(w, r)
			return
		}
		next.ServeHTTP(w, r)
	})
}
