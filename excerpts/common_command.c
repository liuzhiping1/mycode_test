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
nm 
readelf
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
traceroute <目的主机地址> 测试与其它主机的网络链接路径
Nslookup  	使用dns服务器查询域名
Hostname 查看当前的主机名。
Dhclient 使用dhclient命令可以从DHCP服务器中申请新的网络配置应用到当前主机。
lsof –i lsof全名为list opened files，即列出系统中已经被打开的文件。查看套接字
df -l
fdisk /dev/sdb
parted /dev/sdb  功能更强大 ，然后输入print list
dd if=   of=    转换文件-convert and copy a file
ldd myprog | grep libc    -- print shared object dependencies
showmount - show mount information for an NFS server
seq 打印数的序列
【制作根文件系统首先要生成一个虚拟磁盘，创建一个虚拟磁盘的两种方法：】
     dd if=/dev/zero of=vexpress.img bs=512 count=$((2*1024*100))
     qemu-img create -f raw vexpress.img 100M
【虚拟磁盘中创建分区并修改】
     (1). fdisk vexpress.img ，然后使用n命令创建分区，各种下一步就行；
     (2). losetup /dev/loop0 vexpress.img ，挂载vexpress.img到/dev/loop0设备上；
     (3). partx -u /dev/loop0 ，使用partx命令让系统刷新系统的分区信息；
     (4). mkfs.ext2 /dev/loop0p1 ，制作ext2格式的文件系统；
     (5). mkdir rootfs ，建立一个rootfs目录用来作为挂载目录，
           mount -o loop /dev/loop0p1 ./rootfs ，将生成的ext2格式的分区挂载到rootfs目录；
     (6). 执行到这里虚拟磁盘就已经制作好了，下面的两个步骤是卸载磁盘时的操作，可以先跳过，直接到第三节去编译 busybox；
     (7). partx -d /dev/loop0 ，卸载loop0设备下的分区； 如果执行不成功可以试试  sudo umount -f rootfs
     (8). losetup -d /dev/loop0 ，卸载loop0设备；
     说明：
     如果直接使用 mkfs.ext3 写入img文件，无法使用fdisk显示分区信息；
     fdisk n命令 默认的first sector 是2048扇区；默认分区名为 文件名+分区序号；
     mount 挂载空分区会报错 wrong fs type, bad option, bad superblock；
     losetup /dev/loop0 vexpress.img 命令相当于mount命令中的 -o loop 参数；
     partx -u /dev/loop0  强制内核刷新可识别分区；
     查看分区类型  df -Th；
ctags -R 生成tags文件
vim command- :set tags=/path/tags 设置当前tag文件，“Ctrl + ]” 定位定义的地方，“Ctrl + o/t ”组合快捷键返回，【重要：gd组合键-对光标所在处的word进行快捷查找定位】
pstree 进程树
type 这个命令很有意思，显示一个命令类型描述。

fg 恢复ctrl+z
　　(1) CTRL+Z挂起进程并放入后台
　　(2) jobs 显示当前暂停的进程
　　(3) bg %N 使第N个任务在后台运行(%前有空格)
　　(4) fg %N 使第N个任务在前台运行