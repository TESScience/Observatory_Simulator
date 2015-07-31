#! /usr/bin/env python
import sys
import struct

def readfile(fname):
    with open(fname) as f:
        return f.readlines()

def packbin(dataarr):
    stream = ''
    for d in dataarr:
        stream += struct.pack(">I",int(d.split('.')[0],16))
    return stream

def main():
    if len(sys.argv)<2:
        print "Error: no files input."
        print "Run as './exceltobin.py [*filenames]'"
        sys.exit()
    for fname in sys.argv[1:]:
        data = readfile(fname)
        with open(fname+'.bin','wb') as newf:
            newf.write(packbin(data))

if __name__=="__main__":
    main()
