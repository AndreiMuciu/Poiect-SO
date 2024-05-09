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
#include <sys/wait.h>

typedef struct metadate{
    long int inode;
    char nume[32];
    long int dim;
    char ultmod[128];
}metadate;

int identific_fisiere_periculoase(const char *fileName) {
    struct stat st;
    if (stat(fileName, &st) == -1) {
        perror("Eroare la verificarea permisiunilor fisierului\n");
        exit(EXIT_FAILURE);
    }
    // Verifică dacă fișierul nu are nicio permisiune
    if ((st.st_mode & S_IRWXU) == 0 && (st.st_mode & S_IRWXG) == 0 && (st.st_mode & S_IRWXO) == 0) {
        //printf("Fișierul %s nu are nicio permisiune! Se rulează scriptul...\n", fileName);
        int pfd[2];
        char buf;
        if(pipe(pfd) == -1)
        {
            perror("Eroare la crearea pipeului");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == 0) {
            // Proces copil
            close(pfd[0]);
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[1]);
            const char *args[] = {"./search_keywords.sh", fileName, NULL};
            execvp(args[0], (char* const*)args);
            perror("Execvp error");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Proces părinte
            close(pfd[1]);
            read(pfd[0], &buf, 1);
            close(pfd[0]);
            wait(NULL);
            return buf - 48;
        } else {
            perror("Eroare la crearea procesului din fis periculoase\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void muta_fisierul(const char *sursa, const char *destinatie) {
    if (rename(sursa, destinatie) == 0) {
    } else {
        perror("Eroare la mutarea fisierului");
        exit(EXIT_FAILURE);
    }
}

void actualizareOutputGeneral(const char* basePath, const char *output)
{
    char path[1000];
    char buffer[2048];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    struct stat fileInfo;
    int len;
    if (!dir) {
        perror("Nu s-a putut deschide directorul");
        exit(EXIT_FAILURE);
    }
    // Deschidem fișierul pentru scriere, creăm dacă nu există, și trunchiem dacă există
    int fd = open(output, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului de iesire");
        exit(EXIT_FAILURE);
    }
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
            if (stat(path, &fileInfo) == 0) {
                len = snprintf(buffer, sizeof(buffer), "I-node: %ld\nNume: %s\nDimensiune: %ld bytes\nUltima modificare: %s\n",
                               (long) fileInfo.st_ino, dp->d_name, (long) fileInfo.st_size, ctime(&fileInfo.st_mtime));
                write(fd, buffer, len);
            } else {
                len = snprintf(buffer, sizeof(buffer), "Stat error on %s: \n", path);
                write(fd, buffer, len);
                exit(EXIT_FAILURE);
            }
            if (S_ISDIR(fileInfo.st_mode))
            {
                actualizareOutputGeneral(path, output);
            }
        }
    }
    closedir(dir);
    close(fd);
}
//aici parcurg directorul de 2 ori, odata pentru a verifica daca informatiile din snapshot s au modificat,
// iar daca se observa vreo modificare atunci scriu metadatele noi
int listFiles(const char *basePath, const char *output, const char *izolare, int adv) {
    metadate prev;
    metadate actual;
    char path[2000];
    char buffer[2048];
    struct dirent *dp;
    DIR *dir = opendir(basePath), *dir1 = opendir(izolare);
    struct stat fileInfo;
    int len, fd, rez;
    int nrMalicious = 0;
    if (dir1 == NULL) {
        if(mkdir(izolare, 0777) == 0)
        {
            printf("S-a creat directorul %s\n", izolare);
        }
        else{
            printf("nu s a putut crea un director pt izolate\n");
            exit(EXIT_FAILURE);
        }
    }
    closedir(dir1);
    if (!dir) {
        perror("Nu s-a putut deschide directorul");
        exit(EXIT_FAILURE);
    }
    fd = open(output, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului de iesire");
        exit(EXIT_FAILURE);
    }
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
    close(fd);
    if (bytesRead == -1) {
        perror("Eroare la citirea metadatelor\n");
        exit(EXIT_FAILURE);
    }
    if(adv == 0)
    {
        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
                snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
                if (stat(path, &fileInfo) == 0) {
                    /*if (S_ISDIR(fileInfo.st_mode)) {  // Dacă este director, procesăm recursiv
                        listFiles(path, output, izolare, 0);  // Apel recursiv pentru a procesa subdirectoare
                    }*/
                    //else{
                    rez = identific_fisiere_periculoase(path);
                    if(rez == 1)
                    {
                        char aux[20];
                        strcpy(aux, izolare);
                        strcat(aux,"/");
                        strcat(aux, dp->d_name);
                        //printf("%s\n", aux);
                        muta_fisierul(path, aux);
                        nrMalicious++;
                    }
                    else{
                        actual.inode = (long) fileInfo.st_ino;
                        strcpy(actual.nume, dp->d_name);
                        actual.dim = (long) fileInfo.st_size;
                        strcpy(actual.ultmod, ctime(&fileInfo.st_mtime));
                        sscanf(buffer, "I-node: %ld\nNume: %s\nDimensiune: %ld bytes\nUltima modificare: %s\n", &prev.inode, prev.nume, &prev.dim, prev.ultmod);
                        if((actual.inode != prev.inode) || (strcmp(prev.nume, actual.nume) != 0) || (actual.dim != prev.dim) || (strcmp(actual.ultmod, prev.ultmod) != 0))
                        {
                            adv = 1;
                            break;
                        }
                    }
                    //}
                }
                else{
                    printf("ERR\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(dir);
    if(adv == 1) {
        fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Eroare la re-deschiderea fisierului pentru scriere");
            exit(EXIT_FAILURE);
        }
        dir = opendir(basePath);  // Redeschidem directorul pentru a actualiza fișierul de output
        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
                snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);
                if (stat(path, &fileInfo) == 0) {
                    /*if (S_ISDIR(fileInfo.st_mode)) {  // Dacă este director, procesăm recursiv
                        listFiles(path, output, izolare, 1);  // Apel recursiv pentru a procesa subdirectoare
                    }*/
                    //else{
                    rez = identific_fisiere_periculoase(path);
                    if(rez == 1)
                    {
                        char aux[20];
                        strcpy(aux, izolare);
                        strcat(aux,"/");
                        strcat(aux, dp->d_name);
                        //printf("%s\n", aux);
                        muta_fisierul(path, aux);
                        nrMalicious++;
                    }
                    else{
                        len = snprintf(buffer, sizeof(buffer), "I-node: %ld\nNume: %s\nDimensiune: %ld bytes\nUltima modificare: %s\n",
                                   (long) fileInfo.st_ino, dp->d_name, (long) fileInfo.st_size, ctime(&fileInfo.st_mtime));
                        write(fd, buffer, len);
                    }
                    //}
                }
                else {
                    len = snprintf(buffer, sizeof(buffer), "Stat error on %s: \n", path);
                    write(fd, buffer, len);
                    exit(EXIT_FAILURE);
                }
            }
        }
        closedir(dir);
        close(fd);
    }
    printf("Avem %d fisier/fisiere malitios/malitioase in %s\n", nrMalicious, basePath);
    return adv;
}

