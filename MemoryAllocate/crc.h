#ifndef _CRC_
#define _CRC_

#include "portable.h"

UINT8_T Crc8(UINT8_T *pcBlock, UINT8_T len);
UINT16_T Crc16(UINT8_T *pcBlock, UINT16_T len);

#endif