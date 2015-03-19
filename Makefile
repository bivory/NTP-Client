CC=gcc
CFLAGS=-Wall -pedantic -std=c99

ntp_client: ntp_client.o
		$(CC) -o ntp_client ntp_client.o
