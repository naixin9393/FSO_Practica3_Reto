//funciones sala
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
pthread_mutex_t cerrojo_sala;
pthread_mutex_t cerrojo_libres;
pthread_mutex_t cerrojo_ocupados;
pthread_mutex_t cerrojo_creada;
pthread_mutex_t cerrojo_eliminada;
pthread_mutex_t cerrojo;
pthread_cond_t cambio_sala;

int reserva_asiento(int id) {
	if (id <= 0) {
		return -1;
	}
	pthread_mutex_lock(&cerrojo);
	//printf("----Res %ld entra\n", pthread_self()%100);
	while (asientosLibres == 0) {
		//printf("----Res %ld duerme\n", pthread_self()%100);
		pthread_cond_wait(&cambio_sala, &cerrojo);
	}
	int ya_sentado;
	do {
		ya_sentado = 0;
		for (int i = 0; i < capacidadSala; i++) {
			if (*(asientos + i) == id) {
				ya_sentado = 1;
				pthread_cond_wait(&cambio_sala, &cerrojo);
				break;
			}
		}
	} while (ya_sentado);

	for (int i = 0; i < capacidadSala; i++) {
		if (*(asientos + i) == 0) {
			//printf("----Res %ld reserva\n", pthread_self()%100);
			*(asientos + i) = id;
			asientosOcupados++;
			asientosLibres--;
			pthread_mutex_unlock(&cerrojo);
			pthread_cond_broadcast(&cambio_sala);
			return i;
		}
	}
	return -1;
}

/*
int libera_asiento(int asiento) {
	pthread_mutex_lock(&cerrojo_ocupados);
	asientosOcupados--;
	pthread_mutex_unlock(&cerrojo_ocupados);

	if (asiento > capacidadSala - 1) {
		return -1;
	}
	pthread_mutex_lock(&cerrojo_sala);
	if (*(asientos + asiento) == 0) {
		pthread_mutex_unlock(&cerrojo_sala);
		return -1;
	}
	int id = *(asientos + asiento);
	*(asientos + asiento) = 0;
	pthread_mutex_unlock(&cerrojo_sala);
	
	pthread_mutex_lock(&cerrojo_libres);
	asientosLibres++;
	pthread_mutex_unlock(&cerrojo_libres);
	
		return id;
}
*/

void libera_asiento() {
	pthread_mutex_lock(&cerrojo);
	//printf("----Lib %ld entra\n", pthread_self()%100);
	//puts("Libera entra");
	while (asientosOcupados == 0) {
		//printf("----Lib %ld duerme\n", pthread_self()%100);
		pthread_cond_wait(&cambio_sala, &cerrojo);
	}
	for (int i = 0; i < capacidadSala; i++) {
		if (*(asientos + i) == 0) {
			continue;
		}
		// int id = *(asientos + i) = 0;
		asientosLibres++;
		asientosOcupados--;
		*(asientos + i) = 0;
		//printf("----Lib %ld libera\n", pthread_self()%100);
		pthread_mutex_unlock(&cerrojo);
		pthread_cond_broadcast(&cambio_sala);
		//return id;
		return;
	}
	pthread_mutex_unlock(&cerrojo);
	//fprintf(stderr, "No hay asiento que liberar-------------\n");
	return;
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
	pthread_mutex_lock(&cerrojo_creada);
	int creada = salaCreada;
	pthread_mutex_unlock(&cerrojo_creada);
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
	pthread_mutex_lock(&cerrojo_eliminada);
	salaEliminada = 1;
	pthread_mutex_unlock(&cerrojo_eliminada);
	
	pthread_mutex_lock(&cerrojo_creada);
	salaCreada = 0;
	pthread_mutex_unlock(&cerrojo_creada);
}

void crea_sala(int capacidad) {
	if (salaCreada) {
		return;
	}
	elimina_sala();
	salaEliminada = 0;
	salaCreada = 1;
	capacidadSala = capacidad;
	asientos = malloc(capacidad * sizeof(int));
	inicializar_asientos(capacidad);
	asientosLibres = capacidad;
	asientosOcupados = 0;
	pthread_mutex_init(&cerrojo, NULL);
	pthread_mutex_init(&cerrojo_sala, NULL);
	pthread_mutex_init(&cerrojo_libres, NULL);
	pthread_mutex_init(&cerrojo_ocupados, NULL);
	pthread_mutex_init(&cerrojo_creada, NULL);
	pthread_mutex_init(&cerrojo_eliminada, NULL);
	pthread_cond_init(&cambio_sala, NULL);
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
	pthread_mutex_lock(&cerrojo_eliminada);
	int eliminada = salaEliminada;
	pthread_mutex_unlock(&cerrojo_eliminada);
	return eliminada;
}


