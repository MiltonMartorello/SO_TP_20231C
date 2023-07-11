#include "../Include/filesystem.h"

int main(int argc, char **argv) {

	logger = iniciar_logger("file_system.log");
	if (logger == NULL){
		log_error(logger, "NO se inicializó el log.");
		exit(1);
	}

	log_info(logger, "MODULO FILE SYSTEM");

	config = iniciar_config(argv[1]);
    if (config == NULL){
		log_error(logger, "NO se inicializó la configuración del FS");
		exit(2);
	}

    cargar_config_fs(config);
    iniciar_fs();
    conectar_con_memoria();
    correr_servidor();

    return EXIT_SUCCESS;
}

void cargar_config_fs(t_config* config) {
    fs_config = malloc(sizeof(t_fs_config));
    fs_config->IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
    fs_config->PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
    fs_config->PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
    fs_config->PATH_SUPERBLOQUE = config_get_string_value(config, "PATH_SUPERBLOQUE");
    fs_config->PATH_BITMAP = config_get_string_value(config, "PATH_BITMAP");
    fs_config->PATH_BLOQUES = config_get_string_value(config, "PATH_BLOQUES");
    fs_config->PATH_FCB = config_get_string_value(config, "PATH_FCB");
    fs_config->RETARDO_ACCESO_BLOQUE = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");

    log_debug(logger, "La configuración se cargó en la estructura fs_config");
}

void iniciar_fs() {
	if (existe_fs() == 0) {
        log_debug(logger, "File System encontrado, recuperando...");
        iniciar_superbloque();
        iniciar_bitmap();
        iniciar_bloques();
        iniciar_FCBs();
    } else {
    	log_debug(logger, "File System NO encontrado, generando...");
        crear_directorio(fs_config->PATH_FCB);
        iniciar_fs();
    }
}

int existe_fs() {
	return access(fs_config->PATH_FCB, F_OK);
}

void iniciar_superbloque(){
	config = iniciar_config(fs_config->PATH_SUPERBLOQUE);
	superbloque = malloc(sizeof(Superbloque));
    superbloque->BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
    superbloque->BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
    log_debug(logger, "ARCHIVO %s LEIDO", fs_config->PATH_SUPERBLOQUE);
	config_destroy(config);
}

void iniciar_bitmap() {

	char* rutaBitmap = fs_config->PATH_BITMAP;
	int fd = open(rutaBitmap, O_CREAT | O_RDWR, 0777);
    int cantidadBloques; // 65536 -> tengo 65536 bloques

    if ((superbloque->BLOCK_COUNT % 8) == 0) {
        cantidadBloques = superbloque->BLOCK_COUNT / 8;
    } else {
        cantidadBloques = (superbloque->BLOCK_COUNT / 8) + 1;
    }

    ftruncate(fd, cantidadBloques);
    mapBitmap = mmap(NULL, cantidadBloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapBitmap == MAP_FAILED) {
    	close(fd);
    	exit(1);
	}
    msync(mapBitmap, sizeof(uint32_t), MS_SYNC); //sincroniza el bitmap cargado en memoria con el archivo en disco
    bitmap = bitarray_create_with_mode(mapBitmap, cantidadBloques, LSB_FIRST);
    log_debug(logger, "El tamaño del bitmap creado es de %zu bits.", bitarray_get_max_bit(bitmap));

    close(fd);
    log_debug(logger, "ARCHIVO %s LEIDO", rutaBitmap);
    //imprimir_bitmap(bitmap);
}

void imprimir_bitmap(t_bitarray* bitmap){
	printf("\n\nimprimiendo bitmap: \n[");
		for (int i = 0; i < superbloque->BLOCK_COUNT; i++) {
	        printf("%d ", bitarray_test_bit(bitmap, i));
	    }
		printf("]\n\n");
}

