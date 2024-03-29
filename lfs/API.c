#include "API.h"

/* procesarCreate() [API]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> tipoDeConsistencia :: char*
 * 	-> numeroDeParticiones :: int
 * 	-> tiempoDeCompactacion :: int
 * Descripcion: permite la creación de una nueva tabla dentro del file system
 * Return: codigos de error o success*/

errorNo procesarCreate(char* nombreTabla, char* tipoDeConsistencia,	char* numeroDeParticiones, char* tiempoDeCompactacion) {

	string_to_upper(nombreTabla);
	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;

	/* Validamos si la tabla existe */
	DIR *dir = opendir(pathTabla);
	if (dir) {
		error = TABLA_EXISTE;
		closedir(dir);
	} else {
		/* Creamos la carpeta de la tabla */
		int resultadoCreacionDirectorio = mkdir(pathTabla, S_IRWXU);
		if (resultadoCreacionDirectorio == -1) {
			error = ERROR_CREANDO_DIRECTORIO;
		} else {

			char* metadataPath = string_from_format("%s/Metadata.bin", pathTabla);

			/* Creamos el archivo Metadata */
			FILE* metadataFile = fopen(metadataPath, "w");
			if (metadataFile == NULL) {
				error = ERROR_CREANDO_ARCHIVO;
			} else {
				fclose(metadataFile);
				t_config *metadataConfig = config_create(metadataPath);
				config_set_value(metadataConfig, "CONSISTENCY",	tipoDeConsistencia);
				config_set_value(metadataConfig, "PARTITIONS", numeroDeParticiones);
				config_set_value(metadataConfig, "COMPACTION_TIME", tiempoDeCompactacion);
				config_save(metadataConfig);
				config_destroy(metadataConfig);
				error = crearParticiones(pathTabla, strtol(numeroDeParticiones, NULL, 10));
			}

			free(metadataPath);
		}
	}

	if(error == SUCCESS){
		pthread_t hiloDeCompactacion;



		t_bloqueo* idYMutexPropio = malloc(sizeof(t_bloqueo));
		idYMutexPropio->id = 0; // 0 seria consola propia, sino son fds de memorias
		pthread_mutex_init(&(idYMutexPropio->mutex), NULL);

		t_hiloTabla* hiloTabla = malloc(sizeof(t_hiloTabla));
		hiloTabla->thread = &hiloDeCompactacion;
		hiloTabla->nombreTabla = strdup(nombreTabla);
		hiloTabla->finalizarCompactacion = 0;
		hiloTabla->cosasABloquear = list_create();
		pthread_mutex_init(&(hiloTabla->mutex), NULL);
		list_add(hiloTabla->cosasABloquear, idYMutexPropio);

		void agregarMemoria(t_int* memoria_fd) {
			t_bloqueo* idYMutex = malloc(sizeof(t_bloqueo));
			idYMutex->id = memoria_fd->valor;
			pthread_mutex_init(&(idYMutex->mutex), NULL);
			list_add(hiloTabla->cosasABloquear, idYMutex);
		}

		pthread_mutex_lock(&mutexMemorias);
		list_iterate(memorias, (void*)agregarMemoria);
		pthread_mutex_unlock(&mutexMemorias);

		pthread_mutex_lock(&mutexTablasParaCompactaciones);
		list_add(tablasParaCompactaciones, hiloTabla);
		pthread_mutex_unlock(&mutexTablasParaCompactaciones);



		if(!pthread_create(&hiloDeCompactacion, NULL, (void*) hiloCompactacion, (void*) strdup(pathTabla))){
			pthread_detach(hiloDeCompactacion);
			log_info(logger_LFS, "Hilo de compactacion de la tabla %s creado", nombreTabla);
		}else{
			log_error(logger_LFS, "Error al crear hilo de compactacion de la tabla %s", nombreTabla);
		}
	}
	free(pathTabla);

	return error;
}

/* crearParticiones()
 * Parametros:
 * -> pathTabla :: char*
 * -> numeroDeParticiones :: char*
 * Descripcion: crea las particiones de una tabla
 * Return: codigo de error o success */
