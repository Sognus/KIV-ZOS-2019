#include "vfs_io.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "parsing.h"
#include "superblock.h"
#include "bitmap.h"


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

    // Použití dočasné proměnné -> při chybě neměnit vfs_filename->offset
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
    if (vfs_file == NULL) {
        return -1;
    }

    // Kontrola ukazatele na místo v paměti pro uložení výsledku
    if (destination == NULL) {
        return -2;
    }

    // Velikost čtení nemůže být menší jak 1
    if (read_item_size < 1) {
        return -3;
    }

    // Počet čtení nemůže být menší jak 1
    if (read_item_count < 1) {
        return -4;
    }

    // Ziskani superbloku
    struct superblock *superblock_ptr = superblock_from_file(vfs_file->vfs_filename);

    // Kontrola čtení superbloku
    if (superblock_ptr == NULL) {
        return -5;
    }

    int32_t temp_offset = vfs_file->offset;
    int32_t temp_filesize = vfs_file->inode_ptr->file_size;
    int32_t temp_total_read_size = read_item_size * read_item_count;
    int32_t temp_can_read = temp_filesize - temp_offset;

    //Pokud je třeba číst víc než můžeme, přečteme pouze to co můžeme
    if (temp_total_read_size > temp_can_read) {
        temp_total_read_size = temp_can_read;
    }

    // Zastavíme funkci pokud jsme za koncem souboru
    if (temp_total_read_size < 1) {
        free(superblock_ptr);
        log_trace("vfs_read: Povolena velikost cteni je mensi nez 1 byte (pravdepodobne chybny offset)!\n");
        return -6;
    }

    // Výpočet v případě čtení vícera databloků
    int32_t skipped_datablocks = temp_offset / superblock_ptr->cluster_size;
    int32_t first_datablock_offset = temp_offset - (skipped_datablocks * superblock_ptr->cluster_size);
    int32_t first_datablock_can_read = superblock_ptr->cluster_size - first_datablock_offset;

    // Logging
    log_trace("vfs_read: Offset -> %d, Size -> %d, Total Read -> %d, Can read -> %d\n", temp_offset, temp_filesize,
              temp_total_read_size, temp_can_read);
    log_trace("vfs_read: skipped_datablock -> %d\n", skipped_datablocks);
    log_trace("vfs_read: first_datablock_offset -> %d\n", first_datablock_offset);
    log_trace("vfs_read: first_datablock_can_read -> %d\n", first_datablock_can_read);

    // Otevření vfs souboru pro čtení
    FILE *file = fopen(vfs_file->vfs_filename, "r+b");

    if (file == NULL) {
        free(superblock_ptr);
        return -7;
    }

    // Všechna data můžeme přečíst z prvního data bloku
    if (temp_total_read_size <= first_datablock_can_read) {
        log_trace("vfs_read: Can read all data from first datablock\n");

        // Z kterého databloku budeme číst
        int32_t datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                    skipped_datablocks);
        // Přičteme offset k adrese
        int32_t datablock_direct_adress = datablock_address + first_datablock_offset;

        // Nastavíme adresu čtení z vfs souboru
        fseek(file, datablock_direct_adress, SEEK_SET);
        // Přečteme data
        fread(destination, read_item_size, read_item_count, file);
        // Logging
        log_trace("vfs_read: Celkem precteno %d byte z 1 databloku.\n", (read_item_count * read_item_size));
        // Posun offsetu o přečtená data
        vfs_seek(vfs_file, temp_total_read_size, SEEK_CUR);

    } else {
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
        int32_t first_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                          skipped_datablocks);
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
        while (read_remaining >= superblock_ptr->cluster_size) {
            // Získání adresy dalšího bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                             curr_datablock_index);

            // Nastavení offsetu
            fseek(file, curr_datablock_address, SEEK_SET);
            // Přečtení celého data bloku
            fread(buffer_seek, superblock_ptr->cluster_size, 1, file);

            // Posun na další data blok
            read_remaining -= superblock_ptr->cluster_size;
            buffer_seek += superblock_ptr->cluster_size;
            log_trace("vfs_read: Precteni databloku na indexu %d, zbyvajici pocet byte: %d\n", curr_datablock_index,
                      read_remaining);
            curr_datablock_index += 1;

        }

        // Přečtení posledního data bloku pokud je nutné
        if (read_remaining > 0 && read_remaining < superblock_ptr->cluster_size) {
            // Získání adresy posledního data bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
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

        // Posun offsetu o přečtená data
        vfs_seek(vfs_file, successfull_read, SEEK_CUR);

        // Uvolnění zdrojů
        free(buffer);

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
    // Kontrola ukazatele na strukturu VFS_FILE_TYPE
    if (vfs_file == NULL) {
        return -1;
    }

    // Kontrola obsahu vfs_file
    if(vfs_file->vfs_filename == NULL || strlen(vfs_file->vfs_filename) < 1){
        return -2;
    }

    // Kontrola obsahu vfs_file 2
    if(vfs_file->inode_ptr == NULL){
        return -2;
    }

    // Kontrola ukazatele na místo v paměti pro uložení výsledku
    if (source == NULL) {
        return -3;
    }

    // Velikost čtení nemůže být menší jak 1
    if (write_item_size < 1) {
        return -4;
    }

    // Počet čtení nemůže být menší jak 1
    if (write_item_count < 1) {
        return -5;
    }

    // Ziskani superbloku
    struct superblock *superblock_ptr = superblock_from_file(vfs_file->vfs_filename);

    // Kontrola čtení superbloku
    if (superblock_ptr == NULL) {
        return -6;
    }

    int32_t cluster_size = superblock_ptr->cluster_size;

    int32_t temp_offset = vfs_file->offset;
    int32_t temp_filesize = vfs_file->inode_ptr->file_size;
    int32_t temp_total_write_size = write_item_size * write_item_count;

    // Kontrola přepisu existujících dat
    int32_t temp_rewritten = temp_offset - temp_filesize;

    // Informační ověření zda přepisuji již zapsaná data
    if (temp_rewritten < 0) {
        log_debug("vfs_write: Prepisuji %d existujich byte pro soubor s inode ID=%d\n", -1 * temp_rewritten,
                  vfs_file->inode_ptr->id);
    }

    // Kolik databloků bude potřeba po zápisu
    int32_t file_size = vfs_file->inode_ptr->file_size;
    int32_t data_block_needed = ceil(
            (double) (file_size + temp_total_write_size) / (double) (superblock_ptr->cluster_size));

    // Alokujeme dokud můžeme
    int32_t allocation_result = 0;
    while (vfs_file->inode_ptr->allocated_clusters < data_block_needed && allocation_result == 0) {
        // Zjištění volného data bloku a jeho adresy
        int32_t free_index = bitmap_find_free_cluster_index(vfs_file->vfs_filename);
        int32_t free_address = bitmap_index_to_cluster_address(vfs_file->vfs_filename, free_index);

        // Pokus o alokaci - 0 = OK
        allocation_result = inode_add_data_address(vfs_file->vfs_filename, vfs_file->inode_ptr, free_address);
    }

    // Alokace nevyšla
    if (allocation_result != 0) {
        log_debug("vfs_write: Nepodaril/y se alokovat data blok/y pro zapis!\n");
        // TODO: skončit?
    }

    // Výpočet v případě zápisu na více databloků
    int32_t skipped_datablocks = temp_offset / cluster_size;

    // Nelze přeskočit víc databloků než je alokováno - zápis do nenaalokovaného místa
    if (skipped_datablocks > vfs_file->inode_ptr->allocated_clusters) {
        log_debug("vfs_write: Nelze zapisovat do nenaalokovaneho mista!\n");
        return -7;
    }

    int32_t first_datablock_offset = temp_offset - (skipped_datablocks * cluster_size);
    int32_t first_datablock_can_write = cluster_size- first_datablock_offset;

    log_debug("vfs_write: First datablock offset -> %d\n", first_datablock_offset);
    log_debug("vfs_write: First datablock can write ->%d\n", first_datablock_can_write);

    // Otevření souboru pro zápis
    FILE *file = fopen(vfs_file->vfs_filename, "r+b");

    // Ověření otevření souboru
    if (file == NULL) {
        free(superblock_ptr);
        return -8;
    }

    // Iterační ukazatel pro zápis
    void *write_pointer = source;

    // Lze zapisovat do 1 data bloku?
    if (temp_total_write_size < first_datablock_can_write) {
        log_trace("vfs_write: Lze zapisovat vsechna data do prvniho databloku.\n");

        // Z kterého databloku budeme číst
        int32_t datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                    skipped_datablocks);
        // Přičteme offset k adrese
        int32_t datablock_direct_adress = datablock_address + first_datablock_offset;

        // Nastavíme adresu čtení z vfs souboru
        fseek(file, datablock_direct_adress, SEEK_SET);
        // Přečteme data
        fwrite(write_pointer, write_item_size, write_item_count, file);
        // Posun ukazatele
        write_pointer += write_item_size * write_item_count;
        // Vypočet velikosti zapsaných dat
        int32_t data_written = (write_pointer - source);
        int32_t data_append = data_written - (-1 * temp_rewritten);
        // Logging
        log_trace("vfs_write: Celkem zapsano %d byte (soubor zvetsen o %d byte)\n", data_written, data_append);

        // Zvětšení velikosti souboru
        vfs_file->inode_ptr->file_size += data_append;
        // Aktualizace inode ve VFS
        inode_write_to_index(vfs_file->vfs_filename, vfs_file->inode_ptr->id - 1, vfs_file->inode_ptr);
        // Posun offsetu
        vfs_seek(vfs_file, data_written, SEEK_CUR);
    } else {
        // Kolik musíme zapsat po 1. databloku
        int32_t remaining_write = temp_total_write_size - first_datablock_can_write;
        // Kolik celých databloků musíme zapsat
        int32_t datablock_count_write = remaining_write / superblock_ptr->cluster_size;
        // Velikost zapisu do posledniho databloku
        int32_t last_datablock_remaining = remaining_write - (datablock_count_write * superblock_ptr->cluster_size);

        // Logging
        log_trace("vfs_write: Remaining data after first datablock -> %d\n", remaining_write);
        log_trace("vfs_write: Whole remaining datablocks write -> %d\n", datablock_count_write);
        log_trace("vfs_write: Last datablock bytes -> %d\n", last_datablock_remaining);

        int32_t curr_remaining = temp_total_write_size;
        void *curr_write_pointer = source;

        // Zápis dat do prvního databloku
        int32_t first_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                          skipped_datablocks);
        // Přičteme offset k adrese
        int32_t datablock_direct_adress = first_datablock_address + first_datablock_offset;
        // Nastavení offsetu
        fseek(file, datablock_direct_adress, SEEK_SET);
        // Zápis do prvního databloku
        fwrite(curr_write_pointer, first_datablock_can_write, 1, file);
        // Posun ukazatele, odečtení "zbytku"
        curr_write_pointer += first_datablock_can_write;
        curr_remaining -= first_datablock_can_write;

        log_trace("vfs_write: Remaining after first data block was written: %d\n", curr_remaining);

        // Zápis celých databloků
        int32_t curr_datablock_index = skipped_datablocks + 1;
        while (curr_remaining >= superblock_ptr->cluster_size) {
            // Získání adresy dalšího bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                             curr_datablock_index);

            // Nastavení adresy
            fseek(file, curr_datablock_address, SEEK_SET);
            // Zapis celýho databloku
            fwrite(curr_write_pointer, cluster_size, 1, file);

            // Posun na další datablok
            curr_remaining -= cluster_size;
            curr_write_pointer += superblock_ptr->cluster_size;
            curr_datablock_index += 1;
            log_trace("vfs_write: Zapis do databloku na indexu %d, zbyvajici pocet byte k zapsani %d\n",
                      curr_datablock_index, curr_remaining);
        }

        // Zapis posledního data bloku
        if (curr_remaining > 0 && curr_remaining < cluster_size) {
            // Získání adresy posledního data bloku
            int32_t curr_datablock_address = inode_get_datablock_index_value(vfs_file->vfs_filename, vfs_file->inode_ptr,
                                                                             curr_datablock_index);

            // Nastavení offsetu
            fseek(file, curr_datablock_address, SEEK_SET);
            // Zapis zbylych dat
            fwrite(curr_write_pointer, curr_remaining, 1, file);

            // Posun na konec kvůli ověřování
            curr_write_pointer += curr_remaining;
            curr_remaining -= curr_remaining;
            curr_datablock_index += 1;
        }

        // Vypočet velikosti zapsaných dat
        int32_t data_written = curr_write_pointer - source;
        int32_t data_append = data_written - (-1 * temp_rewritten);
        // Logging
        log_trace("vfs_write: Celkem zapsano %d byte (soubor zvetsen o %d byte)\n", data_written, data_append);

        // Zvětšení velikosti souboru
        vfs_file->inode_ptr->file_size += data_append;
        // Aktualizace inode ve VFS
        inode_write_to_index(vfs_file->vfs_filename, vfs_file->inode_ptr->id - 1, vfs_file->inode_ptr);

        // Posun offsetu
        vfs_seek(vfs_file, data_written, SEEK_CUR);
    }



    free(superblock_ptr);
    fclose(file);


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
    vfs_file_open->vfs_filename = malloc(sizeof(char) * strlen(vfs_file) + 1);
    strcpy(vfs_file_open->vfs_filename, vfs_file);

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
bool vfs_close(VFS_FILE *file) {
    // Ověření zda je třeba uvolňovat
    if (file == NULL) {
        return FALSE;
    }

    free(file->inode_ptr);
    free(file->vfs_filename);
    free(file);

    return TRUE;
}
