#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <mach/regs-lcd.h>
#include <mach/regs-gpio.h>
#include <mach/fb.h>
#include <linux/pm.h>

/*FrameBuffer设备名称*/
static char driver_name[] = "my2440_lcd";

/*定义一个结构体用来维护驱动程序中各函数中用到的变量
  先别看结构体要定义这些成员，到各函数使用的地方就明白了*/
struct my2440fb_var
{
    int lcd_irq_no;           /*保存LCD中断号*/
    struct clk *lcd_clock;    /*保存从平台时钟队列中获取的LCD时钟*/
    struct resource *lcd_mem; /*LCD的IO空间*/
    void __iomem *lcd_base;   /*LCD的IO空间映射到虚拟地址*/
    struct device *dev;

    struct s3c2410fb_hw regs; /*表示5个LCD配置寄存器，s3c2410fb_hw定义在mach-s3c2410/include/mach/fb.h中*/

    /*定义一个数组来充当调色板。
    据数据手册描述，TFT屏色位模式为8BPP时，调色板(颜色表)的长度为256，调色板起始地址为0x4D000400*/
    u32    palette_buffer[256]; 
    u32 pseudo_pal[16];   
    unsigned int palette_ready; /*标识调色板是否准备好了*/
};

/*用做清空调色板(颜色表)*/
#define PALETTE_BUFF_CLEAR (0x80000000)    

/*LCD平台驱动结构体，平台驱动结构体定义在platform_device.h中，该结构体成员接口函数在第②步中实现*/
static struct platform_driver lcd_fb_driver = 
{
    .probe     = lcd_fb_probe,               /*FrameBuffer设备探测*/
    .remove    = __devexit_p(lcd_fb_remove), /*FrameBuffer设备移除*/
    .suspend   = lcd_fb_suspend,             /*FrameBuffer设备挂起*/
    .resume    = lcd_fb_resume,              /*FrameBuffer设备恢复*/
    .driver    = 
    {
        /*注意这里的名称一定要和系统中定义平台设备的地方一致，这样才能把平台设备与该平台设备的驱动关联起来*/
        .name = "s3c2410-lcd",
        .owner = THIS_MODULE,
    },
};

static int __init lcd_init(void)
{
    /*在Linux中，帧缓冲设备被看做是平台设备，所以这里注册平台设备*/
    return platform_driver_register(&lcd_fb_driver);
}

