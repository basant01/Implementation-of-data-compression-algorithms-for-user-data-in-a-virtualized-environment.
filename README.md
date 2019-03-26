# Implementation-of-data-compression-algorithms-for-user-data-in-a-virtualized-environment.

Data compression using various algorithms on the host OS and getting the compressed data on guest OS running on a virtual machine using automation.The primary purpose of this project is to implement various data-compression techniques using the C programming language. Data compression seeks to reduce the number of bits used to store or transmit information, thus in project we are migrating the data between the host OS and the guest OS running on VM and automating the complete procedure of migration using the OS commands i.e. crontab is being used for this purpose. 

The Compression Algorithms implemented are LZSS ,LZW12 in this project.

lzsse.c - An LZSS compression module

lzssd.c - An LZSS decompression module

LZWe.c - A simple 12 bit LZW compression module

LZWd.c - A simple 12 bit LZW decompression module
 
Upscript.sh - A shell script file for controlling the state of virtual machine using the VMrun commands .The guest OS is turned on using the VMrun power commands. Generation of workload on host OS and automating it using crontab.The program implementing compression algorithms takes the data as the input.The compression algorithm encodes the data.Starting Ftp local sevrer

Downscript.sh - A shell script file for Generation of workload on guest OS and automating it using crontab. The compressed data is received to the user and various user groups on Guest OS
