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
	attempts := GetAttemptsFromContext(r)
	if attempts > 3 {
		log.Printf("%s(%s) Max attemps reached, terminating\n", r.RemoteAddr, r.URL.Path)
		http.Error(w, "Service not available", http.StatusServiceUnavailable)
		return
	}

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

// route traffic only to healthy backends, two ways:
// active way:while current request coming, select a backend if it's unresponsive, mark it as down
// passive way: ping backends on fixed intervals and check status

// for active check way:
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

  const (
	  Attempts 	int = iota
	  Retry
  )

  func GetRetryFromContext(r *http.Request) int {
	  if retry, ok := r.Context().Value(Retry).(int); ok{
		  return retry
	  }
	  return 0
  }

// passive check way:
func  isBackendAlive(u *url.URL) bool {
	timeout := 2 * time.Second
	conn, err := net.DialTimeout("tcp", u.Host, timeout)
	if err != nil{
		log.Println("Site unreachable, error: ", err)
		return false
	}
	_ = conn.Close()
	return true
}

// iterate servers, mark their status
func (s *ServerPool) HealthCheck() {
	for _, b := range s.backends {
		status := "up"
		alive := isBackendAlive(b.URL)
		b.SetAlive(alive)
		if !alive {
			status = "down"
		}
		log.Printf("%s [%s]\n", b.URL, status)
	}
}

// healthCheck runs a routine for check status of the backends 
// every 20 secs
func healthCheck() {
	t := time.NewTicker(time.Second * 20)
	for {
		select {
		case <-t.C:
			log.Println("Starting health check...")
      		serverPool.HealthCheck()
      		log.Println("Health check completed")
		}
	}
}

var serverPool ServerPool
func main() {
	var serverList string
	var port int
	flag.StringVar(&serverList, "backends", "", "Load balanced backends, use commas to separate")
	flag.IntVar(&port, "port", 3030, "Port to serve")
	flag.Parse()

	if len(serverList) == 0 {
		log.Fatal("Please provide one or more backends to load balance")
	}

	// parse servers
	tokens := strings.Split(serverList, ",")
	for _, tok := range tokens {
		serverUrl, err := url.Parse(tok)
		if err != nil {
			log.Fatal(err)
		}

		proxy := httputil.NewSingleHostReverseProxy(serverUrl)
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

		serverPool.AddBackend(&Backend{
			URL:          serverUrl,
			Alive:        true,
			ReverseProxy: proxy,
		})
		log.Printf("Configured server: %s\n", serverUrl)
	}

	// create http server
	server := http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: http.HandlerFunc(lb),
	}

	// start health checking
	go healthCheck()

	log.Printf("Load Balancer started at :%d\n", port)
	if err := server.ListenAndServe(); err != nil {
		log.Fatal(err)
	}
}