static void __exit lcd_exit(void)
{
    /*注销平台设备*/
    platform_driver_unregister(&lcd_fb_driver);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huang Gang");
MODULE_DESCRIPTION("My2440 LCD FrameBuffer Driver");
②、LCD平台设备各接口函数的实现：
/*LCD FrameBuffer设备探测的实现，注意这里使用一个__devinit宏，到lcd_fb_remove接口函数实现的地方讲解*/
static int __devinit lcd_fb_probe(struct platform_device *pdev)
{
    int i;
    int ret;
    struct resource *res;  /*用来保存从LCD平台设备中获取的LCD资源*/
    struct fb_info  *fbinfo; /*FrameBuffer驱动所对应的fb_info结构体*/
    struct s3c2410fb_mach_info *mach_info; /*保存从内核中获取的平台设备数据*/
    struct my2440fb_var *fbvar; /*上面定义的驱动程序全局变量结构体*/
    struct s3c2410fb_display *display; /*LCD屏的配置信息结构体，该结构体定义在mach-s3c2410/include/mach/fb.h中*/

    /*获取LCD硬件相关信息数据，在前面讲过内核使用s3c24xx_fb_set_platdata函数将LCD的硬件相关信息保存到
     了LCD平台数据中，所以这里我们就从平台数据中取出来在驱动中使用*/
    mach_info = pdev->dev.platform_data;
    if(mach_info == NULL)
    {
        /*判断获取数据是否成功*/
        dev_err(&pdev->dev, "no platform data for lcd/n");
        return -EINVAL;
    }

    /*获得在内核中定义的FrameBuffer平台设备的LCD配置信息结构体数据*/
    display = mach_info->displays + mach_info->default_display;
    /*给fb_info分配空间，大小为my2440fb_var结构的内存，framebuffer_alloc定义在fb.h中在fbsysfs.c中实现*/
    fbinfo = framebuffer_alloc(sizeof(struct my2440fb_var), &pdev->dev);
    if(!fbinfo)
    {
        dev_err(&pdev->dev, "framebuffer alloc of registers failed/n");
        ret = -ENOMEM;
        goto err_noirq;
    }
    platform_set_drvdata(pdev, fbinfo);/*重新将LCD平台设备数据设置为fbinfo，好在后面的一些函数中来使用*/

    /*这里的用途其实就是将fb_info的成员par(注意是一个void类型的指针)指向这里的私有变量结构体fbvar，
     目的是到其他接口函数中再取出fb_info的成员par，从而能继续使用这里的私有变量*/
    fbvar = fbinfo->par;
    fbvar->dev = &pdev->dev;

    /*在系统定义的LCD平台设备资源中获取LCD中断号,platform_get_irq定义在platform_device.h中*/
    fbvar->lcd_irq_no = platform_get_irq(pdev, 0);
    if(fbvar->lcd_irq_no < 0)
    {
        /*判断获取中断号是否成功*/
        dev_err(&pdev->dev, "no lcd irq for platform/n");
        return -ENOENT;
    }

    /*获取LCD平台设备所使用的IO端口资源，注意这个IORESOURCE_MEM标志和LCD平台设备定义中的一致*/
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(res == NULL)
    {
        /*判断获取资源是否成功*/
        dev_err(&pdev->dev, "failed to get memory region resource/n");
        return -ENOENT;
    }

    /*申请LCD IO端口所占用的IO空间(注意理解IO空间和内存空间的区别),request_mem_region定义在ioport.h中*/
    fbvar->lcd_mem = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
    if(fbvar->lcd_mem == NULL)
    {
        /*判断申请IO空间是否成功*/
        dev_err(&pdev->dev, "failed to reserve memory region/n");
        return -ENOENT;
    }

    /*将LCD的IO端口占用的这段IO空间映射到内存的虚拟地址，ioremap定义在io.h中
     注意：IO空间要映射后才能使用，以后对虚拟地址的操作就是对IO空间的操作*/
    fbvar->lcd_base = ioremap(res->start, res->end - res->start + 1);
    if(fbvar->lcd_base == NULL)
    {
        /*判断映射虚拟地址是否成功*/
        dev_err(&pdev->dev, "ioremap() of registers failed/n");
        ret = -EINVAL;
        goto err_nomem;
    }

    /*从平台时钟队列中获取LCD的时钟，这里为什么要取得这个时钟，从LCD屏的时序图上看，各种控制信号的延迟
     都跟LCD的时钟有关。系统的一些时钟定义在arch/arm/plat-s3c24xx/s3c2410-clock.c中*/
    fbvar->lcd_clock = clk_get(NULL, "lcd");
    if(!fbvar->lcd_clock)
    {
        /*判断获取时钟是否成功*/
        dev_err(&pdev->dev, "failed to find lcd clock source/n");
        ret = -ENOENT;
        goto err_nomap;
    }
    /*时钟获取后要使能后才可以使用，clk_enable定义在arch/arm/plat-s3c/clock.c中*/
    clk_enable(fbvar->lcd_clock);

    /*申请LCD中断服务，上面获取的中断号lcd_fb_irq，使用快速中断方式:IRQF_DISABLED
     中断服务程序为:lcd_fb_irq，将LCD平台设备pdev做参数传递过去了*/
    ret = request_irq(fbvar->lcd_irq_no, lcd_fb_irq, IRQF_DISABLED, pdev->name, fbvar);
    if(ret)
    {
        /*判断申请中断服务是否成功*/
        dev_err(&pdev->dev, "IRQ%d error %d/n", fbvar->lcd_irq_no, ret);
        ret = -EBUSY;
        goto err_noclk;
    }
    /*好了，以上是对要使用的资源进行了获取和设置。下面就开始初始化填充fb_info结构体*/

    /*首先初始化fb_info中代表LCD固定参数的结构体fb_fix_screeninfo*/
    /*像素值与显示内存的映射关系有5种，定义在fb.h中。现在采用FB_TYPE_PACKED_PIXELS方式，在该方式下，
    像素值与内存直接对应，比如在显示内存某单元写入一个"1"时，该单元对应的像素值也将是"1"，这使得应用层
    把显示内存映射到用户空间变得非常方便。Linux中当LCD为TFT屏时，显示驱动管理显示内存就是基于这种方式*/
    strcpy(fbinfo->fix.id, driver_name);/*字符串形式的标识符*/
    fbinfo->fix.type = FB_TYPE_PACKED_PIXELS;
    fbinfo->fix.type_aux = 0;/*以下这些根据fb_fix_screeninfo定义中的描述，当没有硬件是都设为0*/
    fbinfo->fix.xpanstep = 0;
    fbinfo->fix.ypanstep = 0;
    fbinfo->fix.ywrapstep= 0;
    fbinfo->fix.accel = FB_ACCEL_NONE;
    /*接着，再初始化fb_info中代表LCD可变参数的结构体fb_var_screeninfo*/
    fbinfo->var.nonstd          = 0;
    fbinfo->var.activate        = FB_ACTIVATE_NOW;
    fbinfo->var.accel_flags     = 0;
    fbinfo->var.vmode           = FB_VMODE_NONINTERLACED;
    fbinfo->var.xres            = display->xres;
    fbinfo->var.yres            = display->yres;
    fbinfo->var.bits_per_pixel  = display->bpp;
    /*指定对底层硬件操作的函数指针, 因内容较多故其定义在第③步中再讲*/
    fbinfo->fbops               = &my2440fb_ops;
    /*初始化色调色板(颜色表)为空*/
    for(i = 0; i < 256; i++)
    {
        fbvar->palette_buffer[i] = PALETTE_BUFF_CLEAR;
    }

    for (i = 0; i < mach_info->num_displays; i++) /*fb缓存的长度*/
    {
        /*计算FrameBuffer缓存的最大大小，这里右移3位(即除以8)是因为色位模式BPP是以位为单位*/
        unsigned long smem_len = (mach_info->displays[i].xres * mach_info->displays[i].yres * mach_info->displays[i].bpp) >> 3;

        if(fbinfo->fix.smem_len < smem_len)
        {
            fbinfo->fix.smem_len = smem_len;
        }
    }

    /*初始化LCD控制器之前要延迟一段时间*/
    msleep(1);

    /*初始化完fb_info后，开始对LCD各寄存器进行初始化，其定义在后面讲到*/
    my2440fb_init_registers(fbinfo);

    /*初始化完寄存器后，开始检查fb_info中的可变参数，其定义在后面讲到*/
    my2440fb_check_var(fbinfo);
    
    /*申请帧缓冲设备fb_info的显示缓冲区空间，其定义在后面讲到*/
    ret = my2440fb_map_video_memory(fbinfo);
    if (ret) 
    {
        dev_err(&pdev->dev, "failed to allocate video RAM: %d/n", ret);
        ret = -ENOMEM;
        goto err_nofb;
    }

    /*最后，注册这个帧缓冲设备fb_info到系统中, register_framebuffer定义在fb.h中在fbmem.c中实现*/
    ret = register_framebuffer(fbinfo);
    if (ret < 0) 
    {
        dev_err(&pdev->dev, "failed to register framebuffer device: %d/n", ret);
        goto err_video_nomem;
    }

    /*对设备文件系统的支持(对设备文件系统的理解请参阅：嵌入式Linux之我行——设备文件系统剖析与使用)
     创建frambuffer设备文件，device_create_file定义在linux/device.h中*/
    ret = device_create_file(&pdev->dev, &dev_attr_debug);
    if (ret) 
    {
        dev_err(&pdev->dev, "failed to add debug attribute/n");
    }

    return 0;

/*以下是上面错误处理的跳转点*/
err_nomem:
    release_resource(fbvar->lcd_mem);
    kfree(fbvar->lcd_mem);

err_nomap:
    iounmap(fbvar->lcd_base);

err_noclk:
    clk_disable(fbvar->lcd_clock);
    clk_put(fbvar->lcd_clock);

err_noirq:
    free_irq(fbvar->lcd_irq_no, fbvar);

err_nofb:
    platform_set_drvdata(pdev, NULL);
    framebuffer_release(fbinfo);

err_video_nomem:
    my2440fb_unmap_video_memory(fbinfo);

    return ret;
}

/*LCD中断服务程序*/
static irqreturn_t lcd_fb_irq(int irq, void *dev_id)
{
    struct my2440fb_var    *fbvar = dev_id;
    void __iomem *lcd_irq_base;
    unsigned long lcdirq;

    /*LCD中断挂起寄存器基地址*/
    lcd_irq_base = fbvar->lcd_base + S3C2410_LCDINTBASE;

    /*读取LCD中断挂起寄存器的值*/
    lcdirq = readl(lcd_irq_base + S3C24XX_LCDINTPND);

    /*判断是否为中断挂起状态*/
    if(lcdirq & S3C2410_LCDINT_FRSYNC)
    {
        /*填充调色板*/
        if (fbvar->palette_ready)
        {
            my2440fb_write_palette(fbvar);
        }

        /*设置帧已插入中断请求*/
        writel(S3C2410_LCDINT_FRSYNC, lcd_irq_base + S3C24XX_LCDINTPND);
        writel(S3C2410_LCDINT_FRSYNC, lcd_irq_base + S3C24XX_LCDSRCPND);
    }

    return IRQ_HANDLED;
}

/*填充调色板*/
static void my2440fb_write_palette(struct my2440fb_var *fbvar)
{
    unsigned int i;
    void __iomem *regs = fbvar->lcd_base;

    fbvar->palette_ready = 0;

    for (i = 0; i < 256; i++) 
    {
        unsigned long ent = fbvar->palette_buffer[i];

        if (ent == PALETTE_BUFF_CLEAR)
        {
            continue;
        }

        writel(ent, regs + S3C2410_TFTPAL(i));

        if (readw(regs + S3C2410_TFTPAL(i)) == ent)
        {
            fbvar->palette_buffer[i] = PALETTE_BUFF_CLEAR;
        }
        else
        {
            fbvar->palette_ready = 1;
        }
    }
}

/*LCD各寄存器进行初始化*/
static int my2440fb_init_registers(struct fb_info *fbinfo)
{
    unsigned long flags;
    void __iomem *tpal;
    void __iomem *lpcsel;

    /*从lcd_fb_probe探测函数设置的私有变量结构体中再获得LCD相关信息的数据*/
    struct my2440fb_var    *fbvar = fbinfo->par;
    struct s3c2410fb_mach_info *mach_info = fbvar->dev->platform_data;

    /*获得临时调色板寄存器基地址，S3C2410_TPAL宏定义在mach-s3c2410/include/mach/regs-lcd.h中。
    注意对于lpcsel这是一个针对三星TFT屏的一个专用寄存器，如果用的不是三星的TFT屏应该不用管它。*/
    tpal = fbvar->lcd_base + S3C2410_TPAL;
    lpcsel = fbvar->lcd_base + S3C2410_LPCSEL;

    /*在修改下面寄存器值之前先屏蔽中断，将中断状态保存到flags中*/
    local_irq_save(flags);

    /*这里就是在上一篇章中讲到的把IO端口C和D配置成LCD模式*/
    modify_gpio(S3C2410_GPCUP, mach_info->gpcup, mach_info->gpcup_mask);
    modify_gpio(S3C2410_GPCCON, mach_info->gpccon, mach_info->gpccon_mask);
    modify_gpio(S3C2410_GPDUP, mach_info->gpdup, mach_info->gpdup_mask);
    modify_gpio(S3C2410_GPDCON, mach_info->gpdcon, mach_info->gpdcon_mask);

    /*恢复被屏蔽的中断*/
    local_irq_restore(flags);

    writel(0x00, tpal);/*临时调色板寄存器使能禁止*/
    writel(mach_info->lpcsel, lpcsel);/*在上一篇中讲到过，它是三星TFT屏的一个寄存器，这里可以不管*/

    return 0;
}

/*该函数实现修改GPIO端口的值，注意第三个参数mask的作用是将要设置的寄存器值先清零*/
static inline void modify_gpio(void __iomem *reg, unsigned long set, unsigned long mask)
{
    unsigned long tmp;

    tmp = readl(reg) & ~mask;
    writel(tmp | set, reg);
}

/*检查fb_info中的可变参数*/
static int my2440fb_check_var(struct fb_info *fbinfo)
{
    unsigned i;

    /*从lcd_fb_probe探测函数设置的平台数据中再获得LCD相关信息的数据*/
    struct fb_var_screeninfo *var = &fbinfo->var;/*fb_info中的可变参数*/
    struct my2440fb_var    *fbvar = fbinfo->par;/*在lcd_fb_probe探测函数中设置的私有结构体数据*/
    struct s3c2410fb_mach_info *mach_info = fbvar->dev->platform_data;/*LCD的配置结构体数据，这个配置结构体的赋值在上一篇章的"3. 帧缓冲设备作为平台设备"中*/

    struct s3c2410fb_display *display = NULL;
    struct s3c2410fb_display *default_display = mach_info->displays + mach_info->default_display;
    int type = default_display->type;/*LCD的类型，看上一篇章的"3. 帧缓冲设备作为平台设备"中的type赋值是TFT类型*/

    /*验证X/Y解析度*/
    if (var->yres == default_display->yres && 
        var->xres == default_display->xres && 
        var->bits_per_pixel == default_display->bpp)
    {
        display = default_display;
    }
    else
    {
        for (i = 0; i < mach_info->num_displays; i++)
        {
            if (type == mach_info->displays[i].type &&
             var->yres == mach_info->displays[i].yres &&
             var->xres == mach_info->displays[i].xres &&
             var->bits_per_pixel == mach_info->displays[i].bpp) 
            {
                display = mach_info->displays + i;
                break;
            }
        }
    }

    if (!display) 
    {
        return -EINVAL;
    }

    /*配置LCD配置寄存器1中的5-6位(配置成TFT类型)和配置LCD配置寄存器5*/
    fbvar->regs.lcdcon1 = display->type;
    fbvar->regs.lcdcon5 = display->lcdcon5;

    /* 设置屏幕的虚拟解析像素和高度宽度 */
    var->xres_virtual = display->xres;
    var->yres_virtual = display->yres;
    var->height = display->height;
    var->width = display->width;

    /* 设置时钟像素，行、帧切换值，水平同步、垂直同步长度值 */
    var->pixclock = display->pixclock;
    var->left_margin = display->left_margin;
    var->right_margin = display->right_margin;
    var->upper_margin = display->upper_margin;
    var->lower_margin = display->lower_margin;
    var->vsync_len = display->vsync_len;
    var->hsync_len = display->hsync_len;

    /*设置透明度*/
    var->transp.offset = 0;
    var->transp.length = 0;

    /*根据色位模式(BPP)来设置可变参数中R、G、B的颜色位域。对于这些参数值的设置请参考CPU数据
    手册中"显示缓冲区与显示点对应关系图"，例如在上一篇章中我就画出了8BPP和16BPP时的对应关系图*/
    switch (var->bits_per_pixel) 
    {
        case 1:
        case 2:
        case 4:
            var->red.offset  = 0;
            var->red.length  = var->bits_per_pixel;
            var->green       = var->red;
            var->blue        = var->red;
            break;
        case 8:/* 8 bpp 332 */
            if (display->type != S3C2410_LCDCON1_TFT) 
            {
                var->red.length     = 3;
                var->red.offset     = 5;
                var->green.length   = 3;
                var->green.offset   = 2;
                var->blue.length    = 2;
                var->blue.offset    = 0;
            }else{
                var->red.offset     = 0;
                var->red.length     = 8;
                var->green          = var->red;
                var->blue           = var->red;
            }
            break;
        case 12:/* 12 bpp 444 */
            var->red.length         = 4;
            var->red.offset         = 8;
            var->green.length       = 4;
            var->green.offset       = 4;
            var->blue.length        = 4;
            var->blue.offset        = 0;
            break;
        case 16:/* 16 bpp */
            if (display->lcdcon5 & S3C2410_LCDCON5_FRM565) 
            {
                /* 565 format */
                var->red.offset      = 11;
                var->green.offset    = 5;
                var->blue.offset     = 0;
                var->red.length      = 5;
                var->green.length    = 6;
                var->blue.length     = 5;
            } else {
                /* 5551 format */
                var->red.offset      = 11;
                var->green.offset    = 6;
                var->blue.offset     = 1;
                var->red.length      = 5;
                var->green.length    = 5;
                var->blue.length     = 5;
            }
            break;
        case 32:/* 24 bpp 888 and 8 dummy */
            var->red.length        = 8;
            var->red.offset        = 16;
            var->green.length      = 8;
            var->green.offset      = 8;
            var->blue.length       = 8;
            var->blue.offset       = 0;
            break;
    }

    return 0;
}

/*申请帧缓冲设备fb_info的显示缓冲区空间*/
static int __init my2440fb_map_video_memory(struct fb_info *fbinfo)
{
    dma_addr_t map_dma;/*用于保存DMA缓冲区总线地址*/
    struct my2440fb_var    *fbvar = fbinfo->par;/*获得在lcd_fb_probe探测函数中设置的私有结构体数据*/
    unsigned map_size = PAGE_ALIGN(fbinfo->fix.smem_len);/*获得FrameBuffer缓存的大小, PAGE_ALIGN定义在mm.h中*/

    /*将分配的一个写合并DMA缓存区设置为LCD屏幕的虚拟地址(对于DMA请参考DMA相关知识)
    dma_alloc_writecombine定义在arch/arm/mm/dma-mapping.c中*/
    fbinfo->screen_base = dma_alloc_writecombine(fbvar->dev, map_size, &map_dma, GFP_KERNEL);

    if (fbinfo->screen_base) 
    {
        /*设置这片DMA缓存区的内容为空*/
        memset(fbinfo->screen_base, 0x00, map_size);

        /*将DMA缓冲区总线地址设成fb_info不可变参数中framebuffer缓存的开始位置*/
        fbinfo->fix.smem_start = map_dma;
    }

    return fbinfo->screen_base ? 0 : -ENOMEM;
}

/*释放帧缓冲设备fb_info的显示缓冲区空间*/
static inline void my2440fb_unmap_video_memory(struct fb_info *fbinfo)
{
    struct my2440fb_var    *fbvar = fbinfo->par;
    unsigned map_size = PAGE_ALIGN(fbinfo->fix.smem_len);

    /*跟申请DMA的地方想对应*/
    dma_free_writecombine(fbvar->dev, map_size, fbinfo->screen_base, fbinfo->fix.smem_start);
}


/*LCD FrameBuffer设备移除的实现，注意这里使用一个__devexit宏，和lcd_fb_probe接口函数相对应。
  在Linux内核中，使用了大量不同的宏来标记具有不同作用的函数和数据结构，这些宏在include/linux/init.h 
  头文件中定义，编译器通过这些宏可以把代码优化放到合适的内存位置，以减少内存占用和提高内核效率。
  __devinit、__devexit就是这些宏之一，在probe()和remove()函数中应该使用__devinit和__devexit宏。
  又当remove()函数使用了__devexit宏时，则在驱动结构体中一定要使用__devexit_p宏来引用remove()，
  所以在第①步中就用__devexit_p来引用lcd_fb_remove接口函数。*/
static int __devexit lcd_fb_remove(struct platform_device *pdev)
{
    struct fb_info *fbinfo = platform_get_drvdata(pdev);
    struct my2440fb_var    *fbvar = fbinfo->par;

    /*从系统中注销帧缓冲设备*/
    unregister_framebuffer(fbinfo);

    /*停止LCD控制器的工作*/
    my2440fb_lcd_enable(fbvar, 0);

    /*延迟一段时间，因为停止LCD控制器需要一点时间 */
    msleep(1);

    /*释放帧缓冲设备fb_info的显示缓冲区空间*/
    my2440fb_unmap_video_memory(fbinfo);

    /*将LCD平台数据清空和释放fb_info空间资源*/
    platform_set_drvdata(pdev, NULL);
    framebuffer_release(fbinfo);

    /*释放中断资源*/
    free_irq(fbvar->lcd_irq_no, fbvar);

    /*释放时钟资源*/
    if (fbvar->lcd_clock) 
    {
        clk_disable(fbvar->lcd_clock);
        clk_put(fbvar->lcd_clock);
        fbvar->lcd_clock = NULL;
    }

    /*释放LCD IO空间映射的虚拟内存空间*/
    iounmap(fbvar->lcd_base);

    /*释放申请的LCD IO端口所占用的IO空间*/
    release_resource(fbvar->lcd_mem);
    kfree(fbvar->lcd_mem);

    return 0;
}

/*停止LCD控制器的工作*/
static void my2440fb_lcd_enable(struct my2440fb_var *fbvar, int enable)
{
    unsigned long flags;

    /*在修改下面寄存器值之前先屏蔽中断，将中断状态保存到flags中*/
    local_irq_save(flags);

    if (enable)
    {
        fbvar->regs.lcdcon1 |= S3C2410_LCDCON1_ENVID;
    }
    else
    {
        fbvar->regs.lcdcon1 &= ~S3C2410_LCDCON1_ENVID;
    }

    writel(fbvar->regs.lcdcon1, fbvar->lcd_base + S3C2410_LCDCON1);

    /*恢复被屏蔽的中断*/
    local_irq_restore(flags);
}

/*对LCD FrameBuffer平台设备驱动电源管理的支持,CONFIG_PM这个宏定义在内核中*/
#ifdef CONFIG_PM
/*当配置内核时选上电源管理，则平台设备的驱动就支持挂起和恢复功能*/
static int lcd_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
    /*挂起LCD设备，注意这里挂起LCD时并没有保存LCD控制器的各种状态，所以在恢复后LCD不会继续显示挂起前的内容
     若要继续显示挂起前的内容，则要在这里保存LCD控制器的各种状态，这里就不讲这个了，以后讲到电源管理再讲*/
    struct fb_info *fbinfo = platform_get_drvdata(pdev);
    struct my2440fb_var    *fbvar = fbinfo->par;

    /*停止LCD控制器的工作*/
    my2440fb_lcd_enable(fbvar, 0);

    msleep(1);

    /*停止时钟*/
    clk_disable(fbvar->lcd_clock);

    return 0;
}

