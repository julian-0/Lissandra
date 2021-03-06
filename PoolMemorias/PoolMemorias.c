#include "PoolMemorias.h"

	pthread_t threadJournal;
	pthread_t threadGossiping;

int main(int argc, char* argv[]) {

	pathConfig = argv[1];

	bool estado = iniciar_programa();
	if(!estado)
		return 0;

	gossiping();

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int errThreadJournal = pthread_create(&threadJournal, &attr, journalConRetardo, NULL);
	if(errThreadJournal != 0) {
		log_info(g_logger,"Hubo un problema al crear el thread journalConRetardo:[%s]", strerror(errThreadJournal));
	}

	pthread_attr_destroy(&attr);

	//pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int errThreadGossiping = pthread_create(&threadGossiping, &attr, gossipingConRetardo, NULL);
	if(errThreadGossiping != 0) {
		log_info(g_logger,"Hubo un problema al crear el thread gossipingConRetardo:[%s]", strerror(errThreadGossiping));
	}
	pthread_attr_destroy(&attr);


	pthread_t conexionesEntrantes;
	pthread_create(&conexionesEntrantes,NULL,(void*) escucharConexiones,NULL);

	pthread_t monitoreador;
	pthread_create(&monitoreador,NULL,(void*) monitorearConfig,NULL);


	consola();

	terminar_programa();


}

void escucharConexiones(){
	conexion_servidor = iniciarServidor();

	struct sockaddr_in cliente;
	socklen_t longc = sizeof(cliente);

	t_list* conexiones = list_create();

	while(ejecutando){
		int *conexion_cliente=malloc(sizeof(int));

		*conexion_cliente= accept(conexion_servidor, (struct sockaddr *) &cliente, &longc);
		log_info(g_logger,"Conectado con Socket:%d ",*conexion_cliente);

		if(*conexion_cliente<0) {
			log_info(g_logger,"Error al aceptar tráfico");
			free(conexion_cliente);
			break;
		} else {
			list_add(conexiones,conexion_cliente);

			int status = 0;
			uint32_t* tipoCliente = malloc(sizeof(uint32_t));
			status = recv(*conexion_cliente,tipoCliente,sizeof(uint32_t),0);

			if(status != sizeof(uint32_t))
				log_info(g_logger, "Error al recibir info del cliente");


			if(*tipoCliente==1){
				log_info(g_logger, "Nueva conexion del kernel");
				send(*conexion_cliente,&memoryNumber,sizeof(memoryNumber),0);
				iniciarHiloKernel(&cliente,&longc,conexion_cliente);
			}
			else if(*tipoCliente==0){
				log_info(g_logger, "Nueva conexion de una memoria");
				send(*conexion_cliente,&memoryNumber,sizeof(memoryNumber),0);
				iniciarHiloMemoria(&cliente,&longc,conexion_cliente);
			}
			else{
				log_info(g_logger,"Error al reconocer cliente");
			}
			free(tipoCliente);
		}

	}
	list_destroy_and_destroy_elements(conexiones,free);
}

void iniciarHiloMemoria(struct sockaddr_in *cliente, socklen_t *longc, int* conexion_cliente){
	pthread_attr_t attr;
	pthread_t thread;

	*longc = sizeof(cliente);
	log_info(g_logger,"Conectado con %s:%d", inet_ntoa(cliente->sin_addr),htons(cliente->sin_port));

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int err = pthread_create(&thread, &attr, escucharMemoria, conexion_cliente);
	if(err != 0) {
		log_info(g_logger,"[esperarClienteNuevo] Hubo un problema al crear el thread para escuchar la memoria:[%s]", strerror(err));
	}
	pthread_attr_destroy(&attr);
}

void escucharMemoria(int *conexion_cliente){
	log_info(g_logger,"Conexion cliente: %d", *conexion_cliente);
	int estado;
	do{
		estado = recibirYmandar(*conexion_cliente);
	}while(estado>0);
	close(*conexion_cliente);
}

void iniciarHiloKernel(struct sockaddr_in *cliente, socklen_t *longc, int* conexion_cliente){
	pthread_attr_t attr;
	pthread_t thread;

	*longc = sizeof(cliente);
	log_info(g_logger,"Conectado con %s:%d", inet_ntoa(cliente->sin_addr),htons(cliente->sin_port));

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int err = pthread_create(&thread, &attr, escucharKernel, conexion_cliente);
	if(err != 0) {
		log_info(g_logger,"[esperarClienteNuevo] Hubo un problema al crear el thread para escuhcar al kernel:[%s]", strerror(err));
	}
	pthread_attr_destroy(&attr);
}

void escucharKernel(int* conexion_cliente){

	resultado res;
	res.resultado = OK;

	while(res.resultado!=SALIR){

		resultadoParser resParser = recibirRequest(*conexion_cliente);

		pthread_mutex_lock(&mJournal);
		res = parsear_mensaje(&resParser);
		pthread_mutex_unlock(&mJournal);

		if(res.resultado == OK)
		{
			log_info(g_logger,res.mensaje);
		}
		else if(res.resultado == ERROR)
		{
			log_error(g_logger,"Ocurrio un error al ejecutar la acción: %s",res.mensaje);
		}
		else if(res.resultado == MENSAJE_MAL_FORMATEADO)
		{
			log_error(g_logger,"Mensaje incorrecto");
		}
		else if(res.resultado == EnJOURNAL)
		{
			log_warning(g_logger,"Se esta haciendo Journaling, ingrese la request mas tarde");
		}
		else if(res.resultado == SALIR)
		{
			log_error(g_logger,"El kernel se desconecto");
			close(*conexion_cliente);
		}
		else if(res.resultado == FULL){
			log_warning(g_logger,"Memoria FULL");
		}

		avisarResultado(res,*conexion_cliente);

		if(res.mensaje!=NULL)
			free(res.mensaje);
	}

}

