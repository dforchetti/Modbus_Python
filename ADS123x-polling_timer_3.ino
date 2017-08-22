/*
   dforchetti 16-06-2017
   Cabezal Balanza ADS1230 x 4
*/

#include "SimpleModbusSlave.h"
#include <avr/wdt.h>

#define NumeroDeBits  24
#define TAMANO_PAQUETE  6  // numero de enteros sin signo usados para transmitir el mensaje en MODBUS

// Digital Pins With Interrupts  Uno, Nano, Mini, other 328-based ....... PIIN : 2, 3

#define NADS   4
#define PWDN   7   // PIN asignado a la señal PWDN
#define DOUT2  6   // PIN asignado a la señal DOUT2
#define DOUT1  5   // PIN asignado a la señal DOUT1
#define DOUT4  4   // PIN asignado a la señal DOUT4
#define DOUT3  3   // PIN asignado a la señal DOUT3
#define SCLK   2   // PIN asignado a la señal SCLK

#define AUXILIAR  8

//Modos de Lectura ADS1234...en el ADS 1231 no existen todos.
#define NORMAL                  1 // Modo Normal
#define CALIBRACION             2 // Modo Calibracion
#define STANDBY                 3 // Modo Standby
#define STANDBY_MAS_CALIBRACION 4 // Modo Standby + Calibracion

int flanco = 0 ;

volatile long DOUT[ NADS ]  = {DOUT1, DOUT2, DOUT3, DOUT4}   ; // Carga de PINs asociados a las señales de los ADS

volatile unsigned int *DATO;      // Este es el vector donde se guardan los datos de los ADS.


void setup() {
   
  wdt_disable();    // Desabilita el WDT

  for (int indice = 0 ; indice < NADS ; indice++)
  {
    pinMode(DOUT[indice], INPUT);  // Define los PINs asociados a DOUT como entrada
  }

  pinMode(SCLK, OUTPUT);      // Define SCLK como salida
  pinMode(PWDN, OUTPUT);      // Define PWDN como salida
  pinMode(AUXILIAR, OUTPUT);  // Define SCLK como salida

  //attachInterrupt(1, INTERRUPCION, FALLING);  // Asocia la interrupcion 1 ( 0 = pin2 ; 1 = pin3 ) a la subrutine INTERRUPCION con el flanco de bajada

  wdt_enable(WDTO_1S);    // WDT seteado en 1 Segundo

  /* parameters(long baudrate,
                unsigned char ID,
                unsigned char transmit enable pin,
                unsigned int holding registers size,
                unsigned char low latency)

     The transmit enable pin is used in half duplex communication to activate a MAX485 or similar
     to deactivate this mode use any value < 2 because 0 & 1 is reserved for Rx & Tx.
     Low latency delays makes the implementation non-standard
     but practically it works with all major modbus master implementations.
  */

  modbus_configure(9600, 1, 2, TAMANO_PAQUETE, 0.1);
}


void loop() {
                            

  digitalWrite(PWDN, LOW);         // Se resetean los ADS inicialmente.
  digitalWrite(PWDN, HIGH);

  wdt_reset();                     // Se atiende la interrupcion del WDT.  

  interrupts();                    // se habilitan las interrupciones.

int  dummy = 0;
  
   while (1)
  {
    wdt_reset();                   // Se atiende la interrupcion del WDT.
  
    modbus_update(DATO);           // Este procedimiento actualiza el registro que usa
                                   // la subrutina de SimpleModbusSlave.h para comunicarse
  
    DATO[1]=dummy;
    DATO[2]=dummy*2;
    DATO[3]=dummy*4;
    DATO[4]=dummy*6;

    if (dummy == 10)
    {
      dummy=0;
      }
  dummy++;
  
  //delay(1000);     
    
  }

}



void INTERRUPCION()
{

  flanco++;

  digitalWrite(AUXILIAR, HIGH) ;    //Señal auxiliar para medir la interrupción.
  
                                    // El primer flanco descendente es solo para avisar que el dato ya está disponible.
                                    // En el segundo flanco descendete recien se puede leer el dato del ADS
    if ( flanco == 2)
  {                                    
   
    LEE_ADS123X( DATO , NORMAL );   // Se leen los 4 ADS al mismo tiempo 
    
    flanco = 0;
    
  }
  
  digitalWrite(AUXILIAR, LOW) ;    //Señal auxiliar para medir la interrupción.
}



int LEE_ADS123X(volatile unsigned int *DATO, int MODO)
{

  noInterrupts();                   // deshabilita las interrupciones


  for (int indice = 0; indice < NADS ; indice++) {
    
    DATO[indice] =  0;              // SETEA a cero los datos de los ADS
  }

  
  //                Lectura de los puertos bit a bit
  //------------------------------------------------------------------------

  for ( int indice = 0 ; indice < NumeroDeBits ; indice++ )
  {
                                                      // pulso alto para iniciar la salida de un BIT
    digitalWrite(SCLK, LOW) ;                         //     ____
    digitalWrite(SCLK, HIGH);                         // ___|

    for (int indice2 = 0; indice2 < NADS ; indice2++)
    {
      DATO[indice2] = (DATO[indice2] | digitalRead (DOUT[indice2])) << 1; // Lee bit a bit comenzando con el MSB
                                                                          // Corriendo un ligar a la izq cada vez que llega un bit nuevo
    }

  }
  //------------------------------------------------------------------------
  //


  switch (MODO) {

    case NORMAL:
      
      digitalWrite(SCLK, LOW);                    // Completa el pulso #24

      digitalWrite(SCLK, HIGH);                   // pulso #25 para forzar a los DOUT-> HIGH (Figura 35 ADS1234.pdf)
      digitalWrite(SCLK, LOW);

      break;

    case CALIBRACION:

      digitalWrite(SCLK, LOW);                    // Completa el pulso #24
      
      digitalWrite(SCLK, HIGH);                   // pulso #25
      digitalWrite(SCLK, LOW);

      digitalWrite(SCLK, HIGH);                   // pulso #26 para forzar la calibraci�n del ADS a los DOUT-> HIGH (Figura 36 ADS1234.pdf)
      digitalWrite(SCLK, LOW);

      break;

    case STANDBY:

      // No Completa el pulso #24 (Figura 37 ADS1234.pdf)
      
      break;


    case STANDBY_MAS_CALIBRACION:

      
      digitalWrite(SCLK, LOW);                  // Completa el pulso #24

      digitalWrite(SCLK, HIGH);                 //pulso #25
      digitalWrite(SCLK, LOW);
     
      digitalWrite(SCLK, HIGH);                 //pulso #26 para forzar la calibraci�n del ADS a los DOUT-> HIGH (Figura 38 ADS1234.pdf)

      break;

    default:  // Igual que modo NORMAL

      digitalWrite(SCLK, LOW);                 // Completa el pulso #24

      digitalWrite(SCLK, HIGH);                //pulso #25 para forzar a los DOUT-> HIGH (Figura 35 ADS1234.pdf) 
      digitalWrite(SCLK, LOW);

      break;

  }

  modbus_update(DATO);                        // Carga los datos en los registros para trasmitir via MODBUS
                                              // realmente no hace falta porque los datos ya estan cargados.
 
  EIFR = 0x01;                                // Resetea Flags de interrupcion que quedan seteados
  
  interrupts();                               // habilita las interrupciones
  
  return 1;
  
}


