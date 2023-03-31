#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
//#include <linux/stdlib.h>
//#include <linux/stdio.h>
#include <linux/delay.h>
//API for device_node
#include <linux/fs.h>
//API for libgpio
#include <linux/gpio.h>
//API for device tree
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/string.h>
//API for ioctl
#include <linux/ioctl.h>
//API for malloc
#include <linux/slab.h>
#include <linux/types.h>
//API for copy_to_user copy_from_user
#include <linux/uaccess.h>
//API for pthread_create
#include <linux/pthread.h>

#define IIC_WR  0
#define IIC_RD  1

#define DEVICE_NAME "i2crdreg"
#define CMD_I2CRD _IO('R', 1)
#define CMD_I2CWR _IO('W', 0)

static u32 *dtsaddr;
static u8 lock_status = 0;
static struct i2c_client *client;
static struct wr_poweron{
	u8 reg;
	u8 val;
	u8 delay_time;
};

static struct rd_poweron{
	u8 reg;
	u8 val;
};
//设置相应地址与值
struct wr_poweron para_write[] = {
	{0x03, 0xDA, 0}, //设置IIC直通使能
	{0x01, 0x02, 0}, //设置开机使用寄存器复位芯片
	{0x5B, 0x23, 0}, //设置Dual双通道模式并设置为双绞线模式
	{0x0d, 0x23, 0}, //GPIO0-3,GPIO5-8设置为输入模式
	{0x0e, 0x33, 0},
	{0x0f, 0x03, 0},
	{0x10, 0x33, 0},
	{0x11, 0x33, 0}
};

//读取相应地址上的值
struct rd_poweron para_read[] = {
	{0x4f, 0}, //判断OLDI接口是单像素还是双像素模式
	{0x5f, 0}, //读取OpenLDI输入视频频率
	{0x1c, 0}, //读取GPIO0-3,GPIO5-8状态位是否为输入模式
	{0x1d, 0},
	{0x0c, 0}, //读取link状态位检测连接是否断开
	{0xf0, 0}, //读取芯片id
	{0xf1, 0},
	{0xf2, 0},
	{0xf3, 0},
	{0xf4, 0},
	{0xf5, 0}
};

static struct wr_poweron write = {0, 0, 0};
static struct rd_poweron read  = {0, 0};

static int i2cwrite_regs_8(struct i2c_client *client, struct wr_poweron *wr);
static int i2cread_regs_8(struct i2c_client *client, struct rd_poweron *rd);
static int i2cwrite_regs_16(struct i2c_client *client, struct wr_poweron *wr);
static int i2cread_regs_16(struct i2c_client *client, struct rd_poweron *rd);

static int set_iis_diable(struct i2c_client *client);
static int set_pdb_enable(struct i2c_client *client);
static int powerup(struct i2c_client *client);
static int read_status(struct i2c_client *client);
static int i2cdelay(struct wr_poweron *wr);

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

static long i2crdreg_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_CRIT"This is %s \n", __func__);
	switch (cmd)
	{
		case CMD_I2CRD:
			printk(KERN_CRIT"I2C READ  \n");
			break;
		case CMD_I2CWR:
			printk(KERN_CRIT"I2C WRITE \n");
			break;
	}
	return 0;
}

static struct file_operations i2crdreg_fops = {
	.owner   =   THIS_MODULE,
	.open    =   i2crdreg_open,
	.release =   i2crdreg_close,
	.write   =   i2crdreg_write,
	.read    =   i2crdreg_read,
	.unlocked =  i2crdreg_ioctl
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &i2crdreg_fops
};

//写寄存器函数: 8位
static int i2cwrite_regs_8(struct i2c_client *client, struct wr_poweron *wr)
{
	int ret;
	u8 buf[2];
	u32 paddr;
	struct i2c_msg msg;
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);
	
	buf[0] = wr->reg;
	buf[1] = wr->val;
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
	i2cdelay(wr);
	printk(KERN_CRIT"default regadd: 0x%02x---val is 0x%02x", wr->reg, wr->val);

	return 0;
}

//读寄存器函数：8位
static int i2cread_regs_8(struct i2c_client *client, struct rd_poweron *rd)
{
	int ret = 0;
	u8 buf[1];
	u32 paddr;
	struct i2c_msg msg[2];
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;

	printk(KERN_CRIT"\n This is %s ", __func__);

	buf[0] = rd->reg;
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
	rd->val = buf[0];
	printk(KERN_CRIT"hex  default regadd: 0x%02x---val is 0x%02x\n", rd->reg, rd->val);
	return 0;
}

