#include "../Include/filesystem.h"

int main(void) {

    logger = iniciar_logger("file_system.log");
    log_info(logger, "MODULO FILE SYSTEM");

    config_fs = iniciar_config(PATH_CONFIG); // Inicializar configuración
    cargarConfigFS(config_fs);
    conectar_con_memoria(); // Conectarse con Memoria
    inicializarFS(); // Inicializar el File System
    correr_servidor(); // Iniciar file system como servidor de kernel
    finalizarFS(socket_fs, logger, config_fs); // Finalizar el File System

    return EXIT_SUCCESS;
}

void cargarConfigFS(t_config* config_fs) {

    fs_config = malloc(sizeof(t_fs_config));

    fs_config->IP_MEMORIA = config_get_string_value(config_fs, "IP_MEMORIA");
    fs_config->PUERTO_MEMORIA = config_get_string_value(config_fs, "PUERTO_MEMORIA");
    fs_config->PUERTO_ESCUCHA = config_get_string_value(config_fs, "PUERTO_ESCUCHA");
    fs_config->PATH_SUPERBLOQUE = config_get_string_value(config_fs, "PATH_SUPERBLOQUE");
    fs_config->PATH_BITMAP = config_get_string_value(config_fs, "PATH_BITMAP");
    fs_config->PATH_BLOQUES = config_get_string_value(config_fs, "PATH_BLOQUES");
    fs_config->PATH_FCB = config_get_string_value(config_fs, "PATH_FCB");
    fs_config->RETARDO_ACCESO_BLOQUE = config_get_int_value(config_fs, "RETARDO_ACCESO_BLOQUE");
    fs_config->PATH_FS = config_get_string_value(config_fs, "PATH_FS");

    log_info(logger, "La configuración se cargó en la estructura 'fs_config'");

}

void cargarConfigFile(t_config* config_file){

    fcb = malloc(sizeof(FCB));

    fcb->NOMBRE_ARCHIVO = config_get_string_value(config_file, "NOMBRE_ARCHIVO");
    fcb->TAMANIO_ARCHIVO = config_get_int_value(config_file, "TAMANIO_ARCHIVO");
    fcb->PUNTERO_DIRECTO = config_get_int_value(config_file, "PUNTERO_DIRECTO");
    fcb->PUNTERO_INDIRECTO = config_get_int_value(config_file, "PUNTERO_INDIRECTO");

    log_info(logger, "La configuración se cargó en la estructura 'fcb' ");

}

void conectar_con_memoria() {
    log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    enviar_handshake(socket_memoria, FILESYSTEM);
}

void inicializarFS() {

    if (existeFS() == 1) {
        log_info(logger, "File System encontrado, recuperando...");
        inicializarSuperBloque();
        inicializarBloques();
        inicializarBitmap();
    }
	else {
        log_info(logger, "File System NO encontrado, generando...");
        crearDirectorio(fs_config->PATH_FS);
        inicializarFS();
    }

}

int existeFS() {
    char* path_fs = fs_config->PATH_FS;
    if (existeArchivo(path_fs) != -1) {
        return 1;
    }
    else {
        return 0;
    }
}

int existeArchivo(char* ruta) {
    return access(ruta, F_OK);
}

void inicializarSuperBloque() {
    config_fs = iniciar_config(fs_config->PATH_SUPERBLOQUE);
    fs_config->BLOCK_COUNT = config_get_int_value(config_fs, "BLOCK_COUNT");
    fs_config->BLOCK_SIZE = config_get_int_value(config_fs, "BLOCK_SIZE");
    log_info(logger, "ARCHIVO %s LEIDO", fs_config->PATH_SUPERBLOQUE);
}

void inicializarBloques() {
    char* rutaBloques = fs_config->PATH_BLOQUES;
    int fd = open(rutaBloques, O_CREAT | O_RDWR, 0777);
    if (fd == -1) {
        log_error(logger, "No se pudo abrir el archivo bloques");
        exit(1);
    }
    close(fd);
    log_info(logger, "ARCHIVO %s LEIDO", rutaBloques);
}

