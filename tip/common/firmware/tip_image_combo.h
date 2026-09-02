// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Nuvoton Technology Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef TIP_IMAGE_COMBO_H_
#define TIP_IMAGE_COMBO_H_

#include "firmware/firmware_image.h"
#include "tip_firmware_component.h"
#include "tip_key_manifest.h"
#include "tip_skmt.h"


#define TIP_COMBO_MAX_COMPONENT_COUNT 8

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