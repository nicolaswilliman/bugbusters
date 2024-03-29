//servidor
#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<signal.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<string.h>
#include<readline/readline.h>

#define TRUE 1
#define FALSE 0

#define PARAMETROS_SELECT 2
#define PARAMETROS_INSERT 3
#define PARAMETROS_INSERT_TIMESTAMP 4
#define PARAMETROS_CREATE 4
#define PARAMETROS_DESCRIBE_GLOBAL 0
#define PARAMETROS_DESCRIBE_TABLA 1
#define PARAMETROS_DROP 1
#define PARAMETROS_JOURNAL 0
#define PARAMETROS_ADD 4
#define PARAMETROS_RUN 1
#define PARAMETROS_METRICS 0

typedef enum
{
	KERNEL,
	MEMORIA,
	LFS
} Componente;

typedef enum
{
	REQUEST,
	GOSSIPING
} rol;

typedef enum
{
	SC,
	SHC,
	EC,
	NINGUNA,
	CONSISTENCIA_INVALIDA = -1
} consistencia;

typedef enum
{
	SUCCESS,
	TABLA_EXISTE,
	TABLA_NO_EXISTE,
	ERROR_CREANDO_ARCHIVO,
	ERROR_CREANDO_DIRECTORIO,
	ERROR_CREANDO_METADATA,
	ERROR_CREANDO_PARTICIONES,
	KEY_NO_EXISTE,
	REQUEST_VACIA,
	COD_REQUEST_INV,
	CANT_PARAM_INV,
	KEY_NO_NUMERICA,
	TIMESTAMP_NO_NUMERICO,
	CONSISTENCIA_NO_VALIDA,
	KEY_MUY_GRANDE,
	VALUE_INVALIDO,
	ERROR_WILLY,
	FAILURE = -1,
	MEMORIA_FULL = 10102,
	COMPONENTE_CAIDO = -2,
	JOURNALTIME = -10103
} errorNo;

typedef enum {
	CONEXION_EXITOSA,
	CONEXION_INVALIDA = 6
} rta_handshake;

typedef enum
{
	SELECT,
	INSERT,
	CREATE,
	DESCRIBE,
	DROP,
	JOURNAL,
	ADD,
	RUN,
	METRICS,
	NUESTRO_ERROR = -1
} cod_request;

typedef struct
{
	int palabraReservada;
	int tamanio;
	char* request;
} t_paquete;

typedef struct
{
	rta_handshake rta;
} t_handshake_rta;

typedef struct
{
	rol tipo_rol;
} t_operacion;

typedef struct
{
	int tamanioIps;
	char* ips;
	int tamanioPuertos;
	char* puertos;
	int tamanioNumeros;
	char* numeros;
	int esDeKernel;
} t_gossiping;

typedef struct
{
	int tamanioValue;
} t_handshake_lfs;

typedef struct
{
	Componente tipoComponente;
} t_handshake;

typedef enum
{
	CONSOLE,
	ANOTHER_COMPONENT
} t_caller;

typedef struct{
	int valor;
} t_int;

int convertirKey(char*);
void convertirTimestamp(char*, unsigned long long*);
void iterator(char*);

char** separarRequest(char*);

unsigned long long obtenerHoraActual();
char** separarString(char*);
int longitudDeArrayDeStrings(char**);
char** obtenerParametros(char*);
int longitudDeArrayDeStrings(char**);
consistencia obtenerEnumConsistencia(char*);

int crearConexion(char*, char*);
t_config* leer_config(char*);

t_paquete* armar_paquete(int, char*);

errorNo validarMensaje(char*, Componente, char**);
int validarParametrosDelRequest(int, char**, Componente);
int obtenerCodigoPalabraReservada(char*, Componente);

int validarValue(char*,char*, int, t_log*);
int esNumero(char* key);

////servidor

void* recibir_buffer(int*, int);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
t_paquete* recibir(int);
t_gossiping* recibirGossiping(int, int*);
t_handshake_lfs* recibirValueLFS(int);
t_handshake* recibirHandshake(int, int*);
t_operacion* recibirOperacion(int, int*);
////cliente

void* serializar_gossiping(t_gossiping*, int);
void* serializar_handshake_lfs(t_handshake_lfs*, int);
void* serializar_paquete(t_paquete* , int);
void* serializar_handshake(t_handshake*, int);
void* serializar_handshake_operacion(t_operacion*, int);
int enviar(cod_request, char*, int);
int enviarGossiping(char*, char*, char*, int, int);
void enviarValueLFS(int, int);
int enviarHandshake(Componente, int);
int enviarTipoOperacion(rol, int);
int enviarRtaHandshake(rta_handshake, int);
t_handshake_rta* recibirRtaHandshake(int, int*);
void eliminar_paquete(t_paquete*);
void liberar_conexion(int);
void liberarArrayDeChar(char**);
void liberarHandshakeMemoria(t_gossiping*);

/* Multiplexacion */
void eliminarClientesCerrados(t_list*, int*);
int maximo(t_list*, int, int);

#endif /* SOCKETS_H_ */
