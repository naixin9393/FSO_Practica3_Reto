// Práctica 3: hilos.
// Hito 3
// PRUEBA

#include <stdio.h>	// printf
#include <stdlib.h>	// exit
#include "../cabeceras/sala.h"
#include "../cabeceras/retardo.h"
#include <pthread.h>
#include <unistd.h>
int n_hilos_gratuito; // número de hilos de reservas gratuitas 
int n_hilos_pago;	// número de hilos de reservas de pago

int hilos_gratuito_terminados = 0;
int hilos_pago_terminados = 0;

int cancelar_hilos_reserva = 0;
int cancelar_hilos_libera = 0;

pthread_mutex_t cerrojo_hilo_reserva_terminado;
pthread_mutex_t cerrojo_hilo_libera_terminado;

pthread_cond_t cambio_en_sala;
pthread_mutex_t cerrojo_sala;

int buscar_asiento_ocupado() {
	int i;
	for (i = 0; i < capacidad(); i++) {
		if (estado_asiento(i) != 0) {
			return i;
		}
	}
}

void* ejecutar_gratuito() {
	for (int i = 0; i < 3; i++) {
		pthread_mutex_lock(&cerrojo_sala);
		while (asientos_libres() == 0) {
			pthread_cond_wait(&cambio_en_sala, &cerrojo_sala);
			if (cancelar_hilos_reserva) {
				fprintf(stdout, "Hilo de reserva %ld no ha podido terminar de reservar.\n", pthread_self());
				pthread_mutex_unlock(&cerrojo_sala);
				pthread_exit(NULL);
			}	
		}
		int asiento;
		do {
			asiento = reserva_asiento(rand() % (capacidad() * 1000));
		} while (asiento == -1);
		pthread_mutex_unlock(&cerrojo_sala);
		pthread_cond_broadcast(&cambio_en_sala);
		pausa_aleatoria(2.f);
	}
	pthread_mutex_lock(&cerrojo_hilo_reserva_terminado);
	hilos_gratuito_terminados++;
	pthread_mutex_unlock(&cerrojo_hilo_reserva_terminado);
}

void* ejecutar_libera() {
	for (int i = 0; i < 3; i++) {
		pthread_mutex_lock(&cerrojo_sala);
		while (asientos_ocupados() == 0) {
			pthread_cond_wait(&cambio_en_sala, &cerrojo_sala);
			if (cancelar_hilos_libera) { // Cancelar hilos libera si no quedan hilos reserva y la sala está vacía.
				fprintf(stdout, "Hilo de liberación %ld no ha podido terminar de liberar.\n", pthread_self());
				pthread_mutex_unlock(&cerrojo_sala);
				pthread_exit(NULL);
			}			
		}
		int asiento = buscar_asiento_ocupado();
		libera_asiento(asiento);
		pthread_mutex_unlock(&cerrojo_sala);
		pthread_cond_broadcast(&cambio_en_sala);
		pausa_aleatoria(2.f);
	}
	pthread_mutex_lock(&cerrojo_hilo_libera_terminado);
	hilos_pago_terminados++;
	pthread_mutex_unlock(&cerrojo_hilo_libera_terminado);
}

void* mostrar_estado_sala() {
	while (1) {
		estado_sala();
		if ((hilos_gratuito_terminados == n_hilos_gratuito) && (asientos_ocupados() == 0)) {
			cancelar_hilos_libera = 1;
			pthread_cond_broadcast(&cambio_en_sala);
			break;
		}
		if ((hilos_pago_terminados == n_hilos_pago) && (asientos_libres() == 0)) {
			cancelar_hilos_reserva = 1;
			pthread_cond_broadcast(&cambio_en_sala);
			break;
		}
		sleep(1);
	}
}

void main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "El número de argumentos no es válido. Debe ser 3: ./multihilos n m\n");
		exit(-1);
	}
	n_hilos_gratuito = atoi(argv[1]);
	n_hilos_pago = atoi(argv[2]);
	
	if (n_hilos_gratuito < 1) {
		fprintf(stderr, "El número de hilos de reservas gratuitas debe ser mayor a 0\n");
		exit(-1);
	}
	if (n_hilos_pago < 1) {
		fprintf(stderr, "El número de hilos de reservas de pago debe ser mayor a 0\n");
		exit(-1);
	}
	srand(getpid()); // Cambiar la semilla para cada ejecución del programa
	pthread_cond_init(&cambio_en_sala, NULL);
	pthread_mutex_init(&cerrojo_sala, NULL);
	pthread_mutex_init(&cerrojo_hilo_reserva_terminado, NULL);
	pthread_mutex_init(&cerrojo_hilo_libera_terminado, NULL);

	pthread_t hilos_gratuito[n_hilos_gratuito];
	pthread_t hilos_pago[n_hilos_pago];
	crea_sala((n_hilos_gratuito + n_hilos_pago) * 2);
	int hilos_gratuito_creados = 0;
	int hilos_pago_creados = 0;
	while (hilos_gratuito_creados < n_hilos_gratuito && hilos_pago_creados < n_hilos_pago) {
		// Crear hilos de reservas gratuitas y de pago de forma aleatoria
		if (rand() % 2) {	
			if (pthread_create(hilos_gratuito + hilos_gratuito_creados, NULL, ejecutar_gratuito, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de reserva\n");
			}
			hilos_gratuito_creados++;
		} else {
			if (pthread_create(hilos_pago + hilos_pago_creados, NULL, ejecutar_libera, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de liberación\n");
			}
			hilos_pago_creados++;
		}
	}
	// Solo queda crear uno de los dos tipos de hilos
	for (hilos_gratuito_creados; hilos_gratuito_creados  < n_hilos_gratuito; hilos_gratuito_creados++) {
		if (pthread_create(hilos_gratuito + hilos_gratuito_creados, NULL, ejecutar_gratuito, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de reserva\n");
		}
	}
	for (hilos_pago_creados; hilos_pago_creados < n_hilos_pago; hilos_pago_creados++) {
		if (pthread_create(hilos_pago + hilos_pago_creados, NULL, ejecutar_libera, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de liberación\n");
		}
	}

	pthread_t hilo_estado;
	if (pthread_create(&hilo_estado, NULL, mostrar_estado_sala, NULL) != 0) {
		fprintf(stderr, "Error al crear hilo estado\n");
	} 
	
	for (int i = 0; i < n_hilos_gratuito; i++) {
		if (pthread_join(hilos_gratuito[i], NULL) != 0) {
			fprintf(stderr, "Error al terminar hilo de reserva\n");
		}
	}

	for (int i = 0; i < n_hilos_pago; i++) {
		if (pthread_join(hilos_pago[i], NULL) != 0) {
			fprintf(stderr, "Error al terminar hilo de liberación\n");
		}
	}
	puts("\n\nEstado final sala:");
	estado_sala();
	elimina_sala();
	puts("\nTerminado.");
	exit(0);
}
