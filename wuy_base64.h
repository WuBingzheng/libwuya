#ifndef WUY_BASE64_H
#define WUY_BASE64_H

int wuy_base64_encode(char *dest, const unsigned char *src, int len);
int wuy_base64_decode(unsigned char *dest, const char *src, int len);

#endif
