// Práctica 3: hilos.
// Reto 2. Hilos gratuitos y de pago.

#include <stdio.h>	// printf
#include <stdlib.h>	// exit
#include "../cabeceras/sala.h"
#include "../cabeceras/retardo.h"
#include <pthread.h>
#include <unistd.h>

int NUM_HILOS_GRATUITO_TOTAL; // número de hilos gratuitos total
int NUM_HILOS_PAGO_TOTAL;	// número de hilos de pago total

// Contadores de hilos gratuitos creados
int num_hilos_reserva_gratuito_creados;
int num_hilos_libera_gratuito_creados;

// Contadores de hilos de gratuitos terminados
int num_hilos_reserva_gratuito_terminados;
int num_hilos_libera_gratuito_terminados;

// Contadores de hilos de pago creados
int num_hilos_reserva_pago_creados;
int num_hilos_libera_pago_creados;

// Contadores de hilos de pago terminados
int num_hilos_reserva_pago_terminados;
int num_hilos_libera_pago_terminados;

// Contadores de reservas gratuitas actuales
int reservas_gratuitas_actuales = 0;
int reservas_pago_actuales = 0;

// Pilas para diferenciar si un id pertenece a una entrada gratuita o de pago
int* asiento_gratuito;
int* asiento_pago;

// Índices que indica la última posición ocupada de la pila
int indice_asiento_gratuito = 0;
int indice_asiento_pago = 0;

// Flag para indicar la cancelación de hilos
int cancelar_hilos_reserva_gratuito = 0;
int cancelar_hilos_reserva_pago = 0;
int cancelar_hilos_libera_gratuito = 0;
int cancelar_hilos_libera_pago = 0;

// Cerrojos para acceder al contador de hilos de distintos tipos
pthread_mutex_t c_num_hilos_reserva_gratuito;
pthread_mutex_t c_num_hilos_libera_gratuito;
pthread_mutex_t c_num_hilos_reserva_pago;
pthread_mutex_t c_num_hilos_libera_pago;

// Cerrojo para acceder al contador de reservas gratuitas actuales
pthread_mutex_t c_reservas_gratuitas_actuales;

// Variables condicion que indica cambios en la sala
pthread_cond_t se_ha_reservado_asiento;
pthread_cond_t se_ha_liberado_asiento;

// Cerrojo para modificar datos de la sala
pthread_mutex_t c_sala;

int NUM_PLAZAS_GRATUITAS;

void inicializar_pthread() {
	//pthread_mutex_init(&c_hilo_reserva_terminado, NULL);
	//pthread_mutex_init(&c_hilo_libera_terminado, NULL);
	pthread_mutex_init(&c_num_hilos_reserva_gratuito, NULL);
	pthread_mutex_init(&c_num_hilos_libera_gratuito, NULL);
	pthread_mutex_init(&c_num_hilos_reserva_pago, NULL);
	pthread_mutex_init(&c_num_hilos_libera_pago, NULL);

	pthread_mutex_init(&c_reservas_gratuitas_actuales, NULL);

	pthread_cond_init(&se_ha_reservado_asiento, NULL);
	pthread_cond_init(&se_ha_liberado_asiento, NULL);

	pthread_mutex_init(&c_sala, NULL);
}

