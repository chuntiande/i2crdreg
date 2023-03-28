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

#include <linux/slab.h>
#include <linux/types.h>

#define IIC_WR  0
#define IIC_RD  1
#define UB948   "_UB948"
#define UB947   "_UB947"
#define UH947   "_UH947"
#define UB949   "_UB949"
#define DEVICE_NAME "i2crdreg"
//#define SLAVE_ADDRESS 0x0c

#define NODE "hsae, rdreg-i2c"

static u32 *dtsaddr;
static struct i2c_client *client;
static int i2cread_regs_8(struct i2c_client *client, u8 reg, u8 *val);
static int i2cwrite_regs_8(struct i2c_client *client, u8 reg, u8 val);

static int i2cset_passthrough(struct i2c_client *client);
static int i2cset_dual_pair(struct i2c_client *client);
static int set_gpio_input(struct i2c_client *client);
static int link_detect_8(struct i2c_client *client);
static int read_video_frequency(struct i2c_client *client);
static int i2cset_reset_bit(struct i2c_client *client);
static int read_reset_bit_8(struct i2c_client *client);
static int read_gpio_status_8(struct i2c_client *client);
static int set_iis_diable(struct i2c_client *client);
static int set_pdb_enable(struct i2c_client *client);
static int check_chip_id(struct i2c_client *client);

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

/*
static int deser_parse_dts(struct i2c_client *client)
{
	int ret=0;
	printk(KERN_CRIT"This is %s", __func__);
	ret = of_property_read_u32(client->dev.of_node, "reg", &paddr);
	if (ret < 0) {
		dev_info(&client->dev, "no serializer addr is parsed\n");
	}
	return 0;
}
*/
//读寄存器函数
static int i2cread_regs_8(struct i2c_client *client, u8 reg, u8 *val)
{
	int ret = 0;
	u8 buf[1];
	int i = 0;
	//client->addr = 0x0c;
	//u32 paddr = 0x0c;
	u32 paddr;
	struct i2c_msg msg[2];
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;

	printk(KERN_CRIT"\n This is %s ", __func__);

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
	*val = buf[0];
	printk(KERN_CRIT"hex  default regadd: 0x%02x---val is 0x%02x\n", reg, *val);
	return 0;
}

//判断OLDI接口是单像素还是双像素模式，再读取OpenLDI的输入视频频率
static int read_video_frequency(struct i2c_client *client)
{
	//将十进制转换为二进制的间接数
	int m, k;
	//存放二进制数的数组
	u8 bin_arr[] = {0};
	u8 reg;
	u8 pixel_mode;
	u8 pixel_frequency = 0;
	u8 video_frequency = 0;
	//读取视频频率的寄存器地址
	u8 pf_addr[] = {0x4f, 0x5f};

	printk(KERN_CRIT"\n This is %s", __func__);
	//判断OLDI接口是单像素还是双像素模式
	reg = pf_addr[0];
	i2cread_regs_8(client, reg, &pixel_mode);
	for(int i = 0;i < 8; i++)
	{
		m = pixel_mode%2;  //取2的余数
		k = pixel_mode/2;  //取被2整除的结果
		pixel_mode = k;
		bin_arr[i] = m;    //将余数存入数组bin_arr数组中
		printk(KERN_CONT"%d", bin_arr[i]);
	}

	//读取OpenLDI输入视频频率
	reg = pf_addr[1];
	i2cread_regs_8(client, reg, &pixel_frequency);
	//读取OpenLDI视频频率
	printk(KERN_CRIT"pixel_frequency is  0x%02x hex, DEC :%d", pixel_frequency, pixel_frequency);
	if(bin_arr[1] == 0)
	{
		video_frequency  = pixel_frequency / 2;
		printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Dual-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
	}
	else if(bin_arr[1] == 1)
	{
		video_frequency  = pixel_frequency;
		printk(KERN_CRIT"bit 6 is %d, and it detect the mode is Single-pixel, so the video_frequency of OLDI is %d", bin_arr[1], video_frequency);
	}
	
	return 0;
}

//读取gpio状态位是否为输入
static int read_gpio_status_8(struct i2c_client *client)
{
	u8 reg = 0;
	int len;
	u8 status;
	//读取gpio状态位的寄存器地址
	u8 gpio_status_addr[] = {0x1c, 0x1d};
	len = sizeof(gpio_status_addr);
	printk(KERN_CRIT"\n This is %s ", __func__);
	for(int i = 0; i < len; i++)
	{
		reg = gpio_status_addr[i];
		i2cread_regs_8(client, reg, &status);
	}
	return 0;
}

//link状态检测连接是否断开
static int link_detect_8(struct i2c_client *client)
{
	u8 link_status = 0;
	//读取LINK状态位的寄存器地址
	u8 reg = 0x0c;
	printk(KERN_CRIT"\n This is %s ", __func__);

	i2cread_regs_8(client, reg, &link_status);
	if(link_status & 0x01 == 1)
	{
		printk(KERN_CRIT"link is detected,  link_status = 1");
	}
	else printk(KERN_CRIT"link is not detected, link_status is 0");
	return 0;
}

//读取开机寄存器复位是否设置成功
static int read_reset_bit_8(struct i2c_client *client)
{
	u8 reset_bit;
	//读取LINK状态位的寄存器地址
	u8 reg = 0x01;

	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cread_regs_8(client, reg, &reset_bit);
	return 0;
}

