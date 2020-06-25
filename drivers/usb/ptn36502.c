#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/uaccess.h>
#include "ptn36502.h"
#include <linux/clk.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

u8 is_ptn36502_safe_mode = 0;

void set_ptn36502_ldo_en(void)
{
	int value=0,gpio56=56;
	u8 r = 0;
	value = gpio_get_value(gpio56);
	printk("gpio56 default value is %d\n",value);
	r = gpio_request(gpio56, "ptn36502_ldo");
		if (r) {
			printk("%s: unable to request nfc reset gpio [%d]\n",
				__func__,
				gpio56);
		}
		r = gpio_direction_output(gpio56, 1);
		if (r) {
			printk("%s: unable to set direction for nfc reset gpio [%d]\n",
					__func__,
					gpio56);
		}
	printk("gpio56 set to output hight\n");
}

struct ptn36502_dev {
	struct	i2c_client	*client;
};

static const struct of_device_id msm_match_table[] = {
	{.compatible = "nxp,ptn36502"},
	{}
};

struct ptn36502_dev *ptn36502_dev;

MODULE_DEVICE_TABLE(of, msm_match_table);

static int ptn36502_i2c_write_reg( struct i2c_client* client,uint8_t reg,uint8_t data)  {

	unsigned char buffer[2];

	buffer[0] = reg;
	buffer[1] = data;

	if (2 != i2c_master_send(client,buffer,2)) {
	    dev_err(&client->dev,"%s: ------- ptn36502_i2c_write_reg  fail------\n", __func__);
	    return -1;
	}

	return 0;
 }

static int ptn36502_i2c_read_reg( struct i2c_client* client,uint8_t reg,uint8_t *data)
{

	// write reg add
	if (1 != i2c_master_send(client, &reg,1)) {
	    dev_err(&client->dev,"%s: ------- i2c_master_send reg add  fail------\n", __func__);
	    return -1;
	}

	// read
	if (1 != i2c_master_recv(client,data,1)) {
	    dev_err(&client->dev,"%s: ------- ptn36502_i2c_read_reg  fail------\n", __func__);
	    return -1;
	}

	return 0;
}

