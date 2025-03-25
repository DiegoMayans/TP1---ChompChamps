#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include<includes/defs.h>

//codigo que va para el master
void * createSHM(char * name, size_t size){
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, 0666); //mode
    if(fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    //Solo para crearla
    if (-1 == ftruncate(fd, size)){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p = MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

game_board_t * get_board_state(){
    int fd;
    fd = shm_open("/game_state", O_RDWR, 0666);
    if (fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }



}



