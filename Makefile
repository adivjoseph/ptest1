
SRCS=main.c cli.c fifo.c port.c mmetx.c  mmerx.c decodersp.c s1utx.c sgirx.c sgitx.c s1urx.c stats.c

ptest : $(SRCS)
	gcc -g -o ptest $(SRCS) -pthread -I. -O0

clean :
	rm ptest
