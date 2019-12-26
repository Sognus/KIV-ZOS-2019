#include "symlink.h"
#include "file.h"
#include "vfs_io.h"
#include <stdlib.h>
#include "structure.h"

/**
 * Vytvoří soubor, a uloží do něj cestu na jiný soubor
 * tím vytvoří symlink
 *
 * @param vfs_filename
 * @param path
 * @param linked
 * @return
 */
int32_t symlink_create(char *vfs_filename, char *path, char *linked){
    int32_t result = file_create(vfs_filename, path);

    // Nepovedlo se
    if(result < 1){
        return result;
    }

    // Otevřít soubor
    VFS_FILE *target = vfs_open(vfs_filename, path);

    if(target == NULL){
        printf("PATH NOT FOUND (nepodarilo se vytvorit symlink)\n");
        return -20;
    }

    // Změna soubor -> symlink
    target->inode_ptr->type = VFS_SYMLINK;
    inode_write_to_index(vfs_filename, target->inode_ptr->id - 1, target->inode_ptr);

    int32_t  written = vfs_write(linked, strlen(linked), 1, target);

    if(written != 0){
        printf("PATH NOT FOUND (nelze zapsat data do symlinku)\n");
        return -30;
    }

    printf("OK\n");

    return 0;
}


/**
 * Otevře soubor, pokud je soubor SYMLINK vrátí cestu na
 * soubor na který ukazuje
 *
 * Pokud soubor neni symlink, vrati ukazatel na puvodni soubor
 *
 * @param vfs_filename
 * @param symlink
 * @return
 */
VFS_FILE *symlink_dereference(char *vfs_filename, VFS_FILE *symlink){
    if(vfs_filename == NULL){
        return symlink;
    }

    if(strlen(vfs_filename) < 1){
        return symlink;
    }

    if(symlink == NULL){
        return NULL;
    }

    // Pokud není symlink, vrátíme původní
    if(symlink->inode_ptr->type != VFS_SYMLINK){
        return symlink;
    }

    // Alokace paměti pro cestu
    char *path = malloc(sizeof(char) * symlink->inode_ptr->file_size + 1);
    memset(path, 0, sizeof(char) * symlink->inode_ptr->file_size + 1);

    // Přečtení cesty ze symlinku
    vfs_seek(symlink, 0, SEEK_SET);
    vfs_read(path, sizeof(char) * symlink->inode_ptr->file_size, 1, symlink);

    // Pokus o otevření cesty ze symlinku
    VFS_FILE *dereferenced = vfs_open(vfs_filename, path);

    // Pokud se nám nepodařilo otevřít symlinked file vrátíme původní VFS_FILE
    if(dereferenced == NULL){
        free(path);
        return symlink;
    }

    // Dereferencovali jsme symlink - hurah
    free(path);
    vfs_close(symlink);
    return dereferenced;

}