static int lcd_fb_resume(struct platform_device *pdev)
{
    /*恢复挂起的LCD设备*/
    struct fb_info *fbinfo = platform_get_drvdata(pdev);
    struct my2440fb_var    *fbvar = fbinfo->par;

    /*开启时钟*/
    clk_enable(fbvar->lcd_clock);

    /*初始化LCD控制器之前要延迟一段时间*/
    msleep(1);

    /*恢复时重新初始化LCD各寄存器*/
    my2440fb_init_registers(fbinfo);

    /*重新激活fb_info中所有的参数配置，该函数定义在第③步中再讲*/
    my2440fb_activate_var(fbinfo);

    /*正与挂起时讲到的那样，因为没保存挂起时LCD控制器的各种状态，
    所以恢复后就让LCD显示空白，该函数定义也在第③步中再讲*/
    my2440fb_blank(FB_BLANK_UNBLANK, fbinfo);

    return 0;
}
#else
/*如果配置内核时没选上电源管理,则平台设备的驱动就不支持挂起和恢复功能,这两个函数也就无需实现了*/
#define lcd_fb_suspend    NULL
#define lcd_fb_resume    NULL
#endif
    fbinfo->flags               = FBINFO_FLAG_DEFAULT;
    fbinfo->pseudo_palette      = &fbvar->pseudo_pal;
 
