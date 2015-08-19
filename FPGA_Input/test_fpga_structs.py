#! /usr/bin/env python
import unittest
import struct
import numpy as np
import fpga_structs as fs

class TestCLVBinary(unittest.TestCase):
    def setUp(self):
        self.obj = fs.CLVMem()
        self.obj.OutputGateCCD0.set(5)
        self.obj.IG1CCD0.set(256)
        self.obj.IG2CCD0.set(0)

    def test_not_all_values_set(self):
        self.obj.ScupperCCD0.set(None)
        self.assertRaises(TypeError,self.obj.get_binary)

    def test_all_values_set(self):
        self.obj.ScupperCCD0.set(12)
        expout = '\x00\x05\x11\x00 \x000\x0c'
        self.assertEqual(expout,self.obj.get_binary())

class TestRegMemBinary(unittest.TestCase):
    def setUp(self):
        self.obj = fs.RegMem()
        self.obj.HskTimeConversion.set(1)
        self.obj.HskWaitTime.set(2)
        self.obj.HskTotalSamples.set(3)

    def test_all_values_set(self):
        expout = '\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03'
        self.assertEqual(expout,self.obj.get_binary())

class TestHskBinary(unittest.TestCase):
    def setUp(self):
        self.obj = fs.HskMem()

    def test_1(self):
        for e,v in zip(self.obj.arr[0:3],[1,3,12]):
            e.set(v)
        expout = '\x01\x03\x0c'+'\x00'*125
        self.assertEqual(expout,self.obj.get_binary())

class TestAddBlock(unittest.TestCase):
    def setUp(self):
        self.obj = fs.SixCycle()

    def test_1(self):
        newblock = [[1]*33]
        self.obj.add_block(newblock,'test_ones')
        self.assertEqual({'stop':[0,0],'test_ones':[256,256]},self.obj.idxs)
        expout = np.array([[0]*33]*256+[[1]*33]+[[0]*33]*255)
        self.assertTrue(np.allclose(expout,self.obj.arr))

if __name__=="__main__":
    unittest.main()
