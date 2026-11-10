#pragma once
#include <stddef.h>
#include <stdint.h>

// A register selection and its entire read share one repeated-start transaction.
template<class WireBus>
int readImuRegister(WireBus &wire,uint8_t address,uint8_t reg,uint8_t *out,size_t size){
    if(!size)return 0;
    wire.beginTransmission(address);
    if(wire.write(reg)!=1){wire.endTransmission(true);return -1;}
    if(wire.endTransmission(false)!=0||wire.requestFrom(address,size,true)!=size)return -1;
    return wire.readBytes(reinterpret_cast<char*>(out),size)==size?0:-1;
}
