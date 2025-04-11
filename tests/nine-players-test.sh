valgrind --leak-check=full --show-leak-kinds=all ./bin/master -p ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player ./bin/random_player -v ./bin/view 
# pkill -f ./bin/player
# pkill -f ./bin/view