#!/bin/bash

# Directorio a formatear (default = directorio actual)
PROJECT_DIR=${1:-.}

# Verificar que clang-format esté instalado
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format no esta instalado. Instalar con sudo apt install clang-format (Linux)"
    exit 1
fi

# Find all .c and .h files in the project directory and format them
echo "Formateando los archivos .c y .h"
find "$PROJECT_DIR" -name "*.c" -o -name "*.h" | while read -r file; do
    echo "Formeateando $file"
    clang-format -i "$file"
done

echo "Formateado Terminado!"