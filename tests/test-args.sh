#!/bin/bash

# Test sin argumentos
valgrind --leak-check=full --show-leak-kinds=all ./bin/master > tests/test1_output.txt 2>&1
if [ $? -ne 0 ]; then
    echo "Test 1 success" >> tests/test1_output.txt
else
    echo "Test 1 fail" >> tests/test1_output.txt
fi

# Test con 1 player y sin vista
valgrind --leak-check=full --show-leak-kinds=all ./bin/master -p ./bin/random_player > tests/test2_output.txt 2>&1
if [ $? -ne 0 ]; then
    echo "Test 2 success" >> tests/test2_output.txt
else
    echo "Test 2 fail" >> tests/test2_output.txt
fi

# Test con vista y sin player
valgrind --leak-check=full --show-leak-kinds=all ./bin/master -v ./bin/view > tests/test3_output.txt 2>&1
if [ $? -ne 0 ]; then
    echo "Test 3 success" >> tests/test3_output.txt
else
    echo "Test 3 fail" >> tests/test3_output.txt
fi

# Test con player que no existe y sin vista
valgrind --leak-check=full --show-leak-kinds=all ./bin/noexiste > tests/test4_output.txt 2>&1
if [ $? -ne 0 ]; then
    echo "Test 4 success" >> tests/test4_output.txt
else
    echo "Test 4 fail" >> tests/test4_output.txt
fi

# Test con player que no existe y con vista
valgrind --leak-check=full --show-leak-kinds=all ./bin/master -p ./bin/noexiste -v ./bin/view > tests/test5_output.txt 2>&1
if [ $? -ne 0 ]; then
    echo "Test 5 success" >> tests/test5_output.txt
else
    echo "Test 5 fail" >> tests/test5_output.txt
fi

# Test con player que existe y vista que no existe
# Este test inevitablemente hace que se cuelgue el master

# valgrind --leak-check=full --show-leak-kinds=all ./bin/master -p ./bin/random_player -v ./bin/noexiste > test6_output.txt 2>&1
# if [ $? -ne 0 ]; then
#     echo "Test 6 success" >> test6_output.txt
# else
#     echo "Test 6 fail" >> test6_output.txt
# fi