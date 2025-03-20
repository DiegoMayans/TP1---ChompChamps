# TP1---ChompChamps

Para crear la imagen crear el container y abrirlo:

docker build -t chompchamps-image .

docker run --name itba-chompchamps chompchamps-image:latest

Para abrir una terminal en el container
docker exec -it itba-chompchamps /bin/bash