③、帧缓冲设备驱动对底层硬件操作的函数接口实现(即：my2440fb_ops的实现)：
/*Framebuffer底层硬件操作各接口函数*/
static struct fb_ops my2440fb_ops = 
{
    .owner          = THIS_MODULE,
    .fb_check_var   = my2440fb_check_var,/*第②步中已实现*/
    .fb_set_par     = my2440fb_set_par,/*设置fb_info中的参数，主要是LCD的显示模式*/
    .fb_blank       = my2440fb_blank,/*显示空白(即：LCD开关控制)*/
    .fb_setcolreg   = my2440fb_setcolreg,/*设置颜色表*/
    /*以下三个函数是可选的，主要是提供fb_console的支持，在内核中已经实现，这里直接调用即可*/
    .fb_fillrect    = cfb_fillrect,/*定义在drivers/video/cfbfillrect.c中*/
    .fb_copyarea    = cfb_copyarea,/*定义在drivers/video/cfbcopyarea.c中*/
    .fb_imageblit   = cfb_imageblit,/*定义在drivers/video/cfbimgblt.c中*/
};

/*设置fb_info中的参数，这里根据用户设置的可变参数var调整固定参数fix*/
static int my2440fb_set_par(struct fb_info *fbinfo)
{
    /*获得fb_info中的可变参数*/
    struct fb_var_screeninfo *var = &fbinfo->var;

    /*判断可变参数中的色位模式，根据色位模式来设置色彩模式*/
    switch (var->bits_per_pixel) 
    {
        case 32:
        case 16:
        case 12:/*12BPP时，设置为真彩色(分成红、绿、蓝三基色)*/
            fbinfo->fix.visual = FB_VISUAL_TRUECOLOR;
            break;
        case 1:/*1BPP时，设置为黑白色(分黑、白两种色，FB_VISUAL_MONO01代表黑，FB_VISUAL_MONO10代表白)*/
            fbinfo->fix.visual = FB_VISUAL_MONO01;
            break;
        default:/*默认设置为伪彩色，采用索引颜色显示*/
            fbinfo->fix.visual = FB_VISUAL_PSEUDOCOLOR;
            break;
    }

    /*设置fb_info中固定参数中一行的字节数，公式：1行字节数=(1行像素个数*每像素位数BPP)/8 */
    fbinfo->fix.line_length = (var->xres_virtual * var->bits_per_pixel) / 8;

    /*修改以上参数后，重新激活fb_info中的参数配置(即：使修改后的参数在硬件上生效)*/
    my2440fb_activate_var(fbinfo);

    return 0;
}

