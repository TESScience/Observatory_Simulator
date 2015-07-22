f2=open('../TextFiles/SeqMem.h','w')
with open('../TextFiles/SeqMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
                f2.write(bin(int(nl,16))[2:].zfill(32))
                f2.write('\r\n')

f2=open('../TextFiles/PrgMem.h','w')
with open('../TextFiles/PrgMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
                f2.write(bin(int(nl,16))[2:])
                f2.write('\r\n')

f2=open('../TextFiles/HskMem.h','w')
with open('../TextFiles/HskMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
                f2.write(bin(int(nl,16))[2:])
                f2.write('\r\n')

f2=open('../TextFiles/ClvMem.h','w')
with open('../TextFiles/ClvMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
                f2.write(bin(int(nl,16))[2:])
                f2.write('\r\n')

f2=open('../TextFiles/RegMem.h','w')
with open('../TextFiles/Reg.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
                f2.write(bin(int(nl,16))[2:])
                f2.write('\r\n')

f2=open('../TextFiles/FPE_Wrapper.h','w')
cnt = 0
with open('../../FPGA_BitFiles/FPE_Wrapper.rbt','r') as f1:
        for line in f1:
                cnt = cnt + 1
                if cnt > 7:
                        f2.write(line)