void inicializarBitmap() {
    char* rutaBitmap = fs_config->PATH_BITMAP;
    int fd = open(rutaBitmap, O_CREAT | O_RDWR, 0777);
    if (fd == -1) {
        log_error(logger, "No se pudo abrir el archivo bitmap");
        exit(1);
    }

    int cantidadBloques;
    if ((fs_config->BLOCK_COUNT % 8) == 0) {
        cantidadBloques = fs_config->BLOCK_COUNT / 8;
    } else {
        cantidadBloques = (fs_config->BLOCK_COUNT / 8) + 1;
    }

    ftruncate(fd, cantidadBloques / 8);

    mapBitmap = mmap(NULL, cantidadBloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mapBitmap == MAP_FAILED) {
        close(fd);
        exit(1);
    }

    memcpy(mapBitmap, &fs_config->BLOCK_SIZE, sizeof(uint32_t));
    memcpy(mapBitmap + sizeof(uint32_t), &fs_config->BLOCK_COUNT, sizeof(uint32_t));
    msync(mapBitmap, sizeof(uint32_t), MS_SYNC);

    bitmap = bitarray_create_with_mode(mapBitmap + sizeof(uint32_t) * 2, cantidadBloques * 8, LSB_FIRST);

    log_info(logger, "El tamaño del bitmap creado es de %d bits.", bitarray_get_max_bit(bitmap));

//    for (int i = 0; i < fs_config->BLOCK_COUNT/8; i++) {
//    	bitarray_clean_bit(bitmap, i);
//	}

	close(fd);
	log_info(logger, "ARCHIVO %s LEIDO", rutaBitmap);
}

void crearDirectorio(char* path) {
    char* sep = strrchr(path, '/');
    if (sep != NULL) {
        *sep = 0;
        crearDirectorio(path);
        *sep = '/';
    }
    mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
}

void leerArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano) {
    printf("Leer Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria, tamano);
    // Agregar código para leer el archivo
}

void escribirArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano) {
    printf("Escribir Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria,  tamano);
    // Agregar código para escribir en el archivo
}

void correr_servidor(void) {
    socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
    log_info(logger, "Iniciada la conexión servidor de FS: %d", socket_fs);
    // Quedar a la espera de la conexión por parte del Kernel
    socket_kernel = esperar_cliente(socket_fs, logger);
    int modulo = recibir_operacion(socket_kernel);

    switch (modulo) {
        case KERNEL:
            log_info(logger, "Kernel Conectado.");
            enviar_mensaje("TODO: Generico", socket_kernel, logger);
            recibir_request_kernel(socket_kernel);
            break;
        case -1:
            log_error(logger, "Se desconectó el cliente.");
            close(socket_kernel);
            exit(EXIT_FAILURE);
        default:
            log_error(logger, "Codigo de operacion desconocido.");
            break;

    }
}

void recibir_request_kernel(int socket_kernel) {
    while(true) {

    	int resultado;

    	int cod_op = recibir_operacion(socket_kernel); // t_codigo_operacionfs

    	log_info(logger, "Recibida operación %d", cod_op);

    	char* nombre_archivo = recibir_string(socket_kernel); //SE RECIBE TAMBIÉN EL NOMBRE DEL ARCHIVO YA QUE ES EL PRIMER PARAMETRO SIEMPRE

    	log_info(logger, "Recibido Archivo %s", nombre_archivo);

    	switch (cod_op) {
			case F_OPEN:
				log_info(logger, "Se recibió un F_OPEN para el archivo %s", nombre_archivo);
				resultado = abrirArchivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_CREATE:
				log_info(logger, "Se recibió un F_CREATE para el archivo %s", nombre_archivo);
				resultado = crearArchivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_TRUNCATE:
				log_info(logger, "Se recibió un F_TRUNCATE para el archivo %s", nombre_archivo);
				resultado = truncarArchivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_READ:
				//TODO
				log_info(logger, "Se recibió un F_READ para el archivo %s", nombre_archivo);
				procesar_f_read(nombre_archivo);
				break;
			case F_WRITE:
				//TODO
				log_info(logger, "Se recibió un F_WRITE para el archivo %s", nombre_archivo);
				procesar_f_write(nombre_archivo);
				break;
			default:
				break;
		}
    }
}

void procesar_f_read(char * nombre_archivo) {
	char* direccion_logica = recibir_string(socket_kernel);
	char* cantidad_de_bytes = recibir_string(socket_kernel);
	//TODO: recordar que existe atoi que toma un char* y devuelve un entero
	// Ej: int tamanio_int = atoi(tamanio);
}
void procesar_f_write(char * nombre_archivo) {
	char* direccion_logica = recibir_string(socket_kernel);
	char* cantidad_de_bytes = recibir_string(socket_kernel);
	//TODO: recordar que existe atoi que toma un char* y devuelve un entero
	// Ej: int tamanio_int = atoi(tamanio);
}

void finalizarFS(int socket_servidor, t_log* logger, t_config* config) {
    log_info(logger, "Finalizando File System...");
    free(fs_config);
    liberar_conexion(socket_fs);
    log_destroy(logger);
    config_destroy(config);
}

int abrirArchivo(const char* nombreArchivo) {
    log_info(logger, "Abrir Archivo: %ss", nombreArchivo);
    DIR* dir = opendir(fs_config->PATH_FCB);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
			levantarFCB(nombreArchivo);
            if (strcmp(fcb->NOMBRE_ARCHIVO, nombreArchivo) == 0) {
                log_info(logger, "El archivo %s existe.", nombreArchivo);
                closedir(dir);
                return F_EXISTS; // OK
            }
        }
    }
    closedir(dir);
    log_info(logger, "El archivo %s no existe.", nombreArchivo);
    return F_NOT_EXISTS;
}

