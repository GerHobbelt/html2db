# Makefile to create html2sgml utility

TIDYDIR=..
LIBDIR=$(TIDYDIR)/lib
INCDIR=$(TIDYDIR)/include -I$(TIDYDIR)/src
OBJS=main.o pprint.o
CFLAGS= -Wall -g
html2db: $(OBJS)
	gcc $(CFLAGS) -o $@ $(OBJS) -L$(LIBDIR) -ltidy

%.o: %.c main.h
	gcc $(CFLAGS) -o $@ -I$(INCDIR) -c $<

# For local purposes
install:
	cp html2db $${HOME}/public_html/tidy; chmod go+r $${HOME}/public_html/tidy/html2db
	cd ..; tar zcvf html2db.tar.gz html2db/main.c html2db/main.h html2db/pprint.c html2db/Makefile; cp html2db.tar.gz $${HOME}/public_html/tidy; chmod go+r $${HOME}/public_html/tidy/html2db.tar.gz

clean:
	rm $(OBJS)
	rm html2db 
