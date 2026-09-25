#include <stdio.h>
int main(){volatile long s=0; for(long i=0;i<1000000;++i) s+=i; printf("hello %ld\n", (long)s); return 0;}
