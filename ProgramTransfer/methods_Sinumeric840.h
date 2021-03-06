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
    led1 = 1; wait(0.05); led1 = 0; wait(0.05); led1 = 1; wait(0.05); led1 = 0; wait(0.05); led1 = 1; wait(0.05); led1 = 0;
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

// usuwa z pliku nagolwki z Servera, oraz nadmiarowa tresc po M30 lub M17 lub M02 lub M90
void CleanTheMessInFile(string pathSentToMachineSD, string programName, string extension){
    
    pc.printf("\nWESZLO\n");
    pc.printf("%s   %s   %s", pathSentToMachineSD, programName, extension);
    FILE *fileOld;
    FILE *fileNew;
    char c = 'a';
    string a1 = "a";
    string signalLong = "aaaaa";
    string signalShort = "aaa";
    bool writeFlag = false;
    bool isProgram = false;
    
    fileOld = fopen((pathSentToMachineSD + programName + "withMess" +  extension).c_str(), "r");
    if(fileOld == NULL) {
        SomethingWrongWithSD();
    }
    
    pc.printf("\notworzylo stary - with mess\n");
    
    fileNew = fopen((pathSentToMachineSD + programName +  extension).c_str(), "w");
    if(fileNew == NULL) {
        SomethingWrongWithSD();
    }
    
    pc.printf("\notworzylo nowy\n");
    
    // pobieraj znaki ze starego pliku, dopoki nie ma jego konca
    while(!feof(fileOld)){
        c = fgetc(fileOld);
        a1 = string(1, c);
        signalLong = signalLong + a1;
        signalLong = signalLong.substr(1,6);
        signalShort = signalShort + a1;
        signalShort = signalShort.substr(1,4);
        if(signalLong == "\r\n\r\n%" || signalShort == "\n\n%"){ // jesli pojawi sie ciag znakow enter, enter, %, to znaczy, ze skonczyl sie naglowek, to co za nim pojawi sie w nowym programie (usuniecie naglowka)
            writeFlag = true; 
            isProgram = true; // to, ze sie pojawil enter, enter i % oznacza, ze to co jest w pliku jest tez programem
        }
        if(writeFlag){ // true ,gdy znaki naleza do programu
            fputc(c, fileNew);
        }
        if(signalShort == "M30" || signalShort == "M17" || signalShort == "M02" || signalShort == "M90"){ // jesli pojawi sie M30 lub M17 lub M02 lub M90, to znak, ze program sie skonczyl
            fputc(13, fileNew);
            fputc(10, fileNew);
            fputc('%', fileNew); // po wychwyceniu konca programu dodaj jeszcze 13, 10 i %
            writeFlag = false;
            signalShort = "";
        }
    }
    
    fclose(fileOld);
    fclose(fileNew);
    
    remove((pathSentToMachineSD + programName + "withMess" +  extension).c_str());
    pc.printf("\nusunelo stary - with mess\n");
    
    if(!isProgram){
        fileNew = fopen((pathSentToMachineSD + programName +  extension).c_str(), "w");
        if(fileNew == NULL) {
            SomethingWrongWithSD();
        }
        fputs(("%MPF" + programName + "\r\n").c_str(), fileNew);
        fputs("NIE MA TAKIEGO PLIKU NA SERWERZE\r\n", fileNew);
        fputs("M30\r\n", fileNew);
        fputs("%", fileNew);
        fclose(fileNew);
    }
}

