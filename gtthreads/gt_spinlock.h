#ifndef __GT_SPINLOCK_H
#define __GT_SPINLOCK_H

typedef struct __gt_spinlock
{
	volatile int locked;
	unsigned int holder;
} gt_spinlock_t;


extern int gt_spinlock_init(gt_spinlock_t* spinlock);
extern int gt_spin_lock(gt_spinlock_t* spinlock);
extern int gt_spin_unlock(gt_spinlock_t *spinlock);

#endif
