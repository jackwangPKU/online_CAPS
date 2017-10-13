USER_CFLAGS := -g -Wall -std=c89
KDIR = /lib/modules/`uname -r`/build

USER_EXE := dumper
USER_OBJ := dumper.o dump-util.o

RESERVOIR_EXE := reservoir
RESERVOIR_OBJ := reservoir.o dump-util.o

obj-m := simple-pebs.o
M := make -C ${KDIR} M=`pwd`

all:
	${M} modules

install:
	${M} modules_install

clean:
	${M} clean
	rm -rf ${USER_EXE} ${USER_OBJS}

dump: ${USER_EXE}
${USER_OBJ} ${USER_EXE}: CFLAGS := ${USER_CFLAGS}

dumper: dump-util.o dumper.o

reser: ${RESERVOIR_EXE}
${RESERVOIR_OBJ} ${RESERVOIR_EXE}: CFLAGS := ${USER_CFLAGS}

reservoir: dump-util.o reservoir.o
