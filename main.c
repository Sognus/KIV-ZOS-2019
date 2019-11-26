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
    const int32_t disk_size = 52428800;                                 // 50MB
    //const int32_t disk_size = 10485760;                                 // 10MB
    //const int32_t  disk_size = 65536;                                     // 64KB

    /*
     * [TEST VYTVOŘENÍ VFS]
     */

    struct superblock *ptr = superblock_impl_alloc(disk_size);
    structure_calculate(ptr);
    superblock_print(ptr);
    vfs_create(FILENAME,ptr);

    /*
     * [TEST VYTVOŘENÍ SLOŽKY]
     */
    directory_create(FILENAME, "/");
    directory_entries_print(FILENAME, "/");

    /*
     * [ALOKACE DATABLOKŮ PRO TEST VFS_IO
     */
    struct inode *iinode = inode_read_by_index( FILENAME,0);
    for(int i = 0; i < 100; i++){
        int32_t free_index = bitmap_find_free_cluster_index(FILENAME);
        int32_t free_address = bitmap_index_to_cluster_address(FILENAME, free_index);

        inode_add_data_address(FILENAME, iinode, free_address);
    }

    /*
     * [ TEST VFS_IO]
     */

    // Příprava dat
    FILE *fff = fopen(FILENAME, "r+b");
    char *data = "data123pocitac243omalovanky989ahojtest56658";
    fseek(fff, 2097153, SEEK_SET);
    fwrite(data, strlen(data), 1, fff);
    fclose(fff);

    // Test
    VFS_FILE *file = vfs_open_inode(FILENAME, 1);
    char *buffer = malloc(sizeof(char) * 32 + 1);
    memset(buffer, 0, sizeof(char) * 32 + 1);
    vfs_seek(file, -5, SEEK_END);
    vfs_read(buffer, 5,1, file);
    printf("Read data: %s\n", buffer);

    //int32_t ggg = inode_get_datablock_index_value(FILENAME, file->inode_ptr, 1050);
    //log_debug("TEST_DATABLOCK_INDEX: %d\n", ggg);

    vfs_close(file);
    free(buffer);
    free(ptr);
    free(iinode);
}