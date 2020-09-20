objects = udriver_usrpart_init.o

udriver_usrpart: $(objects)
	gcc -g -o udriver_usrpart $(objects) -lpthread
udriver_usrpart_init.o: udriver_usrpart_init.c  udriver_usrpart_init.h udriver_mmap_struct.h
	gcc -c udriver_usrpart_init.c
clean:
	rm udriver_usrpart $(objects)
