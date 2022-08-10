#include "bitmap.h"
#include <arpa/inet.h>
#include <string.h>
#include "macros.h"

Error bitmap_header_from_bytes(BitmapHeader *b, const u8 *data,
                               const usize len) {
  memset(b, 0, sizeof(BitmapHeader));
  b->magic[0] = (char)data[0];
  b->magic[1] = (char)data[1];

  memcpy(&b->size, data + 2, sizeof(u32));
  memcpy(&b->offset, data + 4 + 4 + 2, sizeof(u32));

  return OK;
}

Error bitmap_image_header_from_bytes(BitmapImageHeader *b, const u8 *data,
                                     const usize len) {
  memset(b, 0, sizeof(BitmapImageHeader));

  memcpy(b, data + 14, sizeof(BitmapImageHeader));

  return OK;
}

Error bitmap_to_1bpp(Buffer *buffer) {
  Buffer src;
  buffer_init(&src);
  src.data = buffer->data;
  src.len = buffer->len;

  buffer_init(buffer);

  // get header from src
  BitmapHeader header;
  BitmapImageHeader img_header;
  if (bitmap_header_from_bytes(&header, src.data, src.len) ||
      bitmap_image_header_from_bytes(&img_header, src.data, src.len)) {
    return ERR_BMP_HEADER;
  }

  const usize img_len = (usize)img_header.w * (usize)img_header.h;
  buffer->len = ((usize)img_header.w / 8 + ((usize)img_header.w % 8 ? 1 : 0)) *
                img_header.h;
  buffer->data = malloc(buffer->len);

  // length of a row of converted bmp1 data
  const usize buffer_row_len = MAX(1, buffer->len / img_header.h);

  memset(buffer->data, 0, buffer->len);

  if (img_header.bpp != 24) {
    return ERR_BMP_UNSUPPORTED_BPP;
  }

  const usize pixel_len = img_header.bpp / 8;
  // const i32 stride = (4 * ((img_header.w * img_header.bpp + 31) / 32));
  const usize padding = (4 - (img_header.w * pixel_len) % 4) % 4;

  u8 *current =
      src.data + header.offset + img_len * pixel_len + padding * img_header.h;
  // now just loop over the 24bpp image
  for (usize y = 0; y < img_header.h; y++) {
    // obtain the current row in reverse order
    u8 *byte = buffer->data + buffer->len - 1 -
               (buffer_row_len * (img_header.h - y)) + buffer_row_len;
    usize bit = 0;

    // rows are aligned! pad now!
    current -= padding;
    for (usize x = 0; x < img_header.w; x++) {
      u32 pixel = 0;
      memcpy(&pixel, current - pixel_len, pixel_len);

      if (pixel != 0) {
        *byte = ((*byte) | ((u32)1 << (7 - bit)));
      }

      bit++;
      current -= pixel_len;
      if (bit == 8) {
        bit = 0;
        byte--;
      }
    }
  }

  buffer_free(&src);

  return OK;
}
