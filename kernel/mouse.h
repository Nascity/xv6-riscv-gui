#ifndef __MOUSE_H__
#define __MOUSE_H__

typedef uint8 u8;
typedef uint16 le16;
typedef uint32 le32;

struct virtio_input_event { 
  le16 type; 
  le16 code; 
  le32 value; 
};

#endif
