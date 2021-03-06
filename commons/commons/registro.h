/*
 * registro.h
 *
 *  Created on: 27 abr. 2019
 *      Author: utnso
 */

#ifndef COMMONS_REGISTRO_H_
#define COMMONS_REGISTRO_H_

#include <inttypes.h>
#include "stdint.h"

typedef struct
{
	char* value;
	uint16_t key;
	uint64_t timestamp;
} registro;


void parseRegistro(char*,registro*,int);

#endif /* COMMONS_REGISTRO_H_ */
