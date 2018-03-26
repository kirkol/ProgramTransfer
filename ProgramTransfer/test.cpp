#include <string>
#include "mbed.h"
#define rts p22
#define cts p17

// PROGRAM DO BEZPOSREDNIEGO POBIERANIA/WYSYLANIA ZNAKOW Z I DO MASZYNY - 

// debbuging (diody na sterowniku)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Seriale
Serial pc(USBTX, USBRX); // przewod sterownik <-> komputer (USB)
Serial uart(p13, p14); // przewod DB9 <-> maszyna

//siemesn
DigitalOut RTSwyjscieRS(p22);
DigitalIn CTSwejscieRS(p17);


int main() {
    
    char a = 'a';
    led1 = 0;
    led2 = 0;
    int i = 0;
    
//    // ustawienie RS232
    uart.baud(2400);
    uart.format(7, Serial::Even, 2);
    //uart.set_flow_control(Serial::RTSCTS, rts, cts);
    
    //uart.printf("Jestem dobrym UARTEM");
    pc.printf("Jestem dobrym PCtem");
    
    while(1) {  
            
        if(i%30000 == 0){
            led4 = 1;
        }else{
            led4 = 0;
        }
        if(i >= 90000){
            i = 0;
        }
        i++;
            
        if(pc.readable()) {       // sprawdza czy jest cos na pinie rx w USB - gniazdo miniUSB (Z PC NA MASZYNE)
            a = pc.getc();                          
            led1 = !led1;
            uart.putc(a);
            pc.printf("%c",a);      // wyswietla znaki w ASCII
        }
        if(uart.readable()) {    // sprawdza czy jest cos na pinie rx w uarcie - pin 14 (Z MASZYNY NA PC)
            a = uart.getc();
            led2 = !led2;
            pc.printf("%d ",a);  // wyswietla znaki w zapisie DECymalnym
        }
    }
}