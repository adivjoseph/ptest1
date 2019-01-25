
SRCS=main.c cli.c fifo.c port.c mmetx.c mmerx.c mmearp.c decodersp.c

ptest : $(SRCS)
	gcc -g -o ptest $(SRCS) -pthread -I. 

clean :
	rm ptest
