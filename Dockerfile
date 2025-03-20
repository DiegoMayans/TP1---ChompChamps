# Usar la imagen base existente
FROM agodio/itba-so-multi-platform:3.0

# Copiar el binario desde el host al contenedor
# Suponiendo que el binario está en el directorio actual
COPY ./ChompChamps /home/user/ChompChamps

# Establecer el directorio de trabajo
WORKDIR /home/user

# Dar permisos de ejecución al binario (si no tiene permisos)
RUN chmod +x /home/user/ChompChamps

# Ejecutar el binario cuando el contenedor inicie
CMD ["/home/user/ChompChamps"]