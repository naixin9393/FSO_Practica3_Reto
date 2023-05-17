// Funciones sala
// Igual que el hito2
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "../retardo/retardo.h"

int* asientos = NULL;
int salaEliminada;
int salaCreada;
int asientosLibres;
int asientosOcupados;
int capacidadSala;
pthread_mutex_t cerrojo;

int reserva_asiento(int id) {
	pthread_mutex_lock(&cerrojo);
	if (asientosLibres == 0) {
		pthread_mutex_unlock(&cerrojo);
		fprintf(stderr, "Error reservando. Sala llena\n");
		return -1;
	}
	if (id <= 0) {
		pthread_mutex_unlock(&cerrojo);
		fprintf(stderr, "Error reservando. Id inválido\n");
		return -1;
	}
	for (int i = 0; i < capacidadSala; i++) {
		if (*(asientos + i) == id) {
			pthread_mutex_unlock(&cerrojo);
			fprintf(stderr, "Persona ya tiene reserva\n");
			return -1;
		}
	}
	for (int i = 0; i < capacidadSala; i++) {
		if (*(asientos + i) == 0) {
			*(asientos + i) = id;
			asientosOcupados++;
			asientosLibres--;
			pthread_mutex_unlock(&cerrojo);
			return i;
		}
	}
	return -1;
}

int libera_asiento(int asiento) {
	if (asiento > capacidadSala - 1) {
		return -1;
	}
	pthread_mutex_lock(&cerrojo);
	if (*(asientos + asiento) == 0) {
		pthread_mutex_unlock(&cerrojo);
		return -1;
	}
	int id = *(asientos + asiento);
	*(asientos + asiento) = 0;
	asientosOcupados--;
	asientosLibres++;
	pthread_mutex_unlock(&cerrojo);
	return id;
}

int estado_asiento(int asiento) {
	if (asiento > capacidadSala - 1) {
		return -1;
	}
	pthread_mutex_lock(&cerrojo);
	int id = *(asientos + asiento);
	pthread_mutex_unlock(&cerrojo);
	return id;
}

int asientos_libres() {
	pthread_mutex_lock(&cerrojo);
	int libre = asientosLibres;
	pthread_mutex_unlock(&cerrojo);
	return libre;
}

int asientos_ocupados() {
	pthread_mutex_lock(&cerrojo);
	int ocupados = asientosOcupados;
	pthread_mutex_unlock(&cerrojo);
	return ocupados;
}

int capacidad() {
	return capacidadSala;
}

int sala_creada() {
	pthread_mutex_lock(&cerrojo);
	int creada = salaCreada;
	pthread_mutex_unlock(&cerrojo);
	return creada;
}

void inicializar_asientos(int nAsientos) {
	if (!sala_creada()) {
		return;
	}
	for (int i = 0; i < nAsientos; i++) {
		*(asientos + i) = 0;
	}
}

void elimina_sala() {
	if (!salaEliminada) {
		free(asientos);
	}
	pthread_mutex_lock(&cerrojo);
	salaEliminada = 1;
	salaCreada = 0;
	pthread_mutex_unlock(&cerrojo);
}

void crea_sala(int capacidad) {
	if (salaCreada) {
		return;
	}
	elimina_sala();
	pthread_mutex_init(&cerrojo, NULL);
	salaEliminada = 0;
	salaCreada = 1;
	capacidadSala = capacidad;
	asientos = malloc(capacidad * sizeof(int));
	inicializar_asientos(capacidad);
	asientosLibres = capacidad;
	asientosOcupados = 0;
}

void estado_sala() {
	printf("\nCapacidad: %d\n", capacidadSala);
	printf("Asientos libres: %d\n", asientosLibres);
	printf("Asientos ocupados: %d\n", asientosOcupados);
	printf("Estado actual de la sala: \n");
	for (int i = 0; i < capacidadSala; i++) {
		printf("Asiento %d : %d\n", i, *(asientos + i));
	}
}

int sala_eliminada() {
	pthread_mutex_lock(&cerrojo);
	int eliminada = salaEliminada;
	pthread_mutex_unlock(&cerrojo);
	return eliminada;
}