void iniciar_bloques(void) {

	char* rutaBloques = fs_config->PATH_BLOQUES;
    FILE* archivo_bloques = fopen(rutaBloques, "rb+");
    if (archivo_bloques == NULL) {
        archivo_bloques = fopen(rutaBloques, "wb+");
        if (archivo_bloques == NULL) {
            log_error(logger, "No se pudo abrir o crear el archivo bloques");
            exit(1);
        }
    }

    fseek(archivo_bloques, 0L, SEEK_END);
    int size_archivo = ftell(archivo_bloques);
    log_debug(logger, "Posicion del archivo: %d", size_archivo);

    if (size_archivo > 0) {
        size_t size = superbloque->BLOCK_COUNT * superbloque->BLOCK_SIZE;
        log_debug(logger, "Size de %zu", size);

        int fd = fileno(archivo_bloques);
        bloques = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (bloques == MAP_FAILED) {
            log_error(logger, "Error al mapear el archivo de bloques");
        } else {
        	log_debug(logger, "Lluege %s", (char*)bloques);
            msync(bloques, sizeof(uint32_t), MS_SYNC); //sincroniza el bitmap cargado en memoria con el archivo en disco
        }
    } else {
        bloques = NULL;
    }

    fclose(archivo_bloques);
    log_debug(logger, "ARCHIVO %s LEIDO", rutaBloques);
}

void iniciar_FCBs(){

	lista_fcb = list_create();
	DIR* dir = opendir(fs_config->PATH_FCB);
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
    	if (entry->d_type == DT_REG){
			char* path = string_new();
			string_append(&path, fs_config->PATH_FCB);
			string_append(&path, "/");
			string_append(&path, entry->d_name);

			t_config* fcb_config = config_create(path);

			char* nombre = config_get_string_value(fcb_config, "NOMBRE_ARCHIVO");
			uint32_t tamanio = config_get_int_value(fcb_config, "TAMANIO_ARCHIVO");
			uint32_t puntero_directo = config_get_int_value(fcb_config, "PUNTERO_DIRECTO");
			uint32_t puntero_indirecto = config_get_int_value(fcb_config, "PUNTERO_INDIRECTO");

			t_fcb* fcb = leer_fcb(nombre, tamanio, puntero_directo, puntero_indirecto);
			list_add(lista_fcb, fcb);
			config_destroy(fcb_config);
			free(path);
    	}
	}

	closedir(dir);
}

t_fcb* leer_fcb(char* nombre, uint32_t tamanio, uint32_t puntero_directo, uint32_t puntero_indirecto){
	t_fcb* fcb;
	fcb = malloc(sizeof(t_fcb));
	fcb->NOMBRE_ARCHIVO = string_new();
	string_append(&fcb->NOMBRE_ARCHIVO, nombre);
	fcb->TAMANIO_ARCHIVO = tamanio;
	fcb->PUNTERO_DIRECTO = puntero_directo;
	fcb->PUNTERO_INDIRECTO = puntero_indirecto;
	return fcb;
}

void crear_directorio(char* ruta) {
    char* sep = strrchr(ruta, '/');
    if (sep != NULL) {
        *sep = '\0';
        crear_directorio(ruta);
        *sep = '/';
    }
    mkdir(ruta, S_IRWXU | S_IRWXG | S_IRWXO);
}

