#!/bin/bash



cd /home/ec2-user

gcc arithd.c
touch MyFile1.txt
./a.out out1.txt myFile1.txt

gcc lzssd.c
touch MyFile2.txt
./a.out out2.txt MyFile2.txt

gcc LZWd.c
touch MyFile3.txt
./a.out out3.txt MyFile3.txt

mkdir /home/ec2-user/Guest13

mkdir /home/ec2-user/Guest14

mkdir /home/ec2-user/Guest15

mv MyFile1.txt /home/ec2-user/Guest13
mv MyFile2.txt /home/ec2-user/Guest14
mv MyFile3.txt /home/ec2-user/Guest15

sudo useradd testuser4
sudo useradd testuser5
sudo useradd testuser6

sudo addgroup group4
sudo addgroup group5
sudo addgroup group6



sudo adduser testuser4 group4
sudo adduser testuser5 group5
sudo adduser testuser6 group6

sudo chown :group4 -R /home/ec2-user/Guest1

sudo chown :group5 -R /home/ec2-user/Guest2


sudo chown :group6 -R /home/ec2-user/Guest3
