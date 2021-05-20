# aflnw_blackbox
利用AFL黑盒测试网络协议

AFL是基于变异的模糊测试方法的代表工作，其主要应用于非结构化数据处理程序的漏洞挖掘中。但使用AFL具有比较多的限制：

1. 本地运行被测程序，从而获取覆盖率等反馈信息 
2. 被测程序从基本输入输出获取数据

因此无法直接使用AFL对远程服务进行黑盒测试

## 现有工作

目前针对限制2已经有一些解决方案：

1. hook socket调用：利用 `preeny`库辅助；AFLplusplus
    1. https://www.cnblogs.com/hac425/p/9416917.html
    2. https://github.com/AFLplusplus/AFLplusplus/tree/stable/utils/socket_fuzzing
2. 修改AFL传递数据的方式：AFLNet: A Greybox Fuzzer for Network Protocols，aflnet在AFL的基础上，将标准输入修改为网络发包的方式
    1. https://github.com/aflnet/aflnet
    2. https://www.comp.nus.edu.sg/~abhik/pdf/AFLNet-ICST20.pdf
3. 修改网络程序接收数据的方式：bind9的代码中专门提供了用于Fuzz的部分。
    1. https://github.com/isc-projects/bind9/tree/main/fuzz
4. 利用AFL Persistent Mode
    1. https://www.fastly.com/blog/how-fuzz-server-american-fuzzy-lop
    2. https://sensepost.com/blog/2017/fuzzing-apache-httpd-server-with-american-fuzzy-lop-%2B-persistent-mode/
5. 利用辅助程序转发AFL的输入
    1. https://github.com/LyleMi/aflnw/blob/main/README.zh-cn.md


但是如果无法将程序放在本地运行，比如物联网设备在拿不到固件的情况下，如何利用AFL的变异方式进行模糊测试。

## 黑盒方案

在aflnw的基础上，对辅助程序的工作方式进行了修改，从而实现在不对AFL和被测程序进行修改的条件下，使用一个辅助程序接收AFL从标准输入传递进来的数据，然后通过网络转发给UPnP服务，辅助程序会间隔性地与UPnP端口建立TCP连接，从而判断测试用例是否导致程序崩溃。

## 如何安装
```
git clone https://github.com/LyleMi/aflnw.gitcd aflnw
export CC=/path/to/afl/afl-clang-fast
mkdir build && cd build && cmake .. && make
```



## 如何使用

1. 使用wireshark采集种子输入（Follow→TCP Stream，保存为raw文件）
2. 确定通信协议（udp/tcp）、服务端监控地址、服务端监控端口、socket本地绑定地址
3. fuzz，以UPnP协议为例
```
afl-fuzz -t 1000+ -i ./soap_input/ -o ./soap_out/ -- ./build/aflnw -a 192.168.2.2 -p 5000 -m tcp
afl-fuzz -t 2000+ -i ./ssdp_input/ -o ./ssdp_out/ -- ./build/aflnw -a 239.255.255.250 -p 1900 -m udp
```
4. 崩溃重放
```
./build/aflnw -a 239.255.255.250 -p 1900 -m udp < soap_out/crashes/id:00000....
./build/aflnw -a 192.168.2.2 -p 5000 -m tcp < ssdp_out/crashes/id:000000.....
```

## 问题
效率很低
