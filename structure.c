#include "structure.h"
#include <string.h>

/**
 * Na základě celkové velikosti FS vypočítá zbylé parametry VFS
 *
 *
 * @param superblock_ptr
 * @return
 */
bool structure_calculate(struct superblock *superblock_ptr) {

    // Kontrola ukazatele na strukturu
    if(superblock_ptr == NULL){
        debug_print("structure_calculate: Ukazatel na strukturu nesmi byt NULL!\n");
        return FALSE;
    }

    // Kontrola nenulové velikosti cílového VFS
    if(superblock_ptr->disk_size < 1){
        debug_print("structure_calculate: Nelze vytvorit VFS nulove velikosti!\n");
        return FALSE;
    }

    debug_print("structure_calculate: Velikost celeho VFS -> %d\n", superblock_ptr->disk_size);

    // Pokud máme nastavenou velikost clusteru a daná hodnota je validní, použijeme ji
    int32_t vfs_cluster_size = IMPL_CLUSTER_SIZE;
    if(superblock_ptr->cluster_size > 1 && is_two_power(superblock_ptr->cluster_size)) {
        vfs_cluster_size = superblock_ptr->cluster_size;
    }
    debug_print("structure_calculate: Velikost clusteru -> %d\n", vfs_cluster_size);

    // Kontrola zda velikost clusteru je mocninou čísla 2
    if(is_two_power(vfs_cluster_size) == FALSE){
        debug_print("structure_calculate: Velikost clusteru musi byt mocninou cisla 2!!\n");
        return FALSE;
    }

    // Výpočet velikosti hlavičky a datové části
    int32_t vfs_size = superblock_ptr->disk_size;
    int32_t vfs_head_size = (int32_t)(ceil((double)(vfs_size)*((double)(IMPL_NON_DATA_PERCENTAGE)/100)));
    int32_t vfs_data_size = vfs_size - vfs_head_size;

    // DEBUG výpisy
    debug_print("structure_calculate: Velikost hlavicky -> %d\n", vfs_head_size);
    debug_print("structure_calculate: Velikost datove casti -> %d\n", vfs_data_size);

    // Výpočet velikosti clusterů
    int32_t vfs_cluster_count = (int32_t)(ceil((double)(vfs_data_size)/(double)(vfs_cluster_size)));
    debug_print("structure_calculate: Pocet clusteru -> %d\n", vfs_cluster_count);

    // Výpočet adres
    int32_t vfs_bitmap_address = sizeof(struct superblock) + 1;
    int32_t vfs_inode_address = vfs_bitmap_address + (vfs_cluster_count * sizeof(int8_t)) + 1;
    int32_t vfs_head_available = vfs_head_size - vfs_inode_address;
    int32_t vfs_inode_count = (int32_t)(ceil((double)(vfs_head_available/(double)(sizeof(struct inode)))));
    int32_t vfs_data_start = vfs_inode_address + vfs_head_available + 1;

    debug_print("structure_calculate: Adresa bitmapy -> %d\n", vfs_bitmap_address);
    debug_print("structure_calculate: Adresa inode -> %d\n", vfs_inode_address);
    debug_print("structure_calculate: Adresa pocatku dat ->  %d\n", vfs_data_start);
    debug_print("structure_calculate: Volne misto pro inode -> %d (byte)\n", vfs_head_available);
    debug_print("structure_calculate: Pocet inode -> %d\n", vfs_inode_count);

    // Zápis vypočtených hodnot do struktury
    superblock_ptr->cluster_size = vfs_cluster_size;
    superblock_ptr->cluster_count = vfs_cluster_count;
    superblock_ptr->bitmap_start_address = vfs_bitmap_address;
    superblock_ptr->inode_start_address = vfs_inode_address;
    superblock_ptr->data_start_address = vfs_data_start;

    return TRUE;
}

/**
 * Vytvoří soubor s daným názvem, který bude fyzickou reprezentací VFS
 *
 * @param vfs_filename název VFS souboru
 * @param superblock_ptr ukazatel na superblock
 * @return úspěch operace (0 | 1)
 */
bool vfs_create(char *vfs_filename, struct superblock *superblock_ptr) {
    // Kontrola ukazatele superbloku
    if(superblock_ptr == NULL) {
        debug_print("vfs_create: Ukazatel na superblock nesmi byt NULL!\n");
        return FALSE;
    }

    // Kontrola jména souboru
    if(strlen(vfs_filename) < 1){
        debug_print("vfs_create: Jmeno souboru nesmi byt prazdne!\n");
        return FALSE;
    }

    // Kontrola obsahu superbloku
    if(superblock_check(superblock_ptr) == FALSE){
        // Debug zprávy uvnitř funkce
        return FALSE;
    }

    // Soubor existuje, je třeba ho smazat
    if(file_exist(vfs_filename)){
        remove(vfs_filename);
    }

    // Otevření sounoru pro binární zápis
    FILE *vfs_file = fopen(vfs_filename, "wb");

    // Ověření otevření souboru
    if (vfs_file == NULL) {
        debug_print("vfs_create: Nelze otevrit soubor pro zapis!\n");
        return FALSE;
    }

    // Zápis superbloku do souboru
    fseek(vfs_file, 0, SEEK_SET);
    fwrite(superblock_ptr, sizeof(struct superblock), 1, vfs_file);

    // Zápis prázdných dat do zbytku souboru
    int32_t used = sizeof(struct superblock);
    while(used < superblock_ptr->disk_size){
        fputc(0, vfs_file);
        used++;
    }

    // Uzavření souboru po zápisu
    fclose(vfs_file);

    return TRUE;


}