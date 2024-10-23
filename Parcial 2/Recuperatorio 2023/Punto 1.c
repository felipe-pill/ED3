/*
Por un pin del ADC del microcontrolador LPC1769 ingresa una tensión de rango dinámico 0 a 3,3[V]
proveniente de un sensor de temperatura. Debido a la baja tasa de variación de la señal, se pide tomar una 
muestra cada 30[s]. Pasados los 2[min] se debe promediar las últimas 4 muestras y en función de este valor, 
tomar una decisión sobre una salida digital de la placa:
● Si el valor es <1 [V] colocar la salida en 0 (0[V]).
● Si el valor es >= 1[V] y <=2[V] modular una señal PWM con un Ciclo de trabajo que va desde el 50% 
hasta el 90% proporcional al valor de tensión, con un periodo de 20[KHz]. 
● Si el valor es > 2[V] colocar la salida en 1 (3,3[V]).
*/
#include "LPC1769XX.H"
#include "lpc17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

void configPINS(void);
void configADC(void);
void configT0(void);
void configT1(uint16_t dutty_actual);

#define BUFFER_SIZE 4


uint8_t t_alto_PWM;
uint8_t t_bajo_PWM;
uint8_t counter;
uint16_t buffer_ADC[BUFFER_SIZE];

int main(void) {
    configPINS();
    configADC();
    configT0();

    counter = 0;
  
    while (1) {
        // El flujo principal está controlado por las interrupciones.
    }

    return 1;
}

void configPINS(void) {
    // Configuración del pin P0.2 para entrada analógica como AD0.7
    PINSEL_CFG_Type pinADC;
    pinADC.Portnum = 0;
    pinADC.Pinnum = 2;
    pinADC.Funcnum = 2;    
    PINSEL_ConfigPin(&pinADC);

    // Configuración del pin P0.0 para salida digital
    PINSEL_CFG_Type pinSalida;
    pinSalida.Portnum = 0;
    pinSalida.Pinnum = 0;
    pinSalida.Funcnum = 0;    
    PINSEL_ConfigPin(&pinSalida);

    // Definir GPIO P0.0 como salida
    GPIO_SetDir(0, 0, 1);
}

void configADC(void) {
    // Configuración del ADC sin modo ráfaga (Burst)
    ADC_Init(LPC_ADC, 200000); // Configurar la tasa de reloj del ADC
    ADC_BurstCmd(LPC_ADC, DISABLE); 
    ADC_ChannelCmd(LPC_ADC, 7, ENABLE); // Habilitar el canal 7
}

void configT0(void) {
    // Configuración del Timer 0 para interrumpir cada 30 segundos
    TIM_TIMERCFG_Type timer0;
    timer0.PrescaleOption = TIM_PRESCALE_USVAL;
    timer0.PrescaleValue = 1000000; // Se cuenta cada 1 segundo

    TIM_MATCHCFG_Type match0;
    match0.MatchChannel = 0;
    match0.IntOnMatch = ENABLE; 
    match0.ResetOnMatch = ENABLE; 
    match0.StopOnMatch = DISABLE;
    match0.MatchValue = 30; // Interrupción cada 30 segundos

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer0);
    TIM_ConfigMatch(LPC_TIM0, &match0);
  
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);
}



void configT1(uint16_t dutty_actual) { 

    // Configuramos el timer 1 para generar PWM

  
    TIM_TIMERCFG_Type timer1;
    timer1.PrescaleOption = TIM_PRESCALE_USVAL;
    timer1.PrescaleValue = 1000000; // Se cuenta cada 1 segundo

    TIM_MATCHCFG_Type match1;
    match1.MatchChannel = 1;
    match1.IntOnMatch = ENABLE; // Que interrumpa para indicar al ADC que debe convertir
    match1.ResetOnMatch = ENABLE; // Que vuelva el TC a cero para volver a contar
    match1.StopOnMatch = DISABLE; // Que no se detenga el timer

    //Periodo de 20K --> 0.00005s la duracion total de la señal    
    uint8_t t_alto_PWM = dutty_actual * 0.00005 / 100; // Calculamos que tiempo va a estar en alto
    uint8_t t_bajo_PWM = 0.00005 - t_alto_PWM; // Calculamos que tiempo va a estar en bajo

    // Arranca en alto, el match corresponde hasta que momento debe mantenerse asi
    match1.MatchValue = t_alto_PWM;
    GPIO_SetValue(0, 0);

    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timer1);
    
    TIM_ConfigMatch(LPC_TIM1, &match2);

    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    NVIC_EnableIRQ(TIMER1_IRQn);

    TIM_Cmd(LPC_TIM2, ENABLE);

}

void TIMER0_IRQHandler(void) {
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

    // Iniciar conversión del ADC
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);

    // Verificar que la conversión haya terminado
    
    while(ADC_ChannelGetStatus(LPC_ADC, 7, ADC_DATA_DONE) != SET) {
               
          }

  
        // Almacenar la muestra en el buffer
        buffer_ADC[counter] = ADC_ChannelGetData(LPC_ADC, 7);
        counter++;

        if (counter >= BUFFER_SIZE) {
            // Calcular el promedio
            int suma = 0;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                suma += buffer_ADC[i];
            }

            float promedio = (float)suma / BUFFER_SIZE;
            voltaje = (promedio * 3.3) / 4095; // Conversión a voltaje

            // Tomar decisiones según el voltaje
            if (voltaje < 1) {
                NVIC_DisableIRQ(TIMER1_IRQn);
                GPIO_ClearValue(0, 0); // Salida en 0V
            } 
            else if (voltaje >= 1 && voltaje <= 2) {
                // PWM proporcional entre 50% y 90%
                duty_cycle = 50 + (voltaje - 1) * 40;
                configT1(duty_cycle);
            } 
            else {
                NVIC_DisableIRQ(TIMER1_IRQn);
                GPIO_SetValue(0, 0); // Salida en 3.3V
            }

            counter = 0; // Reiniciar el contador para las nuevas muestras
        }
    }
}


void TIMER1_IRQHandler(void) {


    if(LPC_TIM1->MR1 == t_alto_PWM){

        TIM_UpdateMatchValue(LPC_TIM2, 1, t_bajo_PWM); 
        GPIO_ClearValue(0, 0);
    }
    else{
        TIM_UpdateMatchValue(LPC_TIM1, 1, t_alto_PWM); 
        GPIO_SetValue(0, 0);
    }

    TIM_ClearIntPending(LPC_TIM2, TIM_MR2_INT);
    TIM_Cmd(LPC_TIM2, ENABLE);

}



}




