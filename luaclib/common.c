#include "common.h"

uint16_t 
checksum(uint16_t* addr,size_t size) {
    uint32_t sum = 0;
    while(size > 1) {
    	sum += *addr++;
    	size-=2;
    }
    if (size > 0) {
    	sum += *(uint8_t*)addr;
    }
    while(sum >> 16) {
    	sum = (sum & 0xffff) + (sum >> 16);
    }
    return (int16_t)~sum;
}