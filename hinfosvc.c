/**
 *@file hinfosvc.c
 *@brief IPK 1. projekt - HTTP server
 *@author Jiří Mládek (xmlade01)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>
#include <math.h>

#define BUFFER_LEN 512
#define LARGER_BUFFER_LEN 1024
#define USER 1
#define NICE 2
#define SYSTEM 3
#define IDLE 4
#define IOWAIT 5
#define IRQ 6
#define SOFTIRQ 7
#define STEAL 8
#define GUEST 9
#define GUEST_NICE 10

/**
 * Funkce pro kontrolu, zda je program spusten se spravnym argumentem
 * @param argc pocet argumentu programu
 * @param argv argumenty programu
 * @param port promenna, do ktere se ulozi cislo portu
 * @returns 0 = OK, 1 = Error
*/
int check_arguments(int argc, char const *argv[], int *port){
    
    if(argc != 2){
        fprintf(stderr, "Wrong format of program arguments!\n");
        return 1;
    }
    //prevod stringu na integer (port)
    char *ptr;
    *port = strtol(argv[1], &ptr, 10);
    if (*port < 0 || *port > 65535 || *ptr != '\0') //kontrola, zda je port ve spravnem rozsahu
    {
        fprintf(stderr, "Port is not valid!\n");
        return 1;
    }
    return 0;
}

/**
 * Funkce, ktera zjisti hostname 
 * @param file promenna, do ktere nactu soubor
 * @param hostname pole, do ktereho ulozim string hostname
 * @returns 0 = OK, 1 = Error when opening file
*/
int GetHostname(FILE *file, char *hostname){

    file = fopen("/proc/sys/kernel/hostname","r");
    if(file == NULL)
    {
        fprintf(stderr, "Error when opening file!");
        return 1;
    }
    fgets(hostname, BUFFER_LEN, file);
    fclose(file);
    return 0;
}

/**
 * Funkce, ktera zjisti cpu-name 
 * @param file promenna, do ktere nactu soubor
 * @param cpuName pole, do ktereho ulozim cpu-name
 * @returns 0 = OK, 1 = Error when opening file
*/
int GetCpuName(FILE *file, char *cpuName){

    //Vycitam informace ze souboru proc/cpuinfo. V radku s "model name" vyberu to, co je za ':', a musim odstranit mezeru za ':'
    file = popen("cat /proc/cpuinfo | grep \"model name\" | cut -d':' -f2 | cut -c 2-", "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error when opening file!");
        return 1;
    }
    fgets(cpuName, BUFFER_LEN, file);
    pclose(file);
    return 0;
}

/**
 * Funkce, ktera zjisti info pro vytizeni procesoru
 * @param file promenna, do ktere nactu soubor
 * @param total promenna pro finalni vypocet vytizeni procesoru
 * @param idle promenna pro finalni vypocet vytizeni procesoru
 * @returns 0 = OK, 1 = Error when opening file
*/
int GetCpuUsage(FILE *file, int *total, int *idle){

    file = fopen("/proc/stat","r");
    if(file == NULL)
    {
        fprintf(stderr, "Error when opening file!");
        return 1;
    }
    char cpuStats[LARGER_BUFFER_LEN];
    fgets(cpuStats, LARGER_BUFFER_LEN, file); //zajima me pouze prvni radek souboru

    //ulozim si do pole 'cpuToken' konkretni ciselne hodnoty z prvniho radku 'cpu' v stringu 'cpuStats'(v poradi: user, nice, system, idle, iowait, irq, softirq, steal, huest, guest_nice)
    int i = 0;
    char *part = strtok (cpuStats, " ");
    char *cpuToken[11];
    while (part != NULL)
    {
        cpuToken[i] = part;
        part = strtok (NULL, " ");
        i++;
    }
    //vypocet potrebnych promennych pro finalni vypocet
    *idle = atoi(cpuToken[IDLE]) + atoi(cpuToken[IOWAIT]);
    int nonIdle = atoi(cpuToken[USER]) + atoi(cpuToken[NICE]) + atoi(cpuToken[SYSTEM]) + atoi(cpuToken[IRQ]) + atoi(cpuToken[SOFTIRQ]) + atoi(cpuToken[STEAL]);
    *total = *idle + nonIdle;

    fclose(file);
    return 0;
}

