# $Id$

PREFIX = /usr

CC = $(SRCROOT)/dist/x86_64-linux-musl-cross/bin/x86_64-linux-musl-gcc
CFLAGS = --sysroot=$(SRCROOT)/root -std=c99 -g -DEXTENDED -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DFILEC -DNLS -DSHORT_STRINGS -D_KMEMUSER -Dunix -D_NETBSD_SOURCE -D_BSD_SOURCE=1 -D__BSD_VISIBLE=1 -DNET2_STAT -DPREFIX=\"$(PREFIX)\" -fcommon -I. -Werror -I$(TOPDIR) -D__musl__ -I$(TOPDIR)/musl-include
LDFLAGS = --sysroot=$(SRCROOT)/root
LIBS = -lfts

# include $(TOPDIR)/makefiles/musl-host.mk
# include $(TOPDIR)/makefiles/musl-gcc.mk
