#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_signal.h"


/**********************************************************************/
/* kthread signal handling */

extern void kthread_install_sighandler(int signo, void (*handler)(int))
{
	sigset_t set;
	struct sigaction act;

	/* Setup the handler */
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	sigaction(signo, &act,0);

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}

extern void kthread_block_signal(int signo)
{
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

	return;
}

extern void kthread_unblock_signal(int signo)
{
	sigset_t set;

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}

extern void kthread_init_vtalrm_timeslice(int kthread_vtalrm_usec)
{
	struct itimerval timeslice;
	unsigned int sysctl_sched_min_granularity = 4000000ULL;

	timeslice.it_interval.tv_sec = KTHREAD_VTALRM_SEC;
	//timeslice.it_interval.tv_usec = KTHREAD_VTALRM_USEC;
	timeslice.it_interval.tv_usec = kthread_vtalrm_usec;
	timeslice.it_value.tv_sec = KTHREAD_VTALRM_SEC;
	//timeslice.it_value.tv_usec = KTHREAD_VTALRM_USEC;
	timeslice.it_value.tv_usec = kthread_vtalrm_usec;

	setitimer(ITIMER_VIRTUAL,&timeslice,NULL);
	return;
}

