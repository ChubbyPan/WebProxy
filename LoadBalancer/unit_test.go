package main

import (

	// "flag"
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

func TestNextIndex(t *testing.T) {
	var serverPool ServerPool
	var backend1 Backend
	var backend2 Backend
	var backend3 Backend
	serverPool.AddBackend(&backend1)
	serverPool.AddBackend(&backend2)
	serverPool.AddBackend(&backend3)
	fmt.Printf(">>> len(serverPool): %d", len(serverPool.backends))

	serverPool.NextIndex()
	if serverPool.current != 1 {
		t.Errorf("1:%d", serverPool.current)
	}

	serverPool.NextIndex()
	if serverPool.current != 2 {
		t.Errorf("2:%d", serverPool.current)
	}

	serverPool.NextIndex()
	if serverPool.current != 3 {
		t.Errorf("3:%d", serverPool.current)
	}
}

func TestGetNextPeer(t *testing.T) {
	var serverPool ServerPool
	backend1 := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("This is a backent")
	}))
	defer backend1.Close()
	URL, err := url.Parse(backend1.URL)
	if err != nil {
		log.Fatal(err)
	}

	serverPool.AddBackend(&Backend{
		URL:          URL,
		Alive:        true,
		ReverseProxy: httputil.NewSingleHostReverseProxy(URL),
	})

}
