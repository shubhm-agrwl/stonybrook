## [M1]
#  Explaint the following here
# Gets the current Directory, takes the last word of the actual path to get the directory name where make file exists
#
#  ...
CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

## [M2]
#  Compliler Settings i.e. CC is simple assigned to use gcc
# Setting compiler flags i.e. CFLAGS using appending assignment
# -g: compile the debug symbols and programs can be debugged
# -Werror: shows warnings as errors by gcc
# -Wall: enable warnings
# LDFLAGS: specifies load/link options; currently nothing is getting appended to these flags
#  ...
CC = gcc
CFLAGS += -g -O2 -Werror -Wall
LDFLAGS +=

## [M3] -- REMOVED
#  ...

## [M5]
#  Explaint the following here
# Assigning source files using wildcard i.e. selecting all the files which have extension - '.c'
#  ...
SRC_FILES = $(wildcard *.c)

## [M6]
#  Explaint the following here
# Assigning Destination files i.e. taking filenames from the source files without the extension. (e.g. client.c -> client)
#  ...
EXE_FILES = $(SRC_FILES:.c=)

## [M7]
#  Explaint the following here
# Main part of the make file where target is assigned to the action mentioned below. The gcc in M8 will store the binary output in the destination files
#  ...
all: $(EXE_FILES)

## [M8] -- UPDATED
#  Explaint the following here
# Use of wild card pattern matching to take all .c files
# Action of the make file which is compiling the C code along with the flags set in M2 along with the destination ($@) and source files ($<).
# The below line will start compiling all the .c files one by one using the compiler options.
#  ...
%:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

## [M9]
# make clean - to remove executable files i.e. client and server executable files
#  ...
clean:
	rm -f $(EXE_FILES)

## [M10]
#  Explaint the following here
# This is used to handle cases where files named "all" and "clean" can be present in the current directory.
# If we don't use this, and a file called "all" exists, make all will not be able to initiate the command and will do nothing.
# Presense of this command will ensure that make all will not check whether the file exists, thus ensuring that the make file behaves as expected
#  ...
.PHONY: all clean
