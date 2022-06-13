# load balancer part
review: 3种 strategy
round robin
weighted round robin
least connections

采用round robin方式,如果某一个backend挂机了,就把流量分配到其他正在运行的backend上

# proxy lab part

- step 1: 创建一个proxy
  - 可以接收来自client的连接；
  - 读取，解析request；
  - 将request发送给server
  - 读取server的response
  - 将response转发给client

- step 2：可以处理多并发连接
- step 3：添加caching的功能
  - 采用LRU，对web content进行缓存

## step 1
1. 首先实现处理HTTP/1.0 GET的请求的proxy （其他请求类似）；
2. proxy需要监听port上的connection请求；
3. 一旦connection已经建立，需要读取来自client的所有request，并解析这些request；需要判断request是否合法；
4. 建立和web server之间的连接，获取request的相关object；
5. 将server给出的response转发给client