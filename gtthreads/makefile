all : gt_include.h gt_kthread.c gt_kthread.h gt_uthread.c gt_uthread.h gt_pq.c gt_pq.h gt_signal.h gt_signal.c gt_spinlock.h gt_spinlock.c gt_matrix.c
	@echo Building...
	@gcc -g -o matrix gt_matrix.c gt_kthread.c gt_pq.c gt_signal.c gt_spinlock.c gt_uthread.c -lrt -lm
	@echo Done!
	@echo Now run './matrix'

.PHONY : clean

clean :
	@rm matrix
	@echo Cleaned!
