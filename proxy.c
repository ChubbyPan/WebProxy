#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 16 // buffer maxsize
#define NTHREADS 4
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

struct URI{
    char hostname[MAXLINE];// hostname
    char port[MAXLINE]; // port
    char path[MAXLINE]; //resource path
};

// define productor-consumer pattern data structure
typedef struct{
    int *buf;       // buffer array
    int n;          // maximum number of slots
    int front;      // buf[(front+1)%n] is the first item
    int rear;       // buf[rear % n] is the last item
    sem_t mutex;    // protects accesses to bufs
    sem_t slots;    // counts available slots
    sem_t items;    // counts available items
}sbuf_t;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, struct URI *uri_data);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

void request_to_proxy(int fd );
int send_to_server(int fd, struct URI *uri_data);
void *thread(void *vargp);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);


sbuf_t sbuf;


int main(int argc, char **argv){
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    /* Check command line args */
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    /* listen from client*/
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);

    for(int i = 0 ; i < NTHREADS; i++){
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
	    clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //proxy connected with client
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); 
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	    // request_to_proxy(connfd);    //client request to proxy, proxy redirect to server
	    // Close(connfd);                                             //line:netp:tiny:close
        sbuf_insert(&sbuf, connfd);

    }

    return 0;
}

void *thread(void *vargp){
    Pthread_detach(pthread_self());
    while(1){
        int connfd = sbuf_remove(&sbuf);
        request_to_proxy(connfd);
        close(connfd);
    }
}

/* 
handle request line and header from client, store, wait to connect with server 
*/
void request_to_proxy(int fd ){

    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio, server_rio;

    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //store in buf
        return;
    printf("buf: %s", buf);
    fflush(stdout);
    sscanf(buf, "%s %s %s", method, uri, version); // client request: method, uri, version
    if (strcasecmp(method, "GET")) {                     //only handle GET request
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }             
    struct URI *uri_data = (struct URI *)malloc(sizeof(struct URI*));    
    parse_uri(uri, uri_data);//parse uri into uri_data,(hostname, port, resource path)
    printf("new uri: %s, new port: %s, resourse located: %s\n", uri_data->hostname, uri_data->port, uri_data->path);
    fflush(stdout);
    // proxy as a client
    int connfd;
    connfd = send_to_server(fd, uri_data);
    printf("have sent to server\n");
    fflush(stdout);
    // save message from server,transfer to browers
    rio_t rio_server_to_proxy;
    char buf_proxy[MAXLINE];
    size_t n ;
    Rio_readinitb(&rio_server_to_proxy, connfd);
    while( (n = Rio_readlineb(&rio_server_to_proxy, buf_proxy, MAXLINE))!= 0){
        printf("recieve from server\n");
        fflush(stdout);
        Rio_writen(fd, buf_proxy, n);
        printf("send to client: %s", buf_proxy);
        fflush(stdout);
    }
    Close(connfd);
}

/* parse uri, split host, port and path into uri_data*/
void parse_uri(char *uri, struct URI *uri_data){
    char *default_hostname = "localhost";
    char *hostpose = strstr(uri, "//");
    printf("hostpose: %s\n", hostpose);
    fflush(stdout);
    if (hostpose == NULL){
        char *pathpose = strstr(uri, "/");
        if (pathpose != NULL){
            strcpy(uri_data->path, pathpose);
        }
        return;
    }
    else{
        char *portpose = strstr(hostpose + 2, ":");
        if (portpose != NULL){
            int tmp;
            // after port: path
            sscanf(portpose + 1, "%d%s", &tmp, uri_data->path);
            sprintf(uri_data->port, "%d", tmp);
            *portpose = '\0';
        }
        else{
            char *pathpose = strstr(hostpose + 2, "/");
            if (pathpose != NULL){
                strcpy(uri_data->path, pathpose);
                *pathpose = '\0';
            }
        }
        strcpy(uri_data->hostname, hostpose + 2);
    }
    return ;
}

// request to server, server's hostname as same with proxy
// proxy has already had listened port -> server has listend port,do not create listened port again
int send_to_server(int fd, struct URI *uri_data ){
    int server_connectfd;
    server_connectfd = Open_clientfd(uri_data->hostname, uri_data->port);
    if(server_connectfd < 0){
        printf("connection failed \n");
        fflush(stdout);
    }

    char buf_client[MAXLINE]; // all request lines and request headers buf
    rio_t rio_server;//read response
    Rio_readinitb(&rio_server, server_connectfd);
    printf("HTTP request start\n");
    /* send request line, headers */
    /* send request line */
    sprintf(buf_client, "GET %s HTTP/1.0\r\n", uri_data->path);
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));
    /* send request headers*/ 
    sprintf(buf_client, "Host: %s\r\n", uri_data->hostname);
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "%s",user_agent_hdr); 
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Connection: close\r\n");
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "Proxy-Connection: close\r\n");
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));
    sprintf(buf_client, "\r\n");
    Rio_writen(server_connectfd, buf_client, strlen(buf_client));

    return server_connectfd;
}


void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}


/* create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

/* clean up buffer sp*/ 
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

/* insert item onto the rear of shared buffer sp*/
void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
} 

int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}



