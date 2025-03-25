# Usar la imagen base existente
FROM agodio/itba-so-multi-platform:3.0

# Copiar el binario desde el host al contenedor
# Suponiendo que el binario está en el directorio actual
COPY . /home/user/TP

# Establecer el directorio de trabajo
WORKDIR /home/user

# Dar permisos de ejecución al binario (si no tiene permisos)
RUN chmod +x /home/user/TP

CMD ["tail", "-f", "/dev/null"]