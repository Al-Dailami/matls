CERT_FILE=matls_cert.crt
CA_CERT_FILE=ca_alice.pem
KEY_FILE=matls_priv.pem

SERVER=server_side_mb
GCC=gcc
CFLAGS=-DDEBUG -o
SERVER_SRC=server_side_mb.c mssl.c
SERVER_OBJ=server_side_mb.o mssl.o
FLAGS=-I/home/hwlee/openssl-splittls-lib/include -L/home/hwlee/openssl-splittls-lib/lib
#FLAGS=-I/home/hwlee/openssl_lib/include -L/home/hwlee/openssl_lib/lib
#FLAGS=-I/usr/local/include -L/usr/local/lib
LDFLAGS=-lssl -lcrypto -ldl -lpthread
GDB=gdb

HOST=127.0.0.1
PORT=443
NUM_OF_THREADS=1

default: server

server:
	${GCC} ${CFLAGS} ${SERVER} ${SERVER_SRC} ${FLAGS} ${LDFLAGS}

sstart:
	./${SERVER} ${PORT} ${CERT_FILE} ${KEY_FILE} ${CA_CERT_FILE}

debug:
	${GDB} ./${EXEC} ${PORT} ${CERT_FILE} ${CA_CERT_FILE} ${KEY_FILE} ${ORIG_CERT_FILE} ${WARRANT_FILE} -tui

clean:
	rm ${SERVER}
