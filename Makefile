CC=gcc
CFLAGS= -std=gnu99 -pedantic -Wall -Wextra

hinfosvc: hinfosvc.c
	$(CC) $(CFLAGS) hinfosvc.c -o hinfosvc -lm
