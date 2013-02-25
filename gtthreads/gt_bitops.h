#ifndef __GT_BITOPS_H
#define __GT_BITOPS_H

/* None of these operations are atomic */

#define SET_BIT(mask, off)				\
do							\
{							\
	__asm__ __volatile__ ( "btsl %1,%0\n"		\
				:"=m" (mask) /* 0 */	\
				:"Ir" (off)  /* 1 */	\
				:"%cc");		\
} while(0)

#define RESET_BIT(mask, off)				\
do							\
{							\
	__asm__ __volatile__ ( "btrl %1,%0\n"		\
				:"=m" (mask) /* 0 */	\
				:"Ir" (off)  /* 1 */	\
				:"%cc");		\
} while(0)

/* XXX: Can use bt instruction */
#define IS_BIT_SET(mask, off) ((mask) & (1<<off))

/* Least bit set corresponds to the highest priority */
#define LOWEST_BIT_SET(mask)			\
({						\
	unsigned int inx;			\
	__asm__ __volatile__("bsfl %1,%0\n"	\
				:"=r"(inx)	\
				:"r"(mask)	\
				:"%cc");	\
	inx;					\
})

#endif
