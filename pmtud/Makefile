#export STAGING_DIR=/home/op/attitude_adjustment/staging_dir
include ../env.mk
CFLAGS=-Wall

all:
	${CC} ${CFLAGS} -o pmtud pmtud.c icmp.c
	${STRIP} pmtud

clean:
	rm -f pmtud

install:
	cp -rf pmtud ${ROOTFS}/usr/sbin
