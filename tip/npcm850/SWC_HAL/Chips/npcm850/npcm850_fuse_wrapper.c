/*---------------------------------------------------------------------------------------------------------*/
/*  Nuvoton Technology Corporation Confidential                                                            */
/*                                                                                                         */
/*  Copyright (c) 2021 by Nuvoton Technology Corporation                                                   */
/*  All rights reserved                                                                                    */
/*                                                                                                         */
/*<<<------------------------------------------------------------------------------------------------------*/
/* File Contents:                                                                                          */
/*                         fuse_wrapper.c                                                                  */
/*                                                                                                         */
/*            This file contains fuse wrapper implementation. it wraps all access to the otp               */
/*                                                                                                         */
/*  Project:  Arbel                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#if defined (FUSE_MODULE_TYPE)


#include "npcm850_fuse_wrapper.h"
#include <string.h>


/*---------------------------------------------------------------------------------------------------------*/
/* This global array is used to host the full encloded key                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define        FUSE_ARRAY_MAX_SIZE    256
static UINT8   fuse_encoded[FUSE_ARRAY_MAX_SIZE] = {0};


/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_get_CRC                                                                           */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  datain -       pointer to encoded data buffer. buffer should be 8 bytes                */
/*                                                                                                         */
/* Returns:         CRC                                                                                    */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Calc the CRC according to hamming. CRC lower bit is the R1 (location 1), etc           */
/*                  MSb is the parity (nidded for double error detection.  )                               */
/*---------------------------------------------------------------------------------------------------------*/
static UINT8 FUSE_get_CRC(UINT8 *datain)
{
    int i;
    UINT8 CRC; // hamming code, 7 bits
    UINT8 R1;
    UINT8 R2;
    UINT8 R4;
    UINT8 R8;
    UINT8 R16;
    UINT8 R32;
    UINT8 R64;
    UINT8 parity = 0;

#define BIT_A(n)  (READ_VAR_BIT(datain[(n-1)>>3], ((n-1) % 8)))

    R1 = BIT_A(1)^BIT_A(2)^BIT_A(4)^BIT_A(5)^BIT_A(7)^BIT_A(9)^BIT_A(11)^
        BIT_A(12)^BIT_A(14)^BIT_A(16)^BIT_A(18)^BIT_A(20)^
        BIT_A(22)^BIT_A(24)^BIT_A(26)^BIT_A(27)^BIT_A(29)^BIT_A(31)^
        BIT_A(33)^BIT_A(35)^BIT_A(37)^BIT_A(39)^BIT_A(41)^BIT_A(43)^
        BIT_A(45)^BIT_A(47)^BIT_A(49)^BIT_A(51)^BIT_A(53)^BIT_A(55)^
        BIT_A(57)^BIT_A(58)^BIT_A(60)^BIT_A(62)^BIT_A(64);

    R2  =   BIT_A(1)^BIT_A(3)^BIT_A(4)^BIT_A(6)^BIT_A(7)^BIT_A(10) ^BIT_A(11)^BIT_A(13)^BIT_A(14)^
        BIT_A(17)^BIT_A(18)^BIT_A(21)^BIT_A(22)^BIT_A(25) ^
        BIT_A(26)^BIT_A(28)^BIT_A(29)^BIT_A(32)^BIT_A(33)^BIT_A(36)^BIT_A(37)^BIT_A(40)^
        BIT_A(41)^BIT_A(44)^BIT_A(45)^BIT_A(48)^BIT_A(49)^BIT_A(52) ^
        BIT_A(53)^BIT_A(56)^BIT_A(57)^BIT_A(59)^BIT_A(60)^BIT_A(63)^BIT_A(64);

    R4 =    BIT_A(2)^BIT_A(3)^BIT_A(4)^
            BIT_A(8)^BIT_A(9)^BIT_A(10)^BIT_A(11)^
        BIT_A(15)^BIT_A(16)^BIT_A(17)^BIT_A(18)^
        BIT_A(23)^BIT_A(24)^BIT_A(25)^BIT_A(26)^
        BIT_A(30)^BIT_A(31)^BIT_A(32)^BIT_A(33)^
        BIT_A(38)^BIT_A(39)^BIT_A(40)^BIT_A(41)^
        BIT_A(46)^BIT_A(47)^BIT_A(48)^BIT_A(49)^
        BIT_A(54)^BIT_A(55)^BIT_A(56)^BIT_A(57)^
        BIT_A(61)^BIT_A(62)^BIT_A(63)^BIT_A(64);

    R8 =    BIT_A(5) ^BIT_A(6)^BIT_A(7)^BIT_A(8)^BIT_A(9)^BIT_A(10) ^BIT_A(11) ^
        BIT_A(19)^BIT_A(20)^BIT_A(21)^BIT_A(22)^BIT_A(23)^BIT_A(24)^BIT_A(25) ^BIT_A(26)^
        BIT_A(34)^BIT_A(35)^BIT_A(36)^BIT_A(37)^BIT_A(38)^BIT_A(39)^BIT_A(40)^BIT_A(41) ^
        BIT_A(50)^BIT_A(51)^BIT_A(52)^BIT_A(53)^BIT_A(54)^BIT_A(55)^BIT_A(56)^BIT_A(57);

    R16 = BIT_A(12)^BIT_A(13)^BIT_A(14)^BIT_A(15)^BIT_A(16)^BIT_A(17) ^BIT_A(18)^BIT_A(19)^ BIT_A(20) ^ BIT_A(21)^BIT_A(22)^BIT_A(23)^BIT_A(24) ^   BIT_A(25)^BIT_A(26)^
        BIT_A(42)^BIT_A(43)^BIT_A(44)^BIT_A(45)^BIT_A(46)^BIT_A(47)^BIT_A(48)^BIT_A(49)^BIT_A(50)^BIT_A(51)^BIT_A(52)^BIT_A(53) ^   BIT_A(54)^BIT_A(55)^BIT_A(56)^BIT_A(57);

    R32 = BIT_A(27)^BIT_A(28)^BIT_A(29)^BIT_A(30)^BIT_A(31)^BIT_A(32) ^BIT_A(33)^BIT_A(34)^BIT_A(35)^
        BIT_A(36)^BIT_A(37)^BIT_A(38)^BIT_A(39) ^
        BIT_A(40)^BIT_A(41)^BIT_A(42)^BIT_A(43)^BIT_A(44)^BIT_A(45)^BIT_A(46)^BIT_A(47)^BIT_A(48)^
        BIT_A(49)^BIT_A(50)^BIT_A(51)^BIT_A(52)^BIT_A(53) ^
        BIT_A(54)^BIT_A(55)^BIT_A(56)^BIT_A(57);

    R64 = BIT_A(58)^BIT_A(59)^BIT_A(60)^BIT_A(61)^BIT_A(62) ^BIT_A(63)^BIT_A(64) ;

    parity = 0;
    for (i = 1; i <= 64; i++)
    {
        parity ^= BIT_A(i);
    }

#undef BIT_A

    R1 = R1 & 0x01;
    R2 = R2 & 0x01;
    R4 = R4 & 0x01;
    R8 = R8 & 0x01;
    R16 = R16 & 0x01;
    R32 = R32 & 0x01;
    R64 = R64 & 0x01;

    CRC = (R1 << 0) + (R2 << 1) + (R4 << 2) + (R8 << 3) + (R16 << 4) + (R32 << 5) + (R64 << 6) + (parity << 7);

    // HAL_PRINT("%d%d%d%d%d%d%d%d b CRC = 0x%02X ( parity = %d)\n", parity, R64, R32, R16, R8, R4, R2 , R1, CRC, parity);
    return CRC;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_64_72_SECDEC_Decode                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  datain -       pointer to encoded data buffer (buffer size should be 2 x dataout)      */
/*                  dataout -      pointer to decoded data buffer                                          */
/*                  encoded_size - size of encoded data (9*(decoded data / 8))                             */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on successful read completion, HAL_ERROR_* otherwise.                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Decodes the data according to hamming code 64\72 scheme.                               */
/*                  Size specifies the encoded data size.                                                  */
/*                  Decodes whole bytes only                                                               */
/*                  Note: if one bit error - it will be fixed. Double bit - return fail status with        */
/*                        the original array                                                               */
/*---------------------------------------------------------------------------------------------------------*/
static DEFS_STATUS FUSE_64_72_SECDEC_Decode (
    UINT8  *datain, // 65 bytes
    UINT8  *dataout, // 64 bytes
    UINT32  encoded_size
)
{
    unsigned int i = 0;
    UINT8 CRC; // hamming code, 7 bits
    UINT8 bit_flipped;

    UINT8 *arr = datain;
    unsigned int cnt = 0;
    DEFS_STATUS status = DEFS_STATUS_OK;
    UINT8 error_byte;
    UINT32 error_bit;

    UINT32 decoded_size = (8*encoded_size) / 9; // assuming the number * 8/9 is an int.

    memcpy(dataout, datain, decoded_size );

    for( cnt = 0 ; cnt < decoded_size ; cnt += 8)
    {
        // each 64 bits (8 bytes) get a CRC byte at the end
        arr = datain + cnt;

        CRC = FUSE_get_CRC(arr);

        // check CRC (one per 64 bits):
        if(CRC != datain[decoded_size + i])
        {
            error_byte = CRC ^ datain[decoded_size + i];

            HAL_PRINT("error byte 0x%02X (0x%02X ^ 0x%02X) \t", error_byte, CRC, datain[decoded_size + i]);

            if((error_byte == 0x01) ||
                (error_byte == 0x02) ||
                (error_byte == 0x04) ||
                (error_byte == 0x08) ||
                (error_byte == 0x10) ||
                (error_byte == 0x20) ||
                (error_byte == 0x40) ||
                (error_byte == 0x80))
            {
                HAL_PRINT("error in parity bit\n"); // nothing to fix in data
            }

            // check if the error is in a parity bit:
            else// error is in one of the data bits
            {
                // double error:
                if ((error_byte & 0x80) == 0)
                {
                    HAL_PRINT("double error\n");

                    status = DEFS_STATUS_BAD_CHECKSUM;
                }

                // single error, lets fix it:
                else
                {
                    // skip the encoding bits ( since they I gathered at the end, instead of standart hamming where they are positioned
                    // in location 1,2,4,8,16,32,64 of the stream:
                    error_bit = (error_byte & 0x7F)  - 1;
                    if (error_bit >= 64) error_bit--;
                    if (error_bit >= 32) error_bit--;
                    if (error_bit >= 16) error_bit--;
                    if (error_bit >=  8) error_bit--;
                    if (error_bit >=  4) error_bit--;
                    if (error_bit >=  2) error_bit--;
                    if (error_bit >=  1) error_bit--;

                    error_bit += 64 * i;

                    // fix the error
                    bit_flipped = (datain[error_bit >> 3] >>  (error_bit % 8)) & 0x01 ;

                    HAL_PRINT("error in  bit %d flip %d (byte %d, bit %d) group = %d \n", error_bit, bit_flipped, error_bit >> 3, error_bit % 8, i);
                    if (bit_flipped >  0)
                        dataout[error_bit >> 3] &= (UINT8)~(0x01 << (error_bit % 8));
                    else
                        dataout[error_bit >> 3] |=   MASK_BIT(error_bit % 8);
                } // end single error
            }
        }
        else    // no single\double bit errors found
        {
            // HAL_PRINT("no errors found\n");
        }

        i++;
    }

    return status;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_64_72_SECDEC_Encode                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  datain -       pointer to decoded data buffer (buffer size should be 2 x dataout)      */
/*                  dataout -      pointer to encoded data buffer                                          */
/*                  encoded_size - size of encoded data (9*(decoded data / 8))                             */
/*                                                                                                         */
/* Returns:         DEFS_STATUS_OK on successful read completion, HAL_ERROR_* otherwise.                   */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  Decodes the data according to hamming code 64\72 scheme.                               */
/*                  Size specifies the encoded data size.                                                  */
/*                  Decodes whole bytes only                                                               */
/*---------------------------------------------------------------------------------------------------------*/
static DEFS_STATUS FUSE_64_72_SECDEC_Encode (
    UINT8 *datain,
    UINT8 *dataout,
    UINT32 encoded_size)
{
    UINT32 i = 0;
    UINT8  CRC; // hamming code, 7 bits + parity
    UINT8 *arr = datain;
    UINT32 cnt = 0;

    UINT32 decoded_size = DIV_CEILING(8*encoded_size , 9);

    memcpy(dataout, datain, encoded_size);

    for( cnt = 0 ; cnt < decoded_size ; cnt += 8)
    {
        // each 64 bits (8 bytes) get a CRC byte at the end
        arr = datain + cnt;

        CRC = FUSE_get_CRC(arr);

        dataout[decoded_size + i++] = CRC;
    }

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_WRPR_set                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  fuse_address    -   address in the fuse\key array                                      */
/*                  fuse_length     -   length in bytes inside the fuse array (before encoding)            */
/*                  fuse_ecc        -   nible parity\majority\none                                         */
/*                  value           -   input buffer (plan data)                                           */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine ...                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FUSE_WRPR_set (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value)
{
    UINT16 iCnt = 0;
    DEFS_STATUS status = DEFS_STATUS_OK;

    // actual size for fuses to read:
    /* UINT16 iSize = fuse_length; */
    ASSERT(iSize < FUSE_ARRAY_MAX_SIZE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Decode , if needed. If decode fails... TBD                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_NIBBLE_PARITY)
    {
        /* iSize = fuse_length / 2; */
        status = FUSE_NibParEccEncode(value, fuse_encoded, fuse_length);
    }

    else if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_MAJORITY)
    {
        /* iSize = fuse_length / 3; */
        status = FUSE_MajRulEccEncode(value, fuse_encoded, fuse_length);
    }

    else if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_64_72)
    {
        /* iSize = fuse_length / 3; */
        status = FUSE_64_72_SECDEC_Encode(value, fuse_encoded, fuse_length);
    }

    if ( status != DEFS_STATUS_OK)
    {
        return status;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* And program to OTP                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    // if no parity
    if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_NONE )
    {
        for (iCnt = 0; iCnt < fuse_length ; iCnt++)
        {
            status  |= FUSE_ProgramByte(0, fuse_address + iCnt , value[iCnt]);
        }
    }
    else
    {
        for (iCnt = 0; iCnt < fuse_length ; iCnt++)
        {
            status  |= FUSE_ProgramByte(0, fuse_address + iCnt , fuse_encoded[iCnt]);
        }
    }

    return status;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_WRPR_get                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  fuse_address    -   address in the fuse\key array                                      */
/*                  fuse_length     -   length in bytes inside the fuse array (before encoding)            */
/*                  fuse_ecc        -   nible parity\majority\none                                         */
/*                  value           -   output buffer (the decoded data)                                   */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine read a value from the fuses.                                              */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FUSE_WRPR_get (UINT16 fuse_address, UINT16 fuse_length, FUSE_ECC_TYPE_T fuse_ecc, UINT8* value)
{
    UINT16 iCnt = 0;

    DEFS_STATUS status = DEFS_STATUS_OK;

    DEFS_STATUS_COND_CHECK(fuse_length <= FUSE_ARRAY_MAX_SIZE, DEFS_STATUS_INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the fuses                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    // if no parity
    if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_NONE )
    {
        for (iCnt = 0; iCnt < fuse_length ; iCnt++)
        {
            FUSE_Read(0, fuse_address + iCnt , &value[iCnt]);
        }
    }
    else
    {
        for (iCnt = 0; iCnt < fuse_length ; iCnt++)
        {
            FUSE_Read(0, fuse_address + iCnt , &fuse_encoded[iCnt]);
        }

    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Either encode the data or read it as is                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_NIBBLE_PARITY)
    {
        status = FUSE_NibParEccDecode(fuse_encoded, value, fuse_length);
    }
    else if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_MAJORITY)
    {
        status = FUSE_MajRulEccDecode(fuse_encoded, value, fuse_length);
    }
    else if ( (FUSE_ECC_TYPE_T)fuse_ecc == FUSE_ECC_64_72)
    {
        status = FUSE_64_72_SECDEC_Decode(fuse_encoded, value, fuse_length);
    }

    return status;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_WRPR_get_CP_Fustrap                                                               */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  value - output buffer (the decoded data)                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine read a value from the fuses. It's for a field that is ten bit :(          */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FUSE_WRPR_get_CP_Fustrap (UINT8* value)
{
    /* Due to architecture bug , CP_FUSTRAP is only 10 bits. In order not to change the rest of the FUSE_Wrapper, this code is set seperetly */

    UINT16 iCnt = 0;
    UINT  bit;
    UINT8 E1, E2, E3;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the fuses                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    for (iCnt = 0; iCnt < FUSE_WRPR_PROP_SIZE(CP_FUSTRAP_PROPERTY) ; iCnt++)
    {
        FUSE_Read(0, FUSE_WRPR_PROP_ADDRESS(CP_FUSTRAP_PROPERTY) + iCnt , &fuse_encoded[iCnt]);
    }

    for (bit = 0; bit < 10; bit++)
    {
        E1 = READ_VAR_BIT(fuse_encoded[(( 0 + bit ) / 8)], (( 0 + bit ) % 8));
        E2 = READ_VAR_BIT(fuse_encoded[((10 + bit ) / 8)], ((10 + bit ) % 8));
        E3 = READ_VAR_BIT(fuse_encoded[((20 + bit ) / 8)], ((20 + bit ) % 8));
        if ((E1+E2+E3) >= 2)
        {
            SET_VAR_BIT(value[(bit / 8)], (bit % 8));    // Majority is 1
        }
        else
        {
             CLEAR_VAR_BIT(value[(bit / 8)], (bit % 8));  // Majority is 0
        }
    } //Inner for (bit)

    return DEFS_STATUS_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function:        FUSE_WRPR_EraseBlock                                                                   */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*                  block    -  block number                                                               */
/*                                                                                                         */
/* Returns:                                                                                                */
/* Side effects:                                                                                           */
/* Description:                                                                                            */
/*                  This routine erases (writes 0xFF) to all bytes in the specified block                  */
/*---------------------------------------------------------------------------------------------------------*/
DEFS_STATUS FUSE_WRPR_EraseBlock (UINT16 block)
{
    DEFS_STATUS status;
    DEFS_STATUS retStatus = DEFS_STATUS_OK;
    UINT16 byteAddr = FUSE_WRPR_BLOCK_ADDR(block);
    UINT16 endAddr  = byteAddr + FUSE_WRPR_BLOCK_LEN(block);

    for (; byteAddr < endAddr; byteAddr++)
    {
        status = FUSE_ProgramByte(KEY_SA, byteAddr, 0xFF);
        if (status != DEFS_STATUS_OK)
            retStatus = status;
    }

    return retStatus;
}

#endif // FUSE_MODULE_TYPE