void avisarResultado(resultado res, int conexion_cliente){
	if(res.resultado==ENVIADO || res.resultado==SALIR)
		return;
	int size_to_send;
	char* paqueteRespuesta = serializarRespuesta(&res, &size_to_send);
	send(conexion_cliente, paqueteRespuesta, size_to_send, 0);
	free(paqueteRespuesta);
}

resultadoParser recibirRequest(int conexion_cliente){
	char* buffer2 = malloc(sizeof(int));
	accion acc;
	resultadoParser rp;
	int status;


	int valueResponse = recv(conexion_cliente, buffer2, sizeof(int), 0);
	memcpy(&acc, buffer2, sizeof(int));

	if(valueResponse < 0) { //Comenzamos a recibir datos del cliente
		log_info(g_logger,"Error al recibir los datos");
		rp.accionEjecutar = SALIR_CONSOLA;
	} else if(valueResponse == 0) {
		rp.accionEjecutar = SALIR_CONSOLA;
	} else {
		rp.accionEjecutar = acc;

		if(rp.accionEjecutar == GOSSIPING){
			int socket = conexion_cliente;
			rp.contenido = malloc(sizeof(int));
			memcpy(rp.contenido,&socket,sizeof(int));

			free(buffer2);
			return rp;
		}
		if(rp.accionEjecutar == JOURNAL){
			free(buffer2);
			rp.contenido = NULL;
			return rp;
		}

		status = recibirYDeserializarPaquete(conexion_cliente, &rp);
		log_info(g_logger,"La accion es:%d", rp.accionEjecutar);
		if(status<0)
			rp.accionEjecutar = SALIR_CONSOLA;
	}
	free(buffer2);
	return rp;
}

