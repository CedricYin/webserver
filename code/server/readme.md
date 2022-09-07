# server
服务器主要代码

## epoller
* 封装了epoll相关的系统调用

## webserver
* 管理服务器信息，包括定时器、线程池、连接池、所有客户端的http请求等
* 利用IO复用技术Epoll与线程池实现多线程的Reactor/Proactor高并发模型