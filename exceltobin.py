#! /usr/bin/env python2.7
import sys,struct
import xlrd

def getoutputsheets(xls):
    sheets = []
    for s in xls.sheets():
        if s.ncols>26 and s.cell(0,25).value=='XfrToBin':
            sheets.append(s)
    return sheets

def getoutputarr(sheet):
    data = []
    i=1
    val = sheet.cell(i,25).value
    while val!='' and i<sheet.nrows-1:
        data.append(str(val))
        i+=1
        val = sheet.cell(i,25).value
    n = int(sheet.cell(0,26).value)
    data+=['0']*(n-len(data))
    return data

def packbin(dataarr):
    stream = ''
    for d in dataarr:
        stream += struct.pack(">I",int(d.split('.')[0],16))
    return stream

def packasc(dataarr,stream=''):
    return '\n'.join(dataarr)

def writefiles(sheet):
    data = getoutputarr(sheet)
    with open(sheet.name+'.bin','wb') as f:
        f.write(packbin(data))
    with open(sheet.name+'.asc','w') as f:
        f.write(packasc(data))
    print sheet.name

def main():
    if len(sys.argv)<2:
        print "Error: no excel file input."
        print "Run as './exceltobin.py [filename]'"
        sys.exit()
    wb = xlrd.open_workbook(sys.argv[1])
    for s in getoutputsheets(wb):
        writefiles(s)


if __name__=="__main__":
    main()
