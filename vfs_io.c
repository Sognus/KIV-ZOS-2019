#include "vfs_io.h"
#include <string.h>
#include <stdlib.h>
#include "parsing.h"
#include "superblock.h"

/**
 * Nastaví offset pro strukturu VFS_FILE
 *
 * @param vfs_file ukazatel na strukturu VFS_FILE
 * @param offset vstupní hodnota nastavení
 * @param type typ nastavení (typy dodržují SEEK_* v stdio.h)
 * @return (return < 0: chyba | return >= 0: v pořádku)
 */
int32_t vfs_seek(VFS_FILE *vfs_file, int64_t offset, int type) {
    // Kontrola ukazatele na strukturu VFS_FILE_TYPE
    if (vfs_file == NULL) {
        return -1;
    }

    // Použití dočasné proměnné -> při chybě neměnit vfs_file->offset
    int64_t temp_offset = vfs_file->offset;

    // Nastavení hodnoty na vstup - <0, FILESIZE>
    if (type == SEEK_SET) {
        temp_offset = offset;
    }

    // Posun dle aktuálního offsetu <-inf, +inf>
    if (type == SEEK_CUR) {
        temp_offset += offset;
    }

    // Posun od hodnoty FILESIZE <-inf,0>
    if (type == SEEK_END) {
        temp_offset = vfs_file->inode_ptr->file_size + offset;
    }

    // Kontrola hodnoty - nelze mít záporný absolutní offset
    if (temp_offset < 0) {
        return -2;
    }

    // Kontrola hodnoty - nelze ukazovat za konec souboru
    if (temp_offset > vfs_file->inode_ptr->file_size) {
        return -3;
    }

    // Vše v pořádku, provedeme zápis do VFS_FILE_TYPE
    vfs_file->offset = temp_offset;

    // Indikujeme výsledek
    return 0;
}

/**
 * Přečte daný počet struktur dané velikosti ze souboru vfs_file uloženého ve virtuálním FS
 *
 * @param destination ukazatel na místo uložení
 * @param read_item_size velikost čtených dat
 * @param read_item_count počet opakování při čtení dat
 * @param vfs_file ukazatel na soubor
 * @return počet přečtených byte
 */
