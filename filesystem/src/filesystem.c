 #include "../Include/filesystem.h"

int main(void){

    Superbloque superbloque;
    BitmapBloques bitmap;
    ArchivoBloques archivoBloques;
    FCB fcb;

    logger = iniciar_logger("file_system.log");
	log_info(logger, "MODULO FILE SYSTEM");

    // Inicializar el File System
	inicializar_fs(&superbloque, &bitmap, &archivoBloques);

    // Inicializar el File System
    finalizar_fs(socket_fs, logger, config_fs);

	return EXIT_SUCCESS;
}

void cargar_config_fs(t_config* config_fs){

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

void inicializar_fs(Superbloque* sb, BitmapBloques* bitmap, ArchivoBloques* archivoBloques) {

	// Inicializar configuración
	config_fs = iniciar_config(PATH_CONFIG);
	cargar_config_fs(config_fs);

	// Conectarse a la Memoria y realizar el handshake
	conectar_con_memoria();

	// Iniciar file system como servidor de kernel
	correr_servidor();

	// Levantar el bitmap de bloques y recorrer el directorio de FCBs

}

void conectar_con_memoria() {

	log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);

	// Conexión con el módulo Memoria
	socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);

	// Realizar handshake inicial
	enviar_handshake(socket_memoria, FILESYSTEM);

    // Verificar la conexión

}

void correr_servidor(void){

	socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
	log_info(logger, "Iniciada la conexión servidor de FS: %d",socket_fs);

	// Quedar a la espera de la conexión por parte del Kernel
	socket_kernel = esperar_cliente(socket_fs, logger);
	//estado_socket_kernel = validar_conexion(socket_kernel);
	// verificar socket != -1

    // Atender las solicitudes del Kernel para operar con los archivos
    while (true) {
        // Recibir solicitud del Kernel

        // Realizar la operación correspondiente según la solicitud

        // Enviar respuesta al Kernel
    }

    while(1){

		int modulo = recibir_operacion(socket_kernel);

		switch(modulo){
			case KERNEL:
				log_info(logger, "Kernel Conectado.");
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

void accederBitmap(uint32_t numeroBloque, int estado) {
    printf("Acceso a Bitmap - Bloque: %d - Estado: %d\n", numeroBloque, estado);
    // Agregar código para acceder al bitmap
}

void accederBloque(const char* nombreArchivo, uint32_t numeroBloqueArchivo, uint32_t numeroBloqueFS) {
    printf("Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d\n", nombreArchivo, numeroBloqueArchivo, numeroBloqueFS);
    // Agregar código para acceder al bloque
}

void finalizar_fs(int socket_servidor, t_log* logger, t_config* config)
{
	liberar_conexion(socket_fs);
	printf("KERNEL FINALIZADO \n");
}
