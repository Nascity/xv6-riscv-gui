#ifndef __MOUSE_H__
#define __MOUSE_H__

#define PCIE_MMIO_VENDOR	0x00
#define PCIE_MMIO_DEVICE	0x02
#define PCIE_MMIO_STATUS	0x06
#define PCIE_MMIO_BAR0		0x10
#define PCIE_MMIO_CAP		0x34

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

struct virtio_pci_cap { 
        u8 cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */ 
        u8 cap_next;    /* Generic PCI field: next ptr. */ 
        u8 cap_len;     /* Generic PCI field: capability length */ 
        u8 cfg_type;    /* Identifies the structure. */ 
        u8 bar;         /* Where to find it. */ 
        u8 padding[3];  /* Pad to full dword. */ 
        le32 offset;    /* Offset within bar. */ 
        le32 length;    /* Length of the structure, in bytes. */ 
};

struct virtio_pci_common_cfg {
        /* About the whole device. */
        le32 device_feature_select;     /* read-write */
        le32 device_feature;            /* read-only for driver */
        le32 driver_feature_select;     /* read-write */
        le32 driver_feature;            /* read-write */
        le16 msix_config;               /* read-write */
        le16 num_queues;                /* read-only for driver */
        u8 device_status;               /* read-write */
        u8 config_generation;           /* read-only for driver */

        /* About a specific virtqueue. */
        le16 queue_select;              /* read-write */
        le16 queue_size;                /* read-write */
        le16 queue_msix_vector;         /* read-write */
        le16 queue_enable;              /* read-write */
        le16 queue_notify_off;          /* read-only for driver */
        le64 queue_desc;                /* read-write */
        le64 queue_driver;              /* read-write */
        le64 queue_device;              /* read-write */
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