int crearArchivo(const char* nombreArchivo) {

    int tamanioArchivo = 0;
    FILE *archivo; // Declarar un puntero de tipo FILE
	char ruta[256];
    sprintf(ruta, "%s/%s", fs_config->PATH_FCB, nombreArchivo);

    archivo = fopen(ruta, "w"); // Abrir el archivo en modo escritura

    if (archivo == NULL) { // Verificar si hubo un error al abrir el archivo
        log_error(logger, "No se pudo crear el archivo %s.", nombreArchivo);
        return F_OP_ERROR; // Salir del programa con código de error
    }

    fprintf(archivo, "NOMBRE_ARCHIVO=%s\n", nombreArchivo);
    fprintf(archivo, "TAMANIO_ARCHIVO=%d\n", tamanioArchivo);
    fprintf(archivo, "PUNTERO_DIRECTO=%s\n", "0");
    fprintf(archivo, "PUNTERO_INDIRECTO=%s\n", "0");

    fclose(archivo); // Cerrar el archivo
    log_info(logger,"Archivo creado exitosamente.");
    return F_OP_OK;
}

int truncarArchivo(const char* nombreArchivo) {
	char* paramTamanio = recibir_string(socket_kernel);
	uint32_t nuevoTamanio = atoi(paramTamanio);
	int cantidadBloques = nuevoTamanio/fs_config->BLOCK_SIZE;
	levantarFCB(nombreArchivo);
	int nuevoP_directo;
	int nuevoP_indirecto;

	log_info(logger, "Truncar Archivo: %s - Tamaño: %d\n", nombreArchivo, nuevoTamanio);

    if (nuevoTamanio > fcb->TAMANIO_ARCHIVO) {
        if(fcb->PUNTERO_DIRECTO == 0){
        	nuevoP_directo = obtenerPosIniDeNBloquesLibresEnBitmap(cantidadBloques);
        	//	obtenerPosicioBloqueLibre();
        	//	bitarray_set_bit(t_bitarray*, off_t bit_index); //si no tiene valor busco el primero disponible
        }
        else{
        	//busco el primero disponible contiguo
        }
    	//asignar más bloques
    } else if (nuevoTamanio < fcb->TAMANIO_ARCHIVO) {
        // Reducir el tamaño del archivo
    }

    actualizarFCB(fcb, nuevoTamanio, nuevoP_directo, nuevoP_indirecto);
    persistirFCB(fcb, nombreArchivo);
    return F_OP_OK;
}

void levantarFCB(const char* nombreArchivo){
	char archivo[256];
    sprintf(archivo, "%s/%s", fs_config->PATH_FCB, nombreArchivo);
    t_config* file = iniciar_config(archivo);
    cargarConfigFile(file);
}

void actualizarFCB(FCB* archivo, int nuevo_tamanio, int nuevo_directo, int nuevo_indirecto) {
    archivo->TAMANIO_ARCHIVO = nuevo_tamanio;
    archivo->PUNTERO_DIRECTO = nuevo_directo;
    archivo->PUNTERO_INDIRECTO = nuevo_indirecto;
}

void persistirFCB(FCB* archivo, const char* nombre_archivo) {
    FILE* archivo_salida = fopen(nombre_archivo, "wb");
    fwrite(archivo, sizeof(FCB), 1, archivo_salida);
    fclose(archivo_salida);
}

int obtenerPosIniDeNBloquesLibresEnBitmap(int cantidadBloques){

	int pos = 0;

	//recorrer el bitmap hasta encontrar N posiciones contiguas
	for(int i = 0; i < fs_config->BLOCK_COUNT; i++){
			if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
				pos = i;
			}
		}

	return pos;
}

