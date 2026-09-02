// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_IMAGE_COMBO_H_
#define TIP_IMAGE_COMBO_H_

#include "firmware/firmware_image.h"
#include "tip_firmware_component.h"
#include "tip_key_manifest.h"
#include "tip_skmt.h"


/**
 * Limit the total TIP combo images on some platforms,
 * where TIP only updates/recovers combo0 images that includes KMT/L0/SKMT/L1.
 */
#ifdef LIMIT_TIP_COMBO_COMPONENT_MAX_COUNT
#define COMBO_MAX_COMPONENT_COUNT     4
#else
#define COMBO_MAX_COMPONENT_COUNT     8
#endif

#define TIP_COMBO_MAX_COMPONENT_COUNT   COMBO_MAX_COMPONENT_COUNT
#define BMC_COMBO1_MAX_COMPONENT_COUNT 4

/**
 * Image combo used by FW updater
 */
struct tip_image_combo {
	struct firmware_image base;  								  	  /**< Firmware image base instance. */
	struct tip_firmware_component img[TIP_COMBO_MAX_COMPONENT_COUNT]; /**< TIP firmware components within the combo. */
	struct tip_primary_key_manifest kmt;						  	  /**< Key manifest used for looking up public keys. */
	struct tip_secondary_key_manifest skmt;						  	  /**< Secondary key manifest used for looking up public keys. */
	IMG_TYPE_E img_type;                                    	  	 /**< Combo type. IMG_COMBO0 or IMG_COMBO1 or IMG_COMBO2. */
};


int tip_image_combo_init (struct tip_image_combo *fw_combo);


#endif /* TIP_IMAGE_COMBO_H_ */
