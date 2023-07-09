#include "../Include/filesystem.h"

int main(int argc, char **argv) {

    logger = iniciar_logger("file_system.log");
    log_info(logger, "MODULO FILE SYSTEM");

    config = iniciar_config("file_system.config");//argv[1]);
    cargar_config_fs(config); //cargar configuración

    //conectar_con_memoria(); //conexion con Memoria
    iniciar_fs(); // inicializar File System
    correr_servidor(); // iniciar file system como servidor de kernel
    finalizar_fs(socket_fs, logger, config); // Finalizar el File System

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

void conectar_con_memoria() {
    log_debug(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    socket_memoria = crear_conexion(fs_config->IP_MEMORIA, fs_config->PUERTO_MEMORIA);
    enviar_handshake(socket_memoria, FILESYSTEM);
}

void iniciar_fs() {
    if (existe_fs() == 0) {
        log_debug(logger, "File System encontrado, recuperando...");
        iniciar_fcbs();
        iniciar_superbloque();
        iniciar_bitmap();
        iniciar_bloques();
        crear_archivo("Consoles");
        abrir_archivo("Consoles");
        truncar_archivo("Consoles");
    } else {
    	log_debug(logger, "File System NO encontrado, generando...");
        crear_directorio(fs_config->PATH_FCB);
        iniciar_fs();
    }
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

int existe_fs() {
	return access(fs_config->PATH_FCB, F_OK);
}

void iniciar_superbloque(){
    config = iniciar_config(fs_config->PATH_SUPERBLOQUE);

    superbloque = malloc(sizeof(Superbloque));
    superbloque->BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
    superbloque->BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
    log_debug(logger, "ARCHIVO %s LEIDO", fs_config->PATH_SUPERBLOQUE);
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

void correr_servidor(void) {
	socket_fs = iniciar_servidor(fs_config->PUERTO_ESCUCHA);
	log_debug(logger, "Iniciada la conexión servidor de FS: %d", socket_fs);
    socket_kernel = esperar_cliente(socket_fs, logger); // quedar a la espera de la conexión por parte del Kernel
    int modulo = recibir_operacion(socket_kernel);

    switch (modulo) {
        case KERNEL:
        	log_debug(logger, "Kernel Conectado.");
            enviar_mensaje("TOC TOC: Soy FS!", socket_kernel, logger);
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

    DIR* dir = opendir(fs_config->PATH_FCB);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
			levantar_fcb(nombreArchivo);
            if (strcmp(fcb->NOMBRE_ARCHIVO, nombreArchivo) == 0) {
                log_debug(logger, "El archivo %s existe.", nombreArchivo);
                closedir(dir);
                return F_EXISTS; // OK
            }
        }
    }
    closedir(dir);
    log_info(logger, "El archivo %s no existe.", nombreArchivo);
    return F_NOT_EXISTS;
}

int crear_archivo(const char* nombreArchivo) {
    log_info(logger, "Crear Archivo: <%s>", nombreArchivo);

    int tamanioArchivo = 0;
    char ruta[256];
    sprintf(ruta, "%s/%s", fs_config->PATH_FCB, nombreArchivo);

    struct stat st;
    if (stat(ruta, &st) != 0) {  // Verificar si el archivo ya existe
		FILE* archivo = fopen(ruta, "w");  // Abrir el archivo en modo escritura
		if (archivo == NULL) {  // Verificar si hubo un error al abrir el archivo
			log_error(logger, "No se pudo crear el archivo %s.", nombreArchivo);
			return F_OP_ERROR;
		}

    fprintf(archivo, "NOMBRE_ARCHIVO=%s\n", nombreArchivo);
    fprintf(archivo, "TAMANIO_ARCHIVO=%d\n", tamanioArchivo);
    fprintf(archivo, "PUNTERO_DIRECTO=%s\n", 0);
    fprintf(archivo, "PUNTERO_INDIRECTO=%s\n", 0);

	lista_archivos[arch_en_mem].NOMBRE_ARCHIVO = nombreArchivo;
	lista_archivos[arch_en_mem].TAMANIO_ARCHIVO = tamanioArchivo;
	lista_archivos[arch_en_mem].PUNTERO_DIRECTO = 0;
	lista_archivos[arch_en_mem].PUNTERO_INDIRECTO = 0;

    arch_en_mem++;

    fclose(archivo);  // Cerrar el archivo

    }

    log_debug(logger, "Archivo creado exitosamente.");
    return F_OP_OK;
}

void leer_archivo(const char* nombreArchivo) {
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

void levantar_fcb(const char* nombreArchivo){
	char ruta[256];
    sprintf(ruta, "%s/%s", fs_config->PATH_FCB, nombreArchivo);
    t_config* fcb_config = iniciar_config(ruta);
    cargar_config_fcb(fcb_config);
}

void cargar_config_fcb(t_config* config_file){
    fcb = malloc(sizeof(FCB));
    fcb->NOMBRE_ARCHIVO = config_get_string_value(config_file, "NOMBRE_ARCHIVO");
    fcb->TAMANIO_ARCHIVO = config_get_int_value(config_file, "TAMANIO_ARCHIVO");
    fcb->PUNTERO_DIRECTO = config_get_int_value(config_file, "PUNTERO_DIRECTO");
    fcb->PUNTERO_INDIRECTO = config_get_int_value(config_file, "PUNTERO_INDIRECTO");
}

void actualizar_fcb(FCB* archivo, int nuevo_tamanio, int nuevo_directo, int nuevo_indirecto) {
    archivo->TAMANIO_ARCHIVO = nuevo_tamanio;
    archivo->PUNTERO_DIRECTO = nuevo_directo;
    archivo->PUNTERO_INDIRECTO = nuevo_indirecto;
    persistir_fcb(archivo);
}

void persistir_fcb(FCB* fcb) {

    FILE* archivo; // Declarar un puntero de tipo FILE
	char ruta[256];
    sprintf(ruta, "%s/%s", fs_config->PATH_FCB, fcb->NOMBRE_ARCHIVO);
    archivo = fopen(ruta, "w"); // Abrir el archivo en modo escritura

    fprintf(archivo, "NOMBRE_ARCHIVO=%s\n", fcb->NOMBRE_ARCHIVO);
    fprintf(archivo, "TAMANIO_ARCHIVO=%d\n", fcb->TAMANIO_ARCHIVO);
    fprintf(archivo, "PUNTERO_DIRECTO=%d\n", fcb->PUNTERO_DIRECTO);
    fprintf(archivo, "PUNTERO_INDIRECTO=%d\n", fcb->PUNTERO_INDIRECTO);

    fclose(archivo);
}

t_bloque* crear_bloque(int bloque_index) {

	t_bloque* bloque = malloc(sizeof(t_bloque));

	int posicion_archivo = bloque_index * superbloque->BLOCK_SIZE;
	bloque->datos = malloc(superbloque->BLOCK_SIZE);
	bloque->inicio = posicion_archivo;
	bloque->fin = posicion_archivo + superbloque->BLOCK_SIZE - 1;
	bitarray_set_bit(bitmap, bloque_index);
	log_info(logger, "Creado bloque [%d - %d]", bloque->inicio, bloque->fin);

    // Llenar el bloque con datos vacíos
    memset(bloque->datos, 0, superbloque->BLOCK_SIZE);
	return bloque;
}

int obtener_bloque_libre(void) {
	//recorrer el bitmap hasta encontrar N posiciones contiguas
	for(int i = 0; i < superbloque->BLOCK_COUNT; i++){
	    //sleep(fs_config->RETARDO_ACCESO_BLOQUE/1000);
	    if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <LIBRE>", i);
			return i;
		}else{
			log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <OCUPADO>", i);
		}
	}
	return -1;
}

void finalizar_fs(int socket_servidor, t_log* logger, t_config* config) {
	log_debug(logger, "Finalizando File System...");
    free(fs_config);
    liberar_conexion(socket_fs);
    log_destroy(logger);
    config_destroy(config);
}

void liberarBloque(int index) {
	bitarray_clean_bit(bitmap, index);
	log_info(logger, "Bloque %d liberado", index);
}

void reservarBloque(int index) {
	bitarray_set_bit(bitmap, index);
	log_info(logger, "Bloque %d reservado", index);
}

void iniciar_fcbs(){
	int contador = cargar_archivos(lista_archivos);
	arch_en_mem = contador;
	for (int i = 0; i < contador; i++) {
		log_debug(logger, "Nombre del archivo: %s", lista_archivos[i].NOMBRE_ARCHIVO);
		log_debug(logger, "Tamaño del archivo: %d", lista_archivos[i].TAMANIO_ARCHIVO);
		log_debug(logger, "Puntero directo: %d", lista_archivos[i].PUNTERO_DIRECTO);
		log_debug(logger, "Puntero indirecto: %d", lista_archivos[i].PUNTERO_INDIRECTO);
	}

}

int cargar_archivos(FCB* lis_archivos){

	DIR* dir = opendir(fs_config->PATH_FCB);
    struct dirent* entry;
    int contador = 0;
    char nombre_archivo[256];

    while ((entry = readdir(dir)) != NULL) {
    	if (entry->d_type == DT_REG){
        strcpy(nombre_archivo, entry->d_name);
        levantar_fcb(nombre_archivo);
        FCB fcb_archivo = *fcb;
        lista_archivos[contador].NOMBRE_ARCHIVO = fcb_archivo.NOMBRE_ARCHIVO;
        lista_archivos[contador].TAMANIO_ARCHIVO = fcb_archivo.TAMANIO_ARCHIVO;
        lista_archivos[contador].PUNTERO_DIRECTO = fcb_archivo.PUNTERO_DIRECTO;
        lista_archivos[contador].PUNTERO_INDIRECTO = fcb_archivo.PUNTERO_INDIRECTO;

        contador++;

        if(contador > MAX_ARCHIVOS)
        	break;
    	}
	}

	closedir(dir);
	return contador;
}

int truncar_archivo(const char* nombreArchivo) {

    //char* paramTamanio = recibir_string(socket_kernel);
	char* paramTamanio = "256";
	uint32_t nuevoTamanio = atoi(paramTamanio);

	log_info(logger, "Truncar Archivo: <%s> - Tamaño: <%d>", nombreArchivo, nuevoTamanio);
	int indice_archivo = 0;
    for (int i = 0; i < MAX_ARCHIVOS; i++) {
        if (strcmp(lista_archivos[i].NOMBRE_ARCHIVO, nombreArchivo) == 0) {
        	indice_archivo = i;
            break;
        }
    }

    if (nuevoTamanio == lista_archivos[indice_archivo].TAMANIO_ARCHIVO) {
        log_debug(logger, "El archivo ya tiene el tamaño especificado.");
        return F_OP_OK;
    }

    if (nuevoTamanio > lista_archivos[indice_archivo].TAMANIO_ARCHIVO) {
    	// Aumentar el tamaño del archivo
		int bloquesNecesarios = (nuevoTamanio - lista_archivos[indice_archivo].TAMANIO_ARCHIVO) / superbloque->BLOCK_SIZE;

        if ((nuevoTamanio - lista_archivos[indice_archivo].TAMANIO_ARCHIVO) % superbloque->BLOCK_SIZE != 0) {
            bloquesNecesarios++;
        }

        int bloquesAsignados = 0;        // tendria que restar los bloques ya existentes
        int bloque = 0;
        int bloqueIndirecto = 0;
    	t_bloque* nuevo_bloqueindirecto;

        if(lista_archivos[indice_archivo].PUNTERO_INDIRECTO > 0){
        	nuevo_bloqueindirecto = obtener_bloque(lista_archivos[indice_archivo].PUNTERO_INDIRECTO);
        	if(!estaLlenoBloque(nuevo_bloqueindirecto->datos)){
        		bloqueIndirecto = lista_archivos[indice_archivo].PUNTERO_INDIRECTO;
        	}
        } else {
            bloqueIndirecto = obtener_bloque_libre();
     	    nuevo_bloqueindirecto = crear_bloque(bloqueIndirecto);
        }

 	    // Buscar bloques disponibles en el bitmap y asignarlos al archivo
        for (int i = 0; i < bloquesNecesarios; i++) {

        	bloque = obtener_bloque_libre();
        	bloquesAsignados++;

     	    t_bloque* nuevo_bloque = crear_bloque(bloque);

     	    lista_archivos[indice_archivo].PUNTERO_DIRECTO = bloque;
           	lista_archivos[indice_archivo].PUNTERO_INDIRECTO = bloqueIndirecto;

           	escribir_en_bloque(bloque, nuevo_bloque->datos);
           	strcat(nuevo_bloqueindirecto->datos, &bloque);
            levantar_fcb(lista_archivos[indice_archivo].NOMBRE_ARCHIVO);
           	actualizar_fcb(fcb, nuevoTamanio, bloque, bloqueIndirecto);
         }

       	escribir_en_bloque(bloqueIndirecto, nuevo_bloqueindirecto->datos);

        if (bloquesAsignados < bloquesNecesarios) {
            log_error(logger, "No hay suficientes bloques disponibles para aumentar el tamaño del archivo.");
            return F_OP_ERROR;
        }

        lista_archivos[indice_archivo].TAMANIO_ARCHIVO = nuevoTamanio;

        log_debug(logger, "Se aumentó el tamaño del archivo %s a %d bytes.", nombreArchivo, nuevoTamanio);

    } else {

    	int bloquesLibres = (lista_archivos[indice_archivo].TAMANIO_ARCHIVO - nuevoTamanio) / fs_config->BLOCK_SIZE;

    	if ((lista_archivos[indice_archivo].TAMANIO_ARCHIVO - nuevoTamanio) % fs_config->BLOCK_SIZE != 0) {
    		bloquesLibres++;
		}

        int bloquesLiberados = 0;
        bool liberarBloque = false;

        for (int i = bloquesLibres - 1; i >= 0; i--) {
        	if (!liberarBloque) {
				liberarBloque = true;
        	}

        	//void* datos = obtener_bloque_ocupado(lista_archivos[indice_archivo].PUNTERO_DIRECTO);
        	//OBTENER ULTIMO BLOQUE DEL ARRAY
        	//liberarBloque();

             bloquesLiberados++;
             if (bloquesLiberados == bloquesLibres) {
            	 break;
             }
        }
    }
    return F_OP_OK;
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

int estaLlenoBloque(char* cadena) {
    return (strlen(cadena) >= superbloque->BLOCK_SIZE);
}

t_bloque* obtener_bloque(int bloque_index) {

    off_t offset = bloque_index * superbloque->BLOCK_SIZE;

    size_t tamano_total = superbloque->BLOCK_SIZE * superbloque->BLOCK_COUNT;

    if (offset >= tamano_total) {
    	log_debug(logger, "Error: el offset está fuera de los límites de la cadena.\n");
        return NULL;
    }

    void* subcadena = (char*)bloques + offset;
    void* subcadena_copiada = malloc(superbloque->BLOCK_SIZE + 1); // +1 para el carácter nulo
    memcpy(subcadena_copiada, subcadena, superbloque->BLOCK_SIZE);

	t_bloque* bloque = malloc(sizeof(t_bloque));
	bloque->datos = subcadena_copiada;

	return bloque;
}
