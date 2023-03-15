/*
 * Copyright (C) 2020-2030 Semidrive, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>

#define IIC_WR 0
#define IIC_RD 1

static int i2cread_regs(struct i2c_client *client, u8 reg, u8 *val);
int ub947_read_reg(struct i2c_client *client, u8 reg, u8 *val);

int ub947_read_reg(struct i2c_client *client, u8 reg,u8 *val)
{
	return i2cread_regs(client, reg, val);
}
//EXPORT_SYMBOL(ub947_read_reg);

int ub947_write_reg_ori(struct i2c_client *client,
						   u8 reg,
						   u8 val)
{
	struct i2c_msg msg;
	u8 buf[2];
	int ret;
	u32 *paddr;
	u8 addr;

	paddr = i2c_get_clientdata(client);
	if (paddr == NULL) {
		dev_err(&client->dev, "get addr from client fail!\n");
		return -EIO;
	}
	dev_info(&client->dev, "%s get addr from client 0x%x", __func__, *paddr);
	addr = *paddr&0xff;
	buf[0] = reg;
	buf[1] = val;
	msg.addr = addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev,
				"%s:  chip 0x%02x error: reg=0x%02x, val=0x%02x\n",
				__func__, msg.addr, reg, val);
		return ret;
	}
	return 0;
}
//EXPORT_SYMBOL(ub947_write_reg_ori);


/*This is a function for i2c address remapping The original address is 0x40 and needs to be ramapped
  for different channel access from soc*/
/* for avm normally the
   channel 0 0x40 -->0x48
   channel 1 0x40 -->0x49
   channel 2 0x40 -->0x4a
   channel 3 0x40 -->0x4b */
int ub947_map_i2c_addr(struct i2c_client *client)
{
	struct i2c_msg msg;
	u8 buf[2];
	int ret;
	u16 reg = 0x00;
	u32 *paddr;
	u8 addr;

	if (client == NULL) {
		printk("no client %s %d", __func__, __LINE__);
		return -ENODEV;
	}
	paddr = i2c_get_clientdata(client);
	dev_info(&client->dev, "%s get addr from client 0x%x", __func__, *paddr);
	addr = *paddr&0xff;
	buf[0] = reg;
	buf[1] = (client->addr<<1);//This is the address from dts
	msg.addr = addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev,
				"%s:  chip 0x%02x error: reg=0x%02x, val=0x%02x\n",
				__func__, msg.addr, reg, buf[1]);
		return ret;
	}
	dev_info(&client->dev,
			 "%s:  chip 0x%02x : reg=0x%02x, val=0x%02x\n",
			 __func__,  msg.addr, reg, buf[1]);
	return 0;
}
//EXPORT_SYMBOL(ub947_map_i2c_addr);

//读寄存器函数
static int i2cread_regs(struct i2c_client *client, u8 reg, u8 *buffer)
{
	int ret;
	u8 buf[1];
	client->addr = 0x0c;
	struct i2c_msg msg[2];

    buf[0] = reg;
	msg[0].addr  = client->addr;
	msg[0].flags = IIC_WR;   //flags为0，是写，表示buf是我们要发送的数据
	msg[0].buf   = buf;             //寄存器地址
	msg[0].len   = sizeof(buf);     //len为buf的大小，单位是字节

	msg[1].addr  = client->addr;
	msg[1].flags = IIC_WR | IIC_RD;   //flags为1，是读，表示buf是我们要接收的数据
	msg[1].buf   = buf;          //寄存器的值
	msg[1].len   = 1;
	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret < 0)
	{
		printk(KERN_CRIT"read regs from i2c has been failed, ret = %d \r\n", ret);
		return ret;
	}
	*buffer = buf[0];
	printk(KERN_CRIT"\n default regadd: %0#x---value is %0#x", reg, *buffer);
	return 0;

}

int ub947_check_chip_id(struct i2c_client *client)
{
	int ret = 0;
	u8 chip_id = 0;
	int i = 0;
	struct i2c_msg msg[2];
	u8 buf[1];
	u16 reg = 0xf0;
	u32 paddr = 0x0c;
	u8 addr;

	//paddr = i2c_get_clientdata(client);
	//dev_info(&client->dev, "%s get addr from client 0x%x", __func__, *paddr);
	addr = paddr&0xff;
	buf[0] = reg;
	msg[0].addr = addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);
	msg[1].addr = addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = 1;
	printk(KERN_CRIT"this is %s ", __func__);
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s:chip 0x%02x error: reg=0x%02x ret %d\n",
				 __func__,   msg[0].addr, reg, ret);
		return ret;
	}
	chip_id = buf[0];
	dev_err(&client->dev, "ub947 dev chipid = 0x%02x\n", chip_id);
	printk(KERN_CRIT"reg=0x%02x, val=0x%02x, char=%c\n", reg, chip_id, chip_id);
	return 0;
}
//EXPORT_SYMBOL(ub947_check_chip_id);

static int ub947_probe(struct i2c_client *client,
						  const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	int ret;
	u8 reg = 0xf0;
	u8 *val;
	u32 *paddr;
	//u32 *addr = kzalloc(sizeof(u32), GFP_KERNEL);
	printk(KERN_CRIT"this is %s\n", __func__);
	/*
	ret = of_property_read_u32(dev->of_node, "rdreg", addr);
	if (ret)
		dev_err(&client->dev, "parse reg_org failed\n");
	*/
	ub947_check_chip_id(client);
	//paddr = i2c_get_clientdata(client);
	return 0;
}

static int ub947_remove(struct i2c_client *client)
{
	u32 *paddr = i2c_get_clientdata(client);

	kfree(paddr);
	paddr = NULL;
	printk(KERN_CRIT"this is %s", __func__);
	return 0;
}

static const struct i2c_device_id ub947_id[] = {
	{"rdreg-i2c", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, ub947_id);

static const struct of_device_id ub947_dt_ids[] = {
	{.compatible = "hsae, rdreg-i2c"},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ub947_dt_ids);

static struct i2c_driver ub947_i2c_driver = {
	.driver = {
		.name = "i2crdreg",
		.of_match_table = ub947_dt_ids,
	},
	.id_table = ub947_id,
	.probe = ub947_probe,
	.remove = ub947_remove,
};

module_i2c_driver(ub947_i2c_driver);


MODULE_DESCRIPTION("ub947 MIPI Camera Subdev Driver");
MODULE_LICENSE("GPL");