errorNo crearParticiones(char* pathTabla, int numeroDeParticiones){
	/* Creamos las particiones */
	errorNo errorNo = SUCCESS;

	for (int i = 0; i < numeroDeParticiones && errorNo == SUCCESS; i++) {
		char* pathParticion = string_from_format("%s/%d.bin", pathTabla, i);

		FILE* particionFile = fopen(pathParticion, "w");
		if (particionFile == NULL) {
			errorNo = ERROR_CREANDO_ARCHIVO;
		} else {
			int bloqueDeParticion = obtenerBloqueDisponible();
			if(bloqueDeParticion == -1){
				log_info(logger_LFS, "no hay bloques disponibles");
			}else{
				char* bloquesParticion = string_from_format("[%d]", bloqueDeParticion);
				t_config *configParticion = config_create(pathParticion);
				config_set_value(configParticion, "SIZE", "0");
				config_set_value(configParticion, "BLOCKS", bloquesParticion);
				config_save(configParticion);
				free(bloquesParticion);
				config_destroy(configParticion);
			}
		}
		fclose(particionFile);
		free(pathParticion);
	}

	if(errorNo ==SUCCESS){

	}

	return errorNo;
}


/* procesarInsert() [API] [VALGRINDEADO]
 * Parametros:
 * 	-> nombreTabla :: char*
 * 	-> key :: uint16_t
 * 	-> value :: char*
 * 	-> timestamp :: unsigned long long
 * Descripcion: permite la creacion y/o actualizacion del valor de una key dentro de una tabla
 * Return:  */
errorNo procesarInsert(char* nombreTabla, uint16_t key, char* value, unsigned long long timestamp) {
	int encontrarTabla(t_tabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}
	string_to_upper(nombreTabla);
	char* pathTabla = string_from_format("%sTablas/%s", pathRaiz, nombreTabla);
	errorNo error = SUCCESS;

	if(strlen(value) > tamanioValue){
		error = VALUE_INVALIDO;
	}

	/* Validamos si la tabla existe */
	if(error == SUCCESS){
		DIR *dir = opendir(pathTabla);
		if (dir) {
			closedir(dir);
			t_registro* registro = (t_registro*) malloc(sizeof(t_registro));
			registro->key = key;
			registro->value = strdup(value);
			registro->timestamp = timestamp;
			pthread_mutex_lock(&mutexMemtable);
			t_tabla* tabla = list_find(memtable->tablas, (void*) encontrarTabla);
			if (tabla == NULL) {
				//log_info(logger_LFS, "Se agrego la tabla a la memtable y se agrego el registro");
				tabla = (t_tabla*) malloc(sizeof(t_tabla));
				tabla->nombreTabla = strdup(nombreTabla);
				tabla->registros = list_create();
				list_add(memtable->tablas, tabla);
			}
			list_add(tabla->registros, registro);
			pthread_mutex_unlock(&mutexMemtable);
		} else {
			error = TABLA_NO_EXISTE;
		}
	}

	free(pathTabla);
	return error;
}


errorNo procesarSelect(char* nombreTabla, char* key, char** mensaje, int fd){

	int ordenarRegistrosPorTimestamp(t_registro* registro1, t_registro* registro2){
		return registro1->timestamp > registro2->timestamp;
	}

	int encontrarTabla(t_hiloTabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}

	int encontrarMutexCorrespondiente(t_bloqueo* idYMutex) {
		return idYMutex->id == fd;
	}

	string_to_upper(nombreTabla);
	errorNo error = SUCCESS;
	t_list* listaDeRegistros = list_create();

	char* pathTabla = string_from_format("%s/Tablas/%s", pathRaiz, nombreTabla);
	DIR* dir = opendir(pathTabla);
	if(dir){
		closedir(dir);
		int _key = convertirKey(key);
		char* pathMetadataTabla = string_from_format("%s/Metadata.bin", pathTabla);
		t_config* configMetadataTabla = config_create(pathMetadataTabla);
		int numeroDeParticiones = config_get_int_value(configMetadataTabla, "PARTITIONS");
		free(pathMetadataTabla);
		config_destroy(configMetadataTabla);
		int particionDeEstaKey = _key % numeroDeParticiones;
		t_list* listaDeRegistrosDeMemtable = obtenerRegistrosDeMemtable(nombreTabla, _key);

		pthread_mutex_lock(&mutexTablasParaCompactaciones);
		t_hiloTabla* tablaEncontrada = list_find(tablasParaCompactaciones, (void*)encontrarTabla);
		t_bloqueo* idYMutexEncontrado = list_find(tablaEncontrada->cosasABloquear, (void*)encontrarMutexCorrespondiente);
		pthread_mutex_unlock(&mutexTablasParaCompactaciones);

		pthread_mutex_lock(&(idYMutexEncontrado->mutex));
		t_list* listaDeRegistrosDeTmp = obtenerRegistrosDeTmp(nombreTabla, _key);
		t_list* listaDeRegistrosDeParticiones = obtenerRegistrosDeParticiones(nombreTabla, particionDeEstaKey, _key);
		list_add_all(listaDeRegistros, listaDeRegistrosDeTmp);
		list_add_all(listaDeRegistros, listaDeRegistrosDeParticiones);
		pthread_mutex_unlock(&(idYMutexEncontrado->mutex));

		list_add_all(listaDeRegistros, listaDeRegistrosDeMemtable);

		if(!list_is_empty(listaDeRegistros)){
			list_sort(listaDeRegistros, (void*) ordenarRegistrosPorTimestamp);
			t_registro* registro = (t_registro*)listaDeRegistros->head->data;
			string_append_with_format(&*mensaje, "%s %u %c %s %c %llu", nombreTabla, registro->key,'"',registro->value,'"', registro->timestamp);
		}else{
			//https://github.com/sisoputnfrba/foro/issues/1406
			error = KEY_NO_EXISTE;
			string_append_with_format(&*mensaje, "Registro no encontrado salu3");
		}
		list_destroy_and_destroy_elements(listaDeRegistrosDeMemtable, (void*)eliminarRegistro);
		list_destroy_and_destroy_elements(listaDeRegistrosDeTmp, (void*)eliminarRegistro);
		list_destroy_and_destroy_elements(listaDeRegistrosDeParticiones, (void*)eliminarRegistro);
	}else{
		error = TABLA_NO_EXISTE;
	}
	free(pathTabla);
	list_destroy(listaDeRegistros);
	return error;
}

