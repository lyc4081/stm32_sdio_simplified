#include "misc.h"
#include "sdio_f1.h"
#include <stdbool.h>

typedef unsigned long u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

static struct {
    u32_t type, rca, size, cid[4], csd[4];  // size in kbytes
} g;

enum {
    SD_R6_GENERAL_UNKNOWN_ERROR = 0x2000, SD_R6_ILLEGAL_CMD = 0x4000,
    SD_R6_COM_CRC_FAILED = 0x8000, SD_VOLTAGE_WINDOW_SD = 0x80100000,
    SD_HIGH_CAPACITY = 0x40000000, SD_STD_CAPACITY = 0x0,
    SD_CHECK_PATTERN = 0x1AA, SD_MAX_VOLT_TRIAL = 0xffff, SD_ALLZERO = 0x0,
    SD_WIDE_BUS_SUPPORT = 0x40000, SD_SINGLE_BUS_SUPPORT = 0x10000,
    SD_CARD_LOCKED = 0x2000000, SD_CARD_PROGRAMMING = 0x7,
    SD_CARD_RECEIVING = 0x6, SD_DATATIMEOUT = 0xfffff, SD_0TO7BITS = 0xff,
    SD_8TO15BITS = 0xff00, SD_16TO23BITS = 0xff0000, SD_24TO31BITS = 0xff000000,
    SD_MAX_DATA_LENGTH = 0x1ffffff, SD_HALffIFO = 0x8, SD_HALffIFOBYTES = 0x20,
    /* Command Class Supported */
    SD_CCCC_LOCK_UNLOCK = 0x80,
    SD_CCCC_WRITE_PROT = 0x40, SD_CCCC_ERASE = 0x20,
};

enum {
    SD_CCC_SWITCH = 0x400, /* class 10 */
    SD_CCC_IO_MODE = 0x200, /* class 9 */
    SD_CCC_APPLICATION_SPECIFIC = 0x100, /* class 8 */
    SD_CCC_LOCK_CARD = 0x80, /* class 7 */
    SD_CCC_WRITE_PROTECTION = 0x40, /* class 6 */
    SD_CCC_ERASE = 0x20, /* class 5 */
    SD_CCC_BLOCK_WRITE = 0x10, /* class 4 */
    SD_CCC_BLOCK_READ = 0x4, /* class 2 */
    SD_CCC_BASIC = 0x1 /* class 0 */
};

enum {
    CMD0 = 0, CMD1 = 1, CMD2 = 2, CMD3 = 3, CMD4 = 4, CMD5 = 5, CMD6 = 6,
    CMD7 = 7, CMD8 = 8, CMD9 = 9, CMD10 = 10, CMD11 = 11, CMD12 = 12,
    CMD13 = 13, CMD14 = 14, CMD15 = 15, CMD16 = 16, CMD17 = 17, CMD18 = 18,
    CMD19 = 19, CMD20 = 20, CMD24 = 24, CMD25 = 25, CMD26 = 26, CMD27 = 27,
    CMD28 = 28, CMD29 = 29, CMD30 = 30, CMD32 = 32, CMD33 = 33, CMD35 = 35,
    CMD36 = 36, CMD38 = 38, CMD39 = 39, CMD40 = 40, CMD42 = 42, CMD55 = 55,
    CMD56 = 56, CMD64 = 64, ACMD6 = 6, ACMD13 = 13, ACMD22 = 22, ACMD23 = 23,
    ACMD41 = 41, ACMD42 = 42, ACMD51 = 51, ACMD52 = 52, ACMD53 = 53,
    ACMD43 = 43, ACMD44 = 44, ACMD45 = 45, ACMD46 = 46, ACMD47 = 47,
    ACMD48 = 48, ACMD18 = 18, ACMD25 = 25, ACMD38 = 38, ACMD49 = 49
};

enum {
    SDTYPE_SDSC_V1_1 = 0x0, SDTYPE_SDSC_V2_0 = 0x1, SDTYPE_SDHC = 0x2,
    SDTYPE_MMC = 0x3, SDTYPE_SDIO = 0x4, SDTYPE_HSMMC = 0x5,
    SDTYPE_SDIO_COMBO = 0x6, SDTYPE_HCMMC = 0x7
};

