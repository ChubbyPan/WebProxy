#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE 10
#define SBUFSIZE 16 // buffer maxsize
#define NTHREADS 4
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

struct URI{
    char hostname[MAXLINE];// hostname
    char port[MAXLINE]; // port
    char path[MAXLINE]; //resource path
};


// cache unit structure
typedef struct 
{
    /* data */
    char obj[MAX_OBJECT_SIZE];
    char uri[MAXLINE];
    int LRU;
    int isEmpty;

    int read_cnt;
    sem_t w;
    sem_t mutex;
}block;

// cache structure
typedef struct 
{
    /* data */
    block data[MAX_CACHE];
    int num;
}Cache;

Cache cache;

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

void init_Cache();
int get_Cache(char *uri);
int get_Index();
void update_LRU(int index);
void write_Cache(char *uri, char *buf);


sbuf_t sbuf;

int main(int argc, char **argv){
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    init_Cache();

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
    
    char cache_tag[MAXLINE];
    strcpy(cache_tag, uri);

    if (strcasecmp(method, "GET")) {                     //only handle GET request
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }             
    struct URI *uri_data = (struct URI *)malloc(sizeof(struct URI*));    

    int i;
    if ((i = get_Cache(cache_tag)) != -1)
    {
        //加锁
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt++;
        if (cache.data[i].read_cnt == 1)
            // printf("P before: cache.data[%d].w = %d\n", i, cache.data[i].w);
            // fflush(stdout);
            P(&cache.data[i].w);
            // printf("P after: cache.data[%d].w = %d\n", i, cache.data[i].w);
            // fflush(stdout);
        V(&cache.data[i].mutex);

        Rio_writen(fd, cache.data[i].obj, strlen(cache.data[i].obj));
        printf("cache hit\n");
        fflush(stdout);
        
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
            // printf("V: cache.data[%d].w = %d\n", i, cache.data[i].w);
            // fflush(stdout);
        V(&cache.data[i].mutex);
        return;
    }
    
    
    printf("cache not hit\n");
    fflush(stdout);

    parse_uri(uri, uri_data);//parse uri into uri_data,(hostname, port, resource path)
    printf("new uri: %s, new port: %s, resourse located: %s\n", uri_data->hostname, uri_data->port, uri_data->path);
    fflush(stdout);
    // proxy as a client
    int connfd;
    connfd = send_to_server(fd, uri_data);
    printf("have sent to server\n");
    fflush(stdout);
    // save message from server,transfer to browers
    char cache_buf[MAX_OBJECT_SIZE];
    rio_t rio_server_to_proxy;
    char buf_proxy[MAXLINE];
    size_t n ;
    int size_buf = 0;
    Rio_readinitb(&rio_server_to_proxy, connfd);
    while( (n = Rio_readlineb(&rio_server_to_proxy, buf_proxy, MAXLINE))!= 0){
        // printf("recieve from server\n");
        // fflush(stdout);
        size_buf += n;
        if(size_buf < MAX_OBJECT_SIZE)
            strcat(cache_buf, buf_proxy);
        printf("read %d bytes from server\n", (int)n);
        fflush(stdout);
        Rio_writen(fd, buf_proxy, n);

        // printf("send to client: %s", buf_proxy);
        // fflush(stdout);
    }
    Close(connfd);
    if(size_buf < MAX_OBJECT_SIZE){

        write_Cache(cache_tag, cache_buf);
    }
}

/* parse uri, split host, port and path into uri_data*/
void parse_uri(char *uri, struct URI *uri_data){
    char *default_hostname = "localhost";
    char *hostpose = strstr(uri, "//");
    // printf("hostpose: %s\n", hostpose);
    // fflush(stdout);
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





void init_Cache()
{
    cache.num = 0;
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        cache.data[i].LRU = 0;
        cache.data[i].isEmpty = 1;
        // w, mutex均初始化为1
        Sem_init(&cache.data[i].w, 0, 1);
        Sem_init(&cache.data[i].mutex, 0, 1);
        cache.data[i].read_cnt = 0;
    }
}

//从Cache中找到内容
int get_Cache(char *url)
{
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        //读者加锁
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt++;
        if (cache.data[i].read_cnt == 1)
            P(&cache.data[i].w);
        V(&cache.data[i].mutex);

        if ((cache.data[i].isEmpty == 0) && (strcmp(url, cache.data[i].uri) == 0))
            break;

        P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
        V(&cache.data[i].mutex);
    }
    if (i >= MAX_CACHE)
        return -1;
    return i;
}

//找到可以存放的缓存行
int get_Index()
{
    int min = __INT_MAX__;
    int minindex = 0;
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        //读锁
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt++;
        if (cache.data[i].read_cnt == 1)
            P(&cache.data[i].w);
        V(&cache.data[i].mutex);

        if (cache.data[i].isEmpty == 1)
        {
            minindex = i;
            P(&cache.data[i].mutex);
            cache.data[i].read_cnt--;
            if (cache.data[i].read_cnt == 0)
                V(&cache.data[i].w);
            V(&cache.data[i].mutex);
            break;
        }
        if (cache.data[i].LRU < min)
        {
            minindex = i;
            P(&cache.data[i].mutex);
            cache.data[i].read_cnt--;
            if (cache.data[i].read_cnt == 0)
                V(&cache.data[i].w);
            V(&cache.data[i].mutex);
            continue;
        }

        P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
        V(&cache.data[i].mutex);
    }

    return minindex;
}
//更新LRU
void update_LRU(int index)
{
    printf("start update_LRU,index = %d\n", index);
    fflush(stdout);
    for (int i = 0; i < MAX_CACHE; i++)
    {
        if (!cache.data[i].isEmpty&& i != index)
        {
            // printf("update_LRU, cache.data[%d].w = %d\t", i, cache.data[i].w);
            // fflush(stdout);

            P(&cache.data[i].w);

            // printf("update_LRU, cache.data[i].obj = %s\n", cache.data[i].obj);
            // fflush(stdout);

            cache.data[i].LRU--;
            V(&cache.data[i].w);
        }
    }
    printf("end update_LRU\n");
    fflush(stdout);
}
//写缓存
void write_Cache(char *uri, char *buf)
{

    int i = get_Index();
    //加写锁
    P(&cache.data[i].w);
    //写入内容
    strcpy(cache.data[i].obj, buf);
    strcpy(cache.data[i].uri, uri);
    cache.data[i].isEmpty = 0;
    cache.data[i].LRU = __INT_MAX__;
    printf("need write in cache[%d] (update others)\n", i);
    fflush(stdout);
    update_LRU(i);
    printf("updated cache\n");
    fflush(stdout);
    V(&cache.data[i].w);
}

