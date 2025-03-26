#!/bin/bash

# Stop and remove container
docker stop itba-chompchamps
docker rm itba-chompchamps

# Build image
docker build -t chompchamps-image .

# Remove unused images
docker image prune -f

# Run container
docker run -d --name itba-chompchamps chompchamps-image:latest