enum {
    SD_OCR_ADDR_OUT_OF_RANGE = 0x80000000, SD_OCR_ADDR_MISALIGNED = 0x40000000,
    SD_OCR_BLOCK_LEN_ERR = 0x20000000, SD_OCR_ERASE_SEQ_ERR = 0x10000000,
    SD_OCR_BAD_ERASE_PARAM = 0x8000000, SD_OCR_WRITE_PROT_VIOLATION = 0x4000000,
    SD_OCR_LOCK_UNLOCK_FAILED = 0x1000000, SD_OCR_COM_CRC_FAILED = 0x800000,
    SD_OCR_ILLEGAL_CMD = 0x400000, SD_OCR_CARD_ECC_FAILED = 0x200000,
    SD_OCR_CC_ERROR = 0x100000, SD_OCR_GENERAL_UNKNOWN_ERROR = 0x80000,
    SD_OCR_STREAM_READ_UNDERRUN = 0x40000,
    SD_OCR_STREAM_WRITE_OVERRUN = 0x20000, SD_OCR_CID_CSD_OVERWRIETE = 0x10000,
    SD_OCR_WP_ERASE_SKIP = 0x8000, SD_OCR_CARD_ECC_DISABLED = 0x4000,
    SD_OCR_ERASE_RESET = 0x2000, SD_OCR_AKE_SEQ_ERROR = 0x8,
    SD_OCR_ERRORBITS = 0xFDffE008
};

const struct {
    u32_t key;
    u32_t val;
} err_lut[] = { {SD_OCR_ADDR_OUT_OF_RANGE, SD_ADDR_OUT_OF_RANGE}, {
        SD_OCR_ADDR_MISALIGNED, SD_ADDR_MISALIGNED}, {SD_OCR_BLOCK_LEN_ERR,
        SD_BLOCK_LEN_ERR}, {SD_OCR_ERASE_SEQ_ERR, SD_ERASE_SEQ_ERR}, {
        SD_OCR_BAD_ERASE_PARAM, SD_BAD_ERASE_PARAM}, {
        SD_OCR_WRITE_PROT_VIOLATION, SD_WRITE_PROT_VIOLATION}, {
        SD_OCR_LOCK_UNLOCK_FAILED, SD_LOCK_UNLOCK_FAILED}, {
        SD_OCR_COM_CRC_FAILED, SD_COM_CRC_FAILED}, {SD_OCR_ILLEGAL_CMD,
        SD_ILLEGAL_CMD}, {SD_OCR_CARD_ECC_FAILED, SD_CARD_ECC_FAILED}, {
        SD_OCR_CC_ERROR, SD_CC_ERROR}, {SD_OCR_GENERAL_UNKNOWN_ERROR,
        SD_GENERAL_UNKNOWN_ERROR}, {SD_OCR_STREAM_READ_UNDERRUN,
        SD_STREAM_READ_UNDERRUN}, {SD_OCR_STREAM_WRITE_OVERRUN,
        SD_STREAM_WRITE_OVERRUN}, {SD_OCR_CID_CSD_OVERWRIETE,
        SD_CID_CSD_OVERWRITE}, {SD_OCR_WP_ERASE_SKIP, SD_WP_ERASE_SKIP}, {
        SD_OCR_CARD_ECC_DISABLED, SD_CARD_ECC_DISABLED}, {SD_OCR_ERASE_RESET,
        SD_ERASE_RESET}, {SD_OCR_AKE_SEQ_ERROR, SD_AKE_SEQ_ERROR}};

#define SDIO_STATIC_FLAGS           ((u32_t)0x5ff)
#define SDIO_CMD0TIMEOUT            10000
#define SDIO_INIT_CLK_DIV           178
#define SDIO_TRANSFER_CLK_DIV       1
#define CMD_EX_DEFAULT              (SDIO_CPSM_Enable | SDIO_Response_Short)
#define CMD_CLEAR_MASK              (0xfffff800UL)
#define DCTRL_CLEAR_MASK            ((u32_t)0xffffff08)

static void SDIO_SendCmdEx(u8_t cmd, u32_t arg, u32_t options)
{
    u32_t tmp;
    SDIO->ARG = arg;
    tmp = (SDIO->CMD & CMD_CLEAR_MASK) | cmd | options;
    SDIO->CMD = tmp;
    ( {  while (SDIO->STA & SDIO_FLAG_CMDACT);});
}

