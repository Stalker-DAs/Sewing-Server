# Sewing-Server
本项目为基于协程的C++服务器，该服务器目前包含日志模块、配置模块、线程模块、协程模块、协程调度模块、IO调度模块、Hook模块、ByteArray模块、Stream模块、Socket模块、TcpServer模块和Http模块。而Http模块中包括HttpRequest、HttpResponse、HttpRequestParser、HttpResponseParser、HttpSession、HttpConnection、Servlet和HttpServer。下面将分别介绍各个模块内容。
整个服务器的模块逻辑如下图所示：
![5184afd8-44d4-4ed4-993d-d516c34d7196](https://github.com/user-attachments/assets/493573dd-49b5-436a-bb09-5d763cfee79c)

## 模块介绍
### 日志模块
日志模块主要是方便开发者和服务器管理人员掌握服务器运行状态，及时发现一些警告和错误。整个日志模块的核心是日志器，包括日志器名称、日志器级别、日志默认格式并维护一个LogAppender的列表。日志器的核心函数为Log函数，需要输入一个LogEvent参数，通过不同的LogAppender输出到对应位置。
### 配置模块
配置模块主要作用是管理项目中一些重要的变量，并使其可通过Yaml文件进行配置修改。配置模块通过一个模板类ConfigVar来保存变量内容和相应功能。在通过一个Config类对这些配置变量进行统一管理。在Config类中还包括对Yaml文件解析的方法，可以通过书写Yaml文件对变量进行配置。
### 线程模块
线程模块主要包括线程创建、线程运行和线程等待。线程创建通过pthread_create创建一个新的线程并将要运行函数的地址送入。线程运行就是执行预先设置的回调函数。线程等待则是对pthread_join的一个封装。
### 协程模块
协程模块提供了一种更轻量级的控制机制，它允许程序在多个不同的执行点之间来回切换，而不仅仅是传统的函数调用和返回，此外，与线程不同，协程的切换始终处于用户态，不需要内核参与，因此上下文切换的开销远低于线程，减少开销。
### 协程调度模块
协程调度模块的主要作用是管理线程与协程，为协程任务分配给不同线程，可以理解成N对M的一个比例，N个协程任务分配给M个不同的线程。
### IO调度模块
IO协程调度模块是协程调度模块和计时器管理器的一个子类，专注于处理与输入/输出操作相关的协程，旨在通过epoll等IO相关技术提高IO操作的效率和系统的响应能力。
### Hook模块
hook的最主要目的是对系统调用API进行封装将一些同步API改成异步API，例如sleep、socket IO等相关的API，可以将其改写成异步的来提高性能。
### ByteArray模块
序列化是指将对象转化为可以存储或传输的二进制字节序列的格式。其主要作用是1.将数据转换为二进制格式，然后存储到磁盘文件中，以便以后恢复或备份。 2.便于将对象或数据结构序列化为二进制流，以便通过网络传输对象。
### Stream模块
流模块主要作用是为了封装统一的接口，比如封装文件或socket的读写数据read、write或关闭close等操作，使用的时候可以采用统一的风格操作。
### Socket模块
Socket模块的主要作用是封装socket类，以提供所有socket API功能，比如accept、listen、connect等。此外，将IPv4，IPv6，Unix地址统一起来设计了一个Address类，并提供了域名、IP的解析功能。
### TcpServer模块
tcpserver模块主要是对socket类进行封装，提供一些简单的API可以快速绑定一个或多个地址、启动服务、监听端口、关闭服务、处理socket连接等功能。
### Http模块
Http模块部分通过使用上面提到的服务器基础模块，搭建了一个支持高并发的Http服务器。该模块中HttpRequest和HttpResponse用于将Http请求和响应的内容进行封装统一管理，类的内部对不同参数设置了相应的set和get请求，方便开发人员获取或设置参数。HttpRequestParser和HttpResponseParser用于解析http命令，并将其保存到前面提到的HttpRequest和HttpResponse。HttpSession和HttpConnection继承了socketStream，分别定义了服务端和客户端接收数据和发送数据的操作。Servlet是用于定义针对不同uri的http请求，服务器需要做出的不同响应，例如doGet、doPost等。HttpServer则是基于TcpServer来实现的，主要实现当一个连接到来之后需要进行的操作。

## 运行环境
运行系统：Ubuntu 20.04

## 其他
版权说明：该项目用于自我学习，不会用于商业用途。项目学习自sylar大佬的C++高性能服务器框架：https://github.com/sylar-yin/sylar。
