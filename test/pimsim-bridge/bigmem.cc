#include <stdio.h>
#include <stdlib.h>
int main(int c, char**v){ setvbuf(stdout,NULL,_IONBF,0); unsigned long n=strtoul(v[1],0,0)<<20; unsigned char*b=(unsigned char*)malloc(n); for(unsigned long i=0;i<n;++i) b[i]=(unsigned char)i; printf("filled %lu MB %d\n", n>>20, b[12345]); return 0; }