static SD_Error IsCardProgramming(u8_t* pstatus)
{
    SD_Error ret = SD_OK;
    u32_t respR1 = 0, status = 0;
    SDIO_SendCmdEx(CMD13, g.rca << 16, CMD_EX_DEFAULT);
    do {
        status = SDIO->STA;
    }while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)));
    if(status & SDIO_FLAG_CTIMEOUT) {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(status & SDIO_FLAG_CCRCFAIL) {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
        return SD_CMD_CRC_FAIL;
    }
    status = (u32_t)SDIO_GetCommandResponse();
    if(status != CMD13)
        return SD_ILLEGAL_CMD;
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
    respR1 = SDIO_GetResponse(SDIO_RESP1);
    *pstatus = (u8_t)((respR1 >> 9) & 0x0000000F);
    if((respR1 & SD_OCR_ERRORBITS) == SD_ALLZERO) {
        return (ret);
    }
    for(int i = 0; i < sizeof(err_lut) / sizeof(err_lut[0]); i++) {
        if(respR1 & err_lut[i].key)
            return err_lut[i].val;
    }
    return (ret);
}

void SDIO_DataCfgEx(u32_t datalength, u32_t blocksize, u32_t dir, u32_t dpsm)
{
    SDIO->DTIMER = SD_DATATIMEOUT;
    SDIO->DLEN = datalength;
    SDIO->DCTRL = (SDIO->DCTRL & DCTRL_CLEAR_MASK)
            | (blocksize | dir | SDIO_TransferMode_Block | dpsm);
}

static void SDIO_SetClockDiv(u32_t clkdiv)
{
    clkdiv &= 0xff;
    SDIO->CLKCR = (SDIO->CLKCR & 0xffffff00) | clkdiv;
}
static void SDIO_SetBusWidth(u32_t buswidth)
{
    buswidth &= (_BV(12) | _BV(11));
    SDIO->CLKCR = (SDIO->CLKCR & ~(_BV(12) | _BV(11))) | buswidth;
}
static SD_Error CmdError(void)
{
    SD_Error ret = SD_OK;
    u32_t timeout;
    timeout = SDIO_CMD0TIMEOUT; /* 10000 */
    while((timeout > 0) && (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) == RESET))
        timeout--;
    if(timeout == 0) {
        ret = SD_CMD_RSP_TIMEOUT;
        return (ret);
    }
    /* Clear all the static flags */
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
    return (ret);
}
static SD_Error CmdResp7Error(void)
{
    SD_Error ret = SD_OK;
    u32_t status;
    u32_t timeout = SDIO_CMD0TIMEOUT;
    status = SDIO->STA;
    while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))
            && (timeout > 0)) {
        timeout--;
        status = SDIO->STA;
    }
    if((timeout == 0) || (status & SDIO_FLAG_CTIMEOUT)) {
        /* Card is not V2.0 complient or card does not support the set voltage range */
        ret = SD_CMD_RSP_TIMEOUT;
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return (ret);
    }
    if(status & SDIO_FLAG_CMDREND) {
        /* Card is SD V2.0 compliant */
        ret = SD_OK;
        SDIO_ClearFlag(SDIO_FLAG_CMDREND);
        return (ret);
    }
    return (ret);
}
static SD_Error CmdResp1Error(u8_t cmd)
{
    SD_Error ret = SD_OK;
    u32_t status;
    u32_t response_r1;
    status = SDIO->STA;
    while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))) {
        status = SDIO->STA;
    }
    if(status & SDIO_FLAG_CTIMEOUT) {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(status & SDIO_FLAG_CCRCFAIL) {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
        return SD_CMD_CRC_FAIL;
    }
    /* Check response received is of desired command */
    if(SDIO_GetCommandResponse() != cmd)
        return SD_ILLEGAL_CMD;
    SDIO_ClearFlag(SDIO_STATIC_FLAGS); /* Clear all the static flags */
    /* We have received response, retrieve it for analysis  */
    response_r1 = SDIO_GetResponse(SDIO_RESP1);
    if((response_r1 & SD_OCR_ERRORBITS) == SD_ALLZERO)
        return (ret);
    for(int i = 0; i < sizeof(err_lut) / sizeof(err_lut[0]); i++) {
        if(response_r1 & err_lut[i].key)
            return err_lut[i].val;
    }
    return (ret);
}
static SD_Error CmdResp3Error(void)
{
    SD_Error ret = SD_OK;
    u32_t status;
    status = SDIO->STA;
    while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT))) {
        status = SDIO->STA;
    }
    if(status & SDIO_FLAG_CTIMEOUT) {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }
    /* Clear all the static flags */
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
    return (ret);
}

