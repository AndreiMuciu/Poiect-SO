#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<dirent.h>
#include<time.h>

void get_file_metadata(const char *filename) {
    struct stat fileStat;

    if (stat(filename, &fileStat) < 0) {
        perror("Eroare la obținerea metadatelor");
        exit(EXIT_FAILURE);
    }

    printf("Nume: %s\n", filename);
    printf("Dimensiune: %ld bytes\n", fileStat.st_size);
    printf("Ultima modificare: %s", ctime(&fileStat.st_mtime));
    printf("-------------------------------------\n");
}

void traverse_directory(const char *dir_name) {
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
            traverse_directory(path);
        } else {
            get_file_metadata(path);
        }
    }

    closedir(dir);
}

void verific_daca_se_repeta(int argc, char *argv[])
{
    int i, j;
    for(i = 0; i < argc; i++)
    {
        for(j = i + 1; j < argc; j++)
        {
            if(strcmp(argv[i], argv[j]) == 0)
            {
                printf("Argumentele date se repeta");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void verific_daca_argumentele_este_director(int argc, char *argv[])   //greseala gramaticala din adins provocata
{
    
}

int main(int argc, char *argv[])
{
    if(argc < 10)
    {
        perror("Numarul de argumente dat este gresit");
        exit(EXIT_FAILURE);
    }
    verific_daca_se_repeta(argc, argv);
    const char *nume_director = argv[1];
    traverse_directory(nume_director);
    return 0;
}