ifeq ($(shell uname -s),Darwin)
LDFLAGS += -framework OpenCL
else
LDFLAGS += -lOpenCL
endif

prog: main.o
	$(CC) $< ${LDFLAGS} -o $@

clean:
	rm -f main.o prog

