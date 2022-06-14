# proxy lab part

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