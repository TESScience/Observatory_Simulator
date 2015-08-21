#! /usr/bin/env python
import struct

class valuetype:
    def __init__(self,value=None):
        self.value = value

    def set(self,value):
        self.value = value

    def pack(self,stream=''):
        return stream + struct.pack(">"+self.format,self.value)

class value8(valuetype):
    def __init__(self,value=None):
        self.value = value
        self.format = "B"

class value16(valuetype):
    def __init__(self,value=None):
        self.value = value
        self.format = "H"

class value64(valuetype):
    def __init__(self,value=None):
        self.value = value
        self.format = "Q"

def pack_mem(arr,vtype,stream=''):
    for e in arr:
        stream = vtype(e).pack(stream)
    return stream

def write_file(fname,stream):
    with open(fname+'.bin','wb') as f:
        f.write(stream)

def upload_to_fpga(REG_MEM=None, SEQ_MEM=None, PRG_MEM=None,
                        HSK_SEL_MEM=None, VOLT_MEM=None):
    """Writes binary files of appropriate type for each array loaded in.
    Iterables passed in must be of the correct order and size. If one of
    the types is not set, it will be skipped (no values will be written)."""
    for arr,vtype,fname in zip([REG_MEM,SEQ_MEM,PRG_MEM,HSK_SEL_MEM,VOLT_MEM],
                                [value16,value64,value64,value8,value16],
                                ['RegMem','SeqMem','PrgMem','HSKMem','CLVMem']):
        if arr is not None:
            write_file(fname,pack_mem(arr,vtype))
