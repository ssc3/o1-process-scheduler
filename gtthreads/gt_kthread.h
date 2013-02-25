#ifndef __GT_KTHREAD_H
#define __GT_KTHREAD_H

#include <stdlib.h>

#define GT_MAX_CORES	16
#define GT_MAX_KTHREADS GT_MAX_CORES

typedef unsigned int kthread_t;

/**********************************************************************/
/* kthread_context */

/* kthread flags */
#define KTHREAD_DONE 0x01 /* Done scheduling. Don't relay signal to this kthread. */

typedef struct __kthread_context
{
	unsigned int cpuid;
	unsigned int cpu_apic_id;
	unsigned int pid;
	unsigned int tid;

	unsigned int kthread_flags;
	void (*kthread_app_func)(void *); /* kthread application function */
	void (*kthread_sched_timer)(int); /* vtalrm signal handler */
	void (*kthread_sched_relay)(int); /* relay(usr1) signal handler*/

	void (*kthread_runqueue_balance)(); /* balance across kthread runqueues */
	sigjmp_buf kthread_env; /* kthread's env to jump to (when done scheduling) */

	kthread_runqueue_t krunqueue;
} kthread_context_t;


/* kthread to cpu context mapping */
extern kthread_context_t *kthread_cpu_map[];
/**********************************************************************/

/* XXX: Move to gt_sched.[ch] */
/* kthreads(virtual processors) share some amount of scheduling 
 * (and other) information.
 * Data properties :
 * M : Data piece written by (m)ultiple cpus. Acquire spinlock.
 * S : Data piece written (s)ingle cpu (eg. Schedule Master). No spinlock needed. */

/* XXX: For kthread runqueue balancing, we need to share more information. 
 * I am planning to provide a minimal dynamic runqueue balancing, but which 
 * doesn't take co-scheduling advantage into account. */
typedef struct __ksched_shared_info
{
	unsigned int uthread_select_criterion; /* (S) : currently just a uthread_group_id */
	unsigned int uthread_group_penalty; /* (M) : penalty for co-scheduling a lower priority uthread */
	unsigned int kthread_tot_uthreads; /* (M) : Set if atleast one uthread was created */
	unsigned int kthread_cur_uthreads; /* (M) : Current uthreads (over all kthreads) */

	unsigned short last_ugroup_kthread[MAX_UTHREAD_GROUPS]; /* (M) : Target cpu for next uthread from group */

	gt_spinlock_t ksched_lock; /* global lock for updating above counters */
	gt_spinlock_t uthread_init_lock; /* global lock for uthread_init (to serialize signal handling stuff in there) */

	gt_spinlock_t __malloc_lock; /* making malloc thread-safe (check particular glibc to see if needed) */
	unsigned int reserved[2];
} ksched_shared_info_t;


extern ksched_shared_info_t ksched_shared_info;

/**********************************************************************/
/* create a kthread */
extern int kthread_create(kthread_t *tid, int (*start_fun)(void *), void *arg);

/**********************************************************************/
/* apic-id of the cpu on which kthread is running (kthread_cpu_map) */
static inline unsigned char kthread_apic_id(void)
{
/* IO APIC id is unique for a core and can be used as cpuid. 
 * EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique.
 * Initial APIC ID for the processor this code is running on.*/
#define INITIAL_APIC_ID_BITS  0xFF000000


	unsigned int Regebx = 0;

	__asm__ __volatile__ (
		"movl $1, %%eax\n\t"
		"cpuid"
		:"=b" (Regebx)
		: :"%eax","%ecx","%edx");

	return((unsigned char)((Regebx & INITIAL_APIC_ID_BITS) >> 24));
#undef INITIAL_APIC_ID_BITS
}


/**********************************************************************/
/* Thread-safe malloc */
static inline void *MALLOC_SAFE(unsigned int size)
{
	void *__ptr;
	gt_spin_lock(&(ksched_shared_info.__malloc_lock));
	__ptr = malloc(size);
	gt_spin_unlock(&(ksched_shared_info.__malloc_lock));
	return(__ptr);
}

/* Zeroes out allocated bytes */
static inline void *MALLOCZ_SAFE(unsigned int size)
{
	void *__ptr;
	gt_spin_lock(&(ksched_shared_info.__malloc_lock));
	__ptr = calloc(1, size);
	gt_spin_unlock(&(ksched_shared_info.__malloc_lock));
	return(__ptr);
}

/**********************************************************************/
/* gt-thread api(s) */
extern void gtthread_app_init();
extern void gtthread_app_exit();

#endif
