#ifndef __USB_H
#define __USB_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "usbd_cdc_if.h"

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
extern USBD_DescriptorsTypeDef FS_Desc;
extern USBD_ClassTypeDef USBD_CDC;
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern USBD_HandleTypeDef hUsbDeviceFS;
void USB_Device_Init(void);
void USB_OTG_FS_PowerOn(void);
void USB_OTG_FS_PowerOff(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_H */
