#!/bin/bash

[ -d ./frames ] || mkdir frames
./obssim_unpack ./frames/ 192.168.100.3 7777 17754432
