// Práctica 3: hilos.
// Reto 2. Hilos gratuitos y de pago.

#include <stdio.h>	// printf
#include <stdlib.h>	// exit
#include "../cabeceras/sala.h"
#include "../cabeceras/retardo.h"
#include <pthread.h>
#include <unistd.h>
int n_hilos_gratuito_totales; // número de hilos de reservas gratuitas
int n_hilos_pago_totales;	// número de hilos de reservas de pago

int n_hilos_reserva_gratuito_creados;
int n_hilos_libera_gratuito_creados;
int n_hilos_reserva_gratuito_terminados;
int n_hilos_libera_gratuito_terminados;

int n_hilos_reserva_pago_creados;
int n_hilos_libera_pago_creados;
int n_hilos_reserva_pago_terminados;
int n_hilos_libera_pago_terminados;

int* asiento_gratuito;
int* asiento_pago;

int cancelar_hilos_reserva_gratuito = 0;
int cancelar_hilos_reserva_pago = 0;
int cancelar_hilos_libera_gratuito = 0;
int cancelar_hilos_libera_pago = 0;

int reservas_gratuitas_actuales = 0;
int reservas_pago_actuales = 0;

//pthread_mutex_t cerrojo_hilo_reserva_terminado;
//pthread_mutex_t cerrojo_hilo_libera_terminado;

pthread_mutex_t cerrojo_hilos_reserva_gratuito;
pthread_mutex_t cerrojo_hilos_libera_gratuito;
pthread_mutex_t cerrojo_hilos_reserva_pago;
pthread_mutex_t cerrojo_hilos_libera_pago;

pthread_mutex_t cerrojo_reservas_gratuitas_actuales;

pthread_cond_t cambio_en_sala;
pthread_mutex_t cerrojo_sala;


int NUM_ASIENTOS_GRATUITOS_MAX;

void inicializar_pthread() {
	pthread_cond_init(&cambio_en_sala, NULL);
	pthread_mutex_init(&cerrojo_sala, NULL);
	//pthread_mutex_init(&cerrojo_hilo_reserva_terminado, NULL);
	//pthread_mutex_init(&cerrojo_hilo_libera_terminado, NULL);
	pthread_mutex_init(&cerrojo_reservas_gratuitas_actuales, NULL);

	pthread_mutex_init(&cerrojo_hilos_reserva_gratuito, NULL);
	pthread_mutex_init(&cerrojo_hilos_libera_gratuito, NULL);
	pthread_mutex_init(&cerrojo_hilos_reserva_pago, NULL);
	pthread_mutex_init(&cerrojo_hilos_libera_pago, NULL);
}

void add_asiento_gratuito(int asiento) {
	for (int i = 0; i < NUM_ASIENTOS_GRATUITOS_MAX; i++) {
		if (asiento_gratuito[i] == 0) {
			asiento_gratuito[i] = asiento;
			return;
		}
	}
}

void add_asiento_pago(int asiento) {
	for (int i = 0; i < capacidad(); i++) {
		if (asiento_pago[i] == 0) {
			asiento_pago[i] = asiento;
			return;
		}
	}
}

void reserva_gratuita() {
	pthread_mutex_lock(&cerrojo_sala);
	while (reservas_gratuitas_actuales + 1 > NUM_ASIENTOS_GRATUITOS_MAX) {
		pthread_cond_wait(&cambio_en_sala, &cerrojo_sala);
		if (cancelar_hilos_reserva_gratuito) {
			pthread_mutex_unlock(&cerrojo_sala);
			fprintf(stdout, "El hilo de reservas gratuitas: %ld no ha podido reservar", pthread_self());
		}
	}
	int asiento;
	do { // si el id generado pertenece a una persona ya sentada, se intenta reservar asiento con otro id.
		asiento = reserva_asiento(rand() % (capacidad() * 1000));
	} while (asiento == -1);
	add_asiento_gratuito(asiento);
	reservas_gratuitas_actuales++;
	pthread_mutex_unlock(&cerrojo_sala);
	pthread_cond_broadcast(&cambio_en_sala);
}

