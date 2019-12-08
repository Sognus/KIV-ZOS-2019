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

#endif //KIV_ZOS_COMMANDS_H