size_t vfs_read(void *destination, size_t read_item_size, size_t read_item_count, VFS_FILE *vfs_file) {
    // Kontrola ukazatele na strukturu VFS_FILE_TYPE
    if(vfs_file == NULL){
        return -1;
    }

    // Kontrola ukazatele na místo v paměti pro uložení výsledku
    if (destination == NULL) {
        return -2;
    }

    // Velikost čtení nemůže být menší jak 1
    if(read_item_size < 1){
        return -3;
    }

    // Počet čtení nemůže být menší jak 1
    if(read_item_count < 1){
        return -4;
    }

    // Ziskani superbloku
    struct superblock *superblock_ptr = superblock_from_file(vfs_file->vfs_file);

    // Kontrola čtení superbloku
    if(superblock_ptr == NULL){
        return -5;
    }

    int32_t temp_offset = vfs_file->offset;
    int32_t temp_filesize = vfs_file->inode_ptr->file_size;
    int32_t temp_total_read_size = read_item_size * read_item_count;
    int32_t temp_can_read = temp_filesize - temp_offset;

    //Pokud je třeba číst víc než můžeme, přečteme pouze to co můžeme
    if(temp_total_read_size > temp_can_read){
        temp_total_read_size = temp_can_read;
    }

    // Zastavíme funkci pokud jsme za koncem souboru
    if(temp_total_read_size < 1){
        free(superblock_ptr);
        log_trace("vfs_read: Povolena velikost cteni je mensi nez 1 byte (pravdepodobne chybny offset)!\n");
        return -6;
    }

    // Výpočet v případě čtení vícera databloků
    int32_t skipped_datablocks = temp_offset / superblock_ptr->cluster_size;
    int32_t first_datablock_offset = temp_offset - (skipped_datablocks * superblock_ptr->cluster_size);
    int32_t first_datablock_can_read = superblock_ptr->cluster_size - first_datablock_offset;

    // Logging
    log_trace("vfs_read: Offset -> %d, Size -> %d, Total Read -> %d, Can read -> %d\n", temp_offset, temp_filesize, temp_total_read_size, temp_can_read);
    log_trace("vfs_read: skipped_datablock -> %d\n", skipped_datablocks);
    log_trace("vfs_read: first_datablock_offset -> %d\n", first_datablock_offset);
    log_trace("vfs_read: first_datablock_can_read -> %d\n", first_datablock_can_read);

    // Otevření vfs souboru pro čtení
    FILE *file = fopen(vfs_file->vfs_file, "r+b");

    if(file == NULL){
        free(superblock_ptr);
        return -7;
    }

    // Všechna data můžeme přečíst z prvního data bloku
    if(temp_total_read_size <= first_datablock_can_read){
        log_trace("vfs_read: Can read all data from first datablock\n");

        // Z kterého databloku budeme číst
        int32_t datablock_address = inode_get_datablock_index_value(vfs_file->vfs_file, vfs_file->inode_ptr, skipped_datablocks);
        // Přičteme offset k adrese
        int32_t datablock_direct_adress = datablock_address + first_datablock_offset;

        // Nastavíme adresu čtení z vfs souboru
        fseek(file, datablock_direct_adress, SEEK_SET);
        // Přečteme data
        fread(destination, read_item_size, read_item_count, file);
        // Logging
        log_trace("vfs_read: Celkem precteno %d byte z 1 databloku.\n", (read_item_count * read_item_size));

    }
    else{
        // Výpočet kolik byte zbývá přečíst po 1. databloku
        int32_t remaining_read = temp_total_read_size - first_datablock_can_read;
        // Kolik celých databloků můžeme přečíst
        int32_t datablocks_count_read = remaining_read / superblock_ptr->cluster_size;
        // Last datablock read
        int32_t last_datablock_remaining = remaining_read - (superblock_ptr->cluster_size * datablocks_count_read);

        // Logging
        log_trace("vfs_read: Remaining data after first datablock -> %d\n", remaining_read);
        log_trace("vfs_read: Whole remaining datablocks -> %d\n", datablocks_count_read);
        log_trace("vfs_read: Last datablock bytes -> %d\n", last_datablock_remaining);

        // Alokace paměti pro čtení
        char *buffer = malloc(sizeof(char) * temp_total_read_size);
        // Nulování buffer paměti
        memset(buffer, 0, sizeof(char) * temp_total_read_size);
        // Vytvoření iteračního ukazatele
        char *buffer_seek = buffer;
        // Zbývající počet byte ke čtení
        int32_t read_remaining = temp_total_read_size;

        // Čtení dat z prvního databloku
        int32_t first_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_file, vfs_file->inode_ptr, skipped_datablocks);
        // Přičteme offset k adrese
        int32_t datablock_direct_adress = first_datablock_address + first_datablock_offset;
        // Nastavení offsetu
        fseek(file, datablock_direct_adress, SEEK_SET);
        // Přečtení prvního databloku
        fread(buffer_seek, first_datablock_can_read, 1, file);
        buffer_seek += first_datablock_can_read;
        read_remaining -= first_datablock_can_read;

        log_trace("vfs_read: Remaining after first data block was read: %d\n", read_remaining);

        int32_t curr_datablock_index = skipped_datablocks + 1;
        // Čtení celých databloků pokud je potřeba
        while(read_remaining >= superblock_ptr->cluster_size){
            // Získání adresy dalšího bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_file, vfs_file->inode_ptr, curr_datablock_index);

            // Nastavení offsetu
            fseek(file, curr_datablock_address, SEEK_SET);
            // Přečtení celého data bloku
            fread(buffer_seek, superblock_ptr->cluster_size, 1, file);

            // Posun na další data blok
            read_remaining -= superblock_ptr->cluster_size;
            buffer_seek += superblock_ptr->cluster_size;
            log_trace("vfs_read: Precteni databloku na indexu %d, zbyvajici pocet byte: %d\n", curr_datablock_index, read_remaining);
            curr_datablock_index += 1;

        }

        // Přečtení posledního data bloku pokud je nutné
        if(read_remaining > 0 && read_remaining < superblock_ptr->cluster_size) {
            // Získání adresy posledního data bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_file, vfs_file->inode_ptr,
                                                                             curr_datablock_index);

            // Nastavení offsetu
            fseek(file, curr_datablock_address, SEEK_SET);
            // Přečtení zbylých dat
            fread(buffer_seek, read_remaining, 1, file);

            // Posun na konec kvůli ověřování
            buffer_seek += read_remaining;
            read_remaining -= read_remaining;
            curr_datablock_index += 1;
        }

        // Celkové přečtení
        int32_t successfull_read = buffer_seek - buffer;
        log_trace("vfs_read: Celkem přečteno %d byte\n", successfull_read);

        // Přesun dat z bufferu do destination
        memcpy(destination, buffer, successfull_read);


        // TODO: TEST memcpy z bufferu do výsledku + uvolnění bufferu
        // TODO: Posun seek o velikost čtení

    }

    // Uvolnění zdrojů
    fclose(file);
    free(superblock_ptr);
    return 0;
}