void reserva_pago() {
	pthread_mutex_lock(&cerrojo_sala);
	while (asientos_libres() == 0) {
		pthread_cond_wait(&cambio_en_sala, &cerrojo_sala);
	}
	int asiento;
	do { // si el id generado pertenece a una persona ya sentada, se intenta reservar asiento con otro id.
		asiento = reserva_asiento(rand() % (capacidad() * 1000));
	} while (asiento == -1);
	add_asiento_pago(asiento);
	reservas_pago_actuales++;
	pthread_mutex_unlock(&cerrojo_sala);
	pthread_cond_broadcast(&cambio_en_sala);
}


void libera_gratuita() {
	pthread_mutex_lock(&cerrojo_sala);
	for (int i = 0; i < NUM_ASIENTOS_GRATUITOS_MAX; i++) {
		if (asiento_gratuito[i] != 0) {
			libera_asiento(asiento_gratuito[i]);
			asiento_gratuito[i] = 0;
			reservas_gratuitas_actuales--;
			pthread_mutex_unlock(&cerrojo_sala);
			pthread_cond_broadcast(&cambio_en_sala);
			pthread_exit(NULL);
		}
	}
	pthread_mutex_unlock(&cerrojo_sala);
	pthread_cond_broadcast(&cambio_en_sala);
}

void libera_pago() {
	pthread_mutex_lock(&cerrojo_sala);
	while (reservas_pago_actuales == 0) {
		pthread_cond_wait(&cambio_en_sala, &cerrojo_sala);
	}
	for (int i = 0; i < capacidad(); i++) {
		if (asiento_pago[i] != 0) {
			libera_asiento(asiento_pago[i]);
			asiento_pago[i] = 0;
			reservas_pago_actuales--;
			pthread_mutex_unlock(&cerrojo_sala);
			pthread_cond_broadcast(&cambio_en_sala);
			pthread_exit(NULL);
		}
	}
	pthread_mutex_unlock(&cerrojo_sala);
	pthread_cond_broadcast(&cambio_en_sala);
}

void* ejecutar_gratuito() {
	if (rand() % 2) {
		pthread_mutex_lock(&cerrojo_hilos_libera_gratuito);
		n_hilos_libera_gratuito_creados++;
		pthread_mutex_unlock(&cerrojo_hilos_libera_gratuito);
		libera_gratuita();
	} else {
		pthread_mutex_lock(&cerrojo_hilos_reserva_gratuito);
		n_hilos_reserva_gratuito_creados++;
		pthread_mutex_unlock(&cerrojo_hilos_reserva_gratuito);
		reserva_gratuita();
	}
	return NULL;
}

void* ejecutar_pago() {
	if (rand() % 2) {
		pthread_mutex_lock(&cerrojo_hilos_libera_pago);
		n_hilos_libera_pago_creados++;
		pthread_mutex_unlock(&cerrojo_hilos_libera_pago);
		libera_pago();
	} else {
		pthread_mutex_lock(&cerrojo_hilos_reserva_pago);
		n_hilos_libera_pago_creados++;
		pthread_mutex_unlock(&cerrojo_hilos_reserva_pago);
		reserva_pago();
	}
	return NULL;
}