SD_Error SD_PowerON(void)
{
    SD_Error ret = SD_OK;
    u32_t response = 0, count = 0;
    bool validvoltage = false;
    u32_t SDType = SD_STD_CAPACITY;
    SDIO_DeInit();
    SDIO_SetClockDiv(SDIO_INIT_CLK_DIV);
    SDIO_SetPowerState(SDIO_PowerState_ON);
    SDIO_ClockCmd(ENABLE);
    SDIO_SendCmdEx(CMD0, 0x0, SDIO_CPSM_Enable);    // CMD0: GO_IDLE_STATE
    ret = CmdError();
    if(ret != SD_OK)
        return (ret);
    SDIO_SendCmdEx(CMD8, SD_CHECK_PATTERN, CMD_EX_DEFAULT); // CMD8: SEND_IF_COND
    ret = CmdResp7Error();
    if(ret == SD_OK) {
        g.type = SDTYPE_SDSC_V2_0; /* SD Card 2.0 */
        SDType = SD_HIGH_CAPACITY;
    }
    else {
        SDIO_SendCmdEx(CMD55, 0x0, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD55);
    }
    SDIO_SendCmdEx(CMD55, 0x0, CMD_EX_DEFAULT);
    ret = CmdResp1Error(CMD55);
// TimeOut:  MMC card; SD_OK: SD card 2.0 (voltage range mismatch) or SD card 1.x
    if(ret == SD_OK) {
        while((!validvoltage) && (count < SD_MAX_VOLT_TRIAL)) {
            SDIO_SendCmdEx(CMD55, 0x0, CMD_EX_DEFAULT);
            ret = CmdResp1Error(CMD55);
            if(ret != SD_OK)
                return (ret);
            SDIO_SendCmdEx(ACMD41, SD_VOLTAGE_WINDOW_SD | SDType,
            CMD_EX_DEFAULT);
            ret = CmdResp3Error();
            if(ret != SD_OK)
                return (ret);
            response = SDIO_GetResponse(SDIO_RESP1);
            validvoltage = (bool)(((response >> 31) == 1) ? 1 : 0);
            count++;
        }
        if(count >= SD_MAX_VOLT_TRIAL)
            return SD_INVALID_VOLTRANGE;
        if(response &= SD_HIGH_CAPACITY)
            g.type = SDTYPE_SDHC;
    }                           // else MMC Card
    return (ret);
}

SD_Error SD_PowerOff(void)
{
    SDIO_SetPowerState(SDIO_PowerState_OFF);
    return SD_OK;
}
static SD_Error CmdResp2Error(void)
{
    SD_Error ret = SD_OK;
    u32_t status;
    status = SDIO->STA;
    while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND))) {
        status = SDIO->STA;
    }
    if(status & SDIO_FLAG_CTIMEOUT) {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(status & SDIO_FLAG_CCRCFAIL) {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
        return SD_CMD_CRC_FAIL;
    }
    SDIO_ClearFlag(SDIO_STATIC_FLAGS); /* Clear all the static flags */
    return (ret);
}
static SD_Error CmdResp6Error(u8_t cmd, u16_t * prca)
{
    SD_Error ret = SD_OK;
    u32_t status;
    u32_t resp_r1;
    status = SDIO->STA;
    while(!(status
            & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND))) {
        status = SDIO->STA;
    }
    if(status & SDIO_FLAG_CTIMEOUT) {
        SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
        return SD_CMD_RSP_TIMEOUT;
    }
    else if(status & SDIO_FLAG_CCRCFAIL) {
        SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
        return SD_CMD_CRC_FAIL;
    }
    if(SDIO_GetCommandResponse() != cmd) // Check response received is of desired command
        return SD_ILLEGAL_CMD;
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);  // Clear all the static flags
    resp_r1 = SDIO_GetResponse(SDIO_RESP1); // received response, retrieve it.  */
    if((resp_r1
            & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD
                    | SD_R6_COM_CRC_FAILED)) == SD_ALLZERO) {
        *prca = (u16_t)(resp_r1 >> 16);
        return (ret);
    }
    if(resp_r1 & SD_R6_GENERAL_UNKNOWN_ERROR)
        return (SD_GENERAL_UNKNOWN_ERROR);
    if(resp_r1 & SD_R6_ILLEGAL_CMD)
        return (SD_ILLEGAL_CMD);
    if(resp_r1 & SD_R6_COM_CRC_FAILED)
        return (SD_COM_CRC_FAILED);
    return (ret);
}

