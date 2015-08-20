#! /usr/bin/env python
import struct
import numpy as np

class ValueType:
    """Value of arbitrary type, which can be packed into binary"""
    def __init__(self,value=None):
        self.value = value

    def set(self,value):
        self.value = value

    def pack(self,stream=''):
        return stream + struct.pack(">"+self.format,self.value)

class Value8(ValueType):
    def __init__(self,value=None):
        self.value = value
        self.format = "B"

class Value16(ValueType):
    def __init__(self,address,value=None):
        self.address = address
        self.value = value
        self.format = "H"

class Value32(ValueType):
    def __init__(self,address,value=None):
        self.address = address
        self.value = value
        self.format = "I"

class CLVValue(Value16):
    def pack(self,stream=''):
        """Combines address and value of a particular voltage input"""
        return stream + struct.pack(">"+self.format,self.address%8*2**12+self.value)

class CLVMem:
    """Contains values for CLV Memory. Each attribute is at a particular
    address and has a particular value (initialized to None -- it is
    an error if any value remains unset)."""
    def __init__(self):
        self.OutputGateCCD0 = CLVValue(0)
        self.IG1CCD0 = CLVValue(1)
        self.IG2CCD0 = CLVValue(2)
        self.ScupperCCD0 = CLVValue(3)
        #etc.

    def get_binary(self,stream=''):
        """Packs the values for each attribute into binary (ordered
        correctly by address)."""
        vals = [getattr(self,e) for e in self.__dict__.keys()]
        addresses = [v.address for v in vals]
        for i in xrange(len(addresses)):
            stream = vals[addresses.index(i)].pack(stream)
        return stream

class RegMem:
    """Contains values for Register Memory. Each attribute is at a particular
    address and has a particular value (initialized to None -- it is
    an error if any value remains unset)."""
    def __init__(self):
        self.HskTimeConversion = Value32(0)
        self.HskWaitTime = Value32(1)
        self.HskTotalSamples = Value32(2)
        #etc.

    def get_binary(self,stream=''):
        """Packs the values for each attribute into binary (ordered
        correctly by address)."""
        vals = [getattr(self,e) for e in self.__dict__.keys()]
        addresses = [v.address for v in vals]
        for i in xrange(len(addresses)):
            stream = vals[addresses.index(i)].pack(stream)
        return stream

class HskMem:
    """Contains requested housekeeping values, initialized to 0."""
    def __init__(self):
        self.arr = [Value8(0) for _ in xrange(128)]

    """Packs housekeeping values into binary"""
    def get_binary(self,stream=''):
        for e in self.arr:
            stream = e.pack(stream)
        return stream

class SixCycle:
    """Contains (binary) blocks of code, organized by name (e.g. serialflush)"""
    def __init__(self):
        self.arr = np.zeros((256,33),dtype=bool)
        self.idxs = {'stop':[0,0]}

    def add_block(self,arr,name):
        """Add a block of binary code, with a particular name so it can later
        be called by program memory. The beginning and end of the block are stored
        with the name, and the block is added to the sequence memory (self.arr)."""
        block = np.zeros((256,33),dtype=bool)
        arr = [tuple(e) for e in arr]
        start = len(self.arr)
        end = len(arr) + start - 1
        block[0:len(arr)] = arr
        self.arr = np.concatenate((self.arr,block))
        self.idxs[name] = [start,end]

class ProgMem:
    """Placeholder for program memory. This will allow blocks from the sequence memory
    to be called."""
    pass

def main():
    pass

if __name__=="__main__":
    main()