void* mostrar_estado_sala() {
	while (1) {
		pthread_mutex_lock(&cerrojo_sala);
		estado_sala();
		printf("Reservas de entradas gratuitas actuales: %d\n", reservas_gratuitas_actuales);
		printf("Reservas de entradas de pago actuales: %d\n", reservas_pago_actuales);
		pthread_mutex_unlock(&cerrojo_sala);
		if ((n_hilos_reserva_gratuito_terminados == n_hilos_reserva_gratuito_creados) && (reservas_gratuitas_actuales == 0)) {
			cancelar_hilos_libera_gratuito = 1;
			pthread_cond_broadcast(&cambio_en_sala);
			break;
		}
		if ((n_hilos_reserva_pago_terminados == n_hilos_reserva_pago_creados) && (asientos_libres() == 0)) {
			cancelar_hilos_reserva_pago = 1;
			pthread_cond_broadcast(&cambio_en_sala);
			break;
		}
		sleep(1);
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "El número de argumentos no es válido. Debe ser 3: ./multihilos n m\n");
		exit(-1);
	}
	n_hilos_gratuito_totales = atoi(argv[1]);
	n_hilos_pago_totales = atoi(argv[2]);
	if (n_hilos_gratuito_totales < 1) {
		fprintf(stderr, "El número de hilos de reservas gratuitas debe ser mayor a 0\n");
		exit(-1);
	}
	if (n_hilos_pago_totales < 1) {
		fprintf(stderr, "El número de hilos de reservas de pago debe ser mayor a 0\n");
		exit(-1);
	}
	crea_sala((n_hilos_gratuito_totales + n_hilos_pago_totales) * 2);
	NUM_ASIENTOS_GRATUITOS_MAX = (int) (0.1 * capacidad());
	asiento_gratuito = malloc(sizeof(int) * NUM_ASIENTOS_GRATUITOS_MAX);
	asiento_pago = malloc(sizeof(int) * capacidad());
	srand(getpid()); // Cambiar la semilla para cada ejecución del programa

	inicializar_pthread();

	pthread_t hilos_gratuito[n_hilos_gratuito_totales];
	pthread_t hilos_pago[n_hilos_pago_totales];

	n_hilos_reserva_gratuito_creados = 0;
	n_hilos_libera_gratuito_creados = 0;

	n_hilos_reserva_pago_creados = 0;
	n_hilos_libera_pago_creados = 0;

	int n_hilos_gratuito_creados = 0;
	int n_hilos_pago_creados = 0;
	while (n_hilos_reserva_gratuito_creados + n_hilos_libera_gratuito_creados < n_hilos_gratuito_totales
			&& n_hilos_reserva_pago_creados + n_hilos_libera_pago_creados < n_hilos_pago_totales) {
		// Crear hilos de reservas gratuitas y de pago de forma aleatoria
		if (rand() % 2) {
			if (pthread_create(hilos_gratuito + n_hilos_gratuito_creados, NULL, ejecutar_gratuito, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de reserva\n");
			}
			n_hilos_gratuito_creados++;
		} else {
			if (pthread_create(hilos_pago + n_hilos_pago_creados, NULL, ejecutar_pago, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de liberación\n");
			}
			n_hilos_pago_creados++;
		}
	}
	// Solo queda crear uno de los dos tipos de hilos
	for (; n_hilos_gratuito_creados  < n_hilos_gratuito_totales; n_hilos_gratuito_creados++) {
		if (pthread_create(hilos_gratuito + n_hilos_gratuito_creados, NULL, ejecutar_gratuito, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de reserva\n");
		}
	}
	for (; n_hilos_pago_creados < n_hilos_pago_totales; n_hilos_pago_creados++) {
		if (pthread_create(hilos_pago + n_hilos_pago_creados, NULL, ejecutar_gratuito, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de liberación\n");
		}
	}

	pthread_t hilo_estado;
	if (pthread_create(&hilo_estado, NULL, mostrar_estado_sala, NULL) != 0) {
		fprintf(stderr, "Error al crear hilo estado\n");
	}

	for (int i = 0; i < n_hilos_gratuito_totales; i++) {
		if (pthread_join(hilos_gratuito[i], NULL) != 0) {
			fprintf(stderr, "Error al terminar hilo de reserva\n");
		}
	}

	for (int i = 0; i < n_hilos_pago_totales; i++) {
		if (pthread_join(hilos_pago[i], NULL) != 0) {
			fprintf(stderr, "Error al terminar hilo de liberación\n");
		}
	}
	puts("\n\nEstado final sala:");
	estado_sala();
	printf("Reservas de entradas gratuitas actuales: %d\n", reservas_gratuitas_actuales);
	printf("Reservas de entradas de pago actuales: %d\n", reservas_pago_actuales);
	elimina_sala();
	free(asiento_gratuito);
	free(asiento_pago);
	puts("\nTerminado.");
	return(0);
}
