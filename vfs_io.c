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
int32_t vfs_seek(VFS_FILE *vfs_file, int64_t offset, int type){
    // TODO: implement
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
size_t vfs_read(void *destination, size_t read_item_size, size_t read_item_count, VFS_FILE *vfs_file){
    // TODO: implement
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
size_t vfs_write(void *source, size_t write_item_size, size_t write_item_count, VFS_FILE *vfs_file){
    // TODO: implement
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
    // TODO: implement
    return 0;
}

/**
 * Vytvoří kontext pro práci souboru - vždycky lze provádět čtení i zápis zároveň
 *
 * @param vfs_file cesta k souboru vfs.dat (apod.)
 * @param vfs_path ID inode k otevření
 * @return
 */
VFS_FILE *vfs_open_inode(char *vfs_file, int32_t inode_id){
    // Kontrola délky názvu souboru
    if(strlen(vfs_file) < 1){
        log_debug("vfs_open_inode: Cesta k souboru VFS nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    // Ověření existence souboru
    if(file_exist(vfs_file) != TRUE){
        log_debug("vfs_open_inode: Soubor VFS %s neexistuje!\n", vfs_file);
        return NULL;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(vfs_file);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("vfs_open_inode: Nepodarilo se precist superblock z VFS!\n");
        return NULL;
    }

    VFS_FILE *vfs_file_open = malloc(sizeof(struct VFS_FILE));

    if(vfs_file_open == NULL){
        free(superblock_ptr);
        log_debug("vfs_open_inode: Nepodarilo se alokovat pamet pro strukturu VFS_FILE!\n");
        return NULL;
    }

    struct inode *inode_ptr = inode_read_by_index(vfs_file, inode_id);

    if(inode_ptr == NULL) {
        free(vfs_file_open);
        free(superblock_ptr);
        log_debug("vfs_open_inode: Nelze precist inode s ID=%d - dana ID neexistuje!\n", inode_id);
        return NULL;
    }

    vfs_file_open->inode_ptr = inode_ptr;
    vfs_file_open->offset = 0;
    vfs_file_open->vfs_file = malloc(sizeof(char) * strlen(vfs_file));
    strcpy(vfs_file_open->vfs_file, vfs_file);

    // Uvolnění zdrojů
    free(superblock_ptr);

    // Návrat VFS_FILE
    return vfs_file_open;
}
