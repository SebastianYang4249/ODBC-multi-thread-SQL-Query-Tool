使用

edit config/config.yaml and config/cmd.txt

config.yaml
configs of thread pool and odbc

hostIp<string> 目标ip
port<int> 目标端口（7432）
database<string> 目标数据库
userName<string> 连接用户名称
password<string> 用户密码

tolerant<int> 最大的容忍时间，超过则会断开连接

bufferSize<int> 线程池的circular_buffer大小，影响不大，大小太小可能出现线程丢失问题

runMode<int> 0 串行 1 并行
numberOfSequentialUser<int> 并发量
parallelOption<int> 0 固定qps 1 可变qps
numberOfParallelUsers<int> qps的数量

问题1 ODBC链接每条语句需要开一个新的连接