SD_Error SD_InitializeCards(void)
{
    SD_Error ret = SD_OK;
    u16_t rca = 0x1;
    if(SDIO_GetPowerState() == SDIO_PowerState_OFF)
        return SD_REQUEST_NOT_APPLICABLE;
    if(SDTYPE_SDIO != g.type) {
        SDIO_SendCmdEx(CMD2, 0x0, SDIO_Response_Long | SDIO_CPSM_Enable); /* Send CMD2 ALL_SEND_CID */
        ret = CmdResp2Error();
        if(SD_OK != ret)
            return (ret);
        g.cid[0] = SDIO_GetResponse(SDIO_RESP1);
        g.cid[1] = SDIO_GetResponse(SDIO_RESP2);
        g.cid[2] = SDIO_GetResponse(SDIO_RESP3);
        g.cid[3] = SDIO_GetResponse(SDIO_RESP4);
    }
    if((SDTYPE_SDSC_V1_1 == g.type) || (SDTYPE_SDSC_V2_0 == g.type)
            || (SDTYPE_SDIO_COMBO == g.type) || (SDTYPE_SDHC == g.type)) {
        /* Send CMD3 SET_REL_ADDR with argument 0, get rca */
        SDIO_SendCmdEx(CMD3, 0x0, CMD_EX_DEFAULT);
        ret = CmdResp6Error(CMD3, &rca);
        if(SD_OK != ret)
            return (ret);
    }
    if(SDTYPE_SDIO != g.type) {
        g.rca = rca;
        /* Send CMD9 SEND_CSD with argument as card's RCA */
        SDIO_SendCmdEx(CMD9, (u32_t)(rca << 16),
            SDIO_Response_Long | SDIO_CPSM_Enable);
        ret = CmdResp2Error();
        if(SD_OK != ret) {
            return (ret);
        }
        g.csd[0] = SDIO_GetResponse(SDIO_RESP1);
        g.csd[1] = SDIO_GetResponse(SDIO_RESP2);
        g.csd[2] = SDIO_GetResponse(SDIO_RESP3);
        g.csd[3] = SDIO_GetResponse(SDIO_RESP4);
    }
    ret = SD_OK; /* All cards get intialized */
    return (ret);
}
static SD_Error SDEnWideBus(void)
{
    SD_Error ret = SD_OK;
    SDIO_SendCmdEx(CMD55, g.rca << 16, CMD_EX_DEFAULT);
    ret = CmdResp1Error(CMD55);
    if(ret != SD_OK)
        return (ret);
    SDIO_SendCmdEx(ACMD6, 0x2, CMD_EX_DEFAULT);
    ret = CmdResp1Error(ACMD6);
    if(ret != SD_OK)
        return (ret);
    return (ret);
}

static void SDIO_DMA_Config(void)
{
    DMA_InitTypeDef dis;
    DMA_Cmd(DMA2_Channel4, DISABLE); /* DMA2 Channel4 disable */
    dis.DMA_PeripheralBaseAddr = (u32_t)&(SDIO->FIFO);
    dis.DMA_DIR = DMA_DIR_PeripheralSRC;
    dis.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dis.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dis.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    dis.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    dis.DMA_Mode = DMA_Mode_Circular;
    dis.DMA_Priority = DMA_Priority_High;
    dis.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel4, &dis);
    DMA_ClearFlag(DMA2_FLAG_TC4);
}

