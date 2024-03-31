#include <iostream>

using namespace std;

typedef unsigned char *byte_pointer;

void show_bytes(byte_pointer start, int len) {
    int i;
    for (i = 0; i < len; i++)
    printf(" %.2x", start[i]);
    printf("\n");
}

int main() {
    int val = 0x87654321;
    byte_pointer valp = (byte_pointer) &val;

    // printf(" %.2x", valp[1]);
    show_bytes(valp, 1);
    show_bytes(valp, 2);
    show_bytes(valp, 3);    
}