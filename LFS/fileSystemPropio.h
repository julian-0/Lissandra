/*
 * fileSystem.h
 *
 *  Created on: 28 abr. 2019
 *      Author: utnso
 */

#ifndef FILESYSTEMPROPIO_H_
#define FILESYSTEMPROPIO_H_

#include <commons/bitarray.h>
#include "LFS.h"

typedef struct{
	int size;
	char** bloques;
	char name[255];
}fs_file;

//Funciones del FS propio
fs_file* fs_fopen(char*);
int fs_fcreate(char* ruta);
void fs_fread(fs_file* file,registro* resultado,int position);
int fs_fprint(fs_file* file, registro* obj);
void fs_fclose(fs_file* fs);
void fs_fdelete(fs_file* fs);

//Funcion auxiliar
int fs_printf_registro(fs_file*,registro*);
void fs_fread_registro(fs_file*,registro*,int);

//Funciones del bitmap
void liberarBloque(int bloque);
int obtenerSiguienteBloque();

//Funciones inicializacion
int inicializarFSPropio();

//Serializacion
char* serializarRegistro(registro* reg);
void deserializarRegistro(char* strReg,registro* reg);

pthread_mutex_t semaforo;


#endif /* FILESYSTEM_H_ */
