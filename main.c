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
#define IMPL_VFS_SIZE 65536

int main(int argc, char *argv[]) {
    // Nastaveni bufferu pro terminaly typu git bash kde stdout je pipe
    #ifdef _WIN32
        setvbuf(stdout, 0, _IONBF, 0);
    #endif

    // Ověření počtu vstupních parametrů [1] = cesta k VFS
    if(argc < 2){
        log_fatal("Program spusten bez parametru: pouzijte ./KIV_ZOS <cesta_k_vfs_souboru>!\n");
        printf("Program spusten bez parametru: pouzijte ./KIV_ZOS <cesta_k_vfs_souboru>!\n");
        return -1;
    }

    if(file_exist(argv[1]) == FALSE){
        FILE *file = fopen(argv[1], "ab+");

        if(file == NULL){
            log_fatal("Nepodarilo se vytvorit %s!\n", argv[1]);
            return -10;
        }

        int64_t size = IMPL_VFS_SIZE;
        // Vytvoření implicitního superbloku
        struct superblock *ptr = superblock_impl_alloc(size);
        // Výpočet hodnot superbloku dle velikosti disku
        structure_calculate(ptr);
        // Vytvoření virtuálního FILESYSTEMU
        vfs_create(argv[1], ptr);
        // Vytvoření kořenové složky
        directory_create(argv[1], "/");

        fclose(file);
    }

    // Vytvoření kontextu
    struct shell *sh = shell_create(argv[1]);

    // Ověření na vytvoření kontextu
    if(sh == NULL){
        log_fatal("Nepodarilo se vytvorit kontext pro praci s VFS!\n");
        return -2;
    }

    // Spuštění hlavní smyčky
    while(1){
        char *line = malloc(sizeof(char) * 256 + 1);
        char *path = directory_get_path(sh->vfs_filename, sh->cwd);

        printf("%s > ", path);
        line = fgets(line, sizeof(char) * 256 + 1, stdin);

        // Kontrola ukončení souboru
        if(strcicmp(line, "exit\n") == 0){
            free(line);
            free(path);
            break;
        }else{
            // Zpracování vstupu
            shell_parse(sh, line);
        }

        // Uvolnění zdrojů
        free(line);
        free(path);
    }

    shell_free(sh);
}