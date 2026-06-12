#include "usb.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_desc.h"

// USBD_HandleTypeDef hUsbDeviceFS; // Defined in usb_device.c

void USB_Device_Init(void)
{
  /* Init Device Library, Add Supported Class and Start the USB Device */
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }
  HAL_Delay(200); // Give host time to enumerate
}

void USB_OTG_FS_PowerOn(void)
{
  HAL_PCD_DevConnect(&hUsbDeviceFS);
}

void USB_OTG_FS_PowerOff(void)
{
  HAL_PCD_DevDisconnect(&hUsbDeviceFS);
}

// Add Error_Handler or ensure it's defined elsewhere if it's not a standard HAL function
__weak void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
}
