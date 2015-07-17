f2=open('SeqMem.h','w')
with open('SeqMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
		f2.write("0x"+nl + ',\r\n')

f2=open('PrgMem.h','w')
with open('PrgMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
		f2.write("0x"+nl + ',\r\n')

f2=open('HskMem.h','w')
with open('HskMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
		f2.write("0x"+nl + ',\r\n')

f2=open('ClvMem.h','w')
with open('ClvMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
		f2.write("0x"+nl + ',\r\n')

f2=open('RegMem.h','w')
with open('RegMem.txt') as f1:
	for line in f1:
                nl = line.rstrip('\n\r')
		f2.write("0x"+nl + ',\r\n')

f2=open('FPE_Wrapper.h','w')
cnt = 0
with open('FPE_Wrapper.rbt','r') as f1:
        for line in f1:
                cnt = cnt + 1
                nl = line.rstrip('\n\r')
                if cnt > 7:
                        numi = int(str(nl),2)
                        f2.write(str(numi)+"u,\n")

