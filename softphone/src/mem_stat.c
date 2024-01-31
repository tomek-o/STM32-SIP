#include "mem_stat.h"
#include "shell.h"
#include "FreeRTOS.h"
#include <stdio.h>

void mem_stat_dump(void)
{
    HeapStats_t stats;
    vPortGetHeapStats(&stats);

    printf("OS HEAP: avail. %u, min. %u, free blocks: %u, range %u...%u, allocs/frees: %u/%u (delta %u)\n",
           stats.xAvailableHeapSpaceInBytes, stats.xMinimumEverFreeBytesRemaining,
           stats.xNumberOfFreeBlocks,
           stats.xSizeOfSmallestFreeBlockInBytes, stats.xSizeOfLargestFreeBlockInBytes,
           stats.xNumberOfSuccessfulAllocations, stats.xNumberOfSuccessfulFrees,
           stats.xNumberOfSuccessfulAllocations - stats.xNumberOfSuccessfulFrees
           );
}


static enum shell_error sh_mem_stat_dump(int argc, char ** argv) {
    mem_stat_dump();
    return SHELL_ERR_NONE;
}

static __attribute__((constructor)) void registerCmd(void)
{
	shell_add("mem_stat_dump", sh_mem_stat_dump, "Show heap usage stats");
}
