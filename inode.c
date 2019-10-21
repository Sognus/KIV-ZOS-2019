#include "inode.h"
#include "debug.h"


/**
 * Vypíše obsah struktury inode
 *
 * @param ptr ukazatel na strukturu inode
 */
void inode_print(struct inode *ptr){
    if (ptr == NULL) {
        log_trace("Ukazatel na strukturu superblock je NULL!\n");
        return;
    }

    log_info("*** INODE\n");
    log_info("ID: %d\n", ptr->id);
    log_info("Type: %d\n", ptr->type);
    log_info("References: %d\n", ptr->references);
    log_info("File size: %d\n", ptr->file_size);
    log_info("Direct pointer 1: %ď\n", ptr->direct1);
    log_info("Direct pointer 2: %ď\n", ptr->direct2);
    log_info("Direct pointer 3: %ď\n", ptr->direct3);
    log_info("Direct pointer 4: %ď\n", ptr->direct4);
    log_info("Direct pointer 5: %ď\n", ptr->direct5);
    log_info("Single indirect pointer: %d\n", ptr->indirect1);
    log_info("Double indirect pointer: %d\n", ptr->indirect2);
    log_info("*** INODE END\n");
}
