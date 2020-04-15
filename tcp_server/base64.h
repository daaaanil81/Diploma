#ifndef _BASE64_
#define _BASE64_
#include <stdio.h>
#include <stdlib.h>

size_t base64_decode(char* in, char* out);
size_t base64_encode(char* in, char* out);
#endif
