cmake_minimum_required(VERSION 3.14)
project(KIV_ZOS C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-lm")

add_executable(KIV_ZOS main.c structure.c structure.h superblock.c superblock.h inode.c inode.h bool.h parsing.c parsing.h debug.h debug.c allocation.c allocation.h bitmap.c bitmap.h vfs_io.c vfs_io.h directory.c directory.h)