/**
 * Funkce, ktera procenta vytizeni procesoru
 * @param file promenna, do ktere nactu soubor
 * @param cpuLoad pole, do ktereho ulozim cpuLoad
 * @returns 0 = OK, 1 = Error when opening file
*/
int GetCpuLoad(FILE *file, char *cpuLoad){
    int prevTotal, total, prevIdle, idle, totalResult, idleResult; //pomocne promenne pro vypocet vytizeni procesoru
    double cpuPercentage;

    if (GetCpuUsage(file, &prevTotal, &prevIdle))
        return 1;
    sleep(1); //uspani procesu na 1s, abychom mohli vypocitat vytizeni procesoru 
    if (GetCpuUsage(file, &total, &idle))
        return 1;

    totalResult = total - prevTotal;
    idleResult = idle - prevIdle;
    cpuPercentage = round(((double)(totalResult - idleResult) / (double)totalResult) * 100.0); //vypocet s prevedenim na procenta
    sprintf(cpuLoad, "%.f%%\n", cpuPercentage); //prevedeni cisla na char*
    return 0;
}

//*************************************************************//
//******************  Hlavni cast programu  *******************//
//*************************************************************//
int main(int argc, char const *argv[])
{
    int port;
    if(check_arguments(argc, argv, &port))
        return 1;

    FILE *file = NULL;
    char hostname[BUFFER_LEN];
    char cpuName[BUFFER_LEN];
    char cpuLoad[BUFFER_LEN];

    //********************* SERVER ************************//
    struct sockaddr_in serverAddress;
    int fd, commSocket;
    int addressLen = sizeof(serverAddress);
    int sockOptOptval = 1;

    //vytvoreni socketu
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        fprintf(stderr, "Error when creating socket!\n");
        return 1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sockOptOptval, sizeof(sockOptOptval))) //pro predejiti erroru pri znovuspusteni serveru
    { 
        fprintf(stderr, "Error when setsockopt!\n");
        return 1;
    } 
    
    //nastaveni parametru adresy
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero);
    //nabindovani socketu na pozadovany port
    if (bind(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0)
    {
        fprintf(stderr, "Error when binding!\n");
        return 1;
    }
    if (listen(fd, 10) < 0) 
    {
        fprintf(stderr, "Error when preparing to accept connection!\n");
        return 1;
    }

    //hlavni loop, ve kterem budu prijimat zpravy od 'klienta'
    while(1)
    {
        if ((commSocket = accept(fd, (struct sockaddr *)&serverAddress, (socklen_t*)&addressLen)) < 0)
        {
            fprintf(stderr, "Error when accepting!\n");
            return 1;
        }
        char buffer[20000] = {0};
        if(read(commSocket, buffer, 20000) < 0){
            fprintf(stderr, "Error when read!\n");
            return 1;
        }
        if (!strncmp(buffer, "GET /hostname ", 14)){
            char hostnameMessage[LARGER_BUFFER_LEN] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
            //Zjisteni hostname
            if (GetHostname(file, hostname))
                return 1;
            strcat(hostnameMessage, hostname);
            write(commSocket , hostnameMessage , strlen(hostnameMessage));
        }
        else if (!strncmp(buffer, "GET /cpu-name ", 14)){
            char cpuMessage[LARGER_BUFFER_LEN] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
            //Zjisteni jmena procesoru
            if (GetCpuName(file, cpuName))
                return 1;
            strcat(cpuMessage, cpuName);
            write(commSocket , cpuMessage , strlen(cpuMessage));
        }
        else if (!strncmp(buffer, "GET /load ", 10)){
            char loadMessage[LARGER_BUFFER_LEN] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
            //zjisteni vytizeni procesoru
            if(GetCpuLoad(file, cpuLoad))
                return 1;
            strcat(loadMessage, cpuLoad);
            write(commSocket , loadMessage , strlen(loadMessage));
        }
        else{
            char errorMessage[LARGER_BUFFER_LEN] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\nBad Request\n";
            write(commSocket , errorMessage , strlen(errorMessage));
        }
        close(commSocket);
    }
    return 0;
}
