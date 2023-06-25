# DNS任务

这是一个简单的DNS客户端和服务器的实现，仅支持A类型记录的查询。DNS客户端用于向任意DNS服务器发送DNS查询请求，DNS服务器接收查询请求并返回相应的IP地址。

## 服务器功能
服务器代码（server.cpp）实现了以下功能：

1. 创建UDP套接字并绑定到指定的IP地址和端口。
2. 接收客户端发送的DNS查询请求。
3. 解析查询请求，获取查询的域名。
4. 构造DNS响应报文，设置相应的标志和资源记录。
5. 发送DNS响应报文给客户端。

## 客户端功能
客户端代码（client.cpp）实现了以下功能：

1. 创建UDP套接字并指定要连接的服务器的IP地址和端口。
2. 构造DNS查询报文，设置查询的域名和查询类型。
3. 想DNS服务器发送DNS查询报文。
4. 接收服务器返回的DNS响应报文。
5. 解析响应报文，提取其中的IP地址并打印到控制台。

## 使用方法
1. 首先获取项目文件`client.cpp`、`server.cpp`和`dns_protocol.h`

2. 开始编译server
   ```
   $ clang server.cpp -g -O3 -Werror -Wall -Wextra -pthread -pedantic -o server
   ```

3. 开始编译client
   ```
   clang client.cpp -g -O3 -Werror -Wall -Wextra -pthread -pedantic -o client
   ```

## 测试方法
1. 使用`dig`命令测试server.cpp是否正常运行

首先启动DNS服务器
   ```
   $ ./server
   ```

然后输入以下命令，向本地的53号端口(默认)发送DNS查询报文，server默认返回0.0.0.域名长度
   ```
   $ dig @127.0.0.1 www.xxxliu.com
   ```
得到命令行输出

![dig-output](/fig/dig.png)


2. 向远程的谷歌DNS服务器8.8.8.8发送查询报文测试client.cpp是否运行

输入以下命令
   ```
   $ ./client www.baidu.com 8.8.8.8
   ```
得到命令行输出 

![client-8.8.8.8](/fig/client-8.8.8.8.png)


3. 测试server和client

首先启动DNS服务器
   ```
   $ ./server
   ```

然后启动DNS客户端，向本地的53号端口发送DNS查询报文。
   ```
   $ ./client www.baidu.com 127.0.0.1
   ```
客户端输出 

![client-output](/fig/server-client.png)

服务器最终输出 

![client-server](/fig/server.png)