int iniciarServidor() {

	int conexion_servidor, puerto;
	struct sockaddr_in servidor;

	puerto = config_get_int_value(g_config,"PUERTO");
	conexion_servidor = socket(AF_INET, SOCK_STREAM, 0);

	bzero((char *)&servidor, sizeof(servidor));
	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(puerto);
	servidor.sin_addr.s_addr = INADDR_ANY;

	int aceptar = 1;
	setsockopt(conexion_servidor, SOL_SOCKET, SO_REUSEADDR, &aceptar, sizeof(int));

	if(bind(conexion_servidor, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
		log_error(g_logger,"Error al asociar el puerto a la conexion. Posiblemente el puerto se encuentre ocupado");
	    close(conexion_servidor);
	    return -1;
	}

	listen(conexion_servidor, 3);
	log_info(g_logger,"A la escucha en el puerto %d", ntohs(servidor.sin_port));

	return conexion_servidor;
}


int conectarAlKernel(int conexion_servidor){
	int conexion_cliente;
	struct sockaddr_in cliente;
	socklen_t longc; //Debemos declarar una variable que contendrá la longitud de la estructura

	conexion_cliente = accept(conexion_servidor, (struct sockaddr *)&cliente, &longc);
	if(conexion_cliente<0)
		log_error(g_logger,"Error al aceptar trafico");
	else{
		longc = sizeof(cliente);
		log_error(g_logger,"Conectando con %s:%d", inet_ntoa(cliente.sin_addr),htons(cliente.sin_port));
	}
	return conexion_cliente;
}

void journalConRetardo(){
	while(ejecutando){
		pthread_mutex_lock(&mRJournal);
		usleep(retardoJournaling*1000);
		pthread_mutex_unlock(&mRJournal);

		journal();
	}
}

void gossipingConRetardo(){
	while(ejecutando){
		pthread_mutex_lock(&mRGossip);
		usleep(retardoGossiping*1000);
		pthread_mutex_unlock(&mRGossip);

		gossiping();
	}
}


void actualizarRetardos(){

	config_destroy(g_config);
	g_config = config_create(pathConfig);

	while(g_config==NULL){
		g_config = config_create(pathConfig);
	}

	while(!config_has_property(g_config,"RETARDO_JOURNAL")){
		config_destroy(g_config);
		g_config = config_create(pathConfig);
	}
	pthread_mutex_lock(&mRJournal);
	retardoJournaling = config_get_int_value(g_config,"RETARDO_JOURNAL");
	pthread_mutex_unlock(&mRJournal);

	while(!config_has_property(g_config,"RETARDO_GOSSIPING")){
		config_destroy(g_config);
		g_config = config_create(pathConfig);
	}
	pthread_mutex_lock(&mRGossip);
	retardoGossiping = config_get_int_value(g_config,"RETARDO_GOSSIPING");
	pthread_mutex_unlock(&mRGossip);

	while(!config_has_property(g_config,"RETARDO_MEM")){
		config_destroy(g_config);
		g_config = config_create(pathConfig);
	}
	pthread_mutex_lock(&mRMem);
	retardoMemoria = config_get_int_value(g_config,"RETARDO_MEM");
	pthread_mutex_unlock(&mRMem);

	while(!config_has_property(g_config,"RETARDO_FS")){
		config_destroy(g_config);
		g_config = config_create(pathConfig);
	}
	pthread_mutex_lock(&mRLfs);
	retardoLFS = config_get_int_value(g_config,"RETARDO_FS");
	pthread_mutex_unlock(&mRLfs);

}


void monitorearConfig() {
    int length, i = 0;
    int fd;
    int wd;
    char buffer[BUF_LEN];

    fd = inotify_init();

    if (fd < 0) {
        perror("inotify_init");
    }

    wd = inotify_add_watch(fd, pathDirectorio,IN_MODIFY);

    while(ejecutando){
    	i=0;
        length = read(fd, buffer, BUF_LEN);

        if (length < 0) {
            perror("read");
        }
        if(length == 0){
        	log_error(g_logger,"No lei cambios en config");
        }

        while (i < length) {
            struct inotify_event *event =
                (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_MODIFY) {
                	if(strcmp(pathConfig,event->name)==0){
                		log_info(g_logger,"El archivo %s fue modificado.", event->name);
                		actualizarRetardos();
                	}
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }

    (void) inotify_rm_watch(fd, wd);
    (void) close(fd);

}

bool iniciar_programa()
{

	pthread_mutex_init(&mMemPrincipal,NULL);
	pthread_mutex_init(&mTabSeg,NULL);
	pthread_mutex_init(&mTabPagGlobal,NULL);
	pthread_mutex_init(&mMemoriasConocidas,NULL);
	pthread_mutex_init(&mBitmap,NULL);
	pthread_mutex_init(&mConexion,NULL);
	pthread_mutex_init(&mJournal,NULL);

	pthread_mutex_init(&mRJournal,NULL);
	pthread_mutex_init(&mRGossip,NULL);
	pthread_mutex_init(&mRMem,NULL);
	pthread_mutex_init(&mRLfs,NULL);


	estaHaciendoJournal = false;
	ejecutando = true;

	getcwd(pathDirectorio, sizeof(pathDirectorio));

	//Inicio el logger
	g_logger = log_create("PoolMemorias.log", "MEM", 1, LOG_LEVEL_INFO);
	log_info(g_logger,"Inicio Aplicacion Pool Memorias");

	//Inicio las configs
	g_config = config_create(pathConfig);
	log_info(g_logger,"Configuraciones inicializadas");

	//Me conecto al LFS
	bool estado = gestionarConexionALFS();
	if(!estado){
		log_error(g_logger,"Error al conectarse al LFS");
		return false;
	}


	//hacer handshake con LFS y obtener tamaño de mem ppl y value
	estado = handshake();
	if(!estado)
		return false;

	retardoJournaling = config_get_int_value(g_config,"RETARDO_JOURNAL");
	retardoGossiping = config_get_int_value(g_config,"RETARDO_GOSSIPING");
	retardoMemoria = config_get_int_value(g_config,"RETARDO_MEM");
	retardoLFS = config_get_int_value(g_config,"RETARDO_FS");
	memoryNumber = config_get_int_value(g_config,"MEMORY_NUMBER");
	char* auxIP = config_get_string_value(g_config,"IP_PROPIA");
	char* auxPuerto = config_get_string_value(g_config,"PUERTO");

	TAM_MEMORIA_PRINCIPAL = config_get_int_value(g_config,"TAM_MEM");

	memoria=malloc(TAM_MEMORIA_PRINCIPAL);

	if(memoria==NULL){
		log_error(g_logger,"Error al inicializar la memoria principal");
		return false;
	}

	offset = sizeof(uint16_t)+sizeof(uint64_t)+tamValue;

	cantidadFrames = TAM_MEMORIA_PRINCIPAL / offset;


	bitmap=calloc(cantidadFrames, sizeof(int));


	posLibres= cantidadFrames;

	pthread_mutex_lock(&mMemoriasConocidas);
	memoriasConocidas = list_create();
	pthread_mutex_unlock(&mMemoriasConocidas);

	yo = malloc(sizeof(Memoria));
	pthread_mutex_init(&mMemYo,NULL);

	pthread_mutex_lock(&mMemYo);
	yo->ip = strdup(auxIP);
	yo->puerto=strdup(auxPuerto);
	yo->numero = memoryNumber;
	yo->socket=-1;
	yo->estado = 1;
	ponerTimestampActual(yo);
	pthread_mutex_unlock(&mMemYo);

	iniciarTablaSeeds();

	pthread_mutex_lock(&mMemoriasConocidas);
	list_add(memoriasConocidas,yo);
	pthread_mutex_unlock(&mMemoriasConocidas);

	iniciar_tablas();

	return true;
}

void iniciarTablaSeeds(){
	int i=0;

	memoriasSeeds = list_create();
	char** ips = config_get_array_value(g_config,"IP_SEEDS");
	char** puertos = config_get_array_value(g_config,"PUERTO_SEEDS");

	while(ips[i]!=NULL){
		Memoria* mem = malloc(sizeof(Memoria));
		mem->ip = strdup(ips[i]);
		mem->puerto = strdup(puertos[i]);
		mem->numero = -1;
		mem->socket = -1;
		mem->estado = 0;
		ponerTimestampActual(mem);
		bool res = conectarMemoria(mem);

		list_add(memoriasSeeds,mem);

		i++;
	}
}




bool handshake(){
	bool estado;
	resultadoParser resParser;
	resParser.accionEjecutar = HANDSHAKE;


	int size_to_send;

	retardarLfs();

	char* pi = serializarPaquete(&resParser, &size_to_send);
	send(serverSocket, pi, size_to_send, 0);
	free(pi);

	accion acc;
	char* buffer = malloc(sizeof(int));
	int valueResponse = recv(serverSocket, buffer, sizeof(int), 0);
	memcpy(&acc, buffer, sizeof(int));
	if(valueResponse < 0) {
		log_error(g_logger,"Error al recibir los datos del handshake");
		estado = false;
	} else {
		resultado res;
		res.accionEjecutar = acc;
		int status = recibirYDeserializarRespuesta(serverSocket, &res);
		if(status<0) {
			log_error(g_logger,"Error al deserializar respuesta");
			estado = false;
		} else {
			log_info(g_logger,"Recibi la respuesta del HANDSHAKE");
			log_info(g_logger,"El tamaño del value es: %i", ((resultadoHandshake*)(res.contenido))->tamanioValue);
			tamValue=((resultadoHandshake*)(res.contenido))->tamanioValue;
			estado = true;
		}
		free(res.mensaje);
	}
	free(buffer);
	return estado;
}


void actualizarTablaGlobal(int nPagina){
	bool mismoNumero(void* elem){
		return ((NodoTablaPaginas* )elem)->pagina->numero_pagina == nPagina;
	}

	pthread_mutex_lock(&mTabPagGlobal);
	NodoTablaPaginas* nodo = list_remove_by_condition(tabla_paginas_global,mismoNumero);
	pthread_mutex_unlock(&mTabPagGlobal);

	list_add(tabla_paginas_global,nodo);
}

Registro* pedirAlLFS(char* nombre_tabla, uint16_t key){

	Registro* registro;

	resultadoParser resParser;
	resParser.accionEjecutar=SELECT;
	contenidoSelect* cont = malloc(sizeof(contenidoSelect));
	cont->nombreTabla = strdup(nombre_tabla);
	cont->key = key;
	resParser.contenido=cont;

	resultado res = mandarALFS(resParser);
	registro=(Registro*)res.contenido;

	free(cont->nombreTabla);
	free(cont);
	return registro;

}


resultado mandarALFS(resultadoParser resParser){

	int size_to_send;

	retardarLfs();
	char* pi = serializarPaquete(&resParser, &size_to_send);
	pthread_mutex_lock(&mConexion);

	send(serverSocket, pi, size_to_send, 0);
	resultado res = recibir();
	pthread_mutex_unlock(&mConexion);

	free(pi);

	return res;
}

resultado recibir(){

	resultado res;
	accion acc;
	char* buffer = malloc(sizeof(int));
	int valueResponse = recv(serverSocket, buffer, sizeof(int), 0);
	memcpy(&acc, buffer, sizeof(int));

	if(valueResponse < 0) {
		res.resultado=ERROR;
		log_error(g_logger,"Error al recibir los datos");
	} else {

		res.accionEjecutar = acc;
		int status = recibirYDeserializarRespuesta(serverSocket, &res);
		if(status<0) {
			res.contenido=NULL;
			log_error(g_logger,"Error al recibir");
		} else if(res.resultado != OK) {
			res.contenido=NULL;
			log_info(g_logger,res.mensaje);
		}
	}

	free(buffer);
	return res;
}

void retardarMem(){
	pthread_mutex_lock(&mRMem);
	usleep(retardoMemoria*1000);
	pthread_mutex_unlock(&mRMem);

}

void retardarLfs(){
	pthread_mutex_lock(&mRLfs);
	usleep(retardoLFS*1000);
	pthread_mutex_unlock(&mRLfs);
}

resultado select_t(char *nombre_tabla, int key){
	Pagina* pagina;

	resultado res;
	res.contenido=NULL;


	Registro* registro;

	if(contieneRegistro(nombre_tabla,key,&pagina)){

		int posicion=(pagina->indice_registro)*offset;

		retardarMem();

		registro = malloc(sizeof(Registro));

		pthread_mutex_lock(&mMemPrincipal);
		registro->value = strdup(&memoria[posicion]);
		memcpy(&registro->key,(&memoria[posicion+tamValue]),sizeof(uint16_t));
		memcpy(&registro->timestamp,(&memoria[posicion+tamValue+sizeof(uint16_t)]),sizeof(uint64_t));
		pthread_mutex_unlock(&mMemPrincipal);

		res.mensaje= strdup(registro->value);
		res.resultado=OK;

		actualizarTablaGlobal(pagina->numero_pagina);
	}
	else{
		log_warning(g_logger,"No encontre el registro, voy a hablar con el LFS");	//Tengo que pedirselo al LFS y agregarlo en la pagina

		registro = pedirAlLFS(nombre_tabla,key);	//mejor pasar un Segmento

		if(registro==NULL){
			char* aux = "No se encontro el registro";
			res.resultado=OK;
			res.mensaje= string_duplicate(aux);
			return res;
		}

		pthread_mutex_lock(&mBitmap);
		int posLibre= espacioLibre();
		pthread_mutex_unlock(&mBitmap);

		if(posLibre>=0){
			almacenarRegistro(nombre_tabla,*registro,posLibre);
			res.resultado=OK;
		}
		else
			res = iniciarReemplazo(nombre_tabla,*registro,0);

		if(res.resultado==OK)
			res.mensaje= string_duplicate((registro)->value);


	}
	res.contenido = registro;

	res.accionEjecutar = SELECT;
	return res;
}

int espacioLibre(){

	for(int posicion=0;posicion<cantidadFrames;posicion++){
		if(bitmap[posicion]==0){
			return posicion;
		}
	}

	return -1;

}

void almacenarRegistro(char *nombre_tabla,Registro registro, int posLibre){
	Segmento *segmento;
	if(!encuentraSegmento(nombre_tabla,&segmento))
		segmento = agregarSegmento(nombre_tabla);

	agregarPagina(registro, segmento, posLibre, 0); //le paso cero como valorFlag porque solo la usamos en el select esta funcion
}

Segmento *agregarSegmento(char *nombre_tabla){
	//creo segmento con el ntabla
	Segmento* segmento=(Segmento *)malloc(sizeof(Segmento));
	segmento->nombre_tabla = malloc(strlen(nombre_tabla)+1);
	pthread_mutex_init(&(segmento->mTablaPaginas),NULL);

	pthread_mutex_lock(&mTabSeg);
	segmento->numero_segmento=tabla_segmentos->elements_count;
	pthread_mutex_unlock(&mTabSeg);

	segmento->puntero_tpaginas=list_create();

//	segmento->nombre_tabla=nombre_tabla;
	strcpy(segmento->nombre_tabla,nombre_tabla);

	pthread_mutex_lock(&mTabSeg);
	list_add(tabla_segmentos,segmento);
	pthread_mutex_unlock(&mTabSeg);

	return segmento;
}


void agregarPagina(Registro registro, Segmento *segmento, int posLibre, int valorFlag){
	Pagina* pagina=malloc(sizeof(Pagina));
	guardarEnMemoria(registro, posLibre);

	pagina->indice_registro=posLibre;

	pthread_mutex_lock(&mTabPagGlobal);
	pagina->numero_pagina=tabla_paginas_global->elements_count;
	pthread_mutex_unlock(&mTabPagGlobal);

	pagina->flag_modificado=valorFlag;

	pthread_mutex_lock(&(segmento->mTablaPaginas));
	list_add(segmento->puntero_tpaginas, pagina);
	pthread_mutex_unlock(&(segmento->mTablaPaginas));


	NodoTablaPaginas* nodo=malloc(sizeof(NodoTablaPaginas));
	nodo->pagina=pagina;
	nodo->segmento=segmento;

	pthread_mutex_lock(&mTabPagGlobal);
	list_add(tabla_paginas_global,nodo);
	pthread_mutex_unlock(&mTabPagGlobal);

}

resultado iniciarReemplazo(char *nombre_tabla,Registro registro, int flagModificado){
	pthread_mutex_lock(&mTabPagGlobal);
	NodoTablaPaginas* nodoPagina = paginaMenosUsada();
	pthread_mutex_unlock(&mTabPagGlobal);

	log_info(g_logger,"Inicio reemplazo");

	resultado res;
	if(nodoPagina==NULL){
		res.resultado = FULL;
		char *aux = "Memoria full";
		res.mensaje=strdup(aux);
	}

	else{
		Segmento *segmento;
		if(!encuentraSegmento(nombre_tabla,&segmento))
			segmento = agregarSegmento(nombre_tabla);

		removerPagina(nodoPagina);

		pthread_mutex_lock(&(segmento->mTablaPaginas));
		list_add(segmento->puntero_tpaginas,nodoPagina->pagina);
		pthread_mutex_unlock(&(segmento->mTablaPaginas));
		nodoPagina->segmento=segmento;

		pthread_mutex_lock(&mTabPagGlobal);
		list_add(tabla_paginas_global,nodoPagina);
		pthread_mutex_unlock(&mTabPagGlobal);

		nodoPagina->pagina->flag_modificado=flagModificado;
		int indice = nodoPagina->pagina->indice_registro;
		guardarEnMemoria(registro,indice);
		res.resultado=OK;
	}
	return res;
}

void consola(){
	char* mensaje;
	resultado res;
	res.resultado= OK;

	while(res.resultado != SALIR)
	{
		mensaje = readline(">");
		if(mensaje)
			add_history(mensaje);

		resultadoParser resParser = parseConsole(mensaje);
		if(estaHaciendoJournal){
			res.resultado=EnJOURNAL;
			res.mensaje = NULL;
		}
		else{
			pthread_mutex_lock(&mJournal);
			res = parsear_mensaje(&resParser);
			pthread_mutex_unlock(&mJournal);

		}
		if(res.resultado == OK)
		{
			log_info(g_logger,res.mensaje);
		}
		else if(res.resultado == ERROR)
		{
			log_error(g_logger,"Ocurrio un error al ejecutar la acción: %s",res.mensaje);
		}
		else if(res.resultado == MENSAJE_MAL_FORMATEADO)
		{
			log_warning(g_logger,"Mensaje incorrecto");
		}
		else if(res.resultado == EnJOURNAL)
		{
			log_warning(g_logger,"Se esta haciendo Journaling, ingrese la request mas tarde");
		}
		else if(res.resultado == SALIR)
		{
			ejecutando = false;
		}
		if(res.mensaje!=NULL)
			free(res.mensaje);
		free(mensaje);
	}
	ejecutando = false;
}

void removerPagina(NodoTablaPaginas *nodo){
	bool numeroPagIgual(void* element){
		return nodo->pagina->numero_pagina==((Pagina*)element)->numero_pagina;
	}
	pthread_mutex_lock(&(nodo->segmento->mTablaPaginas));
	list_remove_by_condition(nodo->segmento->puntero_tpaginas,numeroPagIgual);
	pthread_mutex_unlock(&(nodo->segmento->mTablaPaginas));
}

NodoTablaPaginas* paginaMenosUsada(){

	return list_remove_by_condition(tabla_paginas_global,noEstaModificada);
}

bool noEstaModificada(void *element){
	return (((NodoTablaPaginas *)element)->pagina->flag_modificado)==0;
}

bool estaModificada(void *element){
	return (((NodoTablaPaginas *)element)->pagina->flag_modificado)==1;
}

bool memoriaFull(){
	return list_any_satisfy(tabla_paginas_global,noEstaModificada);
}

void journal(){

//	pthread_mutex_lock(&mJournal);
//	estaHaciendoJournal=true;
//	pthread_mutex_unlock(&mJournal);
	pthread_mutex_lock(&mJournal);
	pthread_mutex_lock(&mConexion);
	pthread_mutex_lock(&mTabPagGlobal);
	log_warning(g_logger,"Journaling, por favor espere");
	list_iterate(tabla_paginas_global,enviarInsert);
	pthread_mutex_unlock(&mTabPagGlobal);
	pthread_mutex_unlock(&mConexion);


	pthread_mutex_lock(&mTabSeg);
	list_clean_and_destroy_elements(tabla_segmentos,liberarSegmento);
	pthread_mutex_unlock(&mTabSeg);

//	pthread_mutex_lock(&mJournal);
//	estaHaciendoJournal=false;
//	pthread_mutex_unlock(&mJournal);
	log_info(g_logger,"Termino el journal, puede ingresar sus request");
	pthread_mutex_unlock(&mJournal);
}

void enviarInsert(void *element){ //ver los casos de error


	if(estaModificada(element)){
		int indice=((NodoTablaPaginas*)element)->pagina->indice_registro;
		int size_to_send;

		resultadoParser resParser;
		resParser.accionEjecutar=INSERT;

		contenidoInsert* cont = malloc(sizeof(contenidoInsert));
		cont->nombreTabla = malloc(strlen(((NodoTablaPaginas*)element)->segmento->nombre_tabla)+1);
		strcpy(cont->nombreTabla,((NodoTablaPaginas*)element)->segmento->nombre_tabla);

		retardarMem();

		pthread_mutex_lock(&mMemPrincipal);
		memcpy(&cont->key,&(memoria[(indice*offset)+tamValue]),sizeof(uint16_t));
		cont->value = strdup(&memoria[indice*offset]);
		memcpy(&cont->timestamp,&(memoria[(indice*offset)+tamValue+sizeof(uint16_t)]),sizeof(cont->timestamp));
		pthread_mutex_unlock(&mMemPrincipal);

		resParser.contenido=cont;

		char* pi = serializarPaquete(&resParser, &size_to_send);
		send(serverSocket, pi, size_to_send, 0);
		free(cont->nombreTabla);
		free(cont->value);
		free(cont);

		pthread_mutex_lock(&(((NodoTablaPaginas*)element)->segmento->mTablaPaginas));
		((NodoTablaPaginas*)element)->pagina->flag_modificado = 0;
		pthread_mutex_unlock(&(((NodoTablaPaginas*)element)->segmento->mTablaPaginas));

//		resultado res = recibir();
//		free(res.mensaje);
//		if(res.contenido!=NULL)
//			free(res.contenido);


	}
	else{
		return;
	}
}

//void cambiarNumerosPaginas(t_list* listaPaginas){
//	for(int i=0;i<listaPaginas->elements_count;i++){
//
//		Pagina *aux = list_get(listaPaginas,i);
//		aux->numero_pagina = i;
//
//		list_replace(listaPaginas,i,aux);
//	}
//}



void guardarEnMemoria(Registro registro, int posLibre){

	retardarMem();

	pthread_mutex_lock(&mMemPrincipal);
	memcpy(&memoria[(posLibre*offset)],registro.value,strlen(registro.value)+1);
	memcpy(&memoria[(posLibre*offset)+tamValue],&(registro.key),sizeof(uint16_t));
	memcpy(&memoria[(posLibre*offset)+tamValue+sizeof(uint16_t)],&(registro.timestamp),sizeof(uint64_t));
	pthread_mutex_unlock(&mMemPrincipal);

	pthread_mutex_lock(&mBitmap);
	bitmap[posLibre]=1;
	pthread_mutex_unlock(&mBitmap);
}

int contieneRegistro(char *nombre_tabla,uint16_t key, Pagina** pagina){
	Segmento* segmento;

	if(encuentraSegmento(nombre_tabla,&segmento)){
		if(encuentraPagina(segmento,key,pagina))
			return true;
	}
	return false;
}

bool encuentraSegmento(char *ntabla,Segmento **segmento){ 	//Me dice si ya existe un segmento de esa tabla y lo mete en la variable segmento, si no NULL
	bool tieneTabla(void *elemento){
		return strcmp(((Segmento *)elemento)->nombre_tabla, ntabla)==0;
	}

	Segmento* s;

	pthread_mutex_lock(&mTabSeg);
	if (list_is_empty(tabla_segmentos)){
		pthread_mutex_unlock(&mTabSeg);
		return false;
	}

	else{
		pthread_mutex_unlock(&mTabSeg);

		pthread_mutex_lock(&mTabSeg);
		s=list_find(tabla_segmentos,tieneTabla);
		pthread_mutex_unlock(&mTabSeg);

		if(s==NULL){
			return false;
		}
		else{
			memcpy(segmento,&s,sizeof(Segmento*));
			return true;

		}
	}

}

bool encuentraPagina(Segmento* segmento,uint16_t key, Pagina** pagina){

	bool tieneKey(void *elemento){

		int posicion=(((Pagina *)elemento)->indice_registro)*offset;
		int i=0;
		retardarMem();

		pthread_mutex_lock(&mMemPrincipal);
		memcpy(&i,&(memoria[posicion+tamValue]),sizeof(uint16_t));
		pthread_mutex_unlock(&mMemPrincipal);

		return i==key;
	}

	pthread_mutex_lock(&(segmento->mTablaPaginas));
	Pagina* paginaAux = list_find(segmento->puntero_tpaginas,tieneKey);
	pthread_mutex_unlock(&(segmento->mTablaPaginas));
//	memcpy(paginaAux,,sizeof(Pagina));

	if(paginaAux==NULL){
		return false;
	}

	memcpy(pagina,&paginaAux,sizeof(Pagina*));

	return true;
}

resultado insert(char *nombre_tabla,uint16_t key,char *value,uint64_t timestamp){
	Segmento* segmento;

	Pagina* pagina;

	Registro registro;
	if(timestamp == 0){
		struct timeval te;
		gettimeofday(&te, NULL);
		registro.timestamp = te.tv_sec*1000LL + te.tv_usec/1000;
	}else{
		registro.timestamp=timestamp;
	}
	registro.key=key;
	registro.value=value;

	pthread_mutex_lock(&mBitmap);
	int posLibre=espacioLibre();
	pthread_mutex_unlock(&mBitmap);

	resultado res;

	if(encuentraSegmento(nombre_tabla,&segmento)){

		if(encuentraPagina(segmento,key,&pagina)){	//en vez de basura(char *) pasarle una pagina
			pthread_mutex_lock(&(segmento->mTablaPaginas));
			actualizarRegistro(pagina,value,timestamp);
			pthread_mutex_unlock(&(segmento->mTablaPaginas));

			actualizarTablaGlobal(pagina->numero_pagina);
			res.resultado=OK;
		}
		else{
			if(posLibre>=0){
				agregarPagina(registro,segmento,posLibre,1);
				res.resultado=OK;
			}
			else
				res = iniciarReemplazo(nombre_tabla, registro, 1);
			}
	}
	else{

		if(posLibre>=0){
			segmento=agregarSegmento(nombre_tabla);
			agregarPagina(registro,segmento,posLibre,1);
			res.resultado=OK;
		}
		else
			res = iniciarReemplazo(nombre_tabla, registro, 1);
		}

	if(res.resultado==OK){
		char *aux = "Registro insertado exitosamente";
		res.mensaje=strdup(aux);
	}

	res.accionEjecutar=INSERT;
	res.contenido=NULL;

	return res;
}

void liberarSegmento(void* elemento){

	Segmento* segmento = (Segmento*) elemento;

	pthread_mutex_lock(&(segmento->mTablaPaginas));
	list_destroy_and_destroy_elements(segmento->puntero_tpaginas,liberarPagina);
	pthread_mutex_unlock(&(segmento->mTablaPaginas));

//	pthread_mutex_destroy(&(segmento->mTablaPaginas));
	free(segmento);
}

void liberarPagina(void* elemento){
	Pagina* pagina = (Pagina*) elemento;

	bool mismaPagina(void* elem){
		return ((NodoTablaPaginas* )elem)->pagina == pagina;
	}

	pthread_mutex_lock(&mTabPagGlobal);
	list_remove_and_destroy_by_condition(tabla_paginas_global,mismaPagina,destroy_nodo_pagina_global); //remuevo de la global
	pthread_mutex_unlock(&mTabPagGlobal);

	pthread_mutex_lock(&mBitmap);
	bitmap[pagina->indice_registro]=0;
	pthread_mutex_unlock(&mBitmap);

	free(pagina);
}

void corregirIndicesTablaSegmentos(){
	pthread_mutex_lock(&mTabSeg);
	for(int i=0;i<tabla_segmentos->elements_count;i++){

		Segmento *aux = list_get(tabla_segmentos,i);
		aux->numero_segmento = i;

	}
	pthread_mutex_unlock(&mTabSeg);

}

void corregirIndicesPaginasGlobal(){
	for(int i=0;i<tabla_paginas_global->elements_count;i++){

		NodoTablaPaginas *aux = list_get(tabla_paginas_global,i);
		aux->pagina->numero_pagina = i;

	}
}

void drop(char* nombre_tabla){

	Segmento* segmento;

	if(encuentraSegmento(nombre_tabla,&segmento)){

		pthread_mutex_lock(&mTabSeg);
		list_remove_and_destroy_element(tabla_segmentos,segmento->numero_segmento,liberarSegmento);
		pthread_mutex_unlock(&mTabSeg);

		corregirIndicesTablaSegmentos();

		pthread_mutex_lock(&mTabPagGlobal);
		corregirIndicesPaginasGlobal();
		pthread_mutex_unlock(&mTabPagGlobal);

		log_info(g_logger, "Se libero el segmento de la tabla %s en memoria",nombre_tabla);
	}
	else{
		log_info(g_logger, "No se encontro el segmento de la tabla %s en memoria",nombre_tabla);
	}
}


void actualizarRegistro(Pagina *pagina,char *value,uint64_t timestamp){

	int posicion=(pagina->indice_registro)*offset;
	retardarMem();

	uint64_t ts;

	pthread_mutex_lock(&mMemPrincipal);
	memcpy(&ts,&(memoria[posicion+tamValue+sizeof(uint16_t)]),sizeof(uint64_t));
	pthread_mutex_unlock(&mMemPrincipal);

	if(timestamp>=ts){
		pthread_mutex_lock(&mMemPrincipal);
		memcpy(&(memoria[posicion]),value,tamValue);
		memcpy(&(memoria[posicion+tamValue+sizeof(uint16_t)]),&timestamp,sizeof(uint64_t));
		pthread_mutex_unlock(&mMemPrincipal);

		pagina->flag_modificado=1;
	}
}

void terminar_programa()
{

	close(conexion_servidor);

	pthread_cancel(threadJournal);
	pthread_cancel(threadGossiping);


	//Destruyo el logger
	log_destroy(g_logger);

	//Destruyo la tabla de segmentos
	pthread_mutex_lock(&mTabSeg);
	list_destroy_and_destroy_elements(tabla_segmentos, destroy_nodo_segmento);
	pthread_mutex_unlock(&mTabSeg);


	//Liberar memoria
	pthread_mutex_lock(&mMemPrincipal);
	free(memoria);
	pthread_mutex_unlock(&mMemPrincipal);

	//Liberar bitmap
	pthread_mutex_lock(&mBitmap);
	free(bitmap);
	pthread_mutex_unlock(&mBitmap);

	//cierro el servidor
	close(serverSocket);

	//Destruyo tabla de paginas global
	list_destroy_and_destroy_elements(tabla_paginas_global,destroy_nodo_pagina_global);

	//Destruyo la lista de memorias seeds
	list_destroy_and_destroy_elements(memoriasSeeds, destroy_nodo_memoria);

	//Destruyo la lista de memorias conocidas
	pthread_mutex_lock(&mMemoriasConocidas);
	list_destroy_and_destroy_elements(memoriasConocidas, destroy_nodo_memoria_conocida);
	pthread_mutex_unlock(&mMemoriasConocidas);

	//Destruyo las configs
	config_destroy(g_config);

	destruirMutexs();
}

void destruirMutexs(){
	pthread_mutex_destroy(&mMemPrincipal);
	pthread_mutex_destroy(&mTabSeg);
	pthread_mutex_destroy(&mTabPagGlobal);
	pthread_mutex_destroy(&mMemoriasConocidas);
	pthread_mutex_destroy(&mBitmap);
	pthread_mutex_destroy(&mJournal);

}

bool gestionarConexionALFS()
{
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(config_get_string_value(g_config, "IP_LFS"), config_get_string_value(g_config, "PUERTO_LFS"), &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion


	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int status = connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

	if((serverSocket==-1) || (status ==-1))
		return false;
	return true;
}

void iniciar_tablas(){
	pthread_mutex_lock(&mTabSeg);
	tabla_segmentos = list_create();
	pthread_mutex_unlock(&mTabSeg);
	tabla_paginas_global = list_create();
}

void destroy_nodo_pagina(void * elem){
	Pagina* nodo_tabla_elem = (Pagina *) elem;
	free(nodo_tabla_elem);
}


void destroy_nodo_segmento(void * elem){
	Segmento* nodo_tabla_elem = (Segmento *) elem;
	pthread_mutex_lock(&(nodo_tabla_elem->mTablaPaginas));
	list_destroy_and_destroy_elements(nodo_tabla_elem->puntero_tpaginas,destroy_nodo_pagina);
	pthread_mutex_unlock(&(nodo_tabla_elem->mTablaPaginas));

	free(nodo_tabla_elem);
}

void destroy_nodo_pagina_global(void * elem){
	NodoTablaPaginas* nodo = (NodoTablaPaginas *) elem;
	free(nodo);
}

resultado parsear_mensaje(resultadoParser* resParser)
{
	resultado res;
	switch(resParser->accionEjecutar){
		case SELECT:
		{
			contenidoSelect* contSel;
			contSel = (contenidoSelect*)resParser->contenido;
			res = select_t(contSel->nombreTabla,contSel->key);
			break;
		}
		case DESCRIBE:
		{
			res = mandarALFS(*resParser);
			log_info(g_logger,"Se envió al LFS");
			break;
		}
		case INSERT:
		{
			contenidoInsert* contenido = resParser->contenido;
			if(strlen(contenido->value)<=tamValue){
				res = insert(contenido->nombreTabla,contenido->key,contenido->value,contenido->timestamp);
			}
			else{
				res.accionEjecutar=INSERT;
				res.contenido=NULL;
				char *aux = "Tamaño value mayor al permitido.";
				res.mensaje=strdup(aux);
				res.resultado=ERROR;
			}
			break;
		}
		case JOURNAL:
		{
			pthread_mutex_unlock(&mJournal);
			journal();
			pthread_mutex_lock(&mJournal);

			res.accionEjecutar = JOURNAL;
			res.resultado = OK;
			res.contenido = NULL;
			char *aux = "Se realizo JOURNAL";
			res.mensaje=strdup(aux);
			break;
		}
		case CREATE:
		{
			res = mandarALFS(*resParser);
			log_info(g_logger,"Se envió al LFS");
			break;
		}
		case DROP:
		{
			contenidoDrop* contDrop = resParser->contenido;
			drop(contDrop->nombreTabla);
			res = mandarALFS(*resParser);
			break;
		}
		case DUMP:
		{
			res = mandarALFS(*resParser);
			log_info(g_logger,"Se envió al LFS");
			break;
		}
		case GOSSIPING:
		{
			mandarTabla(*((int*)(resParser->contenido)));
			res.resultado = ENVIADO;
			res.mensaje = NULL;
			log_info(g_logger,"Se retorno tabla gossiping");
			break;
		}
		case ERROR_PARSER:
		{
			res.resultado = MENSAJE_MAL_FORMATEADO;
			res.mensaje = NULL;
			resParser->contenido = NULL;
			break;
		}
		case SALIR_CONSOLA:
		{
			resParser->contenido = malloc(0);
			res.resultado = SALIR;
			res.mensaje = NULL;
			break;
		}
		default:
		{
			res.resultado = SALIR;
			res.mensaje = NULL;
			break;
		}

	}

	if(resParser->contenido!=NULL)
		free(resParser->contenido);
	return res;

}

