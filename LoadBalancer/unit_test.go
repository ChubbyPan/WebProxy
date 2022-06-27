package main

import (

	// "flag"
	"context"
	"fmt"
	"log"
	"net/http"
	"net/http/httptest"
	"net/http/httputil"
	"net/url"
	"testing"
)

func TestAddBackend(t *testing.T) {
	backendServer1 := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "This is a backend ")
	}))
	defer backendServer1.Close()
	URL, err := url.Parse(backendServer1.URL)

	if err != nil {
		log.Fatal(err)
	}

	var serverPool ServerPool
	serverPool.AddBackend(&Backend{
		URL:          URL,
		Alive:        true,
		ReverseProxy: httputil.NewSingleHostReverseProxy(URL),
	})
	if len(serverPool.backends) != 1 {
		t.Errorf("AddBackend() error!")
	}
}

func TestSetAliveAndIsAlive(t *testing.T) {
	var serverPool ServerPool
	backend := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("This is a new backend")
	}))
	URL, err := url.Parse(backend.URL)
	if err != nil {
		log.Fatal(err)
	}

	serverPool.AddBackend(&Backend{
		URL:   URL,
		Alive: false,
	})
	if len(serverPool.backends) != 1 {
		t.Errorf("total counts error")
	}
	serverPool.backends[0].SetAlive(true)
	if serverPool.backends[0].IsAlive() != true {
		t.Errorf("haven't set true")
	}

}

func GetNewBackend() *url.URL {
	backendServer := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("this is the first backend")
	}))
	defer backendServer.Close()
	URL, _ := url.Parse(backendServer.URL)
	return URL
}
func TestGetNextPeer(t *testing.T) {
	var serverPool ServerPool
	URL1 := GetNewBackend()
	URL2 := GetNewBackend()
	URL3 := GetNewBackend()

	serverPool.AddBackend(&Backend{
		URL:   URL1,
		Alive: true,
	})
	serverPool.AddBackend(&Backend{
		URL:   URL2,
		Alive: false,
	})
	serverPool.AddBackend(&Backend{
		URL:   URL3,
		Alive: true,
	})
	serverPool.current = 0
	if serverPool.GetNextPeer() != serverPool.backends[2] {
		t.Errorf("GetNextPeer() error")
	}

}

func TestMarkBackendStatus(t *testing.T) {
	var serverPool ServerPool
	backendurl := GetNewBackend()
	serverPool.AddBackend(&Backend{
		URL:   backendurl,
		Alive: false,
	})
	if len(serverPool.backends) != 1 || serverPool.backends[0].Alive != false {
		log.Fatalf("init error")
	}
	serverPool.MarkBackendStatus(serverPool.backends[0].URL, true)
	if serverPool.backends[0].Alive != true {
		t.Errorf("MarkBackendStatus() error")
	}
}

func TestGetAttempsFromContext(t *testing.T) {

	req, err := http.NewRequest("GET", "https://www.baidu.com", nil)
	if err != nil {
		t.Errorf("%v", err)
	}

	attemps := 0
	ctx := context.WithValue(req.Context(), Attempts, attemps+1) // set key:Attemps value: 1 to req.context
	req = req.WithContext(ctx)                                   // change req context
	ans := GetAttemptsFromContext(req)
	if ans != 1 {
		t.Errorf("req.Context().Value(Attempts).(int) = %d; want 1", ans)
	}

}

func TestGetRetryFromContext(t *testing.T) {
	req, _ := http.NewRequest("GET", "https://www.baidu.com", nil)

	retry := 2

	ctx := context.WithValue(req.Context(), Retry, retry+1) // set
	req = req.WithContext(ctx)
	ans := GetRetryFromContext(req)
	if ans != 3 {
		t.Errorf("req.Context().Value(Attempts).(int) = %d; want 3", ans)
	}

}

func TestLbMoreThanThree(t *testing.T) {
	var serverPool ServerPool
	backendServerURL := GetNewBackend()

	proxy := httputil.NewSingleHostReverseProxy(backendServerURL)
	proxy.ErrorHandler = func(writer http.ResponseWriter, request *http.Request, e error) {
		// attempts := GetAttemptsFromContext(request)
		attempts := 4
		log.Printf("%s(%s) Attempting retry %d\n", request.RemoteAddr, request.URL.Path, attempts)
		ctx := context.WithValue(request.Context(), Attempts, attempts+1)
		// attempts < 3, dead loop, recurse
		lb(writer, request.WithContext(ctx))
	}
	serverPool.AddBackend(&Backend{
		URL:          backendServerURL,
		Alive:        true,
		ReverseProxy: proxy,
	})
	frontProxy := httptest.NewServer(serverPool.GetNextPeer().ReverseProxy)
	defer frontProxy.Close()

	req, err := http.NewRequest("GET", frontProxy.URL, nil)
	if err != nil {
		t.Errorf("%v", err)
	}

	client := http.Client{}
	client.Do(req)
}
