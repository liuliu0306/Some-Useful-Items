# 个人项目(部分)

这些是我近两年来在课题组团队项目之外的一些个人项目，涵盖了ITMO大学的课程作业、自己的需求和课题组的任务。这些项目涉及了不同的编程语言和领域。

## 1. 简易分布式银行系统
- 来源: ITMO-Distributed computing(分布式计算)-课程作业
- 编程语言: C语言
- 涉及内容: 管道通信、Lamport逻辑时钟
- 介绍：这是一个用C语言编写的分布式银行系统，可以模拟多个账户之间的转账操作，并记录每个时间点的总金额。该系统使用Lamport的标量时间代替物理时间，来确定事件的顺序和因果关系。该系统可以在不同的进程或机器上运行，通过管道通信来交换信息。

## 2. 实现DNS服务器和客户端
- 来源: ITMO-Network programming(网络编程)-课程作业
- 编程语言: C语言
- 涉及内容: DNS报文结构，网络通信
- 介绍：这是一个简单的DNS客户端和服务器的实现，仅支持A类型记录的查询。DNS客户端用于向任意DNS服务器发送DNS查询请求，并解析返回的DNS响应报文。DNS服务器接收查询请求并返回相应的IP地址。

## 3. join不同学院导师的信息
- 来源: 自己的需求-考研复试收集导师信息
- 编程语言: Python
- 涉及内容: 爬虫
- 介绍：这是一个用Python编写的爬虫程序，用于从杭州电子科技大学计算机学院和圣光机学院的官网上抓取共同导师的信息，并将其合并为一个表格。该程序可以帮助考研复试的同学快速了解不同学院导师的研究方向和联系方式。

## 4. 同步数据流(SDF)的实现
- 来源: ITMO-Computational Process Organization(计算过程组织)-课程作业
- 编程语言: Python
- 涉及内容: 同步数据流(SDF)的理论
- 介绍：这是一个用Python编写的同步数据流（Synchronous Data Flow）的计算模型，用来处理数据流的运算和传输。同步数据流是一种基于数据依赖性的并行计算模型，其中每个运算节点都有固定数量的输入和输出数据。该程序可以根据给定的SDF图和初始数据，自动执行SDF调度算法，并输出每个节点的执行次数和最终结果。

## 5. 脑机接口(BCI)系统前端
- 来源: 课题组任务
- 编程语言: JavaScript
- 涉及内容: 幻灯片框架-reveal.js
- 介绍：这是一个用JavaScript编写的脑机接口系统前端，用于进行脑机接口系统的运动想象实验。该前端使用reveal.js框架制作了一个幻灯片展示，其中包含了实验的说明、指导和反馈。该前端可以与后端的脑电信号采集和处理模块进行交互，实现脑机接口系统的完整功能。
