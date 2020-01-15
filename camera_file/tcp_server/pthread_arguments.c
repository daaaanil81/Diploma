#include "pthread_arguments.h"
void gen_random(unsigned char *s, const int len) 
{
    srand(time(0)); 
    static const char alphanum[] = "ABCDEF0G1H2I3J4K5L6M7N8O9PQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}