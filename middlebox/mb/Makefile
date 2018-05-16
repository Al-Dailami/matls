CERT_FILE=matls_cert.crt
CA_CERT_FILE=ca_alice.pem
KEY_FILE=matls_priv.pem
FORWARD_FILE=forward.txt

SERVER=mb
GCC=gcc
CFLAGS=-DDEBUG -o
SERVER_SRC=mb.c mssl.c table.c common.c
SERVER_OBJ=mb.o mssl.o table.o common.o
#FLAGS=-I../../openssl-matls-lib/include -L../../openssl-matls-lib/lib
FLAGS=-I/home/hwlee/openssl-matls-lib/include -L/home/hwlee/openssl-matls-lib/lib
#FLAGS=-I/home/hwlee/openssl_lib/include -L/home/hwlee/openssl_lib/lib
#FLAGS=-I/usr/local/include -L/usr/local/lib
LDFLAGS=-lssl -lcrypto -ldl -lpthread
GDB=gdb

HOST=127.0.0.1
PORT=443
NUM_OF_THREADS=1

SERVER_1=mb1
SERVER_2=mb2
SERVER_3=mb3
PORT_1=5554
PORT_2=5555
PORT_3=5556
FORWARD_FILE_1=forward_1.txt
FORWARD_FILE_2=forward_2.txt
FORWARD_FILE_3=forward_3.txt

default: mb_server

mb_server:
	${GCC} ${CFLAGS} ${SERVER} ${SERVER_SRC} ${FLAGS} ${LDFLAGS}
	${GCC} ${CFLAGS} ${SERVER_1} ${SERVER_SRC} ${FLAGS} ${LDFLAGS}
	${GCC} ${CFLAGS} ${SERVER_2} ${SERVER_SRC} ${FLAGS} ${LDFLAGS}
	${GCC} ${CFLAGS} ${SERVER_3} ${SERVER_SRC} ${FLAGS} ${LDFLAGS}

mstart:
	sudo ./${SERVER} ${PORT} ${CERT_FILE} ${KEY_FILE} ${FORWARD_FILE}

mstart1:
	./${SERVER_1} ${PORT_1} ${CERT_FILE} ${KEY_FILE} ${FORWARD_FILE_1}

mstart2:
	./${SERVER_2} ${PORT_2} ${CERT_FILE} ${KEY_FILE} ${FORWARD_FILE_2}

mstart3:
	./${SERVER_3} ${PORT_3} ${CERT_FILE} ${KEY_FILE} ${FORWARD_FILE_3}

debug:
	${GDB} ./${EXEC} ${PORT} ${CERT_FILE} ${CA_CERT_FILE} ${KEY_FILE} ${ORIG_CERT_FILE} ${WARRANT_FILE} -tui

clean:
	rm ${SERVER} ${SERVER_1} ${SERVER_2} ${SERVER_3}