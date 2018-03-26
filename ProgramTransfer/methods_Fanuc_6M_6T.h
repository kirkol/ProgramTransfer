#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include <cctype>
#include <vector>

// debbuging (diody na sterowniku)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// debbuging (diody zewnetrzne)
DigitalOut ledCzerwona(p26);
DigitalOut ledZolta(p25);

// Serial dla PC
Serial pc(USBTX, USBRX);

// dla RS232
Serial uart(p13, p14);
DigitalOut RTSwyjscieRS(p22);
DigitalIn CTSwejscieRS(p17);

// internety
EthernetInterface eth;
Ethernet ether;
TCPSocket sock;

// karta SD
SDBlockDevice sd(p5,p6,p7,p9);
FATFileSystem fs("sd", &sd);

// mruga diodami i wyswietla, ze udalo sie nawiazac polaczenie z Socketem
void SocketOK(){
    
    pc.printf("\n SOCKET OPENED \n");
    led1 = 1; wait(0.2); led1 = 0; wait(0.2); led1 = 1; wait(0.2); led1 = 0; wait(0.2); led1 = 1; wait(0.2); led1 = 0;
}

// mruga diodami i wyswietla, ze NIE udalo sie nawiazac polaczenia z Socketem
void SocketFAIL(){

    pc.printf("\n SOCKET IS UNAVAILABLE \n");
    led2 = 1; ledCzerwona = 1; ledZolta = 1; wait(0.5); led2 = 0; ledCzerwona = 0; ledZolta = 0; wait(0.5); 
    led2 = 1; ledCzerwona = 1; ledZolta = 1; wait(0.5); led2 = 0; ledCzerwona = 0; ledZolta = 0; wait(0.5); 
    led2 = 1; ledCzerwona = 1; ledZolta = 1; wait(0.5); led2 = 0; ledCzerwona = 0; ledZolta = 0;
}

// funkcja wywolujaca sie, gdy cos jest nie tak z karta SD (nie ma jej lub jest uszkodzona) UWAGA: po restarcie sterownika musi minac sekunda-dwie, zeby pojawilo sie zasilanie na karcie SD, wczesniej moze pojawic sie ten blad
void SomethingWrongWithSD(){
    
    led1 = 1; led2 = 1; led3 = 1; led4 = 1; wait(0.5); led1 = 0; led2 = 0; led3 = 0; led4 = 0; wait(0.5); // sygnalizuje blad karty SD - zapala i gasi wszystkie diody na raz
    led1 = 1; led2 = 1; led3 = 1; led4 = 1; wait(0.5); led1 = 0; led2 = 0; led3 = 0; led4 = 0; wait(0.5);
    led1 = 1; led2 = 1; led3 = 1; led4 = 1; wait(0.5); led1 = 0; led2 = 0; led3 = 0; led4 = 0;
    NVIC_SystemReset();
    error("Could not open file for write\n");
}

int CheckMbedIsStillAlive(int *i){
    
    if(*i%30000 == 0){
        led1 = 1; 
    }else{
        led1 = 0;
    }
    if(*i >= 90000){
        *i = 0;
    }
    return *i;
}

// jesli nie ma Internetu (np. przewod jest odlaczony), to ta funkcja bedzie sie wywolywac caly czas w petli
void ReconnectIfNoNet(){
    
    pc.printf("\n INTERNET IS UNAVAILABLE, BUT I'M TRYING TO CONNECT ANYWAY... \n");
    led4 = 1; ledCzerwona = 1; ledZolta = 1;
    eth.connect(); 
    wait(1);
    led4 = 0; ledCzerwona = 0; ledZolta = 0;
}

// usuwa wszystko co przed N1;, wszystkie N1; i do poczatku pliku dopisuje nazwe programu  
void CleanMessInFile(string pathReceiveFromMachineSD, string extension, string programName){
    
    char c = 'a';
    FILE *fileOld;
    FILE *fileNew;
    string line = "";
    
    fileOld = fopen((pathReceiveFromMachineSD + programName + "WithMess" + extension).c_str(), "r");
    if(fileOld == NULL) {
        SomethingWrongWithSD();
    }
    
    fileNew = fopen((pathReceiveFromMachineSD + programName + extension).c_str(), "w");
    if(fileNew == NULL) {
        SomethingWrongWithSD();
    }
    
    while(!feof(fileOld)){
        c = fgetc(fileOld);
        
        if(c != 13){
            
        }
    }
    
    
    
    
}



// funkcja do debbugingu (nie jest potrzebna w programie! Zrobilam ja na potrzeby debbugowania programu)
//void FunkcjaDoDebbugingu(string pathSentToMachineSD, string programName, string extension){
//    
//    FILE *file;
//    char c = 'a';
//        
//    file = fopen((pathSentToMachineSD + programName +  extension).c_str(), "r");
//    if(file == NULL) {
//        SomethingWrongWithSD();
//    }
//    while(!feof(file)){
//        wait(0.03);
//        c = fgetc(file);
//        uart.putc(c);
//    }
//    fclose(file);
//}
