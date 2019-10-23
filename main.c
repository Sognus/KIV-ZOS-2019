#include <stdio.h>
#include "structure.h"
#include "superblock.h"
#include <stdlib.h>
#include "bitmap.h"

#define FILENAME "test.dat"

/*
 * TODO:
 *      Vyřešit alokaci větších souborů, zápis datových odkazů
 *          1) Kontrola rozšíření alokace clusterů složky pro více než CLUSTER_SIZE / (DIR_ITEM) -> 4096 / 16 -> 256 souborů
 *          2) Kontrola alokace clusterů pro soubor
 */


int main() {

    //const int32_t disk_size = 629145600;                                // 600MB
    //const int32_t disk_size = 52428800;                                 // 50MB
    //const int32_t disk_size = 10485760;                                 // 10MB
    const int32_t  disk_size = 65536;                                     // 64KB

    /*
     * [TEST VYTVOŘENÍ SOUBORU]
     */

    struct superblock *ptr = superblock_impl_alloc(disk_size);
    structure_calculate(ptr);
    superblock_print(ptr);
    vfs_create(FILENAME,ptr);
    free(ptr);

    /*
     * [TEST BITMAPY]
     */
    printf("\n\n\n");
    bitmap_print(FILENAME);
    printf("Remaining: %d\n", bitmap_set(FILENAME, 0, 15, TRUE));
    bitmap_print(FILENAME);
    printf("Remaining: %d\n", bitmap_set(FILENAME, 2, 5, FALSE));
    bitmap_print(FILENAME);
    printf("Index 3: %d\n", bitmap_get(FILENAME, 3));
    printf("Index 8: %d\n", bitmap_get(FILENAME, 8));
    printf("Index 20: %d\n", bitmap_get(FILENAME, 20));
}