t_list* obtenerRegistrosDeTmp(char* nombreTabla, int key){
	char* pathTabla;
	struct dirent* file;
	char* pathFile;
	t_list* listaDeRegistros = list_create();
	t_list* listaDeRegistrosEnBloques;
	string_to_upper(nombreTabla);
	pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		while ((file = readdir (tabla)) != NULL) {
			//ignora . y ..
			if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) continue;

			if(string_ends_with(file->d_name, ".tmp") || string_ends_with(file->d_name, ".tmpc")){
				pathFile = string_from_format("%s/%s", pathTabla, file->d_name);
				t_config* file = config_create(pathFile);
				char* bloquesString = config_get_string_value(file, "BLOCKS");
				bloquesString++;
				bloquesString[strlen(bloquesString) -1] = 0;
				char** bloques = string_split(bloquesString, ",");
				int size = config_get_int_value(file, "SIZE");
				listaDeRegistrosEnBloques = buscarEnBloques(bloques, size, key);
				list_add_all(listaDeRegistros, listaDeRegistrosEnBloques);
				list_destroy(listaDeRegistrosEnBloques);
				liberarArrayDeChar(bloques);
				free(pathFile);
				config_destroy(file);
			}
		}
		closedir(tabla);
	}
	free(pathTabla);
	return listaDeRegistros;
}

t_list* obtenerRegistrosDeParticiones(char* nombreTabla, int particion, int key){
	t_list* listaDeRegistros = list_create();
	t_list* listaDeRegistrosEnBloques;
	char* pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		char* pathParticion = string_from_format("%s/%i.bin", pathTabla, particion);
		t_config* file = config_create(pathParticion);
		char* bloquesString = config_get_string_value(file, "BLOCKS");
		bloquesString++;
		bloquesString[strlen(bloquesString) -1] = 0;
		char** bloques = string_split(bloquesString, ",");
		int size = config_get_int_value(file, "SIZE");
		listaDeRegistrosEnBloques = buscarEnBloques(bloques, size, key);
		list_add_all(listaDeRegistros, listaDeRegistrosEnBloques);
		list_destroy(listaDeRegistrosEnBloques);
		liberarArrayDeChar(bloques);
		free(pathParticion);
		config_destroy(file);
		closedir(tabla);
	}
	free(pathTabla);
	return listaDeRegistros;
}

t_list* obtenerRegistrosDeMemtable(char* nombreTabla, int key){
	int encontrarTabla(t_tabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}

	int encontrarRegistro(t_registro* registro) {
		return registro->key == key;
	}

	t_list* listaDeRegistros;
	pthread_mutex_lock(&mutexMemtable);
	t_tabla* table = list_find(memtable->tablas, (void*) encontrarTabla);
	if(table == NULL){
		listaDeRegistros = list_create();
	}else{
		t_list* aux = list_filter(table->registros, (void*) encontrarRegistro);
		t_registro* identidad(t_registro* registro){
			t_registro* registroRetorno = malloc(sizeof(t_registro));
			registroRetorno->key = registro->key;
			registroRetorno->timestamp = registro->timestamp;
			registroRetorno->value = strdup(registro->value);
			return registroRetorno;
		}
		listaDeRegistros = list_map(aux, (void*)identidad);
		list_destroy(aux);
	}
	pthread_mutex_unlock(&mutexMemtable);
	return listaDeRegistros;
}


