/***
 * Based on DPDK (rte_spinlock.h) && VPP(vppinfra/lock.h)
 */

#ifndef __include_sf_spinlock_h__
#define __include_sf_spinlock_h__

#include <vppinfra/lock.h>


typedef struct {
	volatile int locked; /**< lock status 0 = unlocked, 1 = locked */
} sf_spinlock_t;


/**
 * Initialize the spinlock to an unlocked state.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
sf_spinlock_init(sf_spinlock_t *sl)
{
	sl->locked = 0;
}


/*************************************/
/********** x86_64 *******************/
/*************************************/
#if __x86_64__


static inline void
sf_spinlock_lock(sf_spinlock_t *sl)
{
	int lock_val = 1;
	asm volatile (
			"1:\n"
			"xchg %[locked], %[lv]\n"
			"test %[lv], %[lv]\n"
			"jz 3f\n"
			"2:\n"
			"pause\n"
			"cmpl $0, %[locked]\n"
			"jnz 2b\n"
			"jmp 1b\n"
			"3:\n"
			: [locked] "=m" (sl->locked), [lv] "=q" (lock_val)
			: "[lv]" (lock_val)
			: "memory");
}

static inline void
sf_spinlock_unlock (sf_spinlock_t *sl)
{
	int unlock_val = 0;
	asm volatile (
			"xchg %[locked], %[ulv]\n"
			: [locked] "=m" (sl->locked), [ulv] "=q" (unlock_val)
			: "[ulv]" (unlock_val)
			: "memory");
}



/*************************************/
/********** ARM    *******************/
/*************************************/
#else //ARM

/**
 * Take the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static_always_inline void
sf_spinlock_lock(sf_spinlock_t *sl)
{
	while (__sync_lock_test_and_set( &(sl->locked) , 1))
		CLIB_PAUSE ();
}

/**
 * Release the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static_always_inline void
sf_spinlock_unlock (sf_spinlock_t *sl)
{
	__sync_lock_release(&sl->locked);
}


#endif // ARM


#endif //__include_sf_spinlock_h__