/*重新激活fb_info中的参数配置*/
static void my2440fb_activate_var(struct fb_info *fbinfo)
{
    /*获得结构体变量*/
    struct my2440fb_var *fbvar = fbinfo->par;
    void __iomem *regs = fbvar->lcd_base;

    /*获得fb_info可变参数*/
    struct fb_var_screeninfo *var = &fbinfo->var;

    /*计算LCD控制寄存器1中的CLKVAL值, 根据数据手册中该寄存器的描述，计算公式如下：
    * STN屏：VCLK = HCLK / (CLKVAL * 2), CLKVAL要求>= 2
    * TFT屏：VCLK = HCLK / [(CLKVAL + 1) * 2], CLKVAL要求>= 0*/
    int clkdiv = my2440fb_calc_pixclk(fbvar, var->pixclock) / 2;

    /*获得屏幕的类型*/
    int type = fbvar->regs.lcdcon1 & S3C2410_LCDCON1_TFT;

    if (type == S3C2410_LCDCON1_TFT) 
    {
        /*根据数据手册按照TFT屏的要求配置LCD控制寄存器1-5*/
        my2440fb_config_tft_lcd_regs(fbinfo, &fbvar->regs);

        --clkdiv;

        if (clkdiv < 0)
        {
            clkdiv = 0;
        }
    } 
    else 
    {
        /*根据数据手册按照STN屏的要求配置LCD控制寄存器1-5*/
        my2440fb_config_stn_lcd_regs(fbinfo, &fbvar->regs);

        if (clkdiv < 2)
        {
            clkdiv = 2;
        }
    }

    /*设置计算的LCD控制寄存器1中的CLKVAL值*/
    fbvar->regs.lcdcon1 |= S3C2410_LCDCON1_CLKVAL(clkdiv);

    /*将各参数值写入LCD控制寄存器1-5中*/
    writel(fbvar->regs.lcdcon1 & ~S3C2410_LCDCON1_ENVID, regs + S3C2410_LCDCON1);
    writel(fbvar->regs.lcdcon2, regs + S3C2410_LCDCON2);
    writel(fbvar->regs.lcdcon3, regs + S3C2410_LCDCON3);
    writel(fbvar->regs.lcdcon4, regs + S3C2410_LCDCON4);
    writel(fbvar->regs.lcdcon5, regs + S3C2410_LCDCON5);

    /*配置帧缓冲起始地址寄存器1-3*/
    my2440fb_set_lcdaddr(fbinfo);

    fbvar->regs.lcdcon1 |= S3C2410_LCDCON1_ENVID,
    writel(fbvar->regs.lcdcon1, regs + S3C2410_LCDCON1);
}