SD_Error SD_Init(void)
{
    SD_Error status = SD_OK;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
    SDIO_DeInit();
    status = SD_PowerON();
    if(status != SD_OK)
        return (status);        // 0
    status = SD_InitializeCards();
    if(status != SD_OK)
        return (status);        // 1
    SDIO_SetClockDiv(SDIO_TRANSFER_CLK_DIV);
//    SDIO->CLKCR |= (1UL << 14); // enable flow ctrl
    SDIO_DMA_Config();
    SDIO_SendCmdEx(CMD7, g.rca << 16, CMD_EX_DEFAULT);
    if((g.csd[0] >> 30) == 0x0) {  // csd v1.0
        u32_t c_size = ((g.csd[1] << 2) | (g.csd[2] >> 30)) & 0xfff;
        g.size = (c_size + 1) * (1 << (((g.csd[2] >> 15) & 0x7) + 2 // abc
                + ((g.csd[1] >> 16) & 0xf) - 10));   // abc
    }
    else if((g.csd[0] >> 30) == 0x1) {   // csd v2.0
        g.size = ((g.csd[1] << 16 | g.csd[2] >> 16) & 0x3fffff) * 512;
    }
    int ret;
    ret = SDEnWideBus();
    if(SD_OK != ret)
        return (ret);
    SDIO_SetBusWidth(SDIO_BusWide_4b);
    SDIO_SendCmdEx(CMD16, 512, CMD_EX_DEFAULT);
    ret = CmdResp1Error(CMD16);
    if(SD_OK != ret)
        return (ret);
    return (status);
}

static u8_t convert_from_bytes_to_power_of_two(u16_t nbytes)
{
    u8_t count = 0;
    while(nbytes != 1) {
        nbytes >>= 1;
        count++;
    }
    return (count);
}

SD_Error SD_ReadBlock(u32_t addr, void *readbuff, int nbytes)
{
    SD_Error ret = SD_OK;
    u8_t power = 0;
    if(readbuff == NULL)
        return SD_INVALID_PARAMETER;
    SDIO_DataCfgEx(0, SDIO_DataBlockSize_1b, SDIO_TransferDir_ToCard,
        SDIO_DPSM_Disable);
    SDIO_DMACmd(DISABLE);
    if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
        return SD_LOCK_UNLOCK_FAILED;
    if(g.type == SDTYPE_SDHC) {
        nbytes = 512;
        addr /= 512;
    }
    if((nbytes > 0) && (nbytes <= 2048) && ((nbytes & (nbytes - 1)) == 0))
        power = convert_from_bytes_to_power_of_two(nbytes);
    else
        return SD_INVALID_PARAMETER;
    SDIO_DataCfgEx(nbytes, (u32_t)power << 4, SDIO_TransferDir_ToSDIO,
        SDIO_DPSM_Enable);
    DMA_Cmd(DMA2_Channel4, DISABLE);
    DMA2_Channel4->CMAR = (u32_t)readbuff;
    DMA2_Channel4->CNDTR = nbytes / 4;
    DMA2_Channel4->CCR = (DMA2_Channel4->CCR & ~(1 << 4))
            | DMA_DIR_PeripheralSRC;
    SDIO_DMACmd(ENABLE);
    DMA_Cmd(DMA2_Channel4, ENABLE);
    SDIO_SendCmdEx(CMD17, addr, CMD_EX_DEFAULT);
    if(SDIO->STA & SDIO_FLAG_DTIMEOUT)
        return SD_DATA_TIMEOUT;
    if(SDIO->STA & SDIO_FLAG_DCRCFAIL)
        return SD_DATA_CRC_FAIL;
    if(SDIO->STA & SDIO_FLAG_RXOVERR)
        return SD_RX_OVERRUN;
    ( {  while (DMA_GetFlagStatus(DMA2_FLAG_TC4) == RESET);});
    DMA_ClearFlag(DMA2_FLAG_TC4);
    ( {  while ((SDIO->STA & SDIO_FLAG_DATAEND) == RESET);});
    return (ret);
}

