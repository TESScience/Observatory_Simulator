import unittest
import numpy as np
import fpga
import struct

class TestPackMem(unittest.TestCase):

    def test_value8(self):
        data = [1,2,3,4]
        expout = '\x01\x02\x03\x04'
        self.assertEqual(expout,fpga.pack_mem(data,fpga.value8))

    def test_empty_array(self):
        data = []
        self.assertEqual('',fpga.pack_mem(data,fpga.value8))

    def test_value16(self):
        data = [1,2,3,4]
        expout = '\x00\x01\x00\x02\x00\x03\x00\x04'
        self.assertEqual(expout,fpga.pack_mem(data,fpga.value16))

class TestUploadToFPGA(unittest.TestCase):

    def get_32bit(self,fname):
        with open(fname,'rb') as f:
            data = f.read()
        return list(struct.unpack(">"+"I"*(len(data)/4),data))

    def setUp(self):
        """Loads a small set of values into memory, of 3 different types"""
        regmem = [1,6,8,4]
        seqmem = []
        prgmem = [18,24,0,5]
        hskmem = [0,1,2,3]
        voltmem = []
        fpga.upload_to_fpga(REG_MEM=regmem, SEQ_MEM=seqmem,
                        PRG_MEM=prgmem,HSK_SEL_MEM=hskmem,
                        VOLT_MEM=voltmem)

    def test_check_regmem(self):
        """Tests register memory (16-bit values)"""
        self.assertEqual([65542,524292],
                self.get_32bit('RegMem.bin'))

    def test_check_seqmem(self):
        """Tests empty array (and therefore file)"""
        self.assertEqual([],
                self.get_32bit('SeqMem.bin'))

    def test_check_prgmem(self):
        """Tests program memory (64-bit values)"""
        self.assertEqual([0,18,0,24,0,0,0,5],
                self.get_32bit('PrgMem.bin'))

    def test_check_hskmem(self):
        """Tests housekeeping memory (8-bit values)"""
        self.assertEqual([66051],
                self.get_32bit('HSKMem.bin'))

    def test_check_clvmem(self):
        self.assertEqual([],
                self.get_32bit('CLVMem.bin'))

if __name__=="__main__":
    unittest.main()