/*计算LCD控制寄存器1中的CLKVAL值*/
static unsigned int my2440fb_calc_pixclk(struct my2440fb_var *fbvar, unsigned long pixclk)
{
    /*获得LCD的时钟*/
    unsigned long clk = clk_get_rate(fbvar->lcd_clock);

    /* 像素时钟单位是皮秒，而时钟的单位是赫兹，所以计算公式为：
     * Hz -> picoseconds is / 10^-12
     */
    unsigned long long div = (unsigned long long)clk * pixclk;

    div >>= 12;            /* div / 2^12 */
    do_div(div, 625 * 625UL * 625); /* div / 5^12, do_div宏定义在asm/div64.h中*/

    return div;
}

/*根据数据手册按照TFT屏的要求配置LCD控制寄存器1-5*/
static void my2440fb_config_tft_lcd_regs(const struct fb_info *fbinfo, struct s3c2410fb_hw *regs)
{
    const struct my2440fb_var *fbvar = fbinfo->par;
    const struct fb_var_screeninfo *var = &fbinfo->var;

    /*根据色位模式设置LCD控制寄存器1和5，参考数据手册*/
    switch (var->bits_per_pixel) 
    {
        case 1:/*1BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT1BPP;
            break;
        case 2:/*2BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT2BPP;
            break;
        case 4:/*4BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT4BPP;
            break;
        case 8:/*8BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT8BPP;
            regs->lcdcon5 |= S3C2410_LCDCON5_BSWP | S3C2410_LCDCON5_FRM565;
            regs->lcdcon5 &= ~S3C2410_LCDCON5_HWSWP;
            break;
        case 16:/*16BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT16BPP;
            regs->lcdcon5 &= ~S3C2410_LCDCON5_BSWP;
            regs->lcdcon5 |= S3C2410_LCDCON5_HWSWP;
            break;
        case 32:/*32BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_TFT24BPP;
            regs->lcdcon5 &= ~(S3C2410_LCDCON5_BSWP | S3C2410_LCDCON5_HWSWP | S3C2410_LCDCON5_BPP24BL);
            break;
        default:/*无效的BPP*/
            dev_err(fbvar->dev, "invalid bpp %d/n", var->bits_per_pixel);
    }

    /*设置LCD配置寄存器2、3、4*/
    regs->lcdcon2 = S3C2410_LCDCON2_LINEVAL(var->yres - 1) |
            S3C2410_LCDCON2_VBPD(var->upper_margin - 1) |
            S3C2410_LCDCON2_VFPD(var->lower_margin - 1) |
            S3C2410_LCDCON2_VSPW(var->vsync_len - 1);

    regs->lcdcon3 = S3C2410_LCDCON3_HBPD(var->right_margin - 1) |
            S3C2410_LCDCON3_HFPD(var->left_margin - 1) |
            S3C2410_LCDCON3_HOZVAL(var->xres - 1);

    regs->lcdcon4 = S3C2410_LCDCON4_HSPW(var->hsync_len - 1);
}