void conectar_con_memoria() {
    log_debug(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    enviar_handshake(socket_memoria, FILESYSTEM);
}

void correr_servidor(void) {
	socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
	log_debug(logger, "Iniciada la conexión servidor de FS: %d", socket_fs);
    socket_kernel = esperar_cliente(socket_fs, logger); // quedar a la espera de la conexión por parte del Kernel
    int modulo = recibir_operacion(socket_kernel);

    switch (modulo) {
        case KERNEL:
            log_info(logger, "Kernel Conectado.");
            enviar_mensaje("TODO: Generico", socket_kernel);
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
    	log_debug(logger, "Recibida operación %d", cod_op);
    	char* nombre_archivo = recibir_string(socket_kernel); //SE RECIBE TAMBIÉN EL NOMBRE DEL ARCHIVO YA QUE ES EL PRIMER PARAMETRO SIEMPRE
    	log_debug(logger, "Recibido Archivo %s", nombre_archivo);

    	switch (cod_op) {
			case F_OPEN:
				log_info(logger, "Se recibió un F_OPEN para el archivo %s", nombre_archivo);
				resultado = abrir_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_CREATE:
				log_info(logger, "Se recibió un F_CREATE para el archivo %s", nombre_archivo);
				resultado = crear_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_TRUNCATE:
				log_info(logger, "Se recibió un F_TRUNCATE para el archivo %s", nombre_archivo);
				resultado = truncar_archivo(nombre_archivo);
				enviar_entero(socket_kernel, resultado);
				break;
			case F_READ:
				//TODO
				log_info(logger, "Se recibió un F_READ para el archivo %s", nombre_archivo);
				leer_archivo(nombre_archivo);
				break;
			case F_WRITE:
				//TODO
				log_info(logger, "Se recibió un F_WRITE para el archivo %s", nombre_archivo);
				escribir_archivo(nombre_archivo);
				break;
			default:
				break;
		}
    }
}

int abrir_archivo(const char* nombreArchivo) {

	log_info(logger, "Abrir Archivo: <%s>", nombreArchivo);

	if(existe_archivo(nombreArchivo)){
		log_debug(logger, "El Archivo <%s> existe.", nombreArchivo);
		return F_OPEN_OK;
	}else{
		log_debug(logger, "El Archivo <%s> no existe.", nombreArchivo);
		return F_NOT_EXISTS;
	}

}

int existe_archivo(char* nombre){
	for(int i=0; i<list_size(lista_fcb);i++){
		t_fcb* fcb= list_get(lista_fcb, i);
		if(string_equals_ignore_case(fcb->NOMBRE_ARCHIVO, nombre)){
			return 1;
		}
	}
	return 0;
}

int crear_archivo(const char* nombreArchivo) {
    log_info(logger, "Crear Archivo: <%s>", nombreArchivo);
	t_fcb* nuevo_fcb = crear_fcb(nombreArchivo);
	list_add(lista_fcb, nuevo_fcb);
    return F_OP_OK;
}

t_fcb* crear_fcb(char* nombre){

	t_fcb* fcb_nuevo= malloc(sizeof(t_fcb));

	fcb_nuevo->NOMBRE_ARCHIVO = string_new();
	string_append(&fcb_nuevo->NOMBRE_ARCHIVO, nombre);
	fcb_nuevo->TAMANIO_ARCHIVO = 0;
	fcb_nuevo->PUNTERO_DIRECTO = 0;
	fcb_nuevo->PUNTERO_INDIRECTO = 0;

	char* path = string_new();

	string_append(&path, fs_config->PATH_FCB);
	string_append(&path, "/");
	string_append(&path, nombre);
	string_append(&path, ".dat");

	FILE* f = fopen(path, "wr");
	fprintf(f, "NOMBRE_ARCHIVO=%s\nTAMANIO_ARCHIVO=0\nPUNTERO_DIRECTO=\nPUNTERO_INDIRECTO=", nombre);
	fclose(f);

	return fcb_nuevo;
}

int truncar_archivo(const char* nombreArchivo) {

	char* paramTamanio = recibir_string(socket_kernel);
	uint32_t nuevoTamanio = atoi(paramTamanio);

	log_info(logger, "Truncar Archivo: <%s> - Tamaño: <%d>", nombreArchivo, nuevoTamanio);

	t_fcb* fcb = obtener_fcb(nombreArchivo);

    if (nuevoTamanio == fcb->TAMANIO_ARCHIVO) {
        log_debug(logger, "El archivo <%s> ya tiene el tamaño especificado.", nombreArchivo);
        return F_OP_OK;
    }

    if (nuevoTamanio > fcb->TAMANIO_ARCHIVO) {

    	// Aumentar el tamaño del archivo
		int bloquesNecesarios = (nuevoTamanio - fcb->TAMANIO_ARCHIVO) / superbloque->BLOCK_SIZE;

        if ((nuevoTamanio - fcb->TAMANIO_ARCHIVO) % superbloque->BLOCK_SIZE != 0) {
            bloquesNecesarios++;
        }

        int bloquesAsignados = 0;

        // Buscar bloques disponibles en el bitmap y asignarlos al archivo
        for (int i = 0; i < bloquesNecesarios; i++) {

        	if(fcb->TAMANIO_ARCHIVO == 0){//si no tiene asignado un puntero directo
        		fcb->PUNTERO_DIRECTO = obtener_bloque_libre();
        		t_bloque* bloque = crear_bloque(fcb->PUNTERO_DIRECTO);
				escribir_en_bloque(fcb->PUNTERO_DIRECTO, bloque->datos);
				fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO + superbloque->BLOCK_SIZE;
	        	bloquesAsignados++;
        	}else{
				if(bloquesAsignados<2){//si no tiene asignado un puntero indirecto
					fcb->PUNTERO_INDIRECTO = obtener_bloque_libre();
					t_bloque* bloque_indirecto = crear_bloque(fcb->PUNTERO_INDIRECTO);
					escribir_en_bloque(fcb->PUNTERO_INDIRECTO, bloque_indirecto->datos);
					fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO + superbloque->BLOCK_SIZE;
		        	bloquesAsignados++;
				}else{
					uint32_t bloque_asignado = obtener_bloque_libre();
					t_bloque* bloque_adicional = crear_bloque(bloque_asignado);
					escribir_en_bloque(bloque_asignado, bloque_adicional->datos);
					fcb->TAMANIO_ARCHIVO = fcb->TAMANIO_ARCHIVO + superbloque->BLOCK_SIZE;
		        	bloquesAsignados++;
				}
        	}
        }

		if (bloquesAsignados < bloquesNecesarios) {
            log_error(logger, "No hay suficientes bloques disponibles para aumentar el tamaño del archivo.");
            return F_OP_ERROR;
        }

        log_debug(logger, "Se aumentó el tamaño del archivo %s a %d bytes.", nombreArchivo, nuevoTamanio);

     }else {

    	int bloquesALiberar = (fcb->TAMANIO_ARCHIVO - nuevoTamanio) / superbloque->BLOCK_SIZE;

    	if ((fcb->TAMANIO_ARCHIVO - nuevoTamanio) % superbloque->BLOCK_SIZE != 0) {
    		bloquesALiberar++;
		}

    	int bloquesAsignados = fcb->TAMANIO_ARCHIVO / superbloque->BLOCK_SIZE;

    	for (int i = 0; i < bloquesALiberar; i++) {

			if(bloquesAsignados==1){
				bitarray_clean_bit(bitmap, fcb->PUNTERO_DIRECTO);
			}else{
				int posicion = fcb->PUNTERO_INDIRECTO* superbloque->BLOCK_SIZE + (bloquesAsignados - 2)*4;
				uint32_t bloque_a_liberar;
				leer_en_bloques((void*)&bloque_a_liberar,posicion, sizeof(uint32_t));
				liberar_bloque(bloque_a_liberar);
				if(bloquesAsignados==2){
					bitarray_clean_bit(bitmap, fcb->PUNTERO_INDIRECTO);//libero el bloque del puntero indirecto
				}
			}
			bloquesAsignados-=1;
        }

    }

   	actualizar_fcb(fcb);
    return F_OP_OK;

}

t_fcb* obtener_fcb(char* archivo){
	for(int i=0;i<list_size(lista_fcb);i++){
		t_fcb* fcb = list_get(lista_fcb, i);
		if(string_equals_ignore_case(fcb->NOMBRE_ARCHIVO, archivo)){
			return fcb;
		}
	}
	return NULL;
}

void actualizar_fcb(t_fcb* fcb) {

	char* path = string_new();
	string_append(&path, fs_config->PATH_FCB);
	string_append(&path, "/");
	string_append(&path, fcb->NOMBRE_ARCHIVO);
	string_append(&path, ".dat");

	FILE* f = fopen(path, "wr");
	fprintf(f, "NOMBRE_ARCHIVO=%s\nTAMANIO_ARCHIVO=%d\nPUNTERO_DIRECTO=%d\nPUNTERO_INDIRECTO=%d", fcb->NOMBRE_ARCHIVO, fcb->TAMANIO_ARCHIVO, fcb->PUNTERO_DIRECTO, fcb->PUNTERO_INDIRECTO);
	fclose(f);
	free(path);
}

void escribir_en_bloque(uint32_t numero_bloque, char* contenido) {

    uint32_t bloque_size = superbloque->BLOCK_SIZE;
    uint32_t offset = numero_bloque * bloque_size;

    FILE* archivo_bloques = fopen(fs_config->PATH_BLOQUES, "rb+");
    if (archivo_bloques == NULL) {
        log_error(logger, "No se pudo abrir el archivo de bloques");
        return;
    }

    fseek(archivo_bloques, offset, SEEK_SET);
    size_t bytes_escritos = fwrite(contenido, sizeof(char), bloque_size, archivo_bloques);
    if (bytes_escritos != bloque_size) {
        log_error(logger, "Error al escribir en el bloque");
    } else {
    	log_debug(logger, "Contenido escrito en el bloque %u", numero_bloque);
    }

    fclose(archivo_bloques);
}

uint32_t obtener_bloque_libre(void) {

	for(int i = 0; i < superbloque->BLOCK_COUNT; i++){
	    if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <LIBRE>", i);
			return i;
		}else{
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <OCUPADO>", i);
		}
	}
	return -1;
}

