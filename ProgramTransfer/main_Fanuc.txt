#include <string>
#include "mbed.h"
#include "EthernetInterface.h"
#include "methods.h"
#include "Socket.h"
#include "TCPSocket.h"

#define rts p22
#define cts p17

// UWAGI OGOLNE: Wszystkie programy musza byc ciagiem 4 cyfr! 555 - zle, 0555 - dobrze

// UWAGI DO PROGRAMOW DLA FANUCA:
// 5002 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z servera

// wzor programu 5002 (Fanuc)
// O5002
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z Servera, tu: 1234
// M30

// 5005 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z sd/archive (to folder na sd, w ktorym zapisuja sie ostatnio pobrana wersja z serwera, dzieki niemu operator ma dostep przynajmniej do starszej wersji)

// wzor programu 5005 (Fanuc)
// O5005
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z sd/archive, tu: 1234
// M30


int main() {
    
    char *server_ip = "192.168.90.35";
    int port = 80;
    int i = 0;
    char a = 'a';
    string a1 = "a";
    string signal = "aaaaa";
    string programName = "";
    bool CatchProgramName = false;
    bool WriteOnSDFlag = false;
    string SD_rec_From_Machine_Path = "/sd/receivedFromMachine/";
    string extension = ".txt";
    FILE *fp;
    
    // polacz z netem
    eth.connect();
    
    pc.printf("MAC: "); pc.printf(eth.get_mac_address()); pc.printf("IP: "); pc.printf(eth.get_ip_address());
    
    // jesli jest polaczenie z netem, to mrugnij szybko pierwsza dioda
    // jesli nie ma polaczenia z netem, to zapal diody od 1 do 4 i je wygas
    if(ether.link()){
        led1=1; wait(0.1); led1=0; wait(0.1); led1=1; wait(0.1); led1=0; wait(0.1); led1=1; wait(0.1); led1=0; wait(0.1);
    }else{
        led1=1; wait(0.1); led2=1; wait(0.1); led3=1; wait(0.1); led4=1; wait(0.1); 
        led4=0; wait(0.1); led3=0; wait(0.1); led2=0; wait(0.1); led1=0; wait(0.1); 
    }
    
    // ustawienie RS232
    uart.baud(1200);
    uart.format(7, Serial::Even, 2);
    
    uart.printf("Jestem dobrym UARTEM");
    pc.printf("Jestem dobrym PCtem");
    //
    //
    //
    //
    //
    while(1){
    
        i++;
        i = CheckMbedIsStillAlive(&i); // mrugnij czasem dioda 1, zeby pokazac, ze sterownik zyje i ma sie dobrze
        
        while(!ether.link()){ // sprawdzaj czy jest net, jak nie, to sprobuj polaczyc i zapal led4, czerwona i zolta, a jesli sie polaczysz, to wygas ledy i lec dalej
            ReconnectIfNoNet();
        }
            
        if(pc.readable()) {       // sprawdza czy jest cos na pinie PC rx w mbedzie (true, gdy wysylamy dane z PC na maszyne)
            a = pc.getc();
            uart.putc(a);           
        }
        
        if(uart.readable()) {    // sprawdza czy jest cos na pinie UART rx w mbedzie (true, gdy wysylamy dane z maszyny na PC)
            
            a = uart.getc();
            pc.printf("%c",a);
            
            // wychwytuje sygnaly komunikacyjne (np. program sciagajacy programy z serwera 5002 lub 5005) itp.)
            //pc.printf("\n signal: %s \n", signal);
            a1 = string(1, a);
            signal = signal + a1;
            signal = signal.substr(1,6);
            
            if(a == 18){ // sygnal, ze maszyna zaczyna nadawac program
                CatchProgramName = true;
            }
            
            if(CatchProgramName){ // pobiera z programu pierwsze 4 znaki, ktore sa cyframi
                if(isdigit(a)){
                    programName = programName + a;
                    if(programName.length() == 4){
                        fp = fopen((SD_rec_From_Machine_Path + programName + "WithMess" + extension).c_str(), "w"); 
                        CatchProgramName = false;
                        WriteOnSDFlag = true; // po pobraniu nr programu kolejne znaki beda zapisywaly sie na SD
                    }
                }
            }
            
            if(WriteOnSDFlag){
                fprintf(fp,"%c", a);
            }
            
            if(strstr(signal.c_str(), "M30") || strstr(signal.c_str(), "M02") || strstr(signal.c_str(), "M90") || strstr(signal.c_str(), "M17")){
                signal = "aaaaa";
                fclose(fp);
                CleanMessInFile(SD_rec_From_Machine_Path, extension, programName); // usuwa wszystko co przed N1;, wszystkie N1; i do poczatku pliku dopisuje nazwe programu  
                pc.printf("FILE IS ON SD");
            }
            
        } 
    
    }
    
}