#include <stdio.h>
#include "structure.h"
#include "superblock.h"
#include <stdlib.h>
#include "bitmap.h"
#include "allocation.h"

#define FILENAME "test.dat"

/*
 * TODO:
 *      Kontrola alokace clusterů pro inode (OTESTOVAT)
 *          Opravit iteraci level 2
 *          optimalizace iterace level 2
 *              přečíst zda je poslední prvek v levelu 2 nastaven na nenulovou hodnotu -> nenulová hodnota = skip
 *      Výpočet ukazatele v inode na základě indexu
 *          1-5 -> direct [1-5]
 *          6 - 1030 -> indirect1
 *          1031 - 2055 -> indirect2 0
 *          2056 - 3080 -> indirect2 1
 *          etc
 */


int main() {
    //const int32_t disk_size = 629145600;                                // 600MB
    //const int32_t disk_size = 52428800;                                 // 50MB
    const int32_t disk_size = 10485760;                                 // 10MB
    //const int32_t  disk_size = 65536;                                     // 64KB

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

    /*
    // Základ
    printf("\n\n\n");
    bitmap_print(FILENAME);
    printf("Remaining: %d\n", bitmap_set(FILENAME, 0, 15, TRUE));
    bitmap_print(FILENAME);
    printf("Remaining: %d\n", bitmap_set(FILENAME, 2, 5, FALSE));
    bitmap_print(FILENAME);
    printf("Index 3: %d\n", bitmap_get(FILENAME, 3));
    printf("Index 8: %d\n", bitmap_get(FILENAME, 8));
    printf("Index 20: %d\n", bitmap_get(FILENAME, 20));
    // Volné clustery
    printf("Volný cluster index (EXPECTED 2): %d\n", bitmap_find_free_cluster_index(FILENAME));
    bitmap_set(FILENAME, 0, 1, FALSE);
    printf("Volný cluster index (EXPECTED 0): %d\n", bitmap_find_free_cluster_index(FILENAME));
    bitmap_set(FILENAME, 0, 30, TRUE);
    printf("Volný cluster index (EXPECTED -4): %d\n", bitmap_find_free_cluster_index(FILENAME));
    bitmap_set(FILENAME, 0, 30, FALSE);
    */

    /*
     * [INODE]
     */
    /*
    // Zapisování
    struct inode *inode_ptr_write = malloc(sizeof(struct inode));
    int32_t inode_free = inode_find_free_index(FILENAME);
    printf("Free inode index 0: %d\n", inode_free);
    inode_ptr_write->id = inode_free+1;
    inode_write_to_index(FILENAME, inode_free, inode_ptr_write);
    // Vyčerpání místa
    while(1){
        memset(inode_ptr_write, 0, sizeof(struct inode));
        inode_free = inode_find_free_index(FILENAME);
        printf("Free inode index %d: %d\n", inode_free, inode_free);
        inode_ptr_write->id = inode_free+1;
        inode_write_to_index(FILENAME, inode_free, inode_ptr_write);

        if(inode_free < 0){
            break;
        }
    }
    // Uvolnění místa
    memset(inode_ptr_write, 0, sizeof(struct inode));
    inode_write_to_index(FILENAME, 33, inode_ptr_write);
    // Vyhledání místa
    inode_free = inode_find_free_index(FILENAME);
    printf("Free inode index (expected 33): %d\n", inode_free);


    // Kontrola zapisované adresy
    struct superblock *sb = malloc(sizeof(struct superblock));
    sb = superblock_from_file(FILENAME);
    int check_addr = sb->data_start_address;
    while(check_addr < sb->disk_size){
        int32_t check_addr_result = inode_data_index_from_address(FILENAME, check_addr);

        if(check_addr_result > -1){
            printf("Address %d can be translated to index %d\n", check_addr, check_addr_result);
        }

        check_addr++;
    }
    */

    /*
     * [PRÁCE S INODE - ZÁPIS]
     */
    struct inode *iinode = malloc(sizeof(struct inode));
    memset(iinode, 0, sizeof(struct inode));
    int32_t iinode_free_index = inode_find_free_index(FILENAME);
    iinode->id = iinode_free_index + 1;
    inode_write_to_index(FILENAME, iinode_free_index, iinode);

    while(1){
        int32_t iicluster = bitmap_find_free_cluster_index(FILENAME);
        int32_t iiaddr = bitmap_index_to_cluster_address(FILENAME, iicluster);

        int32_t iiaddr_add_result = inode_add_data_address(FILENAME, iinode, iiaddr);

        if(iiaddr_add_result == 0){
        }
        else{
            printf("Test ended with code %d\n", iiaddr_add_result);
            break;
        }
    }

    free(iinode);

    /*
     * [ALOKACE]
     */
    //printf("Počet alokovaných clusterů: %d\n",allocate_bytes(FILENAME, 4097, NULL));


}