//*****************************************************************************
// Universidad del Valle de Guatemala
// Digital 2
// Archivo: Lab 2
// Hardware: ATMEGA328P
// Autor: Esteban Longo
// Carnet: 22649
//*****************************************************************************
#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "LCD8bits/LCD8bits.h"
#include "ADC/ADC.h"

// UART Baud rate
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD) - 1)

// Variables globales
volatile uint8_t Val1 = 0, Val2 = 0, caso = 0;
volatile uint8_t contador = 0;
volatile uint8_t updateLCD = 0; // Variable de estado para actualizar la LCD
volatile uint8_t adcUpdated = 0; // Variable de estado para actualizar valores ADC

char lista1[8], lista2[8], lista3[4] = {'0', '0', '0', '0'};

// Prototipos de funciones
void setup(void);
void actualizarVoltaje(char *lista, uint8_t valor);
void enviarUART(char data);
void enviarStringUART(const char* str);
void actualizarLista(char *lista, int valor);
void actualizarLCD(void);

// Configuraci�n inicial del sistema
void setup(void) {
    cli();  // Deshabilitar interrupciones globales

    // Configuraci�n de puertos
    DDRD = 0xFF;  // Puerto D como salida
    DDRB = 0xFF;  // Puerto B como salida
    DDRC = 0;     // Puerto C como entrada

    // Configuraci�n UART
    UBRR0H = (BRC >> 8);  // Configurar baud rate alto
    UBRR0L = BRC;         // Configurar baud rate bajo
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0); // Habilitar transmisi�n, recepci�n e interrupci�n RX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Configurar formato: 8 bits de datos, 1 bit de parada

    // Inicializaci�n de LCD y ADC
    Lcd_Init8bits();  // Inicializar pantalla LCD
    _delay_ms(50);    // Esperar para asegurarse de que la LCD se inicialice correctamente
    Lcd_Clear();      // Limpiar pantalla LCD
    initADC();        // Inicializar ADC
    ADCSRA |= (1 << ADSC);  // Iniciar la primera conversi�n ADC

    sei();  // Habilitar interrupciones globales
}

// Funci�n para convertir un valor ADC a voltaje y actualizar una cadena
void actualizarVoltaje(char *lista, uint8_t valor) {
    float voltaje = (valor * 5.0) / 255.0;
    uint16_t int_part = (uint16_t)voltaje;
    uint16_t dec_part = (uint16_t)((voltaje - int_part) * 100);  // Dos decimales

    if (int_part < 10) {
        lista[0] = '0' + int_part;
        lista[1] = '.';
        lista[2] = '0' + (dec_part / 10);
        lista[3] = '0' + (dec_part % 10);
        lista[4] = 'V';
        lista[5] = '\0';
    } else {
        lista[0] = '0' + (int_part / 10);
        lista[1] = '0' + (int_part % 10);
        lista[2] = '.';
        lista[3] = '0' + (dec_part / 10);
        lista[4] = '0' + (dec_part % 10);
        lista[5] = 'V';
        lista[6] = '\0';
    }
}

// Funci�n para actualizar una cadena con un valor de 3 d�gitos
void actualizarLista(char *lista, int valor) {
    lista[0] = '0' + (valor / 100);
    lista[1] = '0' + ((valor / 10) % 10);
    lista[2] = '0' + (valor % 10);
    lista[3] = '\0';
}

// Funci�n para enviar un dato por UART
void enviarUART(char data) {
    while (!(UCSR0A & (1 << UDRE0)));  // Esperar a que el buffer est� vac�o
    UDR0 = data;  // Enviar dato
}

// Funci�n para enviar una cadena de caracteres por UART
void enviarStringUART(const char* str) {
    while (*str) {
        enviarUART(*str++);
    }
}

// Funci�n para actualizar la pantalla LCD
void actualizarLCD(void) {
    Lcd_Clear();  // Limpiar pantalla
    Lcd_Set_Cursor(0, 2);
    Lcd_Write_String("S1:");  // Escribir etiqueta de Sensor 1
    Lcd_Set_Cursor(0, 8);
    Lcd_Write_String("S2:");  // Escribir etiqueta de Sensor 2
    Lcd_Set_Cursor(0, 13);
    Lcd_Write_String("S3:");  // Escribir etiqueta de Sensor 3

    // Actualizar las cadenas con los valores actuales
    actualizarVoltaje(lista1, Val1);
    actualizarVoltaje(lista2, Val2);
    actualizarLista(lista3, contador);

    // Mostrar los valores en la LCD
    Lcd_Set_Cursor(1, 1);
    Lcd_Write_String(lista1);
    Lcd_Set_Cursor(1, 7);
    Lcd_Write_String(lista2);
    Lcd_Set_Cursor(1, 13);
    Lcd_Write_String(lista3);
}

// Funci�n principal
int main(void) {
    setup();  // Configuraci�n inicial del sistema
    
    // Variables para guardar los valores anteriores
    uint8_t prevVal1 = 255, prevVal2 = 255, prevContador = 255;

    while (1) {
        // Verificar si hay cambios en los valores del ADC o el contador
        if ((Val1 != prevVal1) || (Val2 != prevVal2) || (contador != prevContador) || updateLCD) {
            actualizarLCD();  // Actualizar la pantalla LCD
            // Guardar los valores actuales como anteriores
            prevVal1 = Val1;
            prevVal2 = Val2;
            prevContador = contador;
            adcUpdated = 0;
            updateLCD = 0;
        }

        // Enviar los valores actuales por UART
        enviarStringUART("S1: ");
        enviarStringUART(lista1);
        enviarStringUART(" S2: ");
        enviarStringUART(lista2);
        enviarStringUART(" S3: ");
        enviarStringUART(lista3);
        enviarUART('\n');
        
        _delay_ms(100);  // Esperar 100ms
    }
}

// Interrupci�n del ADC
ISR(ADC_vect) {
    if (caso == 0) {
        ADMUX &= ~((1 << MUX2) | (1 << MUX1) | (1 << MUX0)); // Seleccionar canal ADC0
        Val1 = ADCH;  // Leer valor alto del ADC
        caso = 1;  // Cambiar a caso 1
    } else {
        ADMUX = (ADMUX & ~((1 << MUX2) | (1 << MUX1) | (1 << MUX0))) | (1 << MUX0); // Seleccionar canal ADC1
        Val2 = ADCH;  // Leer valor alto del ADC
        caso = 0;  // Cambiar a caso 0
    }
    ADCSRA |= (1 << ADSC);  // Iniciar la pr�xima conversi�n ADC
    adcUpdated = 1; // Indicar que se debe actualizar la LCD
}

// Interrupci�n UART para recibir datos
ISR(USART_RX_vect) {
    char received = UDR0;  // Leer dato recibido
    if (received == '+') {
        if (contador < 255) {
            contador++;  // Incrementar contador si es menor a 255
        }
    } else if (received == '-') {
        if (contador > 0) {
            contador--;  // Decrementar contador si es mayor a 0
        }
    }
    updateLCD = 1; // Indicar que se debe actualizar la LCD
}
