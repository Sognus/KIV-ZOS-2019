#include "superblock.h"
#include <stdlib.h>
#include <string.h>
#include "parsing.h"

/*
 * Konstanty
 */
const char IMPL_SIGNATURE[9] = "viteja\0";
const char IMPL_VOLUME_DESCRIPTOR[251] = "Semestralni prace KIV/ZOS v akademickem roce 2019/2020 - Virtualni filesystem zalozeny na inode\0";

/**
 * Alokuje strukturu superblok a vyplní ji implicitními daty
 *
 *
 *
 * @param disk_size celková velikost VFS
 * @return ukazatel na superblock
 */
struct superblock* superblock_impl_alloc(int32_t disk_size){
    struct superblock *ptr = malloc(sizeof(struct superblock));

    if(ptr == NULL){
        debug_print("superblock_impl_alloc: Nepodarilo se alokovat pamet!\n");
        return NULL;
    }

    ptr->disk_size = disk_size;
    superblock_set_signature(ptr, (char*)IMPL_SIGNATURE);
    superblock_set_volume_descriptor(ptr, (char*)IMPL_VOLUME_DESCRIPTOR);

    return ptr;
}

/**
 * Kompletně alokuje a vyplní strukturu superblock
 *
 * @param disk_size celková velikost VFS
 * @param signature signature superbloku
 * @param volume_descriptor volume_descriptor superbloku
 * @return ukazatel na superblock
 */
struct superblock* superblock_alloc(int32_t disk_size, int32_t cluster_size, char signature[9], char volume_descriptor[251]) {
    struct superblock *ptr = malloc(sizeof(struct superblock));

    if(ptr == NULL){
        debug_print("superblock_impl_alloc: Nepodarilo se alokovat pamet!\n");
        return NULL;
    }

    ptr->disk_size = disk_size;
    ptr->cluster_size = cluster_size;
    superblock_set_signature(ptr,signature);
    superblock_set_volume_descriptor(ptr, volume_descriptor);

    return ptr;
}


/**
 * Nastaví signature struktury superblock na novou hodnotu
 *
 * @param ptr ukazatel na strukturu superblock
 * @param signature nová hodnota signature
 * @return úspěch operace 0/1
 */
bool superblock_set_signature(struct superblock *ptr, char signature[9]){
    // Ověření existence struktury
    if(ptr == NULL){
        debug_print("superblock_set_volume_descriptor: Ukazatel na strukturu nemuze byt NULL!\n");
        return FALSE;
    }

    // Ověření délky řetězce
    if(strlen(signature) < 1 || strlen(signature) > 9)
    {
        debug_print("superblock_set_volume_descriptor: Vstupni retezec musi byt dlouhy 1-9 znaku!\n");
        return FALSE;
    }

    // Zápis dat
    strcpy(ptr->signature, signature);
    return TRUE;
}

/**
 * Nastaví hodnotu volume_descriptor struktury superblock na novou hodnotu
 *
 * @param ptr ukazatel na strukturu superblock
 * @param signature nová hodnota volume_descriptor
 * @return úspěch operace 0/1
 */
bool superblock_set_volume_descriptor(struct superblock *ptr, char *volume_descriptor) {
    // Ověření existence struktury
    if(ptr == NULL){
        debug_print("superblock_set_volume_descriptor: Ukazatel na strukturu nemuze byt NULL!\n");
        return FALSE;
    }

    // Ověření délky řetězce
    if(strlen(volume_descriptor) < 1 || strlen(volume_descriptor) > 251)
    {
        debug_print("superblock_set_volume_descriptor: Vstupni retezec musi byt dlouhy 1-251 znaku!!\n");
        return FALSE;
    }

    // Zápis dat
    strcpy(ptr->volume_descriptor, volume_descriptor);
    return TRUE;
}

/**
 * Ověří obsah struktury superblock
 *
 * @param ptr ukazatel na strukturu superblock
 * @return validita superblocku (0 | 1)
 */
bool superblock_check(struct superblock *ptr){
    if(ptr == NULL){
        debug_print("superblock_check: Superblock neni validni -> ukazatel je NULL!\n");
        return FALSE;
    }

    // Kontrola velikosti disku
    if(ptr->disk_size < 1){
        debug_print("superblock_check: Superblock neni validni -> disk_size!\n");
        return FALSE;
    }

    // Kontrola velikosti clusteru
    if(is_two_power(ptr->cluster_size) == FALSE)
    {
        debug_print("superblock_check: Superblock neni validni -> cluster_size = %d", ptr->cluster_size);
        return FALSE;
    }

    // Kontrola adresy bitmapy
    if(ptr->bitmap_start_address < sizeof(struct superblock)){
        debug_print("superblock_check: Superblock neni validni -> data_start_address\n");
        return FALSE;
    }

    // Kontrola adresy i-uzlů
    if(ptr->inode_start_address < (ptr->bitmap_start_address + (ptr->cluster_count * sizeof(int8_t)))){
        debug_print("superblock_check: Superblock neni validni -> inode_start_address\n");
        return FALSE;
    }

    // Kontrola adresy data bloků
    if(ptr->data_start_address < 1 || ptr->data_start_address < ptr->inode_start_address){
        debug_print("superblock_check: Superblock neni validni -> data_start_address\n");
        return FALSE;
    }

    debug_print("superblock_check: Superblock je validni!\n");
    return TRUE;
}

