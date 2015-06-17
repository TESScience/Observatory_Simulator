#!/usr/bin/python

import struct

fileno=0
ramp=-1

while True:
    fname = "./test/obssim-%d.bin" % (fileno,)
    try:
        fp = file(fname)
        data = fp.read(2)
        while data:
            (dataval,) = struct.unpack("<H",data)
            if ramp < 0:
                ramp = dataval + 1
            else:
                if dataval <> ramp:
                    print "File: %s Ramp error: data=%04x expected=%04x" \
                        % (fname,ramp,dataval)
                    ramp = dataval
                ramp = ramp + 1
            ramp = ramp & 0xffff
            data = fp.read(2)
        fileno = fileno + 1
    except:
        raise