//写寄存器函数: 16位
static int i2cwrite_regs_16(struct i2c_client *client, struct wr_poweron *wr)
{
	int ret;
	u8 buf[3];
	u32 paddr;
	struct i2c_msg msg;
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);
	
	buf[0] = (wr->reg)>>8;
	buf[1] = (wr->reg)&0xff;
	buf[2] = wr->val;
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
	i2cdelay(wr);
	printk(KERN_CRIT"default regadd: 0x%02x---val is 0x%02x", wr->reg, wr->val);

	return 0;
}

//读寄存器函数：16位
static int i2cread_regs_16(struct i2c_client *client, struct rd_poweron *rd)
{
	int ret = 0;
	u8 buf[2];
	//client->addr = 0x0c;
	//u32 paddr = 0x0c;
	u32 paddr;
	struct i2c_msg msg[2];
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;

	printk(KERN_CRIT"\n This is %s ", __func__);

	buf[0] = (rd->reg)>>8;
	buf[1] = (rd->reg)&0xff;
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
	rd->val = buf[0];
	printk(KERN_CRIT"hex  default regadd: 0x%02x---val is 0x%02x\n", rd->reg, rd->val);
	return 0;
}

//延时函数
static int i2cdelay(struct wr_poweron *wr)
{
	int time;
	time = wr->delay_time;
	mdelay(time);
	return 0;
}

//写指定数组，开机初始化
static int powerup(struct i2c_client *client)
{
	int i;
	struct wr_poweron *wr;
	int len;
	len = sizeof(para_write) / (3 * sizeof(u8));
	printk(KERN_CRIT"len is %d\n", len);
	for(i = 0; i < len; i++)
	{
		wr = &para_write[i];
		printk(KERN_CRIT"write reg and val");
		i2cwrite_regs_8(client, wr);
	}
	return 0;
}

//读指定数组，判断相应状态与值
static int read_status(struct i2c_client *client)
{
	pthread_t sta;
    int len, len_line, len_column, end_len;
	u8 chip_val[6] = {0};
    u8 bin_arr[] = {0};
	u8 chip_arr[6] = {0};
	struct rd_poweron *rd;
	u8 othch[][6] = {
		{'_', 'U', 'B', '9', '4', '7'}, 
		{'_', 'U', 'B', '9', '4', '8'}, 
		{'_', 'U', 'B', '9', '4', '9'}, 
		{'_', 'U', 'H', '9', '4', '7'}
	};
	int i = 0;
	int ret;
    int m, k, j, n;
    u8 video_frequency = 0;
    len = sizeof(para_read) / (2 * sizeof(u8));
	printk(KERN_CRIT"This is %s ", __func__);
	printk(KERN_CRIT"len is %d ", len);
	for(i = 0; i < len; i++)
	{
		rd = &para_read[i];
		i2cread_regs_8(client, rd);
		switch(i) {
            case 0: 
                printk(KERN_CRIT"to judge if it is dual or single mode\n");
                for(j = 0;j < 8; j++)
                {
                    m = rd->val%2;  //取2的余数
                    k = rd->val/2;  //取被2整除的结果
                    rd->val = k;
                    bin_arr[j] = m;    //将余数存入数组bin_arr数组中
                    printk(KERN_CONT"%d", bin_arr[j]);
                }
                break;
            case 1: 
				//读取是单像素还是双像素模式
                printk(KERN_CRIT"Read the video_frequency"); 
                if(bin_arr[1] == 0)
                {
                    video_frequency  = (rd->val) / 2;
                    printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Dual-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
                }
                else if(bin_arr[1] == 1)
                {
                    video_frequency  = rd->val;
                    printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Single-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
                }
                break;
			case 2: 
				//读取GPIO0357状态位
				printk(KERN_CRIT"GPIO0357 status:%ld", rd->val);break;
            case 3: 
				//读取GPIO8状态位
				printk(KERN_CRIT"GPIO8    status:%ld", rd->val);break;
			case 4: 
				//检测link状态位
                printk(KERN_CRIT"Link_status");
                lock_status = rd->val;
				ret = pthread_create(&sta, NULL, judge_lock_status, NULL);
				if
                break;
            case 5:  printk(KERN_CRIT"CHIP0  %c", rd->val); chip_val[0] = rd->val;break;
            case 6:  printk(KERN_CRIT"CHIP1  %c", rd->val); chip_val[1] = rd->val;break;
            case 7:  printk(KERN_CRIT"CHIP2  %c", rd->val); chip_val[2] = rd->val;break;
            case 8:  printk(KERN_CRIT"CHIP3  %c", rd->val); chip_val[3] = rd->val;break;
            case 9:  printk(KERN_CRIT"CHIP4  %c", rd->val); chip_val[4] = rd->val;break;
            case 10: printk(KERN_CRIT"CHIP5  %c", rd->val); chip_val[5] = rd->val;
				//判断芯片型号
				len_line = sizeof(othch) / sizeof(othch[0]);
				len_column = sizeof(othch[0]) / sizeof(char);
				for(n = 0; n < len_line; n++)
				{
					for(int j = 0; j < len_column; j++)
					{
						//printk(KERN_CRIT"chip_arr[%d] is %c, othch[%d][%d] is %c", j, chip_arr[j], n, j, othch[n][j]);
						if((char)(chip_val[j]) == othch[n][j])
						{
							end_len = len_column - 1;
							if(j == end_len)
							{
								printk(KERN_CRIT"This chip is ");
								for(int k = 0;k < len_column; k++)
								{
									printk(KERN_CONT"%c", othch[n][k]);
								}
								break;
							}
						}
						else break;
					}
				}
				break;
            default: printk(KERN_CRIT"error");break;
        }
	}
	return 0;
}

