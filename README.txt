Opal file manifest program
version 1

This program is under development and currently not usable.

This program creates a manifest file containing metadata of files in a file hierarchy. The files are specified on the command line and the manifest file is output to standard output. A manifest file that needs to be updated can be input through standard input.

options
h: output help and exit
v: verbose mode
t: file types to output
u: update mode
m: types of metadata to include
H: process the files pointed at by symlinks specified in the command line instead of the symlinks themselves
L: process the files pointed at by all symlinks encountered in the hierarchy instead of the symlinks themselves

The t, u, and m options are followed by characters that specify their behavior.

file type options
r: regular files
d: directories
c: character devices
b: block devices
l: symbolic links
f: fifos (pipes)

update options
a: add
r: remove
m: update modified

metadata type options
s: file size
m: modification time

