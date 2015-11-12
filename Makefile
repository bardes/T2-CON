# Grupo: 25a
# Paulo Bardes
# Luz Padilla

EXEC = filter

WARNINGS = -pedantic -Wall -Wextra -Wcast-align -Wcast-qual \
		   -Wdisabled-optimization -Wformat=2 -Wlogical-op -Wmissing-declarations \
		   -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion \
		   -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused

DANGER = -funsafe-math-optimizations -ffinite-math-only -fsingle-precision-constant \
		 -funroll-loops -faggressive-loop-optimizations 

CFLAGS = $(WARNINGS) -std=c11 -march=native -Ofast -fopenmp 

CPPFLAGS = -D_POSIX_SOURCE

LDFLAGS = -lrt

SRCS = $(shell find -name '*.c')
HDRS = $(shell find -name '*.h')
RES  = README

OBJS = $(SRCS:=.o)
DEPS = $(SRCS:=.dep)

.PHONY: clean all run zip debug noomp

all: $(EXEC)

debug: CFLAGS += -g -Og
debug: CPPFLAGS += -DDEBUG
debug: all

noomp: CFLAGS += -fno-openmp
noomp: debug


#Linka e gera o executavel
$(EXEC): $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) $(LDFLAGS)

#Compila cada arquivo
%.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

#Gera as dependencias
%.dep: %
	$(CC) -MM -MT "$@ $(@:.dep=.o)" $(CPPFLAGS) $< -MF $@

clean:
	rm -f $(OBJS) $(DEPS) $(EXEC)

zip:
	zip -r $(EXEC).zip $(SRCS) $(HDRS) $(RES) Makefile 

-include $(DEPS)
