#!/bin/bash

echo "Inicio"

cd ../lib/sala
gcc -c sala.c
cd ../retardo
gcc -c retardo.c
cd ..
ar crs libsala.a sala/sala.o
ar crs libretardo.a retardo/retardo.o
cd ../fuentes
gcc -pthread multihilos.c -o multihilos -lsala -lretardo -L../lib

echo "Compilado"
