#ifndef AXUDP_V2_H
#define AXUDP_V2_H

#include <Arduino.h>

bool sendAxudpV2Frame(const String &tnc2Frame, int rssi, int snr, const String &host, uint16_t port);

#endif
