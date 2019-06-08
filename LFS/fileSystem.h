/*
 * fileSystem.h
 *
 *  Created on: 28 abr. 2019
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "registro.h"
#include "LFS.h"
#include <dirent.h>
#include <sys/types.h>


metadataTabla obtenerMetadata(char* nombreTabla);
int	existeMetadata(char* nombreTabla);
char* obtenerMetadataPath(char* nombreTabla);
char* obtenerTablePath();
registro* fs_select(char* nombreTabla, int key, int partition);
registro* fs_select_partition(char* nombreTabla, int key, int partition);
registro* fs_select_temporal(char* nombreTabla, int key);
registro* obtenerRegistroDeArchivo(FILE* file, int key);
t_list* obtenerTodasMetadata();

#endif /* FILESYSTEM_H_ */
