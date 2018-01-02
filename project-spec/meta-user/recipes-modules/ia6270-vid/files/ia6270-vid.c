/*  ia6270-vid.c - The simplest kernel module.

* Copyright (C) 2013 - 2016 Xilinx, Inc
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.

*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along
*   with this program. If not, see <http://www.gnu.org/licenses/>.

*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

/* Standard module information, edit as appropriate */
MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Xilinx Inc.");
MODULE_DESCRIPTION
    ("ia6270-vid - IA6270 Video DMA enable module");

#define DRIVER_NAME "ia6270-vid"

uint32_t BufferSize = 8*1024*1024;
size_t HBlank = 378;
size_t VBlank = 100440;
size_t LineSize = 4096;

module_param(BufferSize, uint, S_IRUGO);
module_param(HBlank, uint, S_IRUGO);
module_param(VBlank, uint, S_IRUGO);
module_param(LineSize, uint, S_IRUGO);

struct vid_regs
{
	volatile uint32_t ctrl;
	volatile uint32_t stat;
	volatile uint32_t line_size;
	volatile uint32_t rsv;
	volatile uint32_t start_address;
	volatile uint32_t end_address;
	volatile uint32_t hblank;
	volatile uint32_t vblank;
};

struct ia6270_vid_local {
	//int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;

	dma_addr_t buf_phys;
	void *buf_start;
};

/*
static irqreturn_t ia6270_vid_irq(int irq, void *lp)
{
	printk("ia6270-vid interrupt\n");
	return IRQ_HANDLED;
}
*/

static int ia6270_vid_probe(struct platform_device *pdev)
{
	//struct resource *r_irq; /* Interrupt resources */
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev;
	struct ia6270_vid_local *lp = NULL;
	struct vid_regs *vid_regs;

	int rc = 0;
	dev_info(dev, "Device Tree Probing\n");
	/* Get iospace for the device */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	lp = (struct ia6270_vid_local *) kmalloc(sizeof(struct ia6270_vid_local), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Cound not allocate ia6270-vid device\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, lp);
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;

	if (!request_mem_region(lp->mem_start,
				lp->mem_end - lp->mem_start + 1,
				DRIVER_NAME)) {
		dev_err(dev, "Couldn't lock memory region at %p\n",
			(void *)lp->mem_start);
		rc = -EBUSY;
		goto error1;
	}

	lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	if (!lp->base_addr) {
		dev_err(dev, "ia6270-vid: Could not allocate iomem\n");
		rc = -EIO;
		goto error2;
	}

	/* Get IRQ for the device */
	/*
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		dev_info(dev, "ia6270-vid at 0x%08x mapped to 0x%08x\n",
			(unsigned int __force)lp->mem_start,
			(unsigned int __force)lp->base_addr);
		return 0;
	}
	lp->irq = r_irq->start;
	rc = request_irq(lp->irq, &ia6270_vid_irq, 0, DRIVER_NAME, lp);
	if (rc) {
		dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
			lp->irq);
		goto error3;
	}
	*/

	dev_info(dev,"ia6270-vid at 0x%08x mapped to 0x%08x\n",
		(unsigned int __force)lp->mem_start,
		(unsigned int __force)lp->base_addr);

	lp->buf_start = dma_zalloc_coherent(dev, BufferSize,
		&lp->buf_phys, GFP_KERNEL);
	if(!lp->buf_start){
		dev_err(dev, "not enough memory\n");
		goto error3;
	}
	dev_info(dev,"DMA memory 0x%08x - 0x%08x\n",
		lp->buf_phys, lp->buf_phys+BufferSize);

	vid_regs = lp->base_addr;

	vid_regs->line_size = LineSize;
	vid_regs->start_address = lp->buf_phys;
	vid_regs->end_address = lp->buf_phys+BufferSize;
	vid_regs->hblank = HBlank;
	vid_regs->vblank = VBlank;

	vid_regs->ctrl = 0x1;
	return 0;
error3:
	//free_irq(lp->irq, lp);
error2:
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return rc;
}

static int ia6270_vid_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ia6270_vid_local *lp = dev_get_drvdata(dev);
	struct vid_regs *vid_regs=lp->base_addr;
	/*free_irq(lp->irq, lp);*/

	vid_regs->ctrl = 0;
	while(vid_regs->stat & 0x1)
		msleep(1);

	dma_free_coherent(dev, BufferSize, lp->buf_start, lp->buf_phys);

	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ia6270_vid_of_match[] = {
	{ .compatible = "ia6270,ia6270-vid", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, ia6270_vid_of_match);
#else
# define ia6270_vid_of_match
#endif


static struct platform_driver ia6270_vid_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= ia6270_vid_of_match,
	},
	.probe		= ia6270_vid_probe,
	.remove		= ia6270_vid_remove,
};

static int __init ia6270_vid_init(void)
{
	printk(KERN_INFO "ia6270-vid started.\n");
	return platform_driver_register(&ia6270_vid_driver);
}


static void __exit ia6270_vid_exit(void)
{
	platform_driver_unregister(&ia6270_vid_driver);
	printk(KERN_INFO "ia6270-vid exit.\n");
}

module_init(ia6270_vid_init);
module_exit(ia6270_vid_exit);
