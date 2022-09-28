#include "mem_stat.h"
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