//判断不同芯片的型号
static int check_chip_id(struct i2c_client *client)
{
	u8 chip_id;
	int len, len_line, len_column;
	u8 chip_arr[6] = {0};
	u8 othch[][6] = {
		{'_', 'U', 'B', '9', '4', '7'}, 
		{'_', 'U', 'B', '9', '4', '8'}, 
		{'_', 'U', 'B', '9', '4', '9'}, 
		{'_', 'U', 'H', '9', '4', '7'}
	};

	int i;
	u8 reg;
	u16 chip_addr[] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5};
	printk(KERN_CRIT"this is %s ", __func__);
	len_line = sizeof(othch) / sizeof(othch[0]);
	len_column = sizeof(othch[0]) / sizeof(char);
				
	for(i = 0;i < len_column; i++)
	{
		reg = chip_addr[i];
		i2cread_regs_8(client, reg, &chip_id);
		chip_arr[i] = chip_id;
		printk(KERN_CRIT"chip_id [0x%02x]=%c, chip_arr[%d]=%c", reg, chip_id, i, chip_arr[i]);
	}
	for(i = 0; i < len_line; i++)
	{
		for(int j = 0; j < len_column; j++)
		{
			//printk(KERN_CRIT"chip_arr[%d] is %c, othch[%d][%d] is %c", j, chip_arr[j], i, j, othch[i][j]);
			if((char)chip_arr[j] == othch[i][j])
			{
				if(j == (len_line - 1))
				{
					printk(KERN_CRIT"This chip is ");
					for(int k = 0;k < len_column; k++)
					{
						printk(KERN_CONT"%c", othch[i][k]);
					}
					break;
				}
			}
			else break;
		}
	}
	return 0;
}

//写寄存器函数
static int i2cwrite_regs_8(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	u8 buf[2];
	//client->addr = 0x0c;
	//u32 paddr = 0x0c;
	u32 paddr;
	struct i2c_msg msg;
	u8 addr;
	paddr = *dtsaddr;
	addr = paddr&0xff;
	printk(KERN_CRIT"\n This is %s ", __func__);
	
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
	printk(KERN_CRIT"default regadd: 0x%02x---val is 0x%02x", reg, val);

	return 0;
}

//设置IIC直通使能
static int i2cset_passthrough(struct i2c_client *client)
{
	//设置IIC直通使能相应的寄存器地址
	u8 reg = 0x03;
	//设置相应寄存器的IIC直通功能使能
	u8 val = 0xDA;

	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cwrite_regs_8(client, reg, val);
	return 0;
}

//设置IIS功能关闭
static int set_iis_diable(struct i2c_client *client)
{
	//设置寄存器复位芯片相应的寄存器地址
	u8 reg[] = {0x54, };
	//设置bit1为复位模式，清除整个数据块
	u8 i2s_disabled = {0x00, };

	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cwrite_regs_8(client, reg, i2s_disabled);
	return 0;
}

//设置开机使用寄存器复位芯片
static int i2cset_reset_bit(struct i2c_client *client)
{
	//设置寄存器复位芯片相应的寄存器地址
	u8 reg = 0x01;
	//设置bit1为复位模式，清除整个数据块
	u8 reset = 0x02;

	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cwrite_regs_8(client, reg, reset);
	return 0;
}

//设置Dual双通道模式并设置为双绞线模式
static int i2cset_dual_pair(struct i2c_client *client)
{
	//设置Dual双通道，双绞线模式相应的寄存器地址
	u8 reg = 0x5B;
	//设置Dual双通道，双绞线模式相应寄存器的值
	u8 dual = 0x23;
	printk(KERN_CRIT"\n This is %s ", __func__);
	i2cwrite_regs_8(client, reg, dual);
	return 0;
}

//设置GPIO为输入模式
static int set_gpio_input(struct i2c_client *client)
{
	//GPIO0-3,GPIO5-8相应的寄存器地址
	u8 addr_arr[] = {0x0d, 0x0e, 0x0f, 0x10, 0x11};
	//GPIO0-3,GPIO5-8的值设置为输入
	u8 val_arr[]  = {0x23, 0x33, 0x03, 0x33, 0x33}; 
	printk(KERN_CRIT"\n This is %s ", __func__);

	for(int i = 0; i < 5; i++)
	{
		u8 reg = addr_arr[i];
		u8 val = val_arr[i];
    	i2cwrite_regs_8(client, reg, val);
	}
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
	//set_pdb_enable(client);
	//获取reg的值，即获取设备地址
	ret = of_property_read_u32(dev->of_node, "reg", &paddr);
	if (ret)
		dev_err(&client->dev, "parse reg failed\n");
	dtsaddr = &paddr;
	printk(KERN_CRIT"It has capture the address of slave_address");
	//printk(KERN_CRIT"dev->of_node is %s\n", *(dev->of_node));
	//设置开机使用寄存器复位芯片
	//i2cset_reset_bit(client);

	//将GPIO 0-3,GPIO 5-8设置输入模式
	//set_gpio_input(client);

	//设置IIS功能关闭
	//set_iis_diable(client);

	//设置Dual双通道模式并设置为双绞线模式
	//i2cset_dual_pair(client);

	//读取开机寄存器相应的寄存器值是否为复位
	//read_reset_bit_8(client);

	//检测link状态
	link_detect_8(client);

	//读取GPIO0-3,GPIO5-8相应寄存器的值并打印
	//读取OLDI的输入视频频率
	//read_video_frequency(client);

	//判断不同芯片的型号
	check_chip_id(client);

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