t_list* buscarEnBloques(char** bloques, int size, int key){
	t_registro* tRegistro;
	int particionDeEstaKey;
	t_list* listaDeRegistros = list_create();

	int existeParticion(t_int* particionAComparar) {
		return particionAComparar->valor == particionDeEstaKey;
	}

	int tieneMismaKey(t_registro* registroBuscado) {
		return tRegistro->key == registroBuscado->key;
	}

	char* fileMetadata = string_from_format("%s/Metadata.bin", pathMetadata);
	t_config* configMetadata = config_create(fileMetadata);
	int tamanioBloque = config_get_int_value(configMetadata, "BLOCK_SIZE");
	free(fileMetadata);
	config_destroy(configMetadata);

	if(size != 0) {
		// Leo de todos los bloques y guardo en un string (datosDelTmpC)
		int cantidadDeBloques = longitudDeArrayDeStrings(bloques);
		char* datos = strdup("");
		char* datosALeer;
		for (int i = 0; i < cantidadDeBloques; i++) {
			char* pathBloque = string_from_format("%sBloques/%s.bin", pathRaiz, bloques[i]);
			int fd = open(pathBloque, O_RDWR, S_IRWXU);
			free(pathBloque);
			if (fd == -1) {
				perror("Error");
			} else {
				if (i == cantidadDeBloques - 1) {
					int sizeToRead = size % tamanioBloque;
					sizeToRead =  size % tamanioBloque == 0 ? tamanioBloque : size % tamanioBloque;
					datosALeer = mmap(NULL, sizeToRead, PROT_READ, MAP_SHARED, fd, 0);
					if(datosALeer == -1){
						perror("error en mmap");
						log_error(logger_LFS, "error en mmap de select");
					}
					string_append_with_format(&datos, "%s", datosALeer);
					munmap(datosALeer, sizeToRead);
				} else {
					datosALeer = mmap(NULL, tamanioBloque, PROT_READ, MAP_SHARED, fd, 0);
					if(datosALeer == -1){
						perror("error en mmap");
						log_error(logger_LFS, "error en mmap");
					}
					string_append_with_format(&datos, "%s", datosALeer);
					munmap(datosALeer, tamanioBloque);
				}
				close(fd);
			}
		}

		char** registros = string_split(datos, "\n");
		free(datos);

		for (int j = 0; registros[j] != NULL; j++) {
			char** registroSeparado = string_split(registros[j], ";");
			if(key == convertirKey(registroSeparado[1])){
				tRegistro = (t_registro*) malloc(sizeof(t_registro));
				convertirTimestamp(registroSeparado[0], &(tRegistro->timestamp));
				tRegistro->key = convertirKey(registroSeparado[1]);
				tRegistro->value = strdup(registroSeparado[2]);
				list_add(listaDeRegistros, tRegistro);
			}

			liberarArrayDeChar(registroSeparado);

		}

		liberarArrayDeChar(registros);
	}

	return listaDeRegistros;
}

errorNo procesarDescribe(char* nombreTabla, char** mensaje){
	errorNo error = SUCCESS;
	char* pathTablas = string_from_format("%sTablas", pathRaiz);
	char* pathTabla;
	if(nombreTabla != NULL){
		string_to_upper(nombreTabla);
		pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
		char* metadata = obtenerMetadata(pathTabla);
		string_append_with_format(&*mensaje, "%s %s", nombreTabla, metadata);
		free(metadata);
		free(pathTabla);
	}else{
		DIR *dir;
		struct dirent* tabla;
		if ((dir = opendir(pathTablas)) != NULL) {
			while ((tabla = readdir (dir)) != NULL) {
				struct stat st;

				//ignora . y ..
				if(strcmp(tabla->d_name, ".") == 0 || strcmp(tabla->d_name, "..") == 0) continue;

				//esta funcion carga en st la informacion del file "tabla", que esta dentro de "dir"
				if (fstatat(dirfd(dir), tabla->d_name, &st, 0) < 0) continue;

				if (S_ISDIR(st.st_mode)) {
					pathTabla = string_from_format("%s/%s", pathTablas, tabla->d_name);
					char* metadata = obtenerMetadata(pathTabla);
					string_append_with_format(&*mensaje,"%s %s;", tabla->d_name, metadata);
					free(metadata);
					metadata = NULL;
					free(pathTabla);
				}
			}

			closedir (dir);
			if(!string_is_empty(*mensaje)) {
				(*mensaje)[strlen(*mensaje) - 1] = 0; // para sacar el ultimo punto y coma
			}
		} else {
			perror("Error al abrir directorio de tablas");
		}
	}
	free(pathTablas);
	return error;
}

