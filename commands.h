#ifndef KIV_ZOS_COMMANDS_H
#define KIV_ZOS_COMMANDS_H


/**
 * Příkaz PWD
 * @param sh kontext virtuálního terminálu
 */
void cmd_pwd(struct shell *sh);

/**
 * Příkaz FORMAT
 *
 * @param sh kontext virtuálního terminálu
 * @param command příkaz
 */
void cmd_format(struct shell *sh, char *command);

/**
 * Příkaz: Změna složky
 *
 * @param sh kontext virtuálního terminálu
 * @param command příkaz
 */
void cmd_cd(struct shell *sh, char *command);

/**
 * Příkaz: Vytvoření adresáře
 *
 * @param sh
 * @param command
 */
void cmd_mkdir(struct shell *sh, char *command);

/**
 * Příkaz: Výpis složky
 *
 * Pokud command == null -> výpis aktuální složky
 *
 * @param sh
 * @param command
 */
void cmd_ls(struct shell *sh, char *command);

#endif //KIV_ZOS_COMMANDS_H
