# proxy lab

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



## file introduction
This directory contains the files you will need for the CS:APP Proxy
Lab.

1. proxy.c
2. csapp.h
3. csapp.c

    These are starter files.  csapp.c and csapp.h are described in
    your textbook. 

    You may make any changes you like to these files.  And you may
    create and handin any additional files you like.

    Please use `port-for-user.pl' or 'free-port.sh' to generate
    unique ports for your proxy or tiny server. 

4. Makefile
    
    This is the makefile that builds the proxy program.  Type "make"
    to build your solution, or "make clean" followed by "make" for a
    fresh build. 

    Type "make handin" to create the tarfile that you will be handing
    in. You can modify it any way you like. Your instructor will use your
    Makefile to build your proxy from source.

5. port-for-user.pl
   
   Generates a random port for a particular user
    usage: ./port-for-user.pl <userID>

6. free-port.sh
    
    Handy script that identifies an unused TCP port that you can use
    for your proxy or tiny. 
    usage: ./free-port.sh

7. driver.sh
    
    The autograder for Basic, Concurrency, and Cache.        
    usage: ./driver.sh

8. nop-server.py
     
     helper for the autograder.         

9. tiny
    
    Tiny Web server from the CS:APP text

