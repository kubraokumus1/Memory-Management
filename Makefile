all: libsbmemlib.a  app app2 create_memory_sb destroy_memory_sb

libsbmemlib.a:  sbmemlib.c
	gcc -Wall -c sbmemlib.c -lpthread
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a

app: app.c
	gcc -Wall -o app app.c -L. -lsbmemlib -lm -lrt -lpthread

app2: app2.c
	gcc -Wall -o app2 app2.c -L. -lsbmemlib -lm -lrt -lpthread

create_memory_sb: create_memory_sb.c
	gcc -Wall -o create_memory_sb create_memory_sb.c -L. -lsbmemlib -lm -lrt -lpthread

destroy_memory_sb: destroy_memory_sb.c
	gcc -Wall -o destroy_memory_sb destroy_memory_sb.c -L. -lsbmemlib -lm -lrt -lpthread

clean: 
	rm -fr *.o *.a *~ a.out  app app2 sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb
