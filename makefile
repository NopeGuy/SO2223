

all: server client transformacoes

server: bin/sdstored

client: bin/sdstore

transformacoes: bin/bcompress bin/bdecompress bin/decrypt bin/encrypt bin/gcompress bin/gdecompress bin/nop

bin/sdstored: obj/sdstored.o ; gcc -g obj/sdstored.o -o bin/sdstored
obj/sdstored.o: src/sdstored.c ; gcc -Wall -g -c src/sdstored.c -o obj/sdstored.o
bin/sdstore: obj/sdstore.o ; gcc -g obj/sdstore.o -o bin/sdstore
obj/sdstore.o: src/sdstore.c ;  gcc -Wall -g -c src/sdstore.c -o obj/sdstore.o
bin/bcompress: obj/bcompress.o ; gcc -g obj/bcompress.o  -o bin/transforms/bcompress
obj/bcompress.o: src/bcompress.c ; gcc -Wall -g -c src/bcompress.c -o obj/bcompress.o
bin/bdecompress: obj/bdecompress.o ; gcc -g obj/bdecompress.o  -o bin/transforms/bdecompress
obj/bdecompress.o: src/bcompress.c ; gcc -Wall -g -c src/bdecompress.c -o obj/bdecompress.o
bin/decrypt: obj/decrypt.o ; gcc -g obj/decrypt.o  -o bin/transforms/decrypt
obj/decrypt.o: src/decrypt.c ; gcc -Wall -g -c src/decrypt.c -o obj/decrypt.o
bin/encrypt: obj/encrypt.o ; gcc -g obj/encrypt.o  -o bin/transforms/encrypt
obj/encrypt.o: src/encrypt.c; gcc -Wall -g -c src/encrypt.c -o obj/encrypt.o
bin/gcompress: obj/gcompress.o ; gcc -g obj/gcompress.o  -o bin/transforms/gcompress
obj/gcompress.o: src/gcompress.c ; gcc -Wall -g -c src/gcompress.c -o obj/gcompress.o
bin/gdecompress: obj/gdecompress.o ; gcc -g obj/gdecompress.o  -o bin/transforms/gdecompress
obj/gdecompress.o: src/gdecompress.c ; gcc -Wall -g -c src/gdecompress.c -o obj/gdecompress.o
bin/nop: obj/nop.o ; gcc -g obj/nop.o  -o bin/transforms/nop
obj/nop.o: src/nop.c ; gcc -Wall -g -c src/nop.c -o obj/nop.o

clean: ; rm -rf obj/* ; rm -rf tmp/* ;rm -rf bin/sdstore ; rm -rf bin/sdstored ;rm -rf bin/transforms/*