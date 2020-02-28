TITLE_COLOR = \033[33m
NO_COLOR = \033[0m

debug: CFLAGS += -DDEBUG

DEFINES = -DSET_MIN_TEMP=15 -DSET_MAX_TEMP=20 -DTIMEOUT=5
IP = 127.0.0.1
PORT = 6543
CC = gcc

SOURCES = main.c connmgr.c datamgr.c sensor_db.c sbuffer.c
OBJECTS = main.o connmgr.o datamgr.o sensor_db.o sbuffer.o

CFLAGS = -c -Wall -Werror -fdiagnostics-color=auto -g
LFLAGS = -Wall -L./lib -Wl,-rpath=./lib -fdiagnostics-color=auto

CLIBF = -c -Wall -std=c11 -Werror -fPIC -fdiagnostics-color=auto
LLIBF = -Wall -shared -lm -fdiagnostics-color=auto

CPP = cppcheck --enable=all --suppress=missingIncludeSystem
VAL = valgrind --tool=memcheck --leak-check=yes

all: sensor_gateway sensor_node file_creator

sensor_gateway : $(SOURCES) lib/libdplist.so lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** CPPCHECK *****$(NO_COLOR)"
	$(CPP) $(SOURCES) $(DEFINES)
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_gateway *****$(NO_COLOR)"
	$(CC) main.c $(CFLAGS) $(DEFINES) -o main.o      
	$(CC) connmgr.c $(CFLAGS) $(DEFINES) -o connmgr.o
	$(CC) datamgr.c $(CFLAGS) $(DEFINES) -o datamgr.o
	$(CC) sensor_db.c $(CFLAGS) $(DEFINES) -o sensor_db.o
	$(CC) sbuffer.c $(CFLAGS) $(DEFINES) -o sbuffer.o
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_gateway *****$(NO_COLOR)"
	$(CC) $(OBJECTS) $(LFLAGS) -ldplist -ltcpsock -lpthread -lsqlite3 -o sensor_gateway

file_creator : file_creator.c
	@echo "$(TITLE_COLOR)\n***** COMPILING file_creator *****$(NO_COLOR)"
	$(CC) file_creator.c $(CFLAGS) -o file_creator.o
	@echo "$(TITLE_COLOR)\n***** LINKING file_creator *****$(NO_COLOR)"
	$(CC) file_creator.o -o file_creator

sensor_node : sensor_node.c lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_node *****$(NO_COLOR)"
	$(CC) sensor_node.c $(CFLAGS) -o sensor_node.o 
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_node *****$(NO_COLOR)"
	$(CC) sensor_node.o $(LFLAGS) -ltcpsock -o sensor_node

libdplist : lib/libdplist.so
libsbuffer : lib/libsbuffer.so
libtcpsock : lib/libtcpsock.so

lib/libdplist.so : lib/dplist.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB dplist *****$(NO_COLOR)"
	$(CC) lib/dplist.c $(CLIBF) -o lib/dplist.o
	@echo "$(TITLE_COLOR)\n***** LINKING LIB dplist *****$(NO_COLOR)"
	$(CC) lib/dplist.o $(LLIBF) -o lib/libdplist.so

lib/libsbuffer.so : lib/sbuffer.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB sbuffer *****$(NO_COLOR)"
	$(CC) lib/sbuffer.c $(CLIBF) -o lib/sbuffer.o
	@echo "$(TITLE_COLOR)\n***** LINKING LIB sbuffer *****$(NO_COLOR)"
	$(CC) lib/sbuffer.o $(LLIBF) -o lib/libsbuffer.so

lib/libtcpsock.so : lib/tcpsock.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB tcpsock *****$(NO_COLOR)"
	$(CC) lib/tcpsock.c $(CLIBF) -o lib/tcpsock.o
	@echo "$(TITLE_COLOR)\n***** LINKING LIB tcpsock *****$(NO_COLOR)"
	$(CC) lib/tcpsock.o $(LLIBF) -o lib/libtcpsock.so

# do not look for files called clean, clean-all or this will be always a target
.PHONY : clean-obj clean-exe clean-so clean-log clean clear

clean : clean-obj clean-exe
	
clear : clean clean-log clean-so
	rm -rf *~

clean-obj :
	@echo "$(TITLE_COLOR)\n***** CLEANING .o files *****$(NO_COLOR)"
	rm -rf *.o lib/*.o

clean-exe :
	@echo "$(TITLE_COLOR)\n***** CLEANING .exe files *****$(NO_COLOR)"
	rm -rf sensor_gateway file_creator sensor_node a.out

clean-so :
	@echo "$(TITLE_COLOR)\n***** CLEANING .so files *****$(NO_COLOR)"
	rm -rf lib/*.so

clean-log : 
	@echo "$(TITLE_COLOR)\n***** CLEANING log files *****$(NO_COLOR)"
	rm -rf gateway.log logFifo Sensor.db  

# test-run

debug: runval

run : sensor_gateway
	@echo "$(TITLE_COLOR)\n***** RUNNING sensor_gateway *****$(NO_COLOR)"
	./sensor_gateway $(PORT)

runval : sensor_gateway
	@echo "$(TITLE_COLOR)\n***** RUNNING sensor_gateway with VALGRIND *****$(NO_COLOR)"
	$(VAL) ./sensor_gateway $(PORT)

# test-scripts

kill : sensor_node
	killall sensor_node

test-scal : sensor_node
	timeout 13s ./sensor_node 132 3 $(IP) $(PORT) &
	timeout 19s ./sensor_node 21 2 $(IP) $(PORT) &
	timeout 28s ./sensor_node 129 1 $(IP) $(PORT) &
	timeout 11s ./sensor_node 37 4 $(IP) $(PORT) &
	timeout 25s ./sensor_node 49 2 $(IP) $(PORT) &
	timeout 22s ./sensor_node 112 1 $(IP) $(PORT) &
	timeout 18s ./sensor_node 15 2 $(IP) $(PORT) &
	timeout 15s ./sensor_node 142 4 $(IP) $(PORT) &

test-stress : sensor_node
	timeout 30s ./sensor_node 132 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 21 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 129 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 37 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 49 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 112 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 15 1 $(IP) $(PORT) &
	timeout 30s ./sensor_node 142 1 $(IP) $(PORT) &

test-endur : sensor_node
	timeout 1h ./sensor_node 132 1 $(IP) $(PORT) &
	timeout 1h ./sensor_node 21 2 $(IP) $(PORT) &
	timeout 1h ./sensor_node 129 3 $(IP) $(PORT) &
	timeout 1h ./sensor_node 37 4 $(IP) $(PORT) &
	timeout 1h ./sensor_node 49 1 $(IP) $(PORT) &
	timeout 1h ./sensor_node 112 2 $(IP) $(PORT) &
	timeout 1h ./sensor_node 15 3 $(IP) $(PORT) &
	timeout 1h ./sensor_node 142 4 $(IP) $(PORT) &

test-timeout : sensor_node
	./sensor_node 132 6 $(IP) $(PORT) &
	./sensor_node 21 3 $(IP) $(PORT) &
	./sensor_node 129 8 $(IP) $(PORT) &
	./sensor_node 37 9 $(IP) $(PORT) &
	./sensor_node 49 4 $(IP) $(PORT) &
	./sensor_node 112 11 $(IP) $(PORT) &
	./sensor_node 15 12 $(IP) $(PORT) &
	./sensor_node 142 13 $(IP) $(PORT) &

s1 : sensor_node
	./sensor_node 15 1 $(IP) $(PORT)
s2 : sensor_node
	./sensor_node 21 3 $(IP) $(PORT)
s3 : sensor_node
	./sensor_node 37 10 $(IP) $(PORT)
s4 : sensor_node
	./sensor_node 49 10 $(IP) $(PORT)