SD_Error SD_ReadMultiBlocks(u32_t addr, void *readbuff, int nbytes, int nblocks)
{
    SD_Error ret = SD_OK;
    u8_t power = 0;
    if(NULL == readbuff)
        return SD_INVALID_PARAMETER;
    SDIO_DataCfgEx(0, SDIO_DataBlockSize_1b, SDIO_TransferDir_ToCard,
        SDIO_DPSM_Disable);
    SDIO_DMACmd(DISABLE);
    if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
        return SD_LOCK_UNLOCK_FAILED;
    if(g.type == SDTYPE_SDHC) {
        nbytes = 512;
        addr /= 512;
    }
    if((nbytes > 0) && (nbytes <= 2048) && (0 == (nbytes & (nbytes - 1)))) {
        power = convert_from_bytes_to_power_of_two(nbytes);
        SDIO_SendCmdEx(CMD16, nbytes, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD16);
        if(SD_OK != ret)
            return (ret);
    }
    else
        return SD_INVALID_PARAMETER;
    if(nblocks > 1) {
        if(nblocks * nbytes > SD_MAX_DATA_LENGTH)
            return SD_INVALID_PARAMETER;
        SDIO_DataCfgEx(nbytes * nblocks, (u32_t)power << 4,
            SDIO_TransferDir_ToSDIO, SDIO_DPSM_Enable);
        SDIO_SendCmdEx(CMD18, addr, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD18);
        if(ret != SD_OK)
            return (ret);
        DMA_Cmd(DMA2_Channel4, DISABLE);
        DMA2_Channel4->CMAR = (u32_t)readbuff;
        DMA2_Channel4->CNDTR = nbytes * nblocks / 4;
        DMA2_Channel4->CCR = (DMA2_Channel4->CCR & ~(1 << 4))
                | DMA_DIR_PeripheralSRC;
        SDIO_DMACmd(ENABLE);
        DMA_Cmd(DMA2_Channel4, ENABLE);
        ( {  while (DMA_GetFlagStatus(DMA2_FLAG_TC4) == RESET);});
        DMA_ClearFlag(DMA2_FLAG_TC4);
        ( {  while ((SDIO->STA & SDIO_FLAG_DATAEND) == RESET);});
        SDIO_SendCmdEx(CMD12, 0, CMD_EX_DEFAULT);   // stop transmission
        ret = CmdResp1Error(CMD12);
        if(ret != SD_OK)
            return ret;
    }
    return (ret);
}

SD_Error SD_WriteBlock(unsigned long addr, void* writebuff, int nbytes)
{
    SD_Error ret = SD_OK;
    u8_t power = 0, state = 0;
    u32_t timeout = 0;
    u32_t cardstatus = 0;
    if(writebuff == NULL)
        return SD_INVALID_PARAMETER;
    SDIO_DataCfgEx(0, SDIO_DataBlockSize_1b, SDIO_TransferDir_ToCard,
        SDIO_DPSM_Disable);
    SDIO_DMACmd(DISABLE);
    if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
        return SD_LOCK_UNLOCK_FAILED;
    if(g.type == SDTYPE_SDHC) {
        nbytes = 512;
        addr /= 512;
    }
    /* Set the block size, both on controller and card */
    if((nbytes > 0) && (nbytes <= 2048) && ((nbytes & (nbytes - 1)) == 0)) {
        power = convert_from_bytes_to_power_of_two(nbytes);
        SDIO_SendCmdEx(CMD16, nbytes, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD16);
        if(ret != SD_OK)
            return (ret);
    }
    else
        return SD_INVALID_PARAMETER;
    /* Wait till card is ready for data Added */
    timeout = SD_DATATIMEOUT;
    do {
        timeout--;
        SDIO_SendCmdEx(CMD13, (u32_t)g.rca << 16, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD13);
        if(ret != SD_OK)
            return (ret);
        cardstatus = SDIO_GetResponse(SDIO_RESP1);
    }while(((cardstatus & 0x100) == 0) && (timeout > 0));
    if(timeout == 0)
        return (SD_ERROR);
    SDIO_SendCmdEx(CMD24, addr, CMD_EX_DEFAULT);
    ret = CmdResp1Error(CMD24);
    if(ret != SD_OK)
        return (ret);

    SDIO_DataCfgEx(nbytes, (u32_t)power << 4, SDIO_TransferDir_ToCard,
        SDIO_DPSM_Enable);

    DMA_Cmd(DMA2_Channel4, DISABLE);
    DMA2_Channel4->CMAR = (u32_t)writebuff;
    DMA2_Channel4->CNDTR = nbytes / 4;
    DMA2_Channel4->CCR = (DMA2_Channel4->CCR & ~(1 << 4))
            | DMA_DIR_PeripheralDST;
    SDIO_DMACmd(ENABLE);
    DMA_Cmd(DMA2_Channel4, ENABLE);
    if(SDIO->STA & SDIO_FLAG_DTIMEOUT)
        return SD_DATA_TIMEOUT;
    if(SDIO->STA & SDIO_FLAG_DCRCFAIL)
        return SD_DATA_CRC_FAIL;
    if(SDIO->STA & SDIO_FLAG_RXOVERR)
        return SD_RX_OVERRUN;
    ( {  while (DMA_GetFlagStatus(DMA2_FLAG_TC4) == RESET);});
    DMA_ClearFlag(DMA2_FLAG_TC4);
    //    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
    do {
        ret = IsCardProgramming(&state);
    }while((ret == SD_OK)
            && ((state == SD_CARD_PROGRAMMING) || (state == SD_CARD_RECEIVING)));
    return (ret);
}

