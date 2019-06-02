/*
 * parser.h
 *
 *  Created on: 29 abr. 2019
 *      Author: utnso
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "string.h"

typedef enum
{
	SELECT,
	SALIR_CONSOLA,
	DESCRIBE,
	INSERT,
	JOURNAL,
	CREATE,
	DROP,
	DUMP,
	RUN,
	ERROR_PARSER
}accion;

typedef struct
{
	char* nombreTabla;
	int key;
	char* value;
	long timestamp;
} contenidoInsert;

typedef struct
{
	char* nombreTabla;
	int key;
} contenidoSelect;

typedef struct
{
	char * nombreTabla;
	char * consistencia;
	int cant_part;
	int tiempo_compresion;
} contenidoCreate;

typedef struct
{
	char* nombreTabla;
} contenidoDescribe;

typedef struct
{
	char* nombreTabla;
} contenidoDrop;

typedef struct
{
	char* path;
} contenidoRun;

typedef struct
{
	accion accionEjecutar;
	void* contenido;
} resultadoParser;

resultadoParser parseConsole(char*);


#endif /* PARSER_H_ */
