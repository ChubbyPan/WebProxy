package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"
	"sync"
	"sync/atomic"
	"time"
)
// backend struct define
type Backend struct{
	URL 	*url.URL
	Alive 	bool
	mux 	sync.RWMutex
	ReverseProxy	*httputil.ReverseProxy
}
// ReverseProxy is an HTTP Handler that takes an incoming request 
// and sends it to another server
// proxying the response back to the client.

// store and track backend
type ServerPool struct{
	backends	[]*Backend
	current 	uint64
} 

u, _ := url.Parse("http://localhost:8080")
rp := httputil.NewSingleHostReverseProxy(u)
http.HandlerFunc(rp.ServeHTTP)
// initialize a reverse proxy  
// relay requests to the passed url(localhost:8080)
// all response sent to client from here

// selection process
// avoid multiple clients request next peer->add mutex
// Traverse the slice as a cycle, get next server index
func (s *ServerPool) NextIndex() int {
	return int(atomic.AddUnit64(&s.current, uint64(1)) % uint64(len(s.backends)))
}
// skip dead backends during the next pick
// treverse find next alive backend
func (s *ServerPool) GetNextPeer() *Backend{
	next := s.NextIndex()
	l := len(s.backends) + next
	for i := next; i < l; i++{
		idx := i % len(s.backends)
		if s.backends[idx].IsAlive(){
			if i != next {
				atomic.StoreUint64(&s.current, uint64(idx))
			}
			return s.backends[idx]
		}
	}
	return nil
}

func (b *Backend) SetAlive() (alive bool){
	b.mux.Lock()
	b.Alive = alive
	b.mux.Unlock()
}

func (b *Backend) IsAlive() (alive bool){
	b.mux.RLock()
	alive = b.Alive
	b.mux.RUnlock()
	return
}

// load balance request
func lb(w http.ReverseWriter, r *http.Request){
	peer := serverPool.GetNextPeer()
	if peer != nil{
		peer.ReverseProxy.ServeHTTP(w, r)
		return 
	}
	http.Error(w, "Service not available", http.StatusServiceUnavailable)
}

server := http.Server{
	Addr:    fmt.Sprintf(":%d", port),
	Handler: http.HandlerFunc(lb),
}

// route traffic only to healthy backends
// active backend:while current request coming, select a backend if it's unresponsive, mark it as down
// passive backend: ping backends on fixed intervals and check status

// for active backends:
proxy.ErrorHandler = func(writer http.ResponseWriter, request *http.Request, e error) {
	log.Printf("[%s] %s\n", serverUrl.Host, e.Error())
	retries := GetRetryFromContext(request)
	if retries < 3 {
	  select {
		case <-time.After(10 * time.Millisecond):
		  ctx := context.WithValue(request.Context(), Retry, retries+1)
		  proxy.ServeHTTP(writer, request.WithContext(ctx))
		}
		return
	  }
  
	// after 3 retries, mark this backend as down
	serverPool.MarkBackendStatus(serverUrl, false)
  
	// if the same request routing for few attempts with different backends, increase the count
	attempts := GetAttemptsFromContext(request)
	log.Printf("%s(%s) Attempting retry %d\n", request.RemoteAddr, request.URL.Path, attempts)
	ctx := context.WithValue(request.Context(), Attempts, attempts+1)
	lb(writer, request.WithContext(ctx))
  }
  