//新建线程：判断LOCK状态
static void *judge_lock_status(void *arg)
{

	printk(KERN_CRIT"create pthread : judge the status of lock");
	if((lock_status & 0x01) == 1)
	{
		printk(KERN_CRIT"link is detected,  link_status = 1");
	}
	else 
	{
		printk(KERN_CRIT"link is not detected, link_status is 0");
	}
}

//设置IIS功能关闭
static int set_iis_diable(struct i2c_client *client)
{
	//设置寄存器复位芯片相应的寄存器地址
	u8 reg[] = {0x54, };
	struct wr_poweron *wr;
	//设置bit1为复位模式，清除整个数据块
	u8 i2s_disabled = {0x00, };
	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cwrite_regs_8(client, wr);
	return 0;
}

//上电控制PDB使能
static int set_pdb_enable(struct i2c_client *client)
{
	int ret;
	int control_gpio = 0;
	struct device_node *test_device_node;

	//获得设备节点
	test_device_node = of_find_node_by_path("/con_test");
	if(test_device_node == NULL)
	{
		printk(KERN_CRIT"of_find_node_by_path is error ");
		return -1;
	}
	printk(KERN_CRIT"of_find_node_by_path is succeed");
	//使用of_get_named_gpio函数获取GPIO编号，此函数会将设备树中<&port4b 9 GPIO_ACTIVE_HIGH>的属性信息转换为对应的GPIO编号
	control_gpio = of_get_named_gpio(test_device_node, "control-gpio", 0);
	if(control_gpio < 0)
	{
		printk(KERN_CRIT"of_get_named_gpio is error ");
		return -1;
	}
	printk(KERN_CRIT"control_gpio is %d", control_gpio);
	//申请一个GPIO管脚
	ret = gpio_request(control_gpio, "control");
	/*if(ret < 0)
	{
		printk(KERN_CRIT"gpio_request is error");
	}*/
	printk(KERN_CRIT"gpio_request is succeed");
	//设置某个GPIO为输出，并且设置默认输出值
	gpio_direction_output(control_gpio, 0);
	printk(KERN_CRIT"gpio_direction is succeed");
	//设置输出低电平
	gpio_set_value(control_gpio, 0);
    
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
	u32 paddr;

	printk(KERN_CRIT"This is %s ", __func__);

	//设置PDB上电使能
	set_pdb_enable(client);
	//获取reg的值，即获取设备地址
	ret = of_property_read_u32(dev->of_node, "reg", &paddr);
	if (ret)
		dev_err(&client->dev, "parse reg failed\n");
	dtsaddr = &paddr;
	printk(KERN_CRIT"The device address has been obtained");
	
	//开机初始化
	//powerup(client);
	//读取指定值，判断
	read_status(client);
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
		printk(KERN_CRIT"i2c_add_driver is error");
		return ret;
	}
	printk(KERN_CRIT"This is %s", __func__);
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