/**
 * Zapíše do souboru vfs_file (do virtuálního FS) danou velikost dat s daným opakováním
 *
 * @param source ukazatel odkud se data budou číst
 * @param write_item_size velikost zapisovaných dat
 * @param write_item_count počet opakování při zápisu
 * @param vfs_file ukazatel na soubor ve VFS
 * @return počet zapsaných byte
 */
size_t vfs_write(void *source, size_t write_item_size, size_t write_item_count, VFS_FILE *vfs_file) {
    // TODO: implement vfs_write
    return 0;
}

/**
 * Vytvoří kontext pro práci souboru - vždycky lze provádět čtení i zápis zároveň
 *
 * @param vfs_file cesta k souboru vfs.dat (apod.)
 * @param vfs_path cesta souboru uvnitř virtuálního filesystému.
 * @return (VFS_FILE* | NULL)
 */
VFS_FILE *vfs_open(char *vfs_file, char *vfs_path) {
    // Kontrola délky názvu souboru
    if (strlen(vfs_file) < 1) {
        log_debug("vfs_open_inode: Cesta k souboru VFS nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    // Ověření existence souboru
    if (file_exist(vfs_file) != TRUE) {
        log_debug("vfs_open_inode: Soubor VFS %s neexistuje!\n", vfs_file);
        return NULL;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(vfs_file);

    // Ověření ziskání superbloku
    if (superblock_ptr == NULL) {
        log_debug("vfs_open_inode: Nepodarilo se precist superblock z VFS!\n");
        return NULL;
    }

    VFS_FILE *vfs_file_open = malloc(sizeof(struct VFS_FILE));

    if (vfs_file_open == NULL) {
        free(superblock_ptr);
        log_debug("vfs_open_inode: Nepodarilo se alokovat pamet pro strukturu VFS_FILE_TYPE!\n");
        return NULL;
    }

    // Otevření root složky
    if (strcmp(vfs_path, "/") == 0) {
        return vfs_open_inode(vfs_file, 1);
    } else {
        // tODO: implement - open nonroot folders (need vfs_write, vfs_read completed first)
    }

    return 0;
}

/**
 * Vytvoří kontext pro práci souboru - vždycky lze provádět čtení i zápis zároveň
 *
 * @param vfs_file cesta k souboru vfs.dat (apod.)
 * @param vfs_path ID inode k otevření
 * @return
 */
VFS_FILE *vfs_open_inode(char *vfs_file, int32_t inode_id) {
    // Kontrola délky názvu souboru
    if (strlen(vfs_file) < 1) {
        log_debug("vfs_open_inode: Cesta k souboru VFS nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    // Ověření existence souboru
    if (file_exist(vfs_file) != TRUE) {
        log_debug("vfs_open_inode: Soubor VFS %s neexistuje!\n", vfs_file);
        return NULL;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(vfs_file);

    // Ověření ziskání superbloku
    if (superblock_ptr == NULL) {
        log_debug("vfs_open_inode: Nepodarilo se precist superblock z VFS!\n");
        return NULL;
    }

    VFS_FILE *vfs_file_open = malloc(sizeof(struct VFS_FILE));

    if (vfs_file_open == NULL) {
        free(superblock_ptr);
        log_debug("vfs_open_inode: Nepodarilo se alokovat pamet pro strukturu VFS_FILE_TYPE!\n");
        return NULL;
    }

    struct inode *inode_ptr = inode_read_by_index(vfs_file, inode_id - 1);

    if (inode_ptr == NULL) {
        free(vfs_file_open);
        free(superblock_ptr);
        log_debug("vfs_open_inode: Nelze precist inode s ID=%d - dana ID neexistuje!\n", inode_id);
        return NULL;
    }

    vfs_file_open->inode_ptr = inode_ptr;
    vfs_file_open->offset = 0;
    vfs_file_open->vfs_file = malloc(sizeof(char) * strlen(vfs_file) +1);
    strcpy(vfs_file_open->vfs_file, vfs_file);

    // Uvolnění zdrojů
    free(superblock_ptr);

    // Návrat VFS_FILE_TYPE
    return vfs_file_open;
}

/**
 * Zavře virtuální soubor a uvolní pamět
 *
 * @param file virtuální soubor k zavření
 * @return indikace výsledku
 */
bool vfs_close(VFS_FILE *file){
    // Ověření zda je třeba uvolňovat
    if(file == NULL){
        return FALSE;
    }

    free(file->inode_ptr);
    free(file->vfs_file);
    free(file);

    return TRUE;
}
