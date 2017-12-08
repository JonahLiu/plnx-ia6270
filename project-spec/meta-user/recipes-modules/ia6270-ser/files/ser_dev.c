#include <linux/delay.h>

#include "ser_dev.h"

extern size_t BufferSize;
extern size_t BlockSize;

void ser_event(struct ser_dev *dev)
{
	wake_up_all(&dev->r_wait);
}

size_t ser_next_read_size(struct ser_dev *dev)
{
	size_t size;
	size = *(uint32_t*)dev->buf_ptr;
	return size;
}

void* ser_next_read_ptr(struct ser_dev *dev)
{
	return dev->buf_ptr+sizeof(uint32_t);
}

size_t ser_pop_read_data(struct ser_dev *dev, size_t size)
{
	if(size>0)
	{
		*((uint32_t*)dev->buf_ptr)=0;
		if(dev->buf_ptr + BlockSize >= dev->buf_end)
			dev->buf_ptr = dev->buf_start;
		else
			dev->buf_ptr += BlockSize;
	}
	return size;
}

int ser_start(struct ser_dev *dev)
{
	struct ser_regs *regs = dev->base_addr;

	dev->buf_start = dma_zalloc_coherent(dev->parent, BufferSize,
			&dev->buf_phys, GFP_KERNEL);
	if(!dev->buf_start){
		dev_err(dev->dev, "not enough memory\n");
		return -1;
	}

	dev->buf_end = dev->buf_start+BufferSize;
	dev->buf_ptr = dev->buf_start;

	dev_err(dev->dev,"DMA memory 0x%08x - 0x%08x\n", 
		dev->buf_phys, dev->buf_phys+BufferSize);

/*
	regs->block_size= BlockSize;
	regs->start_address = dev->buf_phys;
	regs->end_address = dev->buf_phys+BufferSize;
	regs->ctrl = 1;
*/
	return 0;
}

int ser_stop(struct ser_dev *dev)
{
	struct ser_regs *regs = dev->base_addr;

/*
	regs->ctrl = 0;
	while(regs->stat & 0x1)
		msleep(1);
*/
		
	dma_free_coherent(dev->parent, BufferSize, dev->buf_start, dev->buf_phys);
	return 0;
}

