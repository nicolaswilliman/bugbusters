#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/inotify.h>

typedef struct
{
	char* ip;
	char* puerto;
	char* numero;
} config_memoria;

typedef struct
{
	cod_request codigo;
	void* request;
} request_procesada;

typedef struct
{
	int cantidadSelectInsert;
	char* numeroMemoria;
} estadisticaMemoria;

t_log* logger_KERNEL;
t_log* logger_METRICAS_KERNEL;
int quantum = 0;
char* ipMemoria;
char* puertoMemoria;
int sleepEjecucion = 0;
int metadataRefresh = 0;
int sleepGossiping = 0;
unsigned int numeroSeedRandom = 0;
t_config* config;
t_queue* new;
t_queue* ready;
config_memoria* memoriaSc;
t_list* memoriasShc;
t_list* memoriasEc;
t_list* memorias;
t_list* tablasSC;
t_list* tablasSHC;
t_list* tablasEC;

// variables de metricas
double tiempoSelectSC = 0.0;
double tiempoInsertSC = 0.0;
int cantidadSelectSC = 0;
int cantidadInsertSC = 0;
int cantidadSelectSCTotal = 0;
int cantidadInsertSCTotal = 0;
t_list* cargaMemoriaSC;
double tiempoSelectSHC = 0.0;
double tiempoInsertSHC = 0.0;
int cantidadSelectSHC = 0;
int cantidadInsertSHC = 0;
int cantidadSelectSHCTotal = 0;
int cantidadInsertSHCTotal = 0;
t_list* cargaMemoriaSHC;
double tiempoSelectEC = 0.0;
double tiempoInsertEC = 0.0;
int cantidadSelectEC = 0;
int cantidadInsertEC = 0;
int cantidadSelectECTotal = 0;
int cantidadInsertECTotal = 0;
t_list* cargaMemoriaEC;

// Variables para inotify
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )
int file_descriptor;
int watch_descriptor;

char* ipPpal;
char* puertoPpal;

sem_t semRequestNew;				// semaforo para planificar requests en new
pthread_mutex_t semMColaNew;		// semafoto mutex para cola de new
sem_t semRequestReady;				// semaforo para planificar requests en ready
pthread_mutex_t semMColaReady;		// semafoto mutex para cola de ready
sem_t semMultiprocesamiento;		// semaforo contador para limitar requests en exec
pthread_mutex_t semMMetricas;		// semaforo mutex para evitar concurrencia en metricas
pthread_mutex_t semMTablasSC;		// semaforo mutex para evitar concurrencia en la lista de tablas sc
pthread_mutex_t semMTablasSHC;		// semaforo mutex para evitar concurrencia en la lista de tablas shc
pthread_mutex_t semMTablasEC;		// semaforo mutex para evitar concurrencia en la lista de tablas ec
pthread_mutex_t semMMemoriaSC;		// semaforo mutex para evitar concurrencia en la lista de memorias sc
pthread_mutex_t semMMemorias;		// semaforo mutex para evitar concurrencia en la lista de memorias
pthread_mutex_t semMMemoriasSHC;	// semaforo mutex para evitar concurrencia en la lista de memorias shc
pthread_mutex_t semMMemoriasEC;		// semaforo mutex para evitar concurrencia en la lista de memorias ec
pthread_mutex_t semMQuantum;		// semaforo mutex para evitar concurrencia en la variable
pthread_mutex_t semMSleepEjecucion;	// semaforo mutex para evitar concurrencia en la variable
pthread_mutex_t semMMetadataRefresh;// semaforo mutex para evitar concurrencia en la variable
pthread_mutex_t semMConfig;			// semaforo mutex para evitar concurrencia en la variable
pthread_mutex_t semMSleepGossiping;	// semaforo mutex para evitar concurrencia en la variable

pthread_t hiloLeerDeConsola;		// hilo que lee de consola
pthread_t hiloConectarAMemoria;		// hilo que conecta a memoria
pthread_t hiloPlanificarNew;		// hilo para planificar requests de new a ready
pthread_t hiloPlanificarExec;		// hilo para planificar requests de ready a exec y viceversa
pthread_t hiloMetricas;				// hilo para loguear metricas cada 30 secs
pthread_t hiloDescribe;				// hilo para hacer describe cada x secs
pthread_t hiloCambioEnConfig;		// hilo que escucha los cambios en el archivo de configuración

void inicializarVariables(void);
void hacerGossiping(void);
void procesarGossiping(t_gossiping*);
void liberarMemoria(void);
void liberarConfigMemoria(config_memoria*);
void liberarRequestProcesada(request_procesada*);
void liberarColaRequest(request_procesada*);
void leerDeConsola(void);
void hacerDescribe(void);
void loguearMetricas(void);
void informarMetricas(int);
void liberarEstadisticaMemoria(estadisticaMemoria*);
void aumentarContadores(char*, cod_request, double, consistencia);
void escucharCambiosEnConfig(void);
int conectarseAMemoria(rol, char*, char*, char*);
// planificar requests
void procesarRequestSinPlanificar(char*);
void planificarRequest(char*);
void planificarNewAReady(void);
void planificarReadyAExec(void);
void reservarRecursos(char*);
// validar + delegar requests
int validarRequest(char *);
int manejarRequest(request_procesada*, int);
// se usa para procesar requests
int enviarMensajeAMemoria(cod_request, char*);
config_memoria* encontrarMemoriaSegunConsistencia(consistencia, int);
unsigned int obtenerIndiceRandom(int);
unsigned int obtenerIndiceHash(int, int);
void actualizarTablas(char*);
void agregarTablaACriterio(char*);
void liberarTabla(char*);
consistencia obtenerConsistenciaTabla(char*);
// procesan requests
void procesarJournal(int);
void procesarRun(t_queue*);
int procesarAdd(char*);
void procesarRequest(request_procesada*);
int reintentarConexion(consistencia, int, int, char**, char**, char**);
void eliminarMemoria(char*, char*, char*);
int mandarHandshake(rol, char*, char*, char*, int);

#endif /* KERNEL_H_*/
