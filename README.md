# ChompChamps

## Requisitos

Para correr la aplicación, es necesario tener **Docker** instalado y utilizar la siguiente imagen oficial del entorno de ITBA:

- **Imagen Docker**: `agodio/itba-so-multi-platform:3.0`

## Cómo correr la aplicación

1. Cloná este repositorio (si no lo hiciste ya):

   `git clone <url-del-repo>`
   
   `cd <nombre-del-repo>`
  
3. Ejecutá el entorno con Docker:

   `./docker-run.sh`

Esto iniciará una terminal dentro del contenedor Docker.

Dentro de la terminal de Docker, dirigite a la carpeta de trabajo:

    `cd /root`
    
Compila utilizando:

    `make`
    
Los ejecutables se generarán en el directorio bin/.

  Uso
El ejecutable principal es master, que lanza los procesos de jugadores y de vista.

    `./bin/master -p [jugadores...] -v [vista] [...otros-args]`

Parámetros
-p [jugadores...]: Lista de ejecutables de jugadores (por ejemplo: ./players/player1 ./players/player2)

-v [vista]: Ejecutable para la vista del juego (por ejemplo: ./view/view)

...otros-args: Argumentos opcionales para configuración del juego (por ejemplo: -h 10 -w 20 -s 42)

