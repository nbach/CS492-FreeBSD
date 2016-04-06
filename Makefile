SRCS=main.c
KMOD=main
COPTS=-g3

.include <bsd.kmod.mk>

all:
	clang kqtest.c -o kqtest