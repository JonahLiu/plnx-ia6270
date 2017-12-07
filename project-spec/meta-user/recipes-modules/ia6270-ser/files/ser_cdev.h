#ifndef _SER_CDEV_H_
#define _SER_CDEV_H_

#include "ser_dev.h"

int ser_setup_cdev(void);
void ser_cleanup_cdev(void);
int ser_add_cdev(struct ser_dev *dev);
int ser_del_cdev(struct ser_dev *dev);

#endif /* _SER_CDEV_H */
