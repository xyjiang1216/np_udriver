objects = app.o

app: $(objects)
	gcc -g -o app $(objects) -lpthread
app.o: app.c udriver_usrpart_pkt.h user_pkt_transfer.h
	gcc -c app.c
clean:
	rm app $(objects)
