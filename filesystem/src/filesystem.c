#include "../Include/filesystem.h"

int main(void){

    logger = iniciar_logger("file_system.log");
    log_info(logger, "MODULO FILE SYSTEM");

    // Inicializar configuración
    config_fs = iniciar_config(PATH_CONFIG);
    cargarConfigFS(config_fs);

    // Conectarse con Memoria
    conectar_con_memoria();

    // Inicializar el File System
    inicializarFS();

    // Iniciar file system como servidor de kernel
    correr_servidor();

    // Finalizar el File System
    finalizarFS(socket_fs, logger, config_fs);

    return EXIT_SUCCESS;
}

void cargarConfigFS(t_config* config_fs){

    fs_config = malloc(sizeof(t_fs_config));

    fs_config->IP_MEMORIA = config_get_string_value(config_fs, "IP_MEMORIA");
    fs_config->PUERTO_MEMORIA = config_get_string_value(config_fs, "PUERTO_MEMORIA");
    fs_config->PUERTO_ESCUCHA = config_get_string_value(config_fs, "PUERTO_ESCUCHA");
    fs_config->PATH_SUPERBLOQUE = config_get_string_value(config_fs, "PATH_SUPERBLOQUE");
    fs_config->PATH_BITMAP = config_get_string_value(config_fs, "PATH_BITMAP");
    fs_config->PATH_BLOQUES = config_get_string_value(config_fs, "PATH_BLOQUES");
    fs_config->PATH_FCB = config_get_string_value(config_fs, "PATH_FCB");
    fs_config->RETARDO_ACCESO_BLOQUE = config_get_int_value(config_fs, "RETARDO_ACCESO_BLOQUE");

    log_info(logger, "La configuración se cargó en la estructura 'fs_config' ");

}

void conectar_con_memoria() {

    log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);

    // Conexión con el módulo Memoria
    socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);

    // Realizar handshake inicial
    enviar_handshake(socket_memoria, FILESYSTEM);

    // TODO Verificar la conexión

}

void inicializarFS() {

    if(existeFS()==1){
        log_info(logger, "File System encontrado, recuperando...");
        inicializarSuperBloque();
        inicializarBloques();
        inicializarBitmap();
    }
    else{
        log_info(logger, "File System NO encontrado, generando...");
        crearDirectorio(fs_config->PATH_FCB);
        inicializarFS();
    }

}

int existeFS(){

    char* path_fs = fs_config->PATH_FCB;

    if(existeArchivo(path_fs)!=-1){
        return 1;
    }
    else{
        return 0;
    }
}

int existeArchivo(char* ruta) {
    // Devuelve -1 si no se encuentra el archivo
    return access(ruta, F_OK);
}

void inicializarSuperBloque(){

    char* rutaSuperBloque = fs_config->PATH_SUPERBLOQUE;

    int fd = open(rutaSuperBloque, O_RDWR, 0777);

    if (fd == -1) {
        log_error(logger, "No se pudo abrir el SuperBloque");
        exit(1);
    }

    //Leer el block_size y block_count del archivo
    fs_config->BLOCK_SIZE = read(fd, &superbloque.block_size, sizeof(uint32_t));
    fs_config->BLOCK_COUNT = read(fd, &superbloque.block, sizeof(uint32_t));

    close(fd);
    log_info(logger, "ARCHIVO %s LEIDO", rutaSuperBloque);
}

void inicializarBloques() {

	char* rutaBloques = fs_config->PATH_BLOQUES;

	int fd = open(rutaBloques, O_CREAT | O_RDWR, 0777);

	if (fd == -1) {
		log_error(logger, "No se pudo abrir el archivo bloques");
		exit(1);
	}

	ftruncate(fd, (fs_config->BLOCK_SIZE*fs_config->BLOCK_COUNT));

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

	if ((superbloque.block % 8) == 0) {
		cantidadBloques = superbloque.block / 8;
	} else {
		cantidadBloques = (superbloque.block / 8) + 1;
	}

	log_info(logger,"int %d",superbloque.block);
	ftruncate(fd, cantidadBloques);

	mapBitmap = mmap(NULL, cantidadBloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (mapBitmap == MAP_FAILED) {
		close(fd);
		exit(1);
	}

	memcpy(mapBitmap, &superbloque.block, sizeof(uint32_t));
	memcpy(mapBitmap + sizeof(uint32_t), &superbloque.block_size, sizeof(uint32_t));
	msync(mapBitmap, sizeof(uint32_t), MS_SYNC);

	bitmap = bitarray_create_with_mode(mapBitmap + sizeof(uint32_t) * 2, cantidadBloques, LSB_FIRST);

	log_info(logger, "El tamaño del bitmap creado es de %d bits.", bitarray_get_max_bit(bitmap));

	size_t tope = bitarray_get_max_bit(bitmap);

	for (int i = 0; (unsigned)i < tope; i++) {
		bitarray_clean_bit(bitmap, i);
	}

	for (int i = 0; (unsigned)i < 100; i++) {
		bitarray_set_bit(bitmap, i);
	}

	close(fd);

	log_info(logger, "ARCHIVO %s LEIDO\n", rutaBitmap);

}

void crearDirectorio(char *path) {

    char *sep = strrchr(path, '/');
    if(sep != NULL) {
        *sep = 0;
        crearDirectorio(path);
        *sep = '/';
    }
    mkdir(path, 0777);

}

void operacion_fs(int op_cod, char* nombreArchivo){

	while(true){

		switch(op_cod)
		{
			case ABRIR:
				abrirArchivo(nombreArchivo);
				return;
				break;

			case CREAR:
				crearArchivo(nombreArchivo);
				return;
				break;

			case TRUNCAR:
				truncarArchivo(nombreArchivo, 0);
				return;
				break;

			case LEER:
				leerArchivo(nombreArchivo,1,1,1);
				return;
				break;

			case ESCRIBIR:
				escribirArchivo(nombreArchivo,1,1,1);
				return;
				break;

			default:
				break;
		}

	}

}

void abrirArchivo(const char* nombreArchivo) {
    printf("Abrir Archivo: %s\n", nombreArchivo);
    // Agregar código para abrir el archivo
}

void crearArchivo(const char* nombreArchivo) {
    printf("Crear Archivo: %s\n", nombreArchivo);
    // Agregar código para crear el archivo
}

void truncarArchivo(const char* nombreArchivo, uint32_t nuevoTamano) {
    printf("Truncar Archivo: %s - Tamaño: %d\n", nombreArchivo, nuevoTamano);
    // Agregar código para truncar el archivo
}

void leerArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano) {
    printf("Leer Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria, tamano);
    // Agregar código para leer el archivo
}

void escribirArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano) {
    printf("Escribir Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria, tamano);
    // Agregar código para escribir en el archivo
}

void correr_servidor(void){

    socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
    log_info(logger, "Iniciada la conexión servidor de FS: %d",socket_fs);

    // Quedar a la espera de la conexión por parte del Kernel
    socket_kernel = esperar_cliente(socket_fs, logger);

    int modulo = recibir_operacion(socket_kernel);

    switch(modulo){
		case KERNEL:
			log_info(logger, "Kernel Conectado.");
			log_info(logger, "TODO: Recibir Operación FS");
			enviar_mensaje("TODO: Generico",socket_kernel, logger);
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

void finalizarFS(int socket_servidor, t_log* logger, t_config* config)
{
    // Liberar la memoria asignada a fs_config
    free(fs_config);
    liberar_conexion(socket_fs);
    log_destroy(logger);
    config_destroy(config_fs);

}