char* obtenerMetadata(char* pathTabla){
	char* mensaje = strdup("");
	DIR* dir = opendir(pathTabla);
	if(dir != NULL){
		closedir(dir);
		char* pathMetadata = string_from_format("%s/%s", pathTabla, "Metadata.bin");
		t_config* metadata = config_create(pathMetadata);
		free(pathMetadata);
		if(metadata != NULL){
			if(config_has_property(metadata, "CONSISTENCY") && config_has_property(metadata, "PARTITIONS") && config_has_property(metadata, "COMPACTION_TIME")){
				string_append_with_format(&mensaje,"%s %i %i", config_get_string_value(metadata, "CONSISTENCY"), config_get_int_value(metadata, "PARTITIONS"), config_get_int_value(metadata, "COMPACTION_TIME"));
			}else{
				log_error(logger_LFS,"El metadata de la tabla %s no tiene alguna de las config", pathTabla);
			}
			config_destroy(metadata);
		}else{
			log_error(logger_LFS, "No se pudo levantar como config el metadata de la tabla %s", pathTabla);
		}
	}else{
		log_error(logger_LFS, "No se pudo abrir el metadata de la tabla %s", pathTabla);
		perror("Error");
	}
	return mensaje;
}

errorNo procesarDrop(char* nombreTabla){
	string_to_upper(nombreTabla);
	int encontrarTabla(t_hiloTabla* tabla) {
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}
	int encontrarTablaMemtable(t_tabla* tabla){
		return string_equals_ignore_case(tabla->nombreTabla, nombreTabla);
	}

	errorNo error = SUCCESS;
	char* pathTabla = string_from_format("%s/%s", pathTablas, nombreTabla);
	pthread_mutex_lock(&mutexTablasParaCompactaciones);
	t_hiloTabla* tablaEncontrada = list_find(tablasParaCompactaciones, (void*)encontrarTabla);
	pthread_mutex_unlock(&mutexTablasParaCompactaciones);
	if(tablaEncontrada != NULL){
		pthread_mutex_lock(&(tablaEncontrada->mutex));
	}
	DIR* tabla = opendir(pathTabla);
	if(tabla){
		borrarArchivosYLiberarBloques(tabla, pathTabla);
		closedir(tabla);
		rmdir(pathTabla);
		tablaEncontrada->finalizarCompactacion = 1;
		pthread_mutex_lock(&mutexMemtable);
		t_tabla* tabla = list_find(memtable->tablas, (void*) encontrarTabla);
		if(tabla != NULL){
			vaciarTabla(tabla);
		}
		pthread_mutex_unlock(&mutexMemtable);
	}else{
		error = TABLA_NO_EXISTE;
	}
	if(tablaEncontrada != NULL){
		pthread_mutex_unlock(&(tablaEncontrada->mutex));
	}

	free(pathTabla);

	return error;
}

void borrarArchivosYLiberarBloques(DIR* tabla, char* pathTabla){
	char* pathFile;
	struct dirent* file;
	char* pathTablaMetadata = string_from_format("%s/%s", pathTabla, "/Metadata.bin");
	remove(pathTablaMetadata);
	free(pathTablaMetadata);
	while ((file = readdir (tabla)) != NULL) {
		//ignora . y ..
		if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) continue;

		if(string_ends_with(file->d_name, ".bin") || string_ends_with(file->d_name, ".tmp") || string_ends_with(file->d_name, ".tmpc")) {
			pathFile = string_from_format("%s/%s", pathTabla, file->d_name);
			t_config* file = config_create(pathFile);
			char* bloquesString = config_get_string_value(file, "BLOCKS");
			bloquesString++;
			bloquesString[strlen(bloquesString) -1] = 0;
			char** bloques = string_split(bloquesString, ",");
			int i = 0;
			while (bloques[i] != NULL) {
				char* pathBloque = string_from_format("%s/%s.bin", pathBloques, bloques[i]);
				FILE* bloque = fopen(pathBloque, "w");
				free(pathBloque);
				fclose(bloque);
				pthread_mutex_lock(&mutexBitmap);
				bitarray_clean_bit(bitarray, strtol(bloques[i], NULL, 10));
				pthread_mutex_unlock(&mutexBitmap);
				i++;
			}
			liberarArrayDeChar(bloques);
			remove(pathFile);
			free(pathFile);
			config_destroy(file);
		}
	}
}
