#!/bin/bash
cd /home/basant/Desktop/MInor

touch out1.txt
touch out2.txt
touch out3.txt


gcc arithe.c
./a.out input.txt out1.txt

gcc lzsse.c
./a.out input.txt out2.txt


gcc LZWe.c
./a.out input.txt out3.txt


 vmrun -T ws start "/home/basant/vmware/Ubuntu 64-bit (2)/Ubuntu 64-bit (2).vmx"
export DISPLAY=":0.0"
Systemctl start vsftpd
