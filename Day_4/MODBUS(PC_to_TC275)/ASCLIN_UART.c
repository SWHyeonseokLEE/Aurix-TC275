#include "IfxAsclin_Asc.h"
#include "IfxCpu_Irq.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define UART_BAUDRATE           115200                                  /* UART baud rate in bit/s                  */

#define UART_PIN_RX             IfxAsclin0_RXB_P15_3_IN                 /* UART receive port pin                    */
#define UART_PIN_TX             IfxAsclin0_TX_P15_3_OUT                 /* UART transmit port pin                   */

/* Definition of the interrupt priorities */
#define INTPRIO_ASCLIN0_RX      18
#define INTPRIO_ASCLIN0_TX      19

#define UART_RX_BUFFER_SIZE     64                                      /* Definition of the receive buffer size    */
#define UART_TX_BUFFER_SIZE     64                                      /* Definition of the transmit buffer size   */
#define SIZE                    13                                      /* Size of the string                       */
#define MAX_RCV_BUFFER_SIZE     20

union
{
    uint16 check_sum;
    uint8 b[2];
} crc_16_val;

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* Declaration of the ASC handle */
static IfxAsclin_Asc g_ascHandle;

/* Declaration of the FIFOs parameters */
static uint8 g_ascTxBuffer[UART_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
static uint8 g_ascRxBuffer[UART_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];

/* Definition of txData and rxData */
uint8 g_txData[] = "Hello World!";
uint8 g_rxData[SIZE] = {''};
uint8 g_rxRcvBuffer[MAX_RCV_BUFFER_SIZE] = {''};
uint8 protocal_test[8] = { 0, };


/* Size of the message */
Ifx_SizeT g_count = sizeof(g_txData);
Ifx_SizeT s_count = sizeof(protocal_test);

int crc16_MODBUS(const uint8*nData, uint16 wLength);
int cnt_rcv = 0;
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Adding of the interrupt service routines */
IFX_INTERRUPT(asclin0TxISR, 0, INTPRIO_ASCLIN0_TX);
void asclin0TxISR(void)
{
    IfxAsclin_Asc_isrTransmit(&g_ascHandle);
}

IFX_INTERRUPT(asclin0RxISR, 0, INTPRIO_ASCLIN0_RX);
void asclin0RxISR(void)
{
    IfxAsclin_Asc_isrReceive(&g_ascHandle);
}

/* This function initializes the ASCLIN UART module */
void init_ASCLIN_UART(void)
{
    /* Initialize an instance of IfxAsclin_Asc_Config with default values */
    IfxAsclin_Asc_Config ascConfig;
    IfxAsclin_Asc_initModuleConfig(&ascConfig, &MODULE_ASCLIN0);

    /* Set the desired baud rate */
    ascConfig.baudrate.baudrate = UART_BAUDRATE;

    /* ISR priorities and interrupt target */
    ascConfig.interrupt.txPriority = INTPRIO_ASCLIN0_TX;
    ascConfig.interrupt.rxPriority = INTPRIO_ASCLIN0_RX;
    ascConfig.interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    /* FIFO configuration */
    ascConfig.txBuffer = &g_ascTxBuffer;
    ascConfig.txBufferSize = UART_TX_BUFFER_SIZE;
    ascConfig.rxBuffer = &g_ascRxBuffer;
    ascConfig.rxBufferSize = UART_RX_BUFFER_SIZE;

    /* Pin configuration */
    const IfxAsclin_Asc_Pins pins =
    {
        NULL_PTR,       IfxPort_InputMode_pullUp,     /* CTS pin not used */
        &UART_PIN_RX,   IfxPort_InputMode_pullUp,     /* RX pin           */
        NULL_PTR,       IfxPort_OutputMode_pushPull,  /* RTS pin not used */
        &UART_PIN_TX,   IfxPort_OutputMode_pushPull,  /* TX pin           */
        IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    ascConfig.pins = &pins;

    IfxAsclin_Asc_initModule(&g_ascHandle, &ascConfig); /* Initialize module with above parameters */
}

int CRC16_MODBUS(const uint8 *nData, uint16 wLength)
{

    static const uint16 wCRCTable[] = { 0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241, 0XC601, 0X06C0, 0X0780,
        0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440, 0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1,
        0XCE81, 0X0E40, 0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841, 0XD801,
        0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40, 0X1E00, 0XDEC1, 0XDF81, 0X1F40,
        0XDD01, 0X1DC0, 0X1C80, 0XDC41, 0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680,
        0XD641, 0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040, 0XF001, 0X30C0,
        0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240, 0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501,
        0X35C0, 0X3480, 0XF441, 0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
        0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840, 0X2800, 0XE8C1, 0XE981,
        0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41, 0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1,
        0XEC81, 0X2C40, 0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640, 0X2200,
        0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041, 0XA001, 0X60C0, 0X6180, 0XA141,
        0X6300, 0XA3C1, 0XA281, 0X6240, 0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480,
        0XA441, 0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41, 0XAA01, 0X6AC0,
        0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840, 0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01,
        0X7BC0, 0X7A80, 0XBA41, 0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
        0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640, 0X7200, 0XB2C1, 0XB381,
        0X7340, 0XB101, 0X71C0, 0X7080, 0XB041, 0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0,
        0X5280, 0X9241, 0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440, 0X9C01,
        0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40, 0X5A00, 0X9AC1, 0X9B81, 0X5B40,
        0X9901, 0X59C0, 0X5880, 0X9841, 0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81,
        0X4A40, 0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41, 0X4400, 0X84C1,
        0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641, 0X8201, 0X42C0, 0X4380, 0X8341, 0X4100,
        0X81C1, 0X8081, 0X4040 };

    uint8 nTemp;
    uint16 wCRCWord = 0xFFFF;

    while (wLength--)
    {
        nTemp = *nData++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }

    return wCRCWord;
}

void MODBUS(void)
{
    int i = 0;

    protocal_test[0] = 174;                 //PC
    protocal_test[1] = 184;                 //IPD_controller
    protocal_test[2] = 'I';                 // PID
    protocal_test[3] = 0x01;                // data number
    protocal_test[4] = 0xff;                // data number

    crc_16_val.check_sum = CRC16_MODBUS(protocal_test, 5);


    protocal_test[5] = crc_16_val.b[1];
    protocal_test[6] = crc_16_val.b[0];

}

/* This function sends and receives the string "Hello World!" */
void send_ASCLIN_UART_message(void)
{
    MODBUS();
    IfxAsclin_Asc_write(&g_ascHandle, protocal_test, &s_count, TIME_INFINITE);   /* Transmit data via TX */
}

