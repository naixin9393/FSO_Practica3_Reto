//cabecera sala

int reserva_asiento(int id);
int libera_asiento(int asiento);
int estado_asiento(int asiento);
int asientos_libres();
int asientos_ocupados();
int capacidad();
void crea_sala(int capacidad);
void elimina_sala();

void estado_sala();
int levantarse(int id);
void inicializar_asientos(int nAsientos);
int sala_eliminada();
int sala_creada();

// Ficheros
int guarda_estado_sala(const char* ruta_fichero);
int recupera_estado_sala(const char* ruta_fichero);
int guarda_estadoparcial_sala(const char* ruta_fichero, size_t num_asientos, int* id_asientos);
int recupera_estadoparcial_sala(const char* ruta_fichero, size_t num_asientos, int* id_asientos);

