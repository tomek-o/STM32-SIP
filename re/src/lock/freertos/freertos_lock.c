#include <re_types.h>
#include <re_mem.h>
#include <re_lock.h>
#include "stm32f4xx_hal.h"


struct lock {
	int dummy;
};


static void lock_destructor(void *data)
{

}


int lock_alloc(struct lock **lp)
{
	struct lock *l;

	if (!lp)
		return EINVAL;

	l = mem_alloc(sizeof(*l), lock_destructor);
	if (!l)
		return ENOMEM;

	*lp = l;
	return 0;
}


void lock_read_get(struct lock *l)
{
    __disable_irq();
}


void lock_write_get(struct lock *l)
{
    __disable_irq();
}

void lock_rel(struct lock *l)
{
    __enable_irq();
}
