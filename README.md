# tp-2019-1c-creativOS

## Contexto
El proyecto formaba parte de la materia Sistemas Operativos y su objetivo consistía en desarrollar una solución que permita la simulación de ciertos aspectos de una base de datos distribuida, es decir donde los distintos recursos del sistema puedan realizar una carga de trabajo ejecutando desde distintos puntos. 
Sus partes eran filesystem, memorias y kernel


## Objetivos
* Adquirir conceptos prácticos del uso de las distintas herramientas de programación e interfaces (APIs) que brindan los sistemas operativos.
* Entiender aspectos del diseño de un sistema operativo.
* Afirmar diversos conceptos teóricos de la materia mediante la implementación práctica de algunos de ellos.
* familiarizarse con técnicas de programación de sistemas, como el empleo de makefiles, archivos de configuración y archivos de log.
* Conocer con grado de detalle la operatoria de Linux mediante la utilización de un lenguaje de programación de relativamente bajo nivel como C.
  * Threads
  * Procesos


## Enunciado:
https://docs.google.com/document/d/1QlzXwpSvI5ua2lbO8pF6ZgjlgMndFlwzlAci7qhZmqE/edit#

## Pruebas finales:
https://docs.google.com/document/d/1m_V2AXpfo8SpeOr330Rwj3uKIe-GHJ3VNfO38FNNc6Q/edit#

## Pasos previos a levantar los proyectos en Eclipse:
1. Si ya estaban instaladas algunas commons en el sistema, pararse en la carpeta de esas commons y ejecutar `sudo make uninstall`

## Pasos para levantar los proyectos en Eclipse:
1. Clonarse el repo
2. Hacer `cd commons` (del repo) y ejecutar `sudo make install`
3. En el Eclipse ir a `New -> Makefile Project with Existing Code -> Importar Kernel y elegir Linux GCC`
4. En el Eclipse ir a `New -> Makefile Project with Existing Code -> Importar PoolMemorias y elegir Linux GCC`
5. En el Eclipse ir a `New -> Makefile Project with Existing Code -> Importar LFS y elegir Linux GCC`

## Pasos para modificar funciones de las commons:
1. En el Eclipse ir a `New -> Makefile Project with Existing Code -> Importar commons y elegir Linux GCC`
2. Agregar el *archivo.c* y *archivo.h* nuevo o editar los ya existentes
3. Abrir una terminal, ir hasta la carpeta `commons` y ejecutar:
    1. `sudo make uninstall`
    2. `sudo make install`
