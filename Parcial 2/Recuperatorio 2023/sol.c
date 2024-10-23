#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"

// Memory addresses
#define SRC_ADDRESS 0x10000800
#define DST_ADDRESS 0x10002800

// DMA definitions
#define DMA_CHANNEL_0 0
#define REQUIRED_TRANSFERS 256; // 0x10003000 - 0x10002800 = 0x200 bytes = 512 bytes -> 512 / 2 bytes (transferencias de 16 bits) = 256 transferencias

// Global variables
volatile uint16_t remaining_transfers = REQUIRED_TRANSFERS; // De 16 bits porque son 256 transferencias (8 bits llega a 255)

// Required structures for DMA
GPDMA_Channel_CFG_Type dma;
GPDMA_LLI_Type lli;

void configure_dma(void) {
    lli.SrcAddr = (uint32_t)SRC_ADDRESS;
    lli.DstAddr = (uint32_t)DST_ADDRESS;
    lli.NextLLI = (uint32_t)&lli;
    lli.Control = 1        // 1 transferencia a la vez
                | (1<<17)  // Transferencia a la fuente de 16 bits
                | (1<<21)  // Transferencia de destino de 16 bits
                | (1<<25)  // Incrementa direccion de origen despues de cada transferencia
                | (1<<26)  // Incrementa direccion de destino despues de cada transferencia
                | (1<<30)  // Generar interrupcion al finalizar transferencia y detener transferencia (se reinicia manualmente)

    // Configuración del canal DMA
    dma.ChannelNum = DMA_CHANNEL_0;                  // Canal DMA 0
    dma.TransferSize = 1;                            // 1 transferencia a la vez
    dma.TransferWidth = GPDMA_WIDTH_HALFWORD;        // Transferencias de 16 bits
    dma.SrcMemAddr = (uint32_t)SRC_ADDRESS;          // Dirección de origen
    dma.DstMemAddr = (uint32_t)DST_ADDRESS;          // Dirección de destino
    dma.TransferType = GPDMA_TRANSFERTYPE_M2M;       // Transferencia memoria a memoria
    dma.DMALLI = (uint32_t)&lli;                     // Linked List Item
    dma.SrcConn = 0;                                 // No hay periférico de origen
    dma.DstConn = 0;                                 // No hay periférico de destino
    
    // Configurar el DMA
    GPDMA_Setup(&dma_cfg);
    
    // Iniciar modulo DMA
    GPDMA_Init();

    // Habilita interrupcion del DMA
    NVIC_EnableIRQ(DMA_IRQn);
    // Transferencia > Orden de transferir otro dato
    NVIC_SetPriority(DMA_IRQn, 0);
}

// Configurar Timer1 y Match0 para generar un evento periódico
void configure_timer1(void) {
    TIM_TIMERCFG_Type timer_cfg;
    TIM_MATCHCFG_Type match_cfg;

    // Configuración básica del timer
    timer_cfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timer_cfg.PrescaleValue = 1;  // El contador aumenta cada 1 us
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timer_cfg);

    // Configurar Match0 para generar eventos
    match_cfg.MatchChannel = 0;
    match_cfg.IntOnMatch = ENABLE;
    match_cfg.ResetOnMatch = ENABLE;
    match_cfg.StopOnMatch = ENABLE; // Cuando se llega al Match, se reinicia y detiene la cuenta hasta finalizar la transferencia
    match_cfg.MatchValue = 1000; // Generar evento cada 1 ms (1 us * 1000)
    TIM_ConfigMatch(LPC_TIM1, &match_cfg);

    // Habilita interrupcion del Timer1
    NVIC_EnableIRQ(Timer1_IRQn);
    // Transferencia > Orden de transferir otro dato
    NVIC_SetPriority(Timer1_IRQn, 3);
}

void TIMER1_IRQHandler(void) {
    // Reiniciar transferencia por DMA
    GPDMA_ChannelCmd(0, ENABLE);
    // Limpiar flag de interrupcion Timer1
    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT); 
}

void DMA_IRQHandler() {
    remaining_transfers--;
    // Si quedan transferencias por hacer, se vuelve a activar el Timer
    if (remaining_transfers != 0) {
      // Limpiar flag de interrupcion DMA
      GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, DMA_CHANNEL_0);
      // Iniciar Timer1
      TIM_Cmd(LPC_TIM1, ENABLE);
    }
}

int main(void) {
    // Iniciar system clk
    SystemInit();

    // Configurar el Timer1 y el DMA
    configure_timer1();
    configure_dma();
   
    // Iniciar Timer1
    TIM_Cmd(LPC_TIM1, ENABLE);

    while (1) {
        // El DMA se encargará de la transferencia
    }

    return 0;
}
