#include <string>
#include "mbed.h"
#include "EthernetInterface.h"
#include "methods.h"
#include "Socket.h"
#include "TCPSocket.h"

#define rts p22
#define cts p17

// UWAGI OGOLNE: Wszystkie programy musza byc ciagiem 4 cyfr! 555 - zle, 0555 - dobrze

// UWAGI OGOLNE DLA SINUMERICA:
// Maszyna poprzedza wysylanie programu ciagiem ~140-170 nulli
// Po nich nastepuje wyslanie wlasciwej tresci programu
// a po zakonczeniu programu wyslana zostaje kolejna seria 140-170 nulli

// Strerownik wychwytuje pierwsza serie nulli i w tym czasie otwiera plik o nazwie "NazwaPliku" do zapisu na sd/receivedFromMachine
// kazdy kolejny znak wyslany przez maszyne jest teraz zapisywany bezposrednio na karcie SD
// gdy nastapi kolejna (konczaca) seria nulli nastepuje zamkniecie pliku.
// Po tej operacji, wywolana zostaje funkcja pobrania nazwy programu z tresci pliku (nastepuje zmiana nazwy z "NazwaPliku" na wlasciwa nazwe programu)
// a nastepnie funkcja "czyszczaca" (nulle zapisuja sie w pliku jako znak ~ (tylda)), ktora usuwa wszystkie ~ z pliku
// Ostatecznie plik zostaje wyslany na Server (miganie led3 oznacza, ze plik jest w trakcie wysylania)

// Sterownik nasluchuje tez sygnalow od maszyny tj. programu 5002 (wysyla zapytanie na Server o okreslony program) lub 5005 (wysyla zapytanie do SD/archive o okreslony program)
// Jesli operator wysle taki program, to sterownik przechwytuje jego tresc, a nastepnie odczytuje z niego jaki program ma zostac pobrany na maszyne
// Operator po wyslaniu takiego programu ma 30sek na ustawienie maszyny w tryb odbioru (DIO)
// Po tym czasie (jesli 5002) Server wysle plik na sd/sentToMachine, nastepnie zostanie wywolana funkcja "czyszczaca" (usuniete zostana dodatkowe dane otrzymane z Servera (naglowki))
// i plik zostanie wyslany z sd/sendToMachine bezposrednio na maszyne. W przypadku programu 5005 sterownik nie laczy sie z Serverem, tylko (jesli istnieje) wysyla plik bezposrednio z sd/archive

// UWAGI DO PROGRAMOW DLA SINUMERICA:
// 5002 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z servera

// wzor programu 5002 (Sinumeric) - wczesniej nie bylo linii (O5002), teraz jest ona wymagana dla Sinumerica, dla Fanuca nalezy ja wykasowac!
// %MPF5002
// (O5002)
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z Servera, tu: 1234
// M30

// 5005 - nazwa programu, w ktorym umieszcza sie nr programu, ktory chcemy sciagnac z sd/archive (to folder na sd, w ktorym zapisuja sie ostatnio pobrana wersja z serwera, dzieki niemu operator ma dostep przynajmniej do starszej wersji)

// wzor programu 5005 (Sinumeric)
// %MPF5005
// (O5005)
// S(1234) - w parametrze S wpisujemy nr programu, ktory chcemy sciagnac z sd/archive, tu: 1234
// M30


