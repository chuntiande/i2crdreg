#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
//#include <linux/stdlib.h>
//#include <linux/stdio.h>
#include <linux/delay.h>

#define IIC_WR  0
#define IIC_RD  1
#define DEVICE_NAME "i2crdreg"
#define SLAVE_ADDRESS 0x0c

static struct i2c_client *client;
static int i2cread_regs_8(struct i2c_client *client);
static int i2cwrite_regs_8(struct i2c_client *client);
static int i2cset_passthrough(struct i2c_client *client)

static int i2crdreg_open(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
} 

static int i2crdreg_close(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static ssize_t i2crdreg_read(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static ssize_t i2crdreg_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

static struct file_operations i2crdreg_fops = {
	.owner   =   THIS_MODULE,
	.open    =   i2crdreg_open,
	.release =   i2crdreg_close,
	.write   =   i2crdreg_write,
	.read    =   i2crdreg_read
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &i2crdreg_fops
};

//读寄存器函数
static int i2cread_regs_8(struct i2c_client *client)
{
	int ret = 0;
	u8 buf[1];
	u8 chip_id = 0;
	int i = 0;
	u8 reg[] = {0x0d, 0x0e, 0x0f, 0x10, 0x11};
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg[2];
	u8 addr;
	addr = paddr&0xff;

	printk(KERN_CRIT"This is %s \n", __func__);

	for(i = 0; i < 5; i++)
	{
		buf[0] = reg[i];
		msg[0].addr  = addr;
		msg[0].flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
		msg[0].buf   = buf;             //寄存器地址
		msg[0].len   = sizeof(buf);     //len为buf的大小，单位是字节

		msg[1].addr  = addr;
		msg[1].flags = client->flags | IIC_RD;   //flags为1，是读，表示buf是我们要接收的数据
		msg[1].buf   = buf;          //寄存器的值
		msg[1].len   = 1;
		ret = i2c_transfer(client->adapter, msg, 2);
		if(ret < 0)
		{
			printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
			return ret;
		}
		chip_id = buf[0];
		printk(KERN_CRIT"\n default regadd: %02x---chip_id is %02x", reg[i], chip_id);
	}
	return 0;
}

//写寄存器函数:将GPIO输出高电平
static int i2cwrite_regs_8(struct i2c_client *client)
{
	int ret;
	int i = 0;
	u8 buf[2];
	u8 chip_id = 0;
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg;
	u8 addr;
	//GPIO0-3,GPIO5-8相应的寄存器地址，增加使能IIC的寄存器地址
	u8 addr_arr[] = {0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x03};
	//GPIO0-3,GPIO5-8的值设置为输出且拉高，增加使能IIC的寄存器值
	u8 val_arr[]  = {0x2A, 0x99, 0x09, 0x99, 0x99, 0xDA}; 
	addr = paddr&0xff;
	printk(KERN_CRIT"This is %s \n", __func__);

	for(i = 0; i < 6; i++)
	{
		u8 reg = addr_arr[i];
		u8 val = val_arr[i];
		
    	buf[0] = reg;
		buf[1] = val;
		msg.addr  = addr;
		msg.flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
		msg.buf   = buf;             //寄存器地址和要写的地址
		msg.len   = sizeof(buf);     //len为buf的大小，单位是字节

		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret < 0)
		{
			printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
			return ret;
		}
		printk(KERN_CRIT"default regadd: 0xf%d---val is %02x", i, val);
	}

	return 0;
}

//写寄存器函数：设置IIC直通使能
static int i2cset_passthrough(struct i2c_client *client)
{
	int ret;
	int i = 0;
	u8 buf[2];
	u8 chip_id = 0;
	//client->addr = 0x0c;
	u32 paddr = 0x0c;
	struct i2c_msg msg;
	u8 addr;
	//设置IIC直通使能相应的寄存器地址
	u8 reg = 0x03;
	//设置相应寄存器的IIC直通功能使能
	u8 val = 0xDA;
	addr = paddr&0xff;
	printk(KERN_CRIT"This is %s \n", __func__);
    buf[0] = reg;
	buf[1] = val;
	msg.addr  = addr;
	msg.flags = client->flags;   //flags为0，是写，表示buf是我们要发送的数据
	msg.buf   = buf;             //寄存器地址和要写的地址
	msg.len   = sizeof(buf);     //len为buf的大小，单位是字节

	ret = i2c_transfer(client->adapter, &msg, 1);
	if(ret < 0)
	{
		printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
		return ret;
	}
	printk(KERN_CRIT"default regadd: %02x---val is %02x", reg, val);
	return 0;
}

//与设备树的compatible相匹配
static const struct of_device_id rdreg_of_match[] = {
	{.compatible = "hsae, rdreg-i2c", 0},
	{}
};

//无设备树的时候匹配ID表
static const struct i2c_device_id rdreg_id[] = {
	{"rdreg-i2c", 0}
};

/*i2c驱动的remove函数*/
static int rdreg_remove(struct i2c_client *i2c_client, const struct i2c_device_id *id)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

/*i2c驱动的probe函数*/
static int rdreg_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
   	int ret;
	
    printk(KERN_CRIT"This is %s \n", __func__);
	//将GPIO0-3,GPIO5-8设置输出模式并拉高
	i2cwrite_regs_8(client);
	//读取GPIO0-3,GPIO5-8相应寄存器的值并打印
    i2cread_regs_8(client);
    return 0;
}
//定义一个i2c_driver结构体
static struct i2c_driver rdreg_driver = {
    	.driver = {
        	.owner = THIS_MODULE,
        	.name  = "i2crdreg",
        	//采样设备树的时候驱动使用的匹配表
        	.of_match_table = rdreg_of_match,
    	},
    	.probe    = rdreg_probe,
    	.remove   = rdreg_remove,
    	.id_table = rdreg_id
};

/*驱动入口函数*/
static int rdreg_driver_init(void)
{
    	int ret;
    	//注册i2c_driver
    	ret = i2c_add_driver(&rdreg_driver);
    	if(ret < 0) {
        	printk(KERN_CRIT"i2c_add_driver is error \n");
			return ret;
		}
	printk(KERN_CRIT"This is %s \n", __func__);
	return 0;
}

/*驱动出口函数*/
static void rdreg_driver_exit(void)
{
	//misc_deregister(&misc);
	//将前面注册的i2c_driver 也从linux内核中注销掉
	i2c_del_driver(&rdreg_driver);
	printk(KERN_CRIT"This is %s \n", __func__);
}

module_init(rdreg_driver_init);
module_exit(rdreg_driver_exit);
MODULE_LICENSE("GPL");