void verific_Linia_de_comanda(int argc, char *argv[]) {
    struct stat statbuf, buf;
    int directory_count = 0, numarPanaLa_o = 1, numarPanaLa_x = 1, val, imp = 0, contorND = 0, gasitAcelasiDirector = 0;
    char output[32], individualOutput[32], izolare[32], numeDirectoare[10][30];
    pid_t pid;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-o") == 0) {
            break;
        }
        numarPanaLa_o++;
    }
    if(argc == numarPanaLa_o) {
        printf("Nu ai specificat fisier de output\n");
        exit(EXIT_FAILURE);
    }
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-x") == 0) {
            break;
        }
        numarPanaLa_x++;
    }
    strcpy(output, argv[numarPanaLa_o + 1]);
    strcpy(individualOutput, "Output_");
    strcpy(izolare, argv[numarPanaLa_x + 1]);
    for (int i = 1; i < numarPanaLa_o; i++) {
        if(lstat(argv[i], &buf) == -1)
        {
            perror("eroare la lstat\n");
            exit(EXIT_FAILURE);
        }
        if (stat(argv[i], &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode) && (!S_ISLNK(buf.st_mode))) {  // Verificăm dacă este director
                for(int j = 0; j < contorND; j++)
                {
                    if(strcmp(numeDirectoare[j], argv[i]) == 0)
                    {
                        gasitAcelasiDirector = 1;
                    }
                }
                if(gasitAcelasiDirector != 1)
                {
                    strcpy(numeDirectoare[contorND], argv[i]);
                    contorND++;
                    gasitAcelasiDirector = 0;
                    directory_count++;  
                    if (directory_count > 10) {  // Dacă numărul directoarelor depășește 10
                        printf("Eroare: Nu poti specifica mai mult de 10 directoare.\n");
                        exit(EXIT_FAILURE);
                    }
                    pid = fork();  // Creează un proces nou
                    if (pid == 0) {  // Codul rulat de procesul copil
                        char iNodeChar[32];
                        snprintf(iNodeChar, sizeof(iNodeChar), "%ld", (long)statbuf.st_ino);
                        strcat(individualOutput, iNodeChar);
                        val = listFiles(argv[i], individualOutput, izolare, 0);
                        if(val == 1) {
                            imp = 1;
                        }
                        exit(imp);  // Copilul iese cu valoarea lui `imp`
                    } else if (pid < 0) {
                        perror("eroare la fork in verific linia de comanda\n");
                        exit(EXIT_FAILURE);
                    }
                    // Părintele continuă for-ul, nu așteaptă aici pentru a permite proceselor copii să ruleze în paralel
                    strcpy(individualOutput, "Output_");
                }
            }
        } else {
            printf("Eroare: Nu se poate accesa '%s'. Verifica existenta si permisiunile.\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    // Părintele ar putea aștepta aici pentru toți copiii să termine, dacă este necesar
    for (int i = 0; i < directory_count; i++) {
        int status;
        wait(&status);  // Așteaptă fiecare proces copil să se termine
        if(WIFEXITED(status)) {
            if(WEXITSTATUS(status) == 1) {
                imp = 1;
            }
        }
    }
    if(imp == 1) {
        int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Eroare la re-deschiderea fisierului pentru scriere");
            exit(EXIT_FAILURE);
        }
        close(fd);
        for (int i = 0; i < contorND; i++) {
            if(lstat(numeDirectoare[i], &buf) == -1)
            {
                perror("eroare la lstat\n");
                exit(EXIT_FAILURE);
            }
            if (stat(numeDirectoare[i], &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode) && (!S_ISLNK(buf.st_mode))) {  // Verificăm dacă este director 
                    actualizareOutputGeneral(numeDirectoare[i], output);
                }
            } else {
                printf("Eroare: Nu se poate accesa '%s'. Verifica existenta si permisiunile.\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    verific_Linia_de_comanda(argc, argv);
    return 0;
}
