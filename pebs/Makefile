USER_CFLAGS := -g -Wall -std=c89
KDIR = /lib/modules/`uname -r`/build

USER_EXE := dumper
USER_OBJ := dumper.o dump-util.o

obj-m := simple-pebs.o
M := make -C ${KDIR} M=`pwd`

all:
	${M} modules

install:
	${M} modules_install

clean:
	${M} clean
	rm -rf ${USER_EXE} ${USER_OBJS}

user: ${USER_EXE}
${USER_OBJ} ${USER_EXE}: CFLAGS := ${USER_CFLAGS}

dumper: dump-util.o dumper.o