void add_asiento_gratuito(int asiento) {
	for (int i = 0; i < NUM_PLAZAS_GRATUITAS; i++) {
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
	pthread_mutex_lock(&c_sala);
	while (reservas_gratuitas_actuales + 1 > NUM_PLAZAS_GRATUITAS) {
		pthread_cond_wait(&se_ha_liberado_asiento, &c_sala);
		if (cancelar_hilos_reserva_gratuito) {
			pthread_mutex_unlock(&c_sala);
			fprintf(stdout, "El hilo de reservas gratuitas: %ld no ha podido reservar", pthread_self());
		}
	}
	int asiento;
	do { // si el id generado pertenece a una persona ya sentada, se intenta reservar asiento con otro id.
		asiento = reserva_asiento(rand() % (capacidad() * 1000));
	} while (asiento == -1);
	asiento_gratuito[indice_asiento_gratuito] = asiento;
	indice_asiento_gratuito++;
	//reservas_gratuitas_actuales++;
	pthread_cond_broadcast(&se_ha_reservado_asiento);
	pthread_mutex_unlock(&c_sala);
}

void reserva_pago() {
	pthread_mutex_lock(&c_sala);
	while (asientos_libres() == 0) {
		pthread_cond_wait(&se_ha_liberado_asiento, &c_sala);
		if (cancelar_hilos_reserva_pago) {
			pthread_mutex_unlock(&c_sala);
			fprintf(stdout, "El hilo de reservas de pago: %ld no ha podido reservar", pthread_self());
		}
	}
	int asiento;
	do { // si el id generado pertenece a una persona ya sentada, se intenta reservar asiento con otro id.
		asiento = reserva_asiento(rand() % (capacidad() * 1000));
	} while (asiento == -1);
	//add_asiento_pago(asiento);
	asiento_pago[indice_asiento_pago] = asiento;
	indice_asiento_pago++;
	//reservas_pago_actuales++;
	pthread_cond_broadcast(&se_ha_reservado_asiento);
	pthread_mutex_unlock(&c_sala);
}


void libera_gratuita() {
	pthread_mutex_lock(&c_sala);
	if (indice_asiento_gratuito == 0) {
		pthread_exit(NULL);
	}
	int asiento = asiento_gratuito[indice_asiento_gratuito - 1];
	asiento_gratuito[indice_asiento_gratuito - 1] = 0;
	indice_asiento_gratuito--;
	libera_asiento(asiento);
	pthread_cond_broadcast(&se_ha_liberado_asiento);
	pthread_mutex_unlock(&c_sala);
}

void libera_pago() {
	pthread_mutex_lock(&c_sala);
	while (reservas_pago_actuales == 0) {
		pthread_cond_wait(&se_ha_reservado_asiento, &c_sala);
	}
	int asiento = asiento_pago[indice_asiento_pago - 1];
	libera_asiento(asiento);
	asiento_pago[indice_asiento_pago - 1] = 0;
	indice_asiento_pago--;
	pthread_mutex_unlock(&c_sala);
	pthread_cond_broadcast(&se_ha_reservado_asiento);
}

void* rutina_gratuito() {
	if (rand() % 2) {
		pthread_mutex_lock(&c_num_hilos_libera_gratuito);
		num_hilos_libera_gratuito_creados++;
		pthread_mutex_unlock(&c_num_hilos_libera_gratuito);
		libera_gratuita();
	} else {
		pthread_mutex_lock(&c_num_hilos_reserva_gratuito);
		num_hilos_reserva_gratuito_creados++;
		pthread_mutex_unlock(&c_num_hilos_reserva_gratuito);
		reserva_gratuita();
	}
	return NULL;
}

void* rutina_pago() {
	if (rand() % 2) {
		pthread_mutex_lock(&c_num_hilos_libera_pago);
		num_hilos_libera_pago_creados++;
		pthread_mutex_unlock(&c_num_hilos_libera_pago);
		libera_pago();
	} else {
		pthread_mutex_lock(&c_num_hilos_reserva_pago);
		num_hilos_reserva_pago_creados++;
		pthread_mutex_unlock(&c_num_hilos_reserva_pago);
		reserva_pago();
	}
	pthread_exit(NULL);
}

void* mostrar_estado_sala() {
	while (1) {
		pthread_mutex_lock(&c_sala);
		estado_sala();
		printf("Reservas de entradas gratuitas actuales: %d\n", indice_asiento_gratuito);
		printf("Reservas de entradas de pago actuales: %d\n", indice_asiento_pago);
		pthread_mutex_unlock(&c_sala);
		if ((num_hilos_reserva_gratuito_terminados == num_hilos_reserva_gratuito_creados) && (reservas_gratuitas_actuales == 0)) {
			cancelar_hilos_libera_gratuito = 1;
			pthread_cond_broadcast(&se_ha_reservado_asiento);
			break;
		}
		if ((num_hilos_reserva_pago_terminados == num_hilos_reserva_pago_creados) && (asientos_libres() == 0)) {
			cancelar_hilos_reserva_pago = 1;
			pthread_cond_broadcast(&se_ha_reservado_asiento);
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
	NUM_HILOS_GRATUITO_TOTAL = atoi(argv[1]);
	NUM_HILOS_PAGO_TOTAL = atoi(argv[2]);
	if (NUM_HILOS_GRATUITO_TOTAL < 1) {
		fprintf(stderr, "El número de hilos de reservas gratuitas debe ser mayor a 0\n");
		exit(-1);
	}
	if (NUM_HILOS_PAGO_TOTAL < 1) {
		fprintf(stderr, "El número de hilos de reservas de pago debe ser mayor a 0\n");
		exit(-1);
	}
	crea_sala((NUM_HILOS_GRATUITO_TOTAL + NUM_HILOS_PAGO_TOTAL) * 2);
	NUM_PLAZAS_GRATUITAS = (int) (0.1 * capacidad()); // 10% de las plazas totales son usadas por entradas gratuitas

	asiento_gratuito = malloc(sizeof(int) * NUM_PLAZAS_GRATUITAS);
	asiento_pago = malloc(sizeof(int) * capacidad());

	srand(getpid()); // Cambiar la semilla para cada ejecución del programa
	inicializar_pthread(); // Inicializar cerrojos y variables condición

	pthread_t hilos_gratuito[NUM_HILOS_GRATUITO_TOTAL];
	pthread_t hilos_pago[NUM_HILOS_PAGO_TOTAL];

	num_hilos_reserva_gratuito_creados = 0;
	num_hilos_libera_gratuito_creados = 0;

	num_hilos_reserva_pago_creados = 0;
	num_hilos_libera_pago_creados = 0;

	int num_hilos_gratuito_creados = 0;
	int num_hilos_pago_creados = 0;

	while (num_hilos_gratuito_creados < NUM_HILOS_GRATUITO_TOTAL
			&& num_hilos_pago_creados < NUM_HILOS_PAGO_TOTAL) {
		// Crear hilos de reservas gratuitas y de pago de forma aleatoria
		if (rand() % 2) {
			if (pthread_create(hilos_gratuito + num_hilos_gratuito_creados, NULL, rutina_gratuito, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de reserva\n");
			}
			num_hilos_gratuito_creados++;
		} else {
			if (pthread_create(hilos_pago + num_hilos_pago_creados, NULL, rutina_pago, NULL) != 0) {
				fprintf(stderr, "Error al crear hilo de liberación\n");
			}
			num_hilos_pago_creados++;
		}
	}
	// Solo queda crear uno de los dos tipos de hilos
	for (; num_hilos_gratuito_creados  < NUM_HILOS_GRATUITO_TOTAL; num_hilos_gratuito_creados++) {
		if (pthread_create(hilos_gratuito + num_hilos_gratuito_creados, NULL, rutina_gratuito, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de reserva\n");
		}
	}
	for (; num_hilos_pago_creados < NUM_HILOS_PAGO_TOTAL; num_hilos_pago_creados++) {
		if (pthread_create(hilos_pago + num_hilos_pago_creados, NULL, rutina_gratuito, NULL) != 0) {
			fprintf(stderr, "Error al crear hilo de liberación\n");
		}
	}

	pthread_t hilo_estado;
	if (pthread_create(&hilo_estado, NULL, mostrar_estado_sala, NULL) != 0) {
		fprintf(stderr, "Error al crear hilo estado\n");
	}

	for (int i = 0; i < NUM_HILOS_GRATUITO_TOTAL; i++) {
		if (pthread_join(hilos_gratuito[i], NULL) != 0) {
			fprintf(stderr, "Error al terminar hilo de reserva\n");
		}
	}

	for (int i = 0; i < NUM_HILOS_PAGO_TOTAL; i++) {
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