//wysyla program na serwer przez HTTP
void Send_From_SD_to_Server_HTTP (string ProgramName, char *server_ip, int port, string path, string extension, string ID_OF_MACHINE){
    
    FILE *file;
    string line = "";
    string request = "";
    char array[500] = "";
    char c = 'a';
    int conn = -1;
    
    //usun plik z Servera, jesli taki juz istnieje (i jest polaczenie z Serverem)
    pc.printf("DELETE IF EXIST \n");
    pc.printf("\n I'M OPENING THE SOCKET! \n");
    sock.open(&eth);
    conn = sock.connect(server_ip, port);
    if(conn == 0){
        SocketOK();
        request = "GET /delete_if_exist.php?program_name=" + ProgramName + "&extension=" + extension + "&machineFolder=" + ID_OF_MACHINE + " HTTP/1.0\r\n\r\n";
        strcpy(array, request.c_str());
        sock.send(array, sizeof(array));  
    }else{
        SocketFAIL();
    }
    sock.close();
    pc.printf("\n OK, I CLOSED IT, CYA \n"); 
    
    // otworz plik na SD
    pc.printf("OPEN A FILE \n");
    file = fopen((path + ProgramName + extension).c_str(), "r");
    if(file == NULL) {
        SomethingWrongWithSD();
    }
    
    while(!feof(file)){ // pobiera znaki az do konca pliku (EOF)
        c = fgetc(file);
    
        if(c != 13 && c != 10){
            if(c == 32){ // zamienia spacje na ~ (wymagane do zapytan PHP, w ktorych nie moze pojawic sie znak spacji, znak '13' ani znak '10' - zamieniane i dodawane sa z powrotem po stronie Servera)
                c = '~';
            }
            line += c;
        }

        //if(c == 13 || c ==10){
          if(c ==10){  
            pc.printf("%s", line);
            pc.printf("\n");
            request = "GET /send_file_to_server_HTTP.php?program_name=" + ProgramName + "&extension=" + extension + "&machineFolder=" + ID_OF_MACHINE + "&line=" + line + " HTTP/1.0\r\n\r\n";
            strcpy(array, request.c_str());
            pc.printf("%s", array);
            pc.printf("\n");
            
            // otworz polaczenie z Serverem
            pc.printf("\n I'M OPENING THE SOCKET! \n");
            sock.open(&eth);
            conn = sock.connect(server_ip, port);
            if(conn == 0){
                SocketOK();
            }else{
                SocketFAIL();
            }
            pc.printf("SOCK IS: %d\n", conn);
            
            sock.send(array, sizeof(array)); // wysyla linie z pliku
            led3 = 1; wait(0.1); led3 = 0; // mrugnie dioda nr3, zeby pokazac, ze wysylanie jeszcze trwa
            
            // zamknij polaczenie z Serverem
            sock.close();
            pc.printf("\n OK, I CLOSED IT, CYA \n"); 
            
            line = "";
        }
    }
    
    if(conn==0){
        //jesli jest polaczenie z Serverem, to usun tez plik z SD
        remove((path + ProgramName + extension).c_str());        
    }
    
    fclose(file);
    pc.printf("FILE CLOSED");
    
}

// szuka nazwy w zapisanym pliku (szuka pierwszych 4 cyfr w programie i traktuje je jako jego nazwe)
string RenameFile(string path, string extension){
    
    char c = 'a';
    string programName = "";
    FILE *file;
    file = fopen((path + "NazwaPliku" + extension).c_str(), "r");
    if(file == NULL) {
        SomethingWrongWithSD();
    }
    while(!feof(file)){
        c = fgetc(file);
        if(isdigit(c)){
            programName = programName + c;
            if(programName.length() == 4){
                fclose(file);
                break;
            }
        }
    }
    
    rename((path + "NazwaPliku" + extension).c_str(), (path + programName + "WithTildas" + extension).c_str());
    
    return programName;
    
}

// dla programow Siemensowych - usuwa znaki tyldy na poczatku i koncu programu
void RemoveTildas(string programName, string path, string extension){

    FILE *fileOld;
    FILE *fileNew;
    char c = 'a';
    
    fileOld = fopen((path + programName + "WithTildas" + extension).c_str(), "r");
    if(fileOld == NULL) {
        SomethingWrongWithSD();
    }
  
    fileNew = fopen((path + programName + extension).c_str(), "w");
    if(fileNew == NULL) {
        SomethingWrongWithSD();
    }
    
    while(!feof(fileOld)){
        c = fgetc(fileOld);
        if(c != '~'){
            fputc(c, fileNew);
        }    
    }
    fclose(fileNew);
    fclose(fileOld);
    
    remove((path + programName + "WithTildas" + extension).c_str());

}

