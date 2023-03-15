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
#define SLAVE_ADDRESS 0x18

static struct i2c_client *client;
static int i2cread_regs(struct i2c_client *client);
static int i2crdreg_open(struct inode *inode, struct file *file)
{
	return 0;
} 

static int i2crdreg_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t i2crdreg_read(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t i2crdreg_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *ppos)
{
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
static int i2cread_regs(struct i2c_client *client)
{
	int ret;
	u8 buf[1];
	u8 chip_id = 0;
	//client->addr = 0x18;
	u16 reg = 0xf0;
	u32 paddr = 0x0c;
	struct i2c_msg msg[2];
	u8 addr;
	
	addr = paddr&0xff;
    buf[0] = reg;
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
	printk(KERN_CRIT"\n default regadd: %0#x---data is %0#x", reg, chip_id);
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
	printk(KERN_CRIT"This is rdreg_remove\n");
	return 0;
}

/*i2c驱动的probe函数*/
static int rdreg_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
   	int ret;
	
    printk(KERN_CRIT"%s\n", __func__);
    i2cread_regs(client);
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
	printk(KERN_CRIT"This is rdreg_driver_init \n");
	return 0;
}

/*驱动出口函数*/
static void rdreg_driver_exit(void)
{
	//misc_deregister(&misc);
	//将前面注册的i2c_driver 也从linux内核中注销掉
	i2c_del_driver(&rdreg_driver);
	printk(KERN_CRIT"This is rdreg_driver_exit \n");
}

module_init(rdreg_driver_init);
module_exit(rdreg_driver_exit);
MODULE_LICENSE("GPL");


