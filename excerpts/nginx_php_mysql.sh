#1、安装 ->#tar zxvf mysql-5.0.40.tar.gz -># ./configure --prefix=/usr/local/mysql ->#make && make install ->#mysql_install_db --user=mysql #初始化MySQL数据库 ->mysqld_safe &          //启动MySQL  ->#mysqladmin -u root password 123456    //设置MySQL密码 ->#echo "/usr/local/mysql/bin/mysqld_safe &" >>/etc/rc.local #自启动
#
#
#
#
#
#
