/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2023 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<-------------------------------------------------------
 * File Contents:
 *   tip_ecc_hw_ncl.h
 *            This file contains the definition of ecc hw engine for ncl
 *  Project:  Arbel
 */

#ifndef TIP_ECC_HW_NCL_H_
#define TIP_ECC_HW_NCL_H_

#include "tip_ecc_hw_ncl_common.h"


int tip_ecc_hw_ncl_init (struct tip_ecc_hw_ncl_engine *engine);
void tip_ecc_hw_ncl_release (struct tip_ecc_hw_ncl_engine *engine);


#endif /* TIP_ECC_HW_NCL_H_ */
