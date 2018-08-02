/*
 * ============================================================================
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#ifndef _LOCK_H_
#define _LOCK_H_

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#define PF_NO_LOCK (0x01)
#define PF_TRY_ONCE_LOCK (0x02)
#define PF_WAIT_FOR_LOCK (0x04)

#define GET_PF_NO_LOCK(flag) (flag & PF_NO_LOCK)
#define GET_PF_TRY_ONCE_LOCK(flag) (flag & PF_TRY_ONCE_LOCK)
#define GET_PF_WAIT_FOR_LOCK(flag) (flag & PF_WAIT_FOR_LOCK)

class LightweightLock {
	public:
		LightweightLock() { locked = 0; }

		/**
		 * Try to acquire a lock once and return even if the lock is busy.
		 * If spin flag is set, then spin until the lock is available.
		 */
		bool lock(uint8_t flag)
		{
			if (GET_PF_WAIT_FOR_LOCK(flag) != PF_WAIT_FOR_LOCK) {
				return !__sync_lock_test_and_set(&locked, 1);
			} else {
				while (__sync_lock_test_and_set(&locked, 1))
					while (locked);
				return true;
			}

			return false;
		}

		void unlock(void)
		{
			__sync_lock_release(&locked);
			return;
		}

	private:
		volatile int locked;
};


#endif