int ptn36502_init(struct i2c_client *client) {
	int r = 0;
	dev_err(&client->dev,"%s: ------- ptn36502_init start------\n", __func__);

	r = 	ptn36502_i2c_write_reg(client,DEVICE_CONTROL,0x81);
	if (r < 0) {
		dev_err(&client->dev,"%s:DEVICE_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,MODE_CONTROL_1,0x08);
	if (r < 0) {
		dev_err(&client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,DP_LINK_CONTROL,0x0e);
	if (r < 0) {
		dev_err(&client->dev,"%s:DP_LINK_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,USB_US_TX_RX_CONTROL,0x51);
	if (r < 0) {
		dev_err(&client->dev,"%s:USB_US_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,USB_DS_TX_RX_CONTROL,0x51);
	if (r < 0) {
		dev_err(&client->dev,"%s:USB_DS_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,DP_LINK_0_TX_RX_CONTROL,0x29);
	if (r < 0) {
		dev_err(&client->dev,"%s:DP_LINK_0_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,DP_LINK_1_TX_RX_CONTROL,0x29);
	if (r < 0) {
		dev_err(&client->dev,"%s:DP_LINK_1_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,DP_LINK_2_TX_RX_CONTROL,0x29);
	if (r < 0) {
		dev_err(&client->dev,"%s:DP_LINK_2_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	r = 	ptn36502_i2c_write_reg(client,DP_LINK_3_TX_RX_CONTROL,0x29);
	if (r < 0) {
		dev_err(&client->dev,"%s:DP_LINK_3_TX_RX_CONTROL write fail.\n", __func__);
		return -ENOMEM;
	}

	dev_err(&client->dev,"%s: ------- ptn36502_init end------\n", __func__);

	return 0;
}

void set_ptn36502_safe_state_mode (void) {
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_safe_state_mode start------\n", __func__);
	ptn36502_init(ptn36502_dev->client);
	__gpio_set_value(56, 0);
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_safe_state_mode end------\n", __func__);
}

void set_ptn36502_usp_3p0_only_mode (int cc_orient_reversed) {
	int r = 0;
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_usp_3p0_only_mode start------\n", __func__);
	__gpio_set_value(56, 1);
	mdelay(10);
	if (cc_orient_reversed == 1) {  // Normal
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x01);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}
	} else if (cc_orient_reversed == 2) {  // Reversed
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x21);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}
	}
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_usp_3p0_only_mode end------\n", __func__);
}

void set_ptn36502_usp3_and_dp2lane_mode (int cc_orient_reversed) {
	int r = 0;
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_usp3_and_dp2lane_mode start------\n", __func__);
	__gpio_set_value(56, 1);
	mdelay(10);
	if (cc_orient_reversed == 1) {  // Normal
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x0a);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,DP_LINK_CONTROL,0x06);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:DP_LINK_CONTROL write fail.\n", __func__);
		}

	} else if (cc_orient_reversed == 2) {  // Reversed
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x2a);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,DP_LINK_CONTROL,0x06);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:DP_LINK_CONTROL write fail.\n", __func__);
		}
	}
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_usp3_and_dp2lane_mode end------\n", __func__);
}

void set_ptn36502_dp4lane_mode (int cc_orient_reversed) {
	int r = 0;
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_dp4lane_mode start------\n", __func__);
	__gpio_set_value(56, 1);
	mdelay(10);
	if (cc_orient_reversed == 1) {  // Normal
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x08);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x0b);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,DP_LINK_CONTROL,0x06);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:DP_LINK_CONTROL write fail.\n", __func__);
		}

	} else if (cc_orient_reversed == 2) {  // Reversed
		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x28);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,MODE_CONTROL_1,0x2b);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:MODE_CONTROL_1 write fail.\n", __func__);
		}

		r = ptn36502_i2c_write_reg(ptn36502_dev->client,DP_LINK_CONTROL,0x06);
		if (r < 0) {
			dev_err(&ptn36502_dev->client->dev,"%s:DP_LINK_CONTROL write fail.\n", __func__);
		}
	}
	dev_err(&ptn36502_dev->client->dev,"%s: ------- set_ptn36502_dp4lane_mode end------\n", __func__);
}


static const struct i2c_device_id nxp_id[] = {
	{"ptn36502-i2c", 0},
	{}
};

static int ptn36502_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int r = 0;
	u8 value = 0;
	dev_err(&client->dev,"%s: ------- ptn36502_probe start------\n", __func__);

	set_ptn36502_ldo_en();
	mdelay(10);
	ptn36502_dev = kzalloc(sizeof(*ptn36502_dev), GFP_KERNEL);
	if (ptn36502_dev == NULL) {
		r = -ENOMEM;
		return r;
	}
	ptn36502_dev->client = client;
	printk("gpio56  value is %d\n",gpio_get_value(56));
	r = ptn36502_i2c_read_reg(client,CHIP_ID,&value);
	if (r < 0) {
		dev_err(&client->dev,"%s:CHIP_ID_reg read fail,please check HW.\n", __func__);
		return -ENOMEM;
	}

	r = ptn36502_init(client);
	if (r < 0) {
		dev_err(&client->dev,"%s:ptn36502_init fail , exit probe.\n", __func__);
		return -ENOMEM;
	}

	is_ptn36502_safe_mode = 1;
	dev_err(&client->dev,"%s:set is_ptn36502_safe_mode = %d .\n", __func__,is_ptn36502_safe_mode);

	__gpio_set_value(56, 0);
	dev_err(&client->dev,"%s: ------- ptn36502_probe end------\n", __func__);

	return 0;

}



static int ptn36502_remove(struct i2c_client *client)
{
	return 0;
}


static struct i2c_driver nxp = {
	.id_table = nxp_id,
	.probe = ptn36502_probe,
	.remove =  ptn36502_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ptn36502",
		.of_match_table = msm_match_table,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};

/*
 * module load/unload record keeping
 */
static int __init nxp_dev_init(void)
{
	return i2c_add_driver(&nxp);
}

static void __exit nxp_dev_exit(void)
{
	i2c_del_driver(&nxp);
}

module_init(nxp_dev_init);
module_exit(nxp_dev_exit);

MODULE_DESCRIPTION("PTN36502 USB");
MODULE_LICENSE("GPL v2");
