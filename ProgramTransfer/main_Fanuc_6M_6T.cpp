#include <string>
#include "mbed.h"
#include "EthernetInterface.h"
#include "methods.h"
#include "Socket.h"
#include "TCPSocket.h"

#define rts p22
#define cts p17

// UWAGI OGOLNE:
// 1. Wszystkie programy musza byc ciagiem 4 cyfr! 555 - zle, 0555 - dobrze
// 2. Jesli maszyna ma ustawiona predkosc (baud) wieksza niz 2400 to moze ucinac kilka pierwszych znakow programu podczas jego wysylania z maszyny na serwer! Nalezy dodac jakies linie neutralne (np. N1N1N1 - ok. 40-50znakow, czyli 20-25x N1)
// 3. BARDZO WAZNE!!!!! W programach Fanuca w tresci (np. komentarzu) NIE moze pojawic sie znak % !!! Program Fanuca musi miec % tylko na poczatku (przed nazwa programu) i na koncu pliku (po M30 itp.)!

// UWAGI DO PROGRAMOW DLA FANUCA:
// 5002 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z servera

// wzor programu 5002
// %
// O5002
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z Servera, tu: 1234
// M30
// %

// 5005 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z sd/archive (to folder na sd, w ktorym zapisuja sie ostatnio pobrana wersja z serwera, dzieki niemu operator ma dostep przynajmniej do starszej wersji)

// wzor programu 5005 (Fanuc)
// %
// O5005
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z sd/archive, tu: 1234
// M30
// %

int main() {
    
    const string ID_OF_MACHINE = "CO63"; // DO TEGO FOLDERU BEDZIE ODWOLYWAL SIE PROGRAM!!! JESLI STEROWNIK PODPIETY JEST POD INNA MASZYNE ZMIEN TA NAZWE I UTWORZ TAKI FOLDER NA SERWERZE MASZYN!!!
    char *server_ip = "192.168.90.33";
    int port = 80;
    int i = 0;
    char a = 'a';
    string a1 = "a";
    string signal = "aaaaa";
    string programName = "";
    string requestedProgram = "";
    string requestedArchiveProgram = "";
    bool CatchProgramName = false;
    bool WriteOnSDFlag = false;
    bool FirstPercent = false;
    bool RequestForProgramFlag = false;
    bool RequestArchiveForProgramFlag = false;
    string SD_rec_From_Machine_Path = "/sd/receivedFromMachine/";
    string SD_sent_To_Machine_Path = "/sd/sentToMachine/";
    string SD_archive = "/sd/Archive/";
    string extension = ".txt";
    FILE *fp;
    string programBeginChars = "%\nO"; // DLA FANUCA INNEGO NIZ 6M
    string newLineChars = "\n"; // DLA FANUCA 6M (u Krzysia Michalaka - CO13)
    
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
    uart.baud(2400);
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
        
        if(uart.readable()) {    // sprawdza czy jest cos na pinie UART rx w mbedzie (true, gdy wysylamy dane z maszyny na np. PC)
            
            a = uart.getc();
            pc.printf("%c",a);
            
            // wychwytuje sygnaly komunikacyjne (np. program sciagajacy programy z serwera 5002 lub 5005) itp.)
            //pc.printf("\n signal: %s \n", signal);
            a1 = string(1, a);
            signal = signal + a1;
            signal = signal.substr(1,6);
            
            if(a == 37){ // sygnal, ze maszyna zaczyna nadawac program lub skonczyla nadawac (procent jest na poczatku i koncu kazdego programu)
                FirstPercent = !FirstPercent;
                if(FirstPercent){
                    CatchProgramName = true;
                    pc.printf("\nSTART\n");
                }else{
                    pc.printf("\nEND\n");
                    a = 'a';
                    fprintf(fp,"%s","%\n");
                    fclose(fp);
                    WriteOnSDFlag = false;
                    AddProgramName(SD_rec_From_Machine_Path, extension, programName, programBeginChars, newLineChars); // dodaje nr programu do poczatku
                    Send_From_SD_to_Server_HTTP(programName, server_ip, port, SD_rec_From_Machine_Path, extension, ID_OF_MACHINE);
                    pc.printf("FILE IS ON SD");
                    programName = "";
                }
            }

            if(WriteOnSDFlag){
                fprintf(fp,"%c", a);
            }
            
            if(CatchProgramName){ // pobiera z programu pierwsze 4 znaki, ktore sa cyframi
                if(isdigit(a)){
                    programName = programName + a;
                    if(programName.length() == 4){
                        pc.printf("\nOTWORZYLEM PLIK\n");
                        pc.printf("%s", (SD_rec_From_Machine_Path + programName + "WithMess" + extension));
                        fp = fopen((SD_rec_From_Machine_Path + programName + "WithMess" + extension).c_str(), "w"); 
                        if(fp == NULL) {
                            SomethingWrongWithSD();
                        }
                        CatchProgramName = false;
                        WriteOnSDFlag = true; // po pobraniu nr programu kolejne znaki beda zapisywaly sie na SD
                    }
                }
            }
            
            // sygnal do pobrania pliku z Servera (w tresci programu 5002 musi znajdowac sie nr programu, ktory chcemy pobrac)
            if(signal == "O5002" || signal == ":5002"){
                RequestForProgramFlag = true;
                fclose(fp);
                a = 'a';
            }
            if(RequestForProgramFlag){
                if(isdigit(a)){ // szuka pierwszych 4 cyfr po 5002 i traktuje je jako nr programu
                    requestedProgram = requestedProgram + a;
                }
                if(requestedProgram.length() == 4){
                    AskServerForProgram(requestedProgram, server_ip, port, extension, SD_sent_To_Machine_Path, SD_archive, ID_OF_MACHINE); // wysyla zapytanie o program do Servera (gdy operator uzyl programu 5002) - w czasie "rzeczywistym" (no, prawie) i wysyla plik z Serwera na SD (do sd/sentToMachine i sd/archive), a nastepnie z SD na maszyne
                    requestedProgram = "";
                    RequestForProgramFlag = false;
                    WriteOnSDFlag = false;
                }
            }
            
            // sygnal do pobrania pliku z /sd/archive/ (w tresci programu 5005 musi znajdowac sie nr programu, ktory chcemy pobrac)
            if(signal == "O5005" || signal == ":5005"){
                RequestArchiveForProgramFlag = true;
                fclose(fp);
                a = 'a';
            }
            if(RequestArchiveForProgramFlag){
                if(isdigit(a)){ // szuka pierwszych 4 cyfr po O5005 i traktuje je jako nr programu
                    requestedArchiveProgram = requestedArchiveProgram + a;
                }
                if(requestedArchiveProgram.length() == 4){
                    wait(30); // czeka 30sek, daje czas operatorowi na przygotowanie maszyny w stan odbioru programu (DIO)
                    AskArchiveForProgram(SD_archive, requestedArchiveProgram, extension); // wysyla zapytanie do folderu /sd/archive o program (gdy operator uzyl programu 5005) - w czasie "rzeczywistym" (no, prawie) i wysyla plik z /sd/archive na maszyne (plik po przeslaniu nadal zostaje w /sd/archive)
                    requestedArchiveProgram = "";
                    RequestArchiveForProgramFlag = false;
                    WriteOnSDFlag = false; // ta instrukcja zostala dodana, poniewaz AskServerForProgram trwa zbyt dlugo i sterownik nie wychwytuje NULLi konczacych program
                }
            }
        } 
    }
}
