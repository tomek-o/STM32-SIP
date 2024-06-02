#include "mem_stat.h"
#include "shell.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>

void mem_stat_dump(const char* file, int line)
{
    HeapStats_t stats;
    vPortGetHeapStats(&stats);

    const char* ptr = strrchr(file, '\\');
    if (ptr) {
        // strip full path from file name
        file = ptr + 1;
    }

    printf("OS HEAP (%s/%04d): avail. %u, min. %u, free blocks: %u, range %u...%u, allocs/frees: %u/%u (delta %u)\n",
           file, line,
           stats.xAvailableHeapSpaceInBytes, stats.xMinimumEverFreeBytesRemaining,
           stats.xNumberOfFreeBlocks,
           stats.xSizeOfSmallestFreeBlockInBytes, stats.xSizeOfLargestFreeBlockInBytes,
           stats.xNumberOfSuccessfulAllocations, stats.xNumberOfSuccessfulFrees,
           stats.xNumberOfSuccessfulAllocations - stats.xNumberOfSuccessfulFrees
           );
}


static enum shell_error sh_mem_stat_dump(int argc, char ** argv) {
    mem_stat_dump(__FILE__, __LINE__);
    return SHELL_ERR_NONE;
}

static __attribute__((constructor)) void registerCmd(void)
{
	shell_add("mem_stat_dump", sh_mem_stat_dump, "Show heap usage stats");
}
