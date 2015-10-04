# Makefile for mnae0183

CC=gcc
CFLAGS=-I.
TARGET = nmea0183
SRC = nmea0183.c
OBJ = nmea0183.o
TESTDATA = data

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

$(OBJ): $(SRC)
	$(CC) -c -o $@ $^ $(CFLAGS)

cleantarget:
	-rm -f $(TARGET)

cleanobj:
	-rm -f $(OBJ)

relink: cleantarget $(TARGET)

clean: cleantarget cleanobj

test: $(TARGET) $(TESTDATA)
	./$(TARGET) <$(TESTDATA) >testoutput
