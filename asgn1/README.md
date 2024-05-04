# Assignment 1 directory

## Short Description:
This directory contains source code and other files for Assignment 1. memory.c has two functions: get and set. Get will take in a command from stdin in the format "get\nlocation\n". When given the get command, it attempt to open location and read all the contents within it to stdout. Set will also take in a command from stdin in the format "get\nlocation\ncontent\_length\ncontent". When given this command, it will attempt to open location and read content\_length of contents into the file location. If there is stuff in the file location, it will be overwritten. If file location does not exist, memory.c will create and open one with the name location.

## Build and Clean
To build the hello executable, type "make all". To clear your directory, type "make clean". This will remove the executable as well as any .o files.