SD_Error SD_WriteMultiBlocks(u32_t addr, u32_t* writebuff, u16_t nbytes,
    u32_t nblocks)
{
    SD_Error ret = SD_OK;
    u8_t power = 0, state = 0;
    if(writebuff == NULL)
        return SD_INVALID_PARAMETER;
    SDIO_DataCfgEx(0, SDIO_DataBlockSize_1b, SDIO_TransferDir_ToCard,
        SDIO_DPSM_Disable);
    SDIO_DMACmd(DISABLE);
    if(SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
        return SD_LOCK_UNLOCK_FAILED;
    if(g.type == SDTYPE_SDHC) {
        nbytes = 512;
        addr /= 512;
    }
    /* Set the block size, both on controller and card */
    if((nbytes > 0) && (nbytes <= 2048) && ((nbytes & (nbytes - 1)) == 0)) {
        power = convert_from_bytes_to_power_of_two(nbytes);
        SDIO_SendCmdEx(CMD16, nbytes, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD16);
        if(ret != SD_OK)
            return (ret);
    }
    else
        return SD_INVALID_PARAMETER;
    /* Wait till card is ready for data Added */
    int timeout = SD_DATATIMEOUT;
    u32_t cardstatus = 0;
    do {
        timeout--;
        SDIO_SendCmdEx(CMD13, (u32_t)g.rca << 16, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD13);
        if(ret != SD_OK)
            return (ret);
        cardstatus = SDIO_GetResponse(SDIO_RESP1);
    }while(((cardstatus & 0x100) == 0) && (timeout > 0));
    if(timeout == 0)
        return (SD_ERROR);

    if(nblocks > 1) {
        /* Common to all modes */
        if(nblocks * nbytes > SD_MAX_DATA_LENGTH)
            return SD_INVALID_PARAMETER;
        if((SDTYPE_SDSC_V1_1 == g.type) || (SDTYPE_SDSC_V2_0 == g.type)
                || (SDTYPE_SDHC == g.type)) {
            SDIO_SendCmdEx(CMD55, (u32_t)(g.rca << 16), CMD_EX_DEFAULT); // To improve performance
            ret = CmdResp1Error(CMD55);
            if(ret != SD_OK)
                return (ret);
            SDIO_SendCmdEx(ACMD23, nblocks, CMD_EX_DEFAULT); // To improve performance
            ret = CmdResp1Error(ACMD23);
            if(ret != SD_OK)
                return (ret);
        }
        /* Send CMD25 WRITE_MULT_BLOCK with argument data address */
        SDIO_DataCfgEx(nbytes * nblocks, (u32_t)power << 4,
            SDIO_TransferDir_ToCard, SDIO_DPSM_Enable);

        SDIO_SendCmdEx(CMD25, addr, CMD_EX_DEFAULT);
        ret = CmdResp1Error(CMD25);
        if(SD_OK != ret)
            return (ret);

        DMA_Cmd(DMA2_Channel4, DISABLE);
        DMA2_Channel4->CMAR = (u32_t)writebuff;
        DMA2_Channel4->CNDTR = nbytes * nblocks / 4;
        DMA2_Channel4->CCR = (DMA2_Channel4->CCR & ~(1 << 4))
                | DMA_DIR_PeripheralDST;
        SDIO_DMACmd(ENABLE);
        DMA_Cmd(DMA2_Channel4, ENABLE);
        ( {  while (DMA_GetFlagStatus(DMA2_FLAG_TC4) == RESET);});
        DMA_ClearFlag(DMA2_FLAG_TC4);
    }
//    SDIO_ClearFlag(SDIO_STATIC_FLAGS);
    ( {  while ((SDIO->STA & SDIO_FLAG_DATAEND) == RESET);});
    SDIO_SendCmdEx(CMD12, 0, CMD_EX_DEFAULT);   // stop transmission
    ret = CmdResp1Error(CMD12);
    if(ret != SD_OK)
        return ret;
    do {
        ret = IsCardProgramming(&state);
    }while((ret == SD_OK)
            && ((state == SD_CARD_PROGRAMMING) || (state == SD_CARD_RECEIVING)));
    return (ret);
}
