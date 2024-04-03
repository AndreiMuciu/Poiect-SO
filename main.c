#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<dirent.h>
#include<time.h>
#include<fcntl.h>

void get_file_metadata(const char *filename, const char *output) {
    struct stat fileStat;
    char metadata[512];
    if (stat(filename, &fileStat) < 0) {
        perror("Eroare la obținerea metadatelor");
        exit(EXIT_FAILURE);
    }
    int fd = open(output, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if(fd == -1)
    {
        perror("Eroare la deschiderea outputului\n");
        exit(EXIT_FAILURE);
    }
    snprintf(metadata, sizeof(metadata), "Nume: %s\n", filename);
    write(fd, metadata, strlen(metadata));

    snprintf(metadata, sizeof(metadata), "Dimensiune: %ld bytes\n", fileStat.st_size);
    write(fd, metadata, strlen(metadata));

    snprintf(metadata, sizeof(metadata), "Ultima modificare: %s", ctime(&fileStat.st_mtime));
    write(fd, metadata, strlen(metadata));

    snprintf(metadata, sizeof(metadata), "-------------------------------------\n");
    write(fd, metadata, strlen(metadata));

    close(fd);
}

void traverse_directory(const char *dir_name, const char *output) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(dir_name))) {
        perror("Eroare la deschiderea directorului");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        if (entry->d_type == DT_DIR) {
            traverse_directory(path, output);
        } else {
            get_file_metadata(path, output);
        }
    }

    closedir(dir);
}

void verific_daca_se_repeta(int argc, char *argv[])
{
    int i, j;
    for(i = 1; i < argc; i++)
    {
        for(j = i + 1; j < argc; j++)
        {
            if(strcmp(argv[i], argv[j]) == 0)
            {
                perror("Argumentele date se repeta");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void verific_daca_argumentele_este_director(int argc, char *argv[], const char *output)   //greseala gramaticala din adins provocata
{
    struct stat fileInfo;
    char scriere[128];
    int fd = open(output, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR), i;
    if(fd == -1)
    {
        perror("Eroare la deschiderea outputului\n");
        exit(EXIT_FAILURE);
    }
    for(i = 1; i < argc - 2; i++)
    {
        if (stat(argv[i], &fileInfo) == 0) {
            if (S_ISDIR(fileInfo.st_mode)) {
                snprintf(scriere, sizeof(scriere), "Snapshotul pentru directorul %s:\n", argv[i]);
                write(fd, scriere, strlen(scriere));
                traverse_directory(argv[i], output);
            } 
        }else {
                snprintf(scriere, sizeof(scriere), "Fisierul %s nu este director !!!\n", argv[i]);
                write(fd, scriere, strlen(scriere));
                //printf("fisierul %s nu este director\n", argv[i]);
                //traverse_directory(argv[i]);
        }
    }
}

void verific_daca_exista_output(int argc, char *argv[], char *numeFisier)
{
    if((strcmp(argv[argc - 2], "-o") == 0))
    {
        strcpy(numeFisier, argv[argc - 1]);
    }
    else{
        perror("Forma incorecta data pentru fisierul output");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    char output[100];
    if(argc > 10)
    {
        perror("Numarul de argumente dat este gresit");
        exit(EXIT_FAILURE);
    }
    verific_daca_se_repeta(argc, argv);
    verific_daca_exista_output(argc, argv, output);
    verific_daca_argumentele_este_director(argc, argv, output);
    return 0;
}