/*根据数据手册按照STN屏的要求配置LCD控制寄存器1-5*/
static void my2440fb_config_stn_lcd_regs(const struct fb_info *fbinfo, struct s3c2410fb_hw *regs)
{
    const struct my2440fb_var    *fbvar = fbinfo->par;
    const struct fb_var_screeninfo *var = &fbinfo->var;

    int type = regs->lcdcon1 & ~S3C2410_LCDCON1_TFT;
    int hs = var->xres >> 2;
    unsigned wdly = (var->left_margin >> 4) - 1;
    unsigned wlh = (var->hsync_len >> 4) - 1;

    if (type != S3C2410_LCDCON1_STN4)
    {
        hs >>= 1;
    }

    /*根据色位模式设置LCD控制寄存器1，参考数据手册*/
    switch (var->bits_per_pixel) 
    {
        case 1:/*1BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_STN1BPP;
            break;
        case 2:/*2BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_STN2GREY;
            break;
        case 4:/*4BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_STN4GREY;
            break;
        case 8:/*8BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_STN8BPP;
            hs *= 3;
            break;
        case 12:/*12BPP*/
            regs->lcdcon1 |= S3C2410_LCDCON1_STN12BPP;
            hs *= 3;
            break;
        default:/*无效的BPP*/
            dev_err(fbvar->dev, "invalid bpp %d/n", var->bits_per_pixel);
    }
    
    /*设置LCD配置寄存器2、3、4, 参考数据手册*/
    if (wdly > 3) wdly = 3;
    if (wlh > 3) wlh = 3;
    regs->lcdcon2 = S3C2410_LCDCON2_LINEVAL(var->yres - 1);

    regs->lcdcon3 = S3C2410_LCDCON3_WDLY(wdly) |
            S3C2410_LCDCON3_LINEBLANK(var->right_margin / 8) |
            S3C2410_LCDCON3_HOZVAL(hs - 1);

    regs->lcdcon4 = S3C2410_LCDCON4_WLH(wlh);
}

