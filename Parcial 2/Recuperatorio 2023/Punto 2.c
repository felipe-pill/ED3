/*
Se tienen tres bloques de datos de 4KBytes de longitud cada uno en el cual se han guardado tres formas 
de onda. Cada muestra de la onda es un valor de 32 bits que se ha capturado desde el ADC. Las direcciones de 
inicio de cada bloque son representadas por macros del estilo DIRECCION_BLOQUE_N, con N=0,1,2.
Se pide que, usando DMA y DAC se genere una forma de onda por la salida analógica de la LPC1769. 
La forma de onda cambiará en función de una interrupción externa conectada a la placa de la siguiente 
manera:
● 1er interrupción: Forma de onda almacenada en bloque 0, con frecuencia de señal de 60[KHz].
● 2da interrupción: Forma de onda almacenada en bloque 1 con frecuencia de señal de 120[KHz].
● 3ra interrupción: Forma de onda almacenada en bloque 0 y bloque 2 (una a continuación de la otra) 
con frecuencia de señal de 450[KHz].
● 4ta interrupción: Vuelve a comenzar con la forma de onda del bloque 0.
En cada caso se debe utilizar el menor consumo de energía posible del DAC.
__________________________________________________________________________

*/

#include "lpc17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"

#define BLOCK_SIZE 1024     // Cantidad de muestras (words) en los 4kBy de cada bloque de memoria
#define DIRECCION_BLOQUE_0 0x2007C000
#define DIRECCION_BLOQUE_1 0x2007D000
#define DIRECCION_BLOQUE_2 0x2007E000




void configPINS(void);
void configEINT(void);
void configDAC(void);
void configDMA(void);
uint8_t counter;

int main(void){
    
  
    configEINT();
    configEINT();
    configDAC();
    uint8_t counter = 0;

  
    while(1){

    }
    return 0;
}



void configPINS(void){

  //P2.10 as EINT0
	PINSEL_CFG_Type pinCfg1;
	pinCfg1.Portnum = PINSEL_PORT_2;
	pinCfg1.Pinnum = PINSEL_PIN_10;
	pinCfg1.Funcnum = PINSEL_FUNC_1;
	pinCfg1.Pinmode = PINSEL_PINMODE_PULLUP;
	PINSEL_ConfigPin(&pinCfg1);


  //P0.26 as AOUT

  PINSEL_CFG_Type pinCfg2;
	pinCfg2.Portnum = PINSEL_PORT_0;
	pinCfg2.Pinnum = PINSEL_PIN_26;
	pinCfg2.Funcnum = PINSEL_FUNC_2;
	PINSEL_ConfigPin(&pinCfg2);
}




void configEINT(void){



//EINT0 configuration
	EXTI_InitTypeDef extiCfg;
	extiCfg.EXTI_Line = EXTI_EINT0;
	extiCfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
	extiCfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
	
  EXTI_Config(&extiCfg);
	
  EXTI_ClearEXTIFlag(EXTI_EINT0);
  NVIC_EnableIRQ(EINT0_IRQn);


}


void configDAC(void){



    DAC_CONVERTER_CFG_Type configDAC;
    configDAC.DBLBUF_ENA = DISABLE;
    configDAC.CNT_ENA = ENABLE;
    configDAC.DMA_ENA = ENABLE;

    DAC_Init(LPC_DAC);
    DAC_ConfigDAConverterControl(LPC_DAC, &configDAC);
    DAC_SetBias(LPC_DAC, 1); //Bajo consumo

}



void configDMA(uint8_t bloque){
    
  
  
    //Configuracion del canal 0
  
    GPDMA_Channel_CFG_Type configDMA;
    configDMA.ChannelNum = 0;
    configDMA.TransferSize = BLOCK_SIZE;
    configDMA.TransferWidth = 4;           // Este creo que no iria porque es P2M

    switch (bloque) {
        case 1:
            congifDMA.SrcMemAddres = DIRECCION_BLOQUE_0;
            break;                                                                         
        case 2:
            congifDMA.SrcMemAddres = DIRECCION_BLOQUE_1;
            break;       
        case 3:
            congifDMA.SrcMemAddres = DIRECCION_BLOQUE_2;
            break;       
        default:
            // Manejo opcional de errores o casos imprevistos
            break;
    }
    
  
  
  
    configDMA.TransferType = GPDMA_TRANSFERTYPE_M2P; //no debo poner ni width ni dstmemaddr por ser M2P
    configDMA.DstConn = GPDMA_CONN_DAC; 
    configDMA.TransferType = GPDMA_TRANSFERTYPE_M2P; //no debo poner ni width ni dstmemaddr por ser M2P
    configDMA.DstConn = GPDMA_CONN_DAC; 
    GPDMA_Init();
    GPDMA_Setup(&configDMA);
    GPDMA_ChannelCmd(0, ENABLE);

}







void EINT0_IRQHandler(void){

    // Incrementar el contador, reseteándolo si es mayor que 4
    counter = (counter < 4) ? counter + 1 : 1;

    switch (counter) {
        case 1:
          case 1:
            configDMA(counter);
            DAC_SetDMATimeOut(400 / 60 * BLOCK_SIZE)                                      // 400 kHz
            break;                                                                          // timeout de 6 ms, que es < settlingTime
        case 2:
            configDMA(counter);
            DAC_SetDMATimeOut(400 / 120 * BLOCK_SIZE)                                      // 400 kHz
            break;       
        case 3:
            configDMA(counter);
            DAC_SetDMATimeOut(400000 / 450 * BLOCK_SIZE)                                      // timeout de 868 us
            break;       
        default:
            // Manejo opcional de errores o casos imprevistos
            break;
    }
}









  


