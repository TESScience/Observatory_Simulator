#!/bin/bash
# Script to load FPGA memory Blocks.

tftp -v -m binary 192.168.100.1 69 -c put SeqMem.bin seqmem
sleep 1
tftp -v -m binary 192.168.100.1 69 -c put RegMem.bin regmem
sleep 1
tftp -v -m binary 192.168.100.1 69 -c put PrgMem.bin prgmem
sleep 1
tftp -v -m binary 192.168.100.1 69 -c put CLVMem.bin clvmem
sleep 1
tftp -v -m binary 192.168.100.1 69 -c put HskMem.bin hskmem
sleep 1
