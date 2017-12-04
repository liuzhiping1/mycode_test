#include <linux/module.h>  
#include <linux/fs.h>  
#include <linux/cdev.h>  
#include <linux/device.h>  
#include <linux/platform_device.h>  
#include <asm/io.h>  
  
static int major = 250;  
static int minor=0;  
static dev_t devno;  
static struct class *cls;  
static struct device *test_device;  
           
#define TCFG0         0x0000                 
#define TCFG1         0x0004                              
#define TCON          0x0008               
#define TCNTB0        0x000C            
#define TCMPB0        0x0010             
  
static unsigned int *gpd0con;  
static void *timer_base;  
  
#define  MAGIC_NUMBER    'k'  
#define  BEEP_ON    _IO(MAGIC_NUMBER    ,0)  
#define  BEEP_OFF   _IO(MAGIC_NUMBER    ,1)  
#define  BEEP_FREQ   _IO(MAGIC_NUMBER   ,2)  
  
static void fs4412_beep_init(void)  
{     
    writel ((readl(gpd0con)&~(0xf<<0)) | (0x2<<0),gpd0con);  
    writel ((readl(timer_base +TCFG0  )&~(0xff<<0)) | (0xff <<0),timer_base +TCFG0);   
    writel ((readl(timer_base +TCFG1 )&~(0xf<<0)) | (0x2 <<0),timer_base +TCFG1 );   
  
    writel (500, timer_base +TCNTB0  );  
    writel (250, timer_base +TCMPB0 );  
    writel ((readl(timer_base +TCON )&~(0xf<<0)) | (0x2 <<0),timer_base +TCON );   
}  
  
void fs4412_beep_on(void)  
{  
    writel ((readl(timer_base +TCON )&~(0xf<<0)) | (0x9 <<0),timer_base +TCON );  
}  
  
void fs4412_beep_off(void)  
{  
    writel ((readl(timer_base +TCON )&~(0xf<<0)) | (0x0 <<0),timer_base +TCON );  
}  
  
static void beep_unmap(void)  
{  
        iounmap(gpd0con);  
        iounmap(timer_base);  
}  
  
static int beep_open (struct inode *inode, struct file *filep)  
{  
    fs4412_beep_on();  
    return 0;  
}  
  
static int beep_release(struct inode *inode, struct file *filep)  
{  
     fs4412_beep_off();  
     return 0;  
}  
  
#define BEPP_IN_FREQ 100000  
static void beep_freq(unsigned long arg)  
{  
    writel(BEPP_IN_FREQ/arg, timer_base +TCNTB0  );  
    writel(BEPP_IN_FREQ/(2*arg), timer_base +TCMPB0 );  
  
}  
  
static long beep_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)  
{  
    switch(cmd)  
    {  
        case BEEP_ON:  
            fs4412_beep_on();  
            break;  
        case BEEP_OFF:  
            fs4412_beep_off();  
            break;  
        case BEEP_FREQ:  
            beep_freq( arg );  
            break;  
        default :  
            return -EINVAL;  
    }  
    return 0;  
}  
  
static struct file_operations beep_ops=  
{  
    .open     = beep_open,  
    .release = beep_release,  
    .unlocked_ioctl      = beep_ioctl,  
};  
  
static int beep_probe(struct platform_device *pdev)  
{  
    int ret;      
    printk("match ok!");  
      
    gpd0con = ioremap(pdev->resource[0].start,pdev->resource[0].end - pdev->resource[0].start);  
    timer_base = ioremap(pdev->resource[1].start, pdev->resource[1].end - pdev->resource[1].start);  
  
    devno = MKDEV(major,minor);  
    ret = register_chrdev(major,"beep",&beep_ops);  
  
    cls = class_create(THIS_MODULE, "myclass");  
    if(IS_ERR(cls))  
    {  
        unregister_chrdev(major,"beep");  
        return -EBUSY;  
    }  
  
    test_device = device_create(cls,NULL,devno,NULL,"beep");//mknod /dev/hello  
    if(IS_ERR(test_device))  
    {  
        class_destroy(cls);  
        unregister_chrdev(major,"beep");  
        return -EBUSY;  
    }  
      
    fs4412_beep_init();  
      
    return 0;  
}  
  
static int beep_remove(struct platform_device *pdev)  
{  
    beep_unmap();  
    device_destroy(cls,devno);  
    class_destroy(cls);   
    unregister_chrdev(major,"beep");  
  
    return 0;  
}  
  
  
static struct platform_driver beep_driver=  
{  
    .driver.name = "bigbang",  
    .probe = beep_probe,  
    .remove = beep_remove,  
};  
   
  
static int beep_init(void)  
{  
    printk("beep_init");  
      
    return platform_driver_register(&beep_driver);  
}  
  
static void beep_exit(void)  
{  
    printk("beep_exit");  
    platform_driver_unregister(&beep_driver);  
      
    return;  
}  
  
  
MODULE_LICENSE("GPL");  
module_init(beep_init);  
module_exit(beep_exit);  