// wysyla zapytanie o program do Servera (gdy operator uzyl programu 5002) - w czasie "rzeczywistym" (no, prawie) i wysyla plik z Serwera na SD (do sd/sentToMachine i sd/archive), a nastepnie z SD na maszyne
void AskServerForProgram(string programName, char *server_ip, int port, string extension, string pathSentToMachineSD, string pathArchive, string ID_OF_MACHINE){
    
    int conn = -1;
    char array[200] = "";
    char arrayNew[700] = "";
    char arrayPrev[700] = "";
    char c = 'a';
    string request = "";
    FILE *file;
    FILE *fileOnSD;
    FILE *fileArchive;
    
    pc.printf("NR PROGRAMU: %s \n",programName);
    
    // TA CZESC PROGRAMU WYSYLA PROGRAM Z SERVERA NA SD (do sd/receivedFromMachine/ i kopiuje do sd/archive/)
    
    //otworz polaczenie z Serverem
    sock.open(&eth);
    conn = sock.connect(server_ip, port);
    if(conn == 0){
        SocketOK();
        //usun poprzedni plik z programem z karty SD, jesli istnieje (w sendToMachine zawsze zostaje najaktualniejszy)
        remove((pathSentToMachineSD + programName + extension).c_str());
        request = "GET /ProgramsExchange/" + ID_OF_MACHINE + "/sentToMachine/" + programName + extension + " HTTP/1.0\r\n\r\n";
        pc.printf("%s", request);
        strcpy(array, request.c_str());
        sock.send(array, sizeof(array));  
        
        file = fopen((pathSentToMachineSD + programName + "withMess" +  extension).c_str(), "a");
        if(file == NULL) {
            SomethingWrongWithSD();
        }
        
        //przechwyc odpowiedz z Servera
        while(1){

            sock.recv(arrayNew, sizeof arrayNew);
            //pc.printf("\n ARRAY - %s \n", arrayNew);
            
            fprintf(file, "%s", arrayNew);
            
            if(memcmp(arrayPrev, arrayNew, sizeof(arrayNew))==0){
                pc.printf("PRZERWANO");
                break;
            }
            
            memcpy(arrayPrev, arrayNew, sizeof(arrayNew));
            
        }
        fclose(file);
        pc.printf("\n PLIK ZAPISANO NA SD/sentToMachine\n");
        
        //usuwa z pliku nagolwki z Servera, oraz nadmiarowa tresc po M30
        CleanTheMessInFile(pathSentToMachineSD, programName, extension);
        
        //kopiuj plik do Archive na SD
        fileOnSD = fopen((pathSentToMachineSD + programName + extension).c_str(), "r");
        if(fileOnSD == NULL) {
            SomethingWrongWithSD();
        }
        fileArchive = fopen((pathArchive + programName + extension).c_str(), "w");
        if(file == NULL) {
            SomethingWrongWithSD();
        }
        while(!feof(fileOnSD)){
            c = fgetc(fileOnSD);
            fputc(c, fileArchive);
        }
        pc.printf("\n PLIK SKOPIOWANO DO ARCHIWUM\n");
        fclose(file);
        fclose(fileArchive);
    
        // TA CZESC PROGRAMU WYSYLA PROGRAM Z SD NA MASZYNE
    
        wait(30); // czekaj 30 sek - daje czas operatorowi na przygotowanie maszyny do odebrania programu
    
        file = fopen((pathSentToMachineSD + programName +  extension).c_str(), "r");
        if(file == NULL) {
            SomethingWrongWithSD();
        }
        while(!feof(file)){
            c = fgetc(file);
            uart.putc(c);
            led3 = 1;
            wait(0.03); // to opoznienie jest z uwagi na to, ze sterownik za szybko wysyla dane (maszyna potrzebuje chwili po kazdym znaku)
            led3 = 0;
        }
        fclose(file);
    
        // usuwa program z /sd/sentToMachine (kopia zostaje w /sd/archive na wszelki wypadek)
        remove((pathSentToMachineSD + programName + extension).c_str());
        
    }else{
        uart.puts(("%MPF"+programName+"\r\n(WYSTAPIL PROBLEM Z SERWEREM - SKONTAKTUJ SIE Z ADMINISTRATOREM)\r\nM30 \r\n").c_str());
        SocketFAIL();
    }
    sock.close();
    pc.printf("\n OK, I CLOSED IT, CYA \n"); 
    
}

// wysyla zapytanie do folderu /sd/archive o program (gdy operator uzyl programu 5005) - w czasie "rzeczywistym" (no, prawie) i wysyla plik z /sd/archive na maszyne (plik po przeslaniu nadal zostaje w /sd/archive)
void AskArchiveForProgram(string pathArchive, string programName, string extension){
    
    FILE *file;
    char c = 'a';
        
    file = fopen((pathArchive + programName +  extension).c_str(), "r");
    if(file == NULL) {
        uart.puts(("%MPF"+programName+"\r\n(NIE MA TAKIEGO PLIKU W ARCHIWUM LUB PROBLEM Z KARTA SD)\r\nM30 \r\n").c_str());
        SomethingWrongWithSD();
    }
    while(!feof(file)){
        wait(0.03);
        c = fgetc(file);
        uart.putc(c);
    }
    fclose(file);
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
