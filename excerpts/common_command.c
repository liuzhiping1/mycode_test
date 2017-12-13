http://man.linuxde.net/ss
常用命令：
1、网络管理
netstat -npt
netstat -a 列出所有端口（监听和未监听的）
ss -pl 查看进程使用的socket

tcpdump 打印所有经过网络接口的数据包的头信息
来自: http://man.linuxde.net/tcpdump

ip -s link list 显示更加详细的设备信息
iptables 常用的防火墙软件
2、系统管理
ps -ef
ps aux
w
who
uname
service
vmstat
uptime
at 在指定时间执行程序
3、编程开发
ldconfig 动态链接库配置命令
objdump 显示二进制信息
gdb
ld
ldd
netstat
ping
traceroute 显示数据包到主机的距离
4、硬件管理
lsusb 显示usb设备列表信息
arch 显示当前主机硬件架构类型
hwclock
5、其他
more 
less
find . -name * |xargs grep key

