#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

#define A 186
#define B 0x21B0A8FD
#define C malloc
#define D 125
#define E 87
#define F block
#define G 58
#define H random
#define I 90
#define J max
#define K flock

char max;
const int files = 3;
int loop = 1;

typedef struct {
    char* data_region;
    size_t size;
    FILE* file;
}thread_data_region;

typedef struct {
    char* data_region;
    int files;
}writer_data_thread;

typedef struct {
    int number_thread;
}reader_data_thread;


void* write_data(void* data){
    thread_data_region* cur_data = (thread_data_region*) data;
    char* block = malloc(cur_data->size * sizeof(char));
    fread(block, 1, cur_data->size, cur_data->file);
    for (size_t i = 0; i < cur_data->size; i ++)
        cur_data->data_region[i] = block[i];
    free(block);
    pthread_exit(0);
}


void fill_data_region(char* data_region) {
    char* address_offset = data_region;
    thread_data_region informations[D];
    pthread_t threads[D];
    size_t size_for_thread = A * 1024 * 1024 / D;
    FILE* random_file = fopen("/dev/urandom", "rb");

    for (int i = 0; i < D; i ++) {
        informations[i].data_region = address_offset;
        informations[i].size = size_for_thread;
        informations[i].file = random_file;
        address_offset += size_for_thread;
    }

    informations[D - 1].size += A * 1024 * 1024 % D;

    for (int i = 0; i < D; i ++)
        pthread_create(&threads[i], NULL, write_data, &informations[i]);

    for (int i = 0; i < D; i ++)
        pthread_join(threads[i], NULL);

    fclose(random_file);
}


void* generate_data(void* data) {
    char* cur_data = (char*) data;
    while(loop == 1){
        fill_data_region(cur_data);
    }
    pthread_exit(0);
}


void write_file(writer_data_thread* data, int idFile) {
    char* defaultname = malloc(sizeof(char) * 5);
    snprintf(defaultname, sizeof(defaultname) + 1, "%d-out",++idFile);
    FILE*  file = fopen(defaultname, "wb");

    struct flock write_lock;
    memset(&write_lock, 0, sizeof(write_lock));
    write_lock.l_type = F_RDLCK;
    fcntl(file, F_SETLKW, &write_lock);

    size_t file_size = E * 1024 * 1024;
    size_t rest = 0;

    while (rest < file_size) {
        long size = file_size - rest >= G ? G : file_size - rest;
        rest += fwrite(data->data_region + (rest % (A * 1024 * 1024)), 1, size, file);
    }

    write_lock.l_type = F_UNLCK;
    fcntl(file, F_SETLKW, &write_lock);
}


void* write_files(void* data) {
    writer_data_thread* cur_data = (writer_data_thread*) data;
    while (loop == 1) {
        write_file(cur_data, cur_data->files);
    }
}


void read_file(int id_thread, int idFile) {
    char* defaultname = malloc(sizeof(char) * 5);
    snprintf(defaultname, sizeof(defaultname) + 1, "%d-out",++idFile);
    FILE*  file = fopen(defaultname, defaultname);
    int max = 0;
    int flag = 1;
    char block[G];

    struct flock read_lock;
    memset(&read_lock, 0, sizeof(read_lock));
    read_lock.l_type = F_RDLCK;
    fcntl(file, F_SETLKW, &read_lock);

    fread(&block, 1 , G, file);
    size_t parts = E * 1024 * 1024 / G;

    for (size_t part = 0; part < parts; part ++) {
        int max_block = 0;
        int flag_block = 1;
        for (size_t i = 0; i < sizeof(block); i += 4){
            unsigned int num = 0;
            for (int j = 0; j < 4; j ++) {
                num = (num<<8) + block[i + j];
            }
            if (flag_block == 1) {
                flag_block = 0;
                max_block = num;
            }
            else
                if (max_block > num)
                    max_block = num;

        }
        if (flag == 1) {
            flag = 0;
            max = max_block;
        }
        else
            if (max > max_block)
                max = max_block;

    }

    read_lock.l_type = F_UNLCK;
    fcntl(file, F_SETLKW, &read_lock);
    fclose(file);
}


void* read_files(void* data) {
    reader_data_thread* cur_data = (reader_data_thread*) data;
    while (loop) {
        for (int i = 0; i < files; i ++)
            read_file(cur_data->number_thread, i);
    }
}

int main() {
    printf("Before allocation\n");
    getchar();

    char* data_region = malloc(A);
    printf("After allocation, enter something\n");
    getchar();

    fill_data_region(data_region);
    printf("After fill data\n");
    getchar();

    free(data_region);
    printf("After deallocation\n");
    getchar();

    pthread_t generate_thread;
    data_region = malloc(A * 1024 * 1024);
    pthread_create(&generate_thread, NULL, generate_data, data_region);

    pthread_t writer_thread[files];
    writer_data_thread writer_information[files];
    for (int  i = 0; i < files; i++) {
        writer_information[i].files = i;
        writer_information[i].data_region = data_region;
    }
    for (int i = 0; i < files; i ++)
        pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);

    pthread_t reader_thread[I];
    reader_data_thread reader_information[I];
    for (int i = 0; i < I; i++) {
        reader_information[i].number_thread = i + 1;
        pthread_create(&reader_thread[i], NULL, read_files, &reader_information[i]);
    }

    printf("Enter to continue");
    getchar();
    printf("End");
    getchar();
    loop = 0;

    for (int i = 0; i < I; i ++)
        pthread_join(reader_thread[i], NULL);

    for (int i = 0; i < files; i ++)
        pthread_join(writer_thread[i], NULL);

    pthread_join(generate_thread, NULL);
    free(data_region);
    data_region = NULL;

    return 0;
}


