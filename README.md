---
title: php-monitor
tags: php内部函数监控扩展
---

＃ 安装

```
phpize
./configure
make & make install
```
需要redis支持，当前只支持php5.5及以上版本
   
＃ 配置

php.ini
```
extension=monitor.so
[monitor]
monitor.enable = 1 #启用
monitor.cache_host = "127.0.0.1" ＃redis 服务器地址
monitor.cache_port = "6379" ＃redis端口
monitor.cache_set_key = "_monitor_php_function"
#redis  set类型数据，用于存放被监测的函数
monitor.cache_list_key = "_monitor_php_function_info" #redis list队列，监测数据可以从这里读取……^_^
```

＃ 例子
```
reids-cli

#设置监控函数

sadd _monitor_php_function mysql_query|table

#读取监控纪录

rpop _monitor_php_function_info
```
