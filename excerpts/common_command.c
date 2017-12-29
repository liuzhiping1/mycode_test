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
du -sh
at 在指定时间执行程序
lsof
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
lscpu
lsdev
5、其他
more 
less
find . -name * |xargs grep key
git 相关
lsmod
insmod
probe
modinfo
udevadm monitor 可以用于将驱动程序核心事件和 udev 事件处理的计时可视化。
od od命令用于输出文件的八进制、十六进制或其它格式编码的字节，通常用于显示或查看文件中不能直接显示在终端的字符。

来自: http://man.linuxde.net/od
fbset显示帧缓冲特性
git push huawei master:huawei_b 把当前master分支推向远端huawei仓库的huawei_b分支