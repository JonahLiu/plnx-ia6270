#ifndef _SER_DEV_H_
#define _SER_DEV_H_

#include <linux/list.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

struct ser_regs
{
	volatile uint32_t ctrl;
	volatile uint32_t stat;
	volatile uint32_t block_size;
	volatile uint32_t rsv;
	volatile uint32_t start_address;
	volatile uint32_t end_address;
};

struct ser_dev
{
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;

	struct device *dev;
	struct cdev cdev;

	struct mutex r_mutex;
	wait_queue_head_t r_wait;

	dma_addr_t buf_phys;
	void *buf_start;
	void *buf_end;
	void *buf_ptr;
	
};

void ser_event(struct ser_dev *dev);
size_t ser_next_read_size(struct ser_dev *dev);
size_t ser_pop_read_data(struct ser_dev *dev, size_t size);
void* ser_next_read_ptr(struct ser_dev *dev);
int ser_start(struct ser_dev *dev);
int ser_stop(struct ser_dev *dev);

#endif /* _SER_DEV_H */
