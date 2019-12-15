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