void reservar_bloque(int index) {
	bitarray_set_bit(bitmap, index);
	log_debug(logger, "Bloque %d reservado", index);
	imprimir_bitmap(bitmap);
}

void leer_en_bloques(void* aLeer, int posicion, int cantidad){
	FILE* archivo_bloques = fopen(fs_config->PATH_BLOQUES, "r+b");
	fseek(archivo_bloques, posicion, SEEK_SET);
	fread(aLeer, cantidad,1, archivo_bloques);
	fclose(archivo_bloques);
}

void liberar_bloque(int index) {
	bitarray_clean_bit(bitmap, index);
	log_debug(logger, "Bloque %d liberado", index);
	imprimir_bitmap(bitmap);
}

t_bloque* crear_bloque(int bloque_index) {

	t_bloque* bloque = malloc(sizeof(t_bloque));

	int posicion_archivo = bloque_index * superbloque->BLOCK_SIZE;
	bloque->datos = malloc(superbloque->BLOCK_SIZE);
	bloque->inicio = posicion_archivo;
	bloque->fin = posicion_archivo + superbloque->BLOCK_SIZE - 1;
	log_info(logger, "Creado bloque [%d - %d]", bloque->inicio, bloque->fin);
	memset(bloque->datos, 0, superbloque->BLOCK_SIZE);
	reservar_bloque(bloque_index);
	return bloque;

}

void finalizar_fs(int socket_servidor, t_log* logger, t_config* config) {
	log_debug(logger, "Finalizando File System...");
    free(fs_config);
    liberar_conexion(socket_fs);
    log_destroy(logger);
    config_destroy(config);
}

void leer_archivo(const char* nombreArchivo) {
    //sleep(fs_config->RETARDO_ACCESO_BLOQUE/1000);
   // printf("Leer Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria, tamano);
	//char* direccion_logica = recibir_string(socket_kernel);
	//char* cantidad_de_bytes = recibir_string(socket_kernel);
	//recordar que existe atoi que toma un char* y devuelve un entero
	// Ej: int tamanio_int = atoi(tamanio);
	// Agregar código para leer el archivo
}

void escribir_archivo(const char* nombreArchivo) {
 //   printf("Escribir Archivo: %s - Puntero: %d - Memoria: %d - Tamaño: %d\n", nombreArchivo, puntero, direccionMemoria,  tamano);
	//char* direccion_logica = recibir_string(socket_kernel);
	//char* cantidad_de_bytes = recibir_string(socket_kernel);
	//recordar que existe atoi que toma un char* y devuelve un entero
	// Ej: int tamanio_int = atoi(tamanio);
	// Agregar código para escribir en el archivo
}
