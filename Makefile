OUTPUT=paladin
OBJS=main.o
CFLAGS=-Wall
LDFLAGS=
LIBS=

all: $(OUTPUT)

clean:
	rm -f $(OUTPUT) *.o

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