int main() {
    
    int i = 0;
    int j = 0;
    char a = 'a';
    string a1 = "a";
    string signal = "aaaaa";
    bool WriteOnSDFlag = false; // jesli jest sygnal od maszyny, ze zaczyna nadawanie (w Scharmannie: duzo znakow NULL na poczatku) to flaga = true, jesli jest sygnal od maszyny, ze konczy nadawanie (w Scharmannie: tez duzo znakow NULL na koncu) to flaga = false
    string ProgramName = "";
    FILE *fp;
    char *server_ip = "192.168.90.35";
    int port = 80;
    string extension = ".txt";
    string SD_rec_From_Machine_Path = "/sd/receivedFromMachine/";
    string SD_sent_To_Machine_Path = "/sd/sentToMachine/";
    string SD_archive = "/sd/Archive/";
    string requestedProgram = "";
    string requestedArchiveProgram = "";
    bool RequestForProgramFlag = false; // jesli jest sygnal od maszyny, ze wywolano program 5002, to flaga = true
    bool RequestArchiveForProgramFlag = false; // jesli jest sygnal od maszyny, ze wywolano program 5005, to flaga = true
    
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
    uart.set_flow_control(Serial::RTSCTS, rts, cts); 
    uart.format(7, Serial::Even, 2);
    
    uart.printf("Jestem dobrym UARTEM");
    pc.printf("Jestem dobrym PCtem");
    //
    //
    //
    //
    //
    while(1) {
        
        i++;
        i = CheckMbedIsStillAlive(&i); // mrugnij czasem dioda 1, zeby pokazac, ze sterownik zyje i ma sie dobrze
        
        while(!ether.link()){ // sprawdzaj czy jest net, jak nie, to sprobuj polaczyc i zapal led4, czerwona i zolta, a jesli sie polaczysz, to wygas ledy
            ReconnectIfNoNet();
        }
            
        if(pc.readable()) {       // sprawdza czy jest cos na pinie PC rx w mbedzie (true, gdy wysylamy dane z PC na maszyne)
            a = pc.getc();
            uart.putc(a);           
        }
        
        if(uart.readable()) {    // sprawdza czy jest cos na pinie UART rx w mbedzie (true, gdy wysylamy dane z maszyny na PC)
            
            a = uart.getc();
            if(a == NULL){ // zamiana NULL na '~' (inaczej nie widac nic na terminalu, poza tym lepiej sie testuje)
                a = '~';
            }
            pc.printf("%c",a);
            
            // wychwytuje sygnaly komunikacyjne (np. sygnal startu/konca nadawania (5xNULL dla Scharmanna), sygnaly komunikacyjne (np. program sciagajacy programy z serwera %5002 lub O5002) itp.)
            //pc.printf("\n signal: %s \n", signal);
            a1 = string(1, a);
            signal = signal + a1;
            signal = signal.substr(1,6);
            
            // licz ile razy pojawil sie sygnal ~~~~~
            // jesli pojawil sie 100 razy, to znaczy, ze zaczela sie transmisja, jesli pojawil znowu, to znaczy, ze transmisja sie zakonczyla
            if(signal == "~~~~~"){
                j++;
            }else{
                j = 0;
            }
            
            // sygnal rozpoczecia lub zakonczenia transmisji danych (wg Siemensa)
            if(j==100){
                signal = "aaaaa";
                pc.printf("A LOT OF NULLS!");
                WriteOnSDFlag = !WriteOnSDFlag; // zmienna odpowiedzialna za "wyczucie" czy to poczatek pliku (pierwsza seria NULLi), czy koniec pliku (druga seria NULLi) 
                if(WriteOnSDFlag){ // jesli NULLe na poczatku, to tworzy lub otwiera plik
                    fp = fopen((SD_rec_From_Machine_Path+"NazwaPliku"+extension).c_str(), "w"); // jesli nie ma, to stworz plik, jesli jest, to dodaj kolejne znaki.
                    if(fp == NULL) {
                        SomethingWrongWithSD();
                    }
                    pc.printf("FILE OPENED");
                }
                if(!WriteOnSDFlag && !RequestForProgramFlag && !RequestArchiveForProgramFlag){ // jesli NULLe na koncu, to zamyka plik + zaczyna procedure wysylania programu z SD na serwer
                    fclose(fp);
                    ProgramName = RenameFile(SD_rec_From_Machine_Path, extension); // odszukuje nazwe programu z pliku (szuka 4 pierwszych CYFR w programie i traktuje je jako jego nazwe)
                    RemoveTildas(ProgramName, SD_rec_From_Machine_Path, extension); // jesli po rozpoczeciu zapisu maszyna jeszcze chwile wysylala NULLe (ktore zostaly zamienione na tyldy), to tu nastepuje ich usuniecie
                    pc.printf("FILE IS ON SD");
                    Send_From_SD_to_Server_HTTP(ProgramName, server_ip, port, SD_rec_From_Machine_Path, extension); // wysylanie programu z SD na Server (ProgramsExchange/receivedFromMachine) linia po linii
                }
            }
            
            // jesli plik jest otwarty, to kolejne znaki zapisuj do pliku na SD
            if(WriteOnSDFlag){
                fprintf(fp,"%c", a);
            }
            
            // sygnal do pobrania pliku z Servera (w tresci programu 5002 musi znajdowac sie nr programu, ktory chcemy pobrac)
            if(signal == "O5002"){
                RequestForProgramFlag = true;
                fclose(fp);
                a = 'a';
            }
            if(RequestForProgramFlag){
                if(isdigit(a)){ // szuka pierwszych 4 cyfr po O5002 i traktuje je jako nr programu
                    requestedProgram = requestedProgram + a;
                }
                if(requestedProgram.length() == 4){
                    AskServerForProgram(requestedProgram, server_ip, port, extension, SD_sent_To_Machine_Path, SD_archive); // wysyla zapytanie o program do Servera (gdy operator uzyl programu 5002) - w czasie "rzeczywistym" (no, prawie) i wysyla plik z Serwera na SD (do sd/sentToMachine i sd/archive), a nastepnie z SD na maszyne
                    requestedProgram = "";
                    RequestForProgramFlag = false;  
                    WriteOnSDFlag = false; // ta instrukcja zostala dodana, poniewaz AskServerForProgram trwa zbyt dlugo i sterownik nie wychwytuje NULLi konczacych program
                }
            }
            
            // sygnal do pobrania pliku z /sd/archive/ (w tresci programu 5005 musi znajdowac sie nr programu, ktory chcemy pobrac)
            if(signal == "O5005"){
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