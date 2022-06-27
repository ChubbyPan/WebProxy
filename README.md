# proxy part
- step 1: 创建一个proxy
- step 2：可以处理多并发连接
- step 3：添加caching的功能
  - 采用LRU，对web content进行缓存

## step 1
1. 首先实现处理HTTP/1.0 GET的请求的proxy （其他请求类似）；
2. proxy需要监听port上的connection请求；
3. 一旦connection已经建立，需要读取来自client的所有request，并解析这些request；需要判断request是否合法；
4. 建立和web server之间的连接，获取request的相关object；
5. 将server给出的response转发给client

## step 2
方案一:
每次有请求连接到来时,对proxy对该连接的connect fd用一块新的内存空间进行保存;
当线程分离之后,用局部变量获取connect fd,并将申请的新内存释放;

**方案二:
使用共享buffer来对新连接进行保存,使用semaphore对buffer互斥访问;**

## step 3
采用读者-写者模型,添加caching功能,使用LRU策略
为了让proxy更高效,采用读优先: 互斥访问信号量, 写操作信号量;
只有第一个读者对写操作加锁, 最后一个读者对写操作解锁;




# load balance part
- 健康检查：
  1. active way： 遍历所有当前serverPool中的backend状态
  2. passive way： 每隔一定的时间，发送自身状态给lb，并对serverPool的记录进行修改。
- 负载策略：
  1. round robin 策略：将负载平均分给每一个server,采用轮询的方式,将所有server用一个list存储, 每次有新的连接到来的时候,都选择下一个alive server进行信息处理;
  2. least connect 策略：when new request incoming，选择一个当前连接数最少的server进行处理；
# 整体收获
1. 熟悉了Go语言
    sync/lock
    http/http_util/http_test
    context
    NewServer
    reverse proxy
2. 改进了csapp中单纯的proxy，功能更丰富了
3. 熟悉了httptest的使用，了解学习开源代码的过程，比起单纯的照搬，试着对每一个函数进行单元测试能够更好的了解整个项目的框架。
# 改进
1. load balance部分，least connect没有全部实现（需要修改每一个backend的结构，添加一个connect counts就行）
   ->还有哪些负载均衡策略呢？ 想一想可以怎么实现？
   weight round robin/ Random / response time/Flash DNS
   >response time： 负载均衡设备对内部各服务器发出一个探测请求（例如Ping），然后根据内部中各服务器对探测请求的最快响应时间来决定哪一台服务器来响应客户端的服务请求。此种均衡算法能较好的反映服务器的当前运行状态，但这最快响应时间仅仅指的是负载均衡设备与服务器间的最快响应时间，而不是客户端与服务器间的最快响应时间。
   >DNS响应均衡（Flash DNS）：在Internet上，无论是HTTP、FTP或是其它的服务请求，客户端一般都是通过域名解析来找到服务器确切的IP地址的。在此均衡算法下，分处在不同地理位置的负载均衡设备收到同一个客户端的域名解析请求，并在同一时间内把此域名解析成各自相对应服务器的IP地址（即与此负载均衡设备在同一位地理位置的服务器的IP地址）并返回给客户端，则客户端将以最先收到的域名解析IP地址来继续请求服务，而忽略其它的IP地址响应。在种均衡策略适合应用在全局负载均衡的情况下，对本地负载均衡是没有意义的。
2. 需要用go语言复现proxy，整合一下项目（当前使用的是API：httputil.NewSingleHostReverseProxy(）