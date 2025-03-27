#!/bin/bash

# Stop and remove container
docker stop itba-chompchamps
docker rm itba-chompchamps

# Mount current directory into container
docker run -it --name itba-chompchamps \
  -v $(pwd):/root \
  agodio/itba-so-multi-platform:3.0