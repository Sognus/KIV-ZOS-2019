#include <stdio.h>
#include "structure.h"
#include "superblock.h"
#include <stdlib.h>
#include "bitmap.h"
#include "allocation.h"
#include "directory.h"
#include "vfs_io.h"

#include "shell.h"
#include "parsing.h"

#define FILENAME "test.dat"

int main(int argc, char *argv[]) {
    // Ověření počtu vstupních parametrů [1] = cesta k VFS
    if(argc < 2){
        log_fatal("Program spusten bez parametru!\n");
        return -1;
    }

    // Vytvoření kontextu
    struct shell *sh = shell_create(argv[1]);

    // Ověření na vytvoření kontextu
    if(sh == NULL){
        log_fatal("Nepodarilo se vytvorit kontext pro praci s VFS!\n");
        return -2;
    }

    // Spuštění hlavní smyčky
    while(0){
        char *line = malloc(sizeof(char) * 256 + 1);

        printf("> ");
        line = fgets(line, sizeof(char) * 256 + 1, stdin);

        // Kontrola ukončení souboru
        if(strcicmp(line, "exit\n") == 0){
            free(line);
            break;
        }else{
            // Zpracování vstupu
            shell_parse(sh, line);
        }

        // Uvolnění zdrojů
        free(line);
    }





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
    directory_create(FILENAME, "/");  // 1
    directory_entries_print(FILENAME, "/");

    directory_create(FILENAME, "/slozka"); // 2
    directory_entries_print(FILENAME, "/");

    directory_create(FILENAME, "/kkk"); // 3
    directory_entries_print(FILENAME, "/");


    printf("\n\n\n");

    directory_create(FILENAME, "/kkk/k1"); // 4
    directory_entries_print(FILENAME, "/kkk/");
    directory_create(FILENAME, "/kkk/k1/test"); // 5
    directory_entries_print(FILENAME, "/kkk/k1/");

    printf("\n\n\n");
    char *path = directory_get_path(FILENAME, 10);
    printf("PATH ID=5: %s\n", path);
    free(path);

    sh->cwd = 4;
    char *t = path_parse_absolute(sh, "test");
    if(t != NULL){
        printf("absolute: %s\n",t);
        free(t);
    }

    t = path_parse_absolute(sh, ".");
    if(t != NULL){
        printf("absolute: %s\n",t);
        free(t);
    }


    /*
     * [UVOLNĚNÍ ZDROJŮ
     */
    free(ptr);
    shell_free(sh);
}