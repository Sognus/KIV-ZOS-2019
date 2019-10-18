#include <stdio.h>
#include "structure.h"
#include "superblock.h"
#include <stdlib.h>

int main() {
    const int32_t disk_size = 629145600;                                // 600MB
    struct superblock *ptr = superblock_impl_alloc(disk_size);
    structure_calculate(ptr);
    vfs_create("test.dat",ptr);
    free(ptr);

    return 0;
}