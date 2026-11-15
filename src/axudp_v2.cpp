#include "axudp_v2.h"

#ifdef ENABLE_WIFI

#include <WiFi.h>
#include <WiFiUdp.h>
#include <cstdio>
#include <cstring>
#include "KISS_TO_TNC2.h"

namespace {
WiFiUDP axudpUdp;
constexpr size_t AXUDP_MAX_DATAGRAM_LENGTH = 512;

uint16_t axudpCrc(const uint8_t *data, size_t length) {
  uint16_t crc = 0xffff;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
  }
  return crc ^ 0xffff;
}
}

bool sendAxudpV2Frame(const String &tnc2Frame, int rssi, int snr, const String &host, uint16_t port) {
  if (host.isEmpty() || port == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  String cleanFrame = tnc2Frame;
  while (!cleanFrame.isEmpty()) {
    const char last = cleanFrame.charAt(cleanFrame.length() - 1);
    if (last != '\r' && last != '\n' && last != '\0') {
      break;
    }
    cleanFrame.remove(cleanFrame.length() - 1);
  }
  const String kissFrame = encode_kiss(cleanFrame);
  if (kissFrame.length() <= 3) {
    return false;
  }

  const String ax25Frame = decapsulateKISS(kissFrame);
  if (ax25Frame.isEmpty()) {
    return false;
  }

  uint8_t datagram[AXUDP_MAX_DATAGRAM_LENGTH];
  datagram[0] = 0x01;
  datagram[1] = '0';
  const int reportLength = snprintf(
    reinterpret_cast<char *>(datagram + 2),
    sizeof(datagram) - 2,
    "V%d S%d ",
    rssi,
    snr
  );
  if (reportLength < 0 || static_cast<size_t>(reportLength) >= sizeof(datagram) - 2) {
    return false;
  }

  size_t position = 2 + static_cast<size_t>(reportLength);
  datagram[position++] = 0;
  if (position > sizeof(datagram) - 2 || ax25Frame.length() > sizeof(datagram) - position - 2) {
    return false;
  }
  memcpy(datagram + position, ax25Frame.c_str(), ax25Frame.length());
  position += ax25Frame.length();

  const uint16_t crc = axudpCrc(datagram, position);
  datagram[position++] = static_cast<uint8_t>(crc & 0xff);
  datagram[position++] = static_cast<uint8_t>(crc >> 8);

  if (!axudpUdp.beginPacket(host.c_str(), port)) {
    return false;
  }
  const size_t written = axudpUdp.write(datagram, position);
  const int sendResult = axudpUdp.endPacket();
  return written == position && sendResult == 1;
}

#endif
