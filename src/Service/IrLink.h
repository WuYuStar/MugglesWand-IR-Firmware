#pragma once
#include <Arduino.h>
void ir_link_init();
void ir_link_receive(const uint8_t *mac, const uint8_t *data, int length);
bool ir_link_command(uint8_t type, uint8_t slot, const char *name = nullptr);
String ir_link_json();
String ir_link_connection_json();
bool ir_link_pair(const char *mac);
bool ir_link_unpair();
