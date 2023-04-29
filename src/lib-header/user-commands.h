#ifndef _USER_COMMANDS
#define _USER_COMMANDS

#include "user-helper.h"

/**
 * Command cd
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_cd(int argc, char **argv);

/**
 * Command ls
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_ls(int argc, char **argv);

/**
 * Command mkdir
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_mkdir(int argc, char **argv);

/**
 * Command cat
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_cat(int argc, char **argv);

/**
 * Command cp
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_cp(int argc, char **argv);

/**
 * Command rm
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_rm(int argc, char **argv);

/**
 * Command mv
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_mv(int argc, char **argv);

/**
 * Search recursively
 * 
 * @param cluster_number starting cluster_number
 * @param name name of object being searched
 * @param ext extension of object being searched (if exists)
 * @param path record the current searched path
 */
void search_recursive(uint32_t cluster_number, char *name, char *ext, char *path);

/**
 * Command whereis
 * 
 * @param argc Number of argument
 * @param argv Arrays of input
 */
void command_whereis(int argc, char **argv);

#endif