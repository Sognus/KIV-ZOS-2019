#include <stdio.h>
#include "structure.h"
#include "superblock.h"
#include <stdlib.h>
#include "bitmap.h"
#include "allocation.h"
#include "directory.h"
#include "vfs_io.h"

#define FILENAME "test.dat"

int main() {
    //const int32_t disk_size = 629145600;                                // 600MB
    //const int32_t disk_size = 52428800;                                 // 50MB
    //const int32_t disk_size = 10485760;                                 // 10MB
    const int32_t  disk_size = 65536;                                     // 64KB

    /*
     * [TEST VYTVOŘENÍ VFS]
     */

    struct superblock *ptr = superblock_impl_alloc(disk_size);
    structure_calculate(ptr);
    superblock_print(ptr);
    vfs_create(FILENAME, ptr);

    /*
     * [TEST VYTVOŘENÍ SLOŽKY]
     */
    directory_create(FILENAME, "/");
    directory_entries_print(FILENAME, "/");

    /*
     * [UVOLNĚNÍ ZDROJŮ
     */
    free(ptr);
}