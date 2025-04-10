#ifndef __MOUSE_H__
#define __MOUSE_H__

#define PCIE_MMIO_VENDOR	0x00
#define PCIE_MMIO_DEVICE	0x02

typedef uint8 u8;
typedef uint16 le16;
typedef uint32 le32;

enum virtio_input_config_select { 
  VIRTIO_INPUT_CFG_UNSET      = 0x00, 
  VIRTIO_INPUT_CFG_ID_NAME    = 0x01, 
  VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02, 
  VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03, 
  VIRTIO_INPUT_CFG_PROP_BITS  = 0x10, 
  VIRTIO_INPUT_CFG_EV_BITS    = 0x11, 
  VIRTIO_INPUT_CFG_ABS_INFO   = 0x12, 
}; 
 
struct virtio_input_absinfo { 
  le32  min; 
  le32  max; 
  le32  fuzz; 
  le32  flat; 
  le32  res; 
}; 
 
struct virtio_input_devids { 
  le16  bustype; 
  le16  vendor; 
  le16  product; 
  le16  version; 
}; 
 
struct virtio_input_config { 
  u8    select; 
  u8    subsel; 
  u8    size; 
  u8    reserved[5]; 
  union { 
    char string[128]; 
    u8   bitmap[128]; 
    struct virtio_input_absinfo abs; 
    struct virtio_input_devids ids; 
  } u; 
};

#endif
