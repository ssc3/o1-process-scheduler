#include "gt_spinlock.h"

/* (http://www.intel.com/cd/ids/developer/asmo-na/eng/dc/threading/333935.htm)
 * With XCHG and CMPXCHG instructions, lock prefix is implicit when used with a 
 * memory operand.
 * Also, spin-locking can be made more efficient by spinning on volatile(dirty) read
 * rather than spinning on atomic instructions (as in the link above). */

/* Locking has no effect on uniprocessor. */
/* CANT EXECUTE A BUS LOCKING INSTRUCTION FROM USER MODE SO THIS IS 
 * THE BEST IMPLEMENTATION OF SPINLOCKS I COULD COME UP WITH
 * HAVE NO EFFECT ON A UNIPROCESSOR SYSTEM AS WE DONT NEED LOCKING THERE */

extern int gt_spinlock_init(gt_spinlock_t* spinlock)
{
	if(!spinlock)
		return -1;
	spinlock->locked = 0;
	return 0;
}

/* spinlock function with backoff */
void gt_actual_spinlock(volatile int * spinlock)
{
	int sp_val = 1;
 
	while(sp_val)
	{
		__asm__ __volatile__ ("pause\n");
		if(!*((volatile int *)spinlock))
		{
		__asm__ __volatile__ (
			"movl $0x01, %%eax\n"
			"xchg %%eax, (%1)\n"
			"movl %%eax, %0\n"
			:"=m"(sp_val) /* 0 */
			: "r"(spinlock) /* 1 */
			: "%eax"); /* No modified flags or clobbered registers */
		}
	}
	return;
}

extern int gt_spin_lock(gt_spinlock_t* spinlock)
{
	if(!spinlock)
		return -1;
	gt_actual_spinlock(&(spinlock->locked));
}

extern int gt_spin_unlock(gt_spinlock_t *spinlock)
{
	if(!spinlock)
		return -1;
	
	if(spinlock->locked) 
		spinlock->locked = 0;
	   
	return 0;
}


int gt_actual_spinlock_custom(volatile int * spinlock)
{
	int sp_val = 1;
 
	while(sp_val)
	{
		__asm__ __volatile__ ("pause\n");
		if(!*((volatile int *)spinlock))
		{
		__asm__ __volatile__ (
			"movl $0x01, %%eax\n"
			"xchg %%eax, (%1)\n"
			"movl %%eax, %0\n"
			:"=m"(sp_val) /* 0 */
			: "r"(spinlock) /* 1 */
			: "%eax"); /* No modified flags or clobbered registers */
		}
		else
		return 0;
	}
	return 1;
}


extern int gt_spin_lock_custom(gt_spinlock_t* spinlock)
{
	if(!spinlock)
		return -1;
	int temp = gt_actual_spinlock_custom(&(spinlock->locked));
	return temp;	
}