/*配置帧缓冲起始地址寄存器1-3，参考数据手册*/
static void my2440fb_set_lcdaddr(struct fb_info *fbinfo)
{
    unsigned long saddr1, saddr2, saddr3;
    struct my2440fb_var *fbvar = fbinfo->par;
    void __iomem *regs = fbvar->lcd_base;

    saddr1 = fbinfo->fix.smem_start >> 1;
    saddr2 = fbinfo->fix.smem_start;
    saddr2 += fbinfo->fix.line_length * fbinfo->var.yres;
    saddr2 >>= 1;
    saddr3 = S3C2410_OFFSIZE(0) | S3C2410_PAGEWIDTH((fbinfo->fix.line_length / 2) & 0x3ff);

    writel(saddr1, regs + S3C2410_LCDSADDR1);
    writel(saddr2, regs + S3C2410_LCDSADDR2);
    writel(saddr3, regs + S3C2410_LCDSADDR3);
}

/*显示空白，blank mode有5种模式，定义在fb.h中，是一个枚举*/
static int my2440fb_blank(int blank_mode, struct fb_info *fbinfo)
{
    struct my2440fb_var *fbvar = fbinfo->par;
    void __iomem *regs = fbvar->lcd_base;

    /*根据显示空白的模式来设置LCD是开启还是停止*/
    if (blank_mode == FB_BLANK_POWERDOWN) 
    {
        my2440fb_lcd_enable(fbvar, 0);/*在第②步中定义*/
    } 
    else 
    {
        my2440fb_lcd_enable(fbvar, 1);/*在第②步中定义*/
    }

    /*根据显示空白的模式来控制临时调色板寄存器*/
    if (blank_mode == FB_BLANK_UNBLANK)
    {
        /*临时调色板寄存器无效*/
        writel(0x0, regs + S3C2410_TPAL);
    }
    else 
    {
        /*临时调色板寄存器有效*/
        writel(S3C2410_TPAL_EN, regs + S3C2410_TPAL);
    }

    return 0;
}

/*设置颜色表*/
static int my2440fb_setcolreg(unsigned regno,unsigned red,unsigned green,unsigned blue,unsigned transp,struct fb_info *fbinfo)
{
    unsigned int val;
    struct my2440fb_var *fbvar = fbinfo->par;
    void __iomem *regs = fbvar->lcd_base;

    switch (fbinfo->fix.visual) 
    {
        case FB_VISUAL_TRUECOLOR:
            /*真彩色*/
            if (regno < 16) 
            {
                u32 *pal = fbinfo->pseudo_palette;

                val = chan_to_field(red, &fbinfo->var.red);
                val |= chan_to_field(green, &fbinfo->var.green);
                val |= chan_to_field(blue, &fbinfo->var.blue);

                pal[regno] = val;
            }
            break;
        case FB_VISUAL_PSEUDOCOLOR:
            /*伪彩色*/
            if (regno < 256) 
            {
                val = (red >> 0) & 0xf800;
                val |= (green >> 5) & 0x07e0;
                val |= (blue >> 11) & 0x001f;

                writel(val, regs + S3C2410_TFTPAL(regno));

                /*修改调色板*/
                schedule_palette_update(fbvar, regno, val);
            }
            break;
        default:
            return 1;
    }

    return 0;
}

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

/*修改调色板*/
static void schedule_palette_update(struct my2440fb_var    *fbvar, unsigned int regno, unsigned int val)
{
    unsigned long flags;
    unsigned long irqen;

    /*LCD中断挂起寄存器基地址*/
    void __iomem *lcd_irq_base = fbvar->lcd_base + S3C2410_LCDINTBASE;

    /*在修改中断寄存器值之前先屏蔽中断，将中断状态保存到flags中*/
    local_irq_save(flags);

    fbvar->palette_buffer[regno] = val;

    /*判断调色板是否准备就像*/
    if (!fbvar->palette_ready) 
    {
        fbvar->palette_ready = 1;

        /*使能中断屏蔽寄存器*/
        irqen = readl(lcd_irq_base + S3C24XX_LCDINTMSK);
        irqen &= ~S3C2410_LCDINT_FRSYNC;
        writel(irqen, lcd_irq_base + S3C24XX_LCDINTMSK);
    }

    /*恢复被屏蔽的中断*/
    local_irq_restore(flags);
}
 