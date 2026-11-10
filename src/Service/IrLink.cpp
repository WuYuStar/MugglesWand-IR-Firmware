#include "IrLink.h"
#include "AP.h"
#include <cstring>
#include <Preferences.h>

namespace {
constexpr uint8_t kSlots = 32, kState = 0x50;
struct Slot { uint8_t state; char name[49]; uint32_t seen; };
Slot slots[kSlots] = {};
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Preferences pairingStore;
uint8_t pairedMac[6] = {}, candidateMac[6] = {};
bool hasPair = false, hasCandidate = false, pairingWritable = false;
uint32_t candidateSeen = 0;
String macText(const uint8_t mac[6]) {
    char text[18];
    snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(text);
}
bool parseMac(const char *text, uint8_t out[6]) {
    if (!text || strlen(text) != 17) return false;
    for (int i = 0; i < 6; ++i) {
        const char *p = text + i * 3;
        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int hi = hex(p[0]), lo = hex(p[1]);
        if (hi < 0 || lo < 0 || (i < 5 && p[2] != ':')) return false;
        out[i] = (hi << 4) | lo;
    }
    return !(out[0] & 1); // Reject broadcast/multicast addresses.
}
String escape(const char *p) {
    String out = "\"";
    for (; *p; ++p) {
        if (*p == '\"' || *p == '\\') out += '\\';
        if (static_cast<uint8_t>(*p) < 32) continue;
        out += *p;
    }
    return out + '"';
}
}
void ir_link_init() {
    pairingWritable = pairingStore.begin("wand-ir", false);
    uint8_t saved[6];
    if (pairingWritable && pairingStore.getBytesLength("mac") == 6 &&
        pairingStore.getBytes("mac", saved, 6) == 6 &&
        memcmp(saved, "\0\0\0\0\0\0", 6)) {
        memcpy(pairedMac, saved, 6);
        hasPair = true;
    }
}
void ir_link_receive(const uint8_t *mac, const uint8_t *data, int length) {
    if (length != 52 || data[0] != kState || data[1] >= kSlots) return;
    portENTER_CRITICAL(&mux);
    if (!hasPair) {
        memcpy(candidateMac, mac, 6);
        candidateSeen = millis();
        hasCandidate = true;
    } else if (memcmp(mac, pairedMac, 6)) {
        portEXIT_CRITICAL(&mux);
        return;
    }
    auto &slot = slots[data[1]];
    slot.state = data[2];
    memcpy(slot.name, data + 3, 49);
    slot.name[48] = 0;
    slot.seen = millis();
    portEXIT_CRITICAL(&mux);
}
bool ir_link_command(uint8_t type, uint8_t slot, const char *name) {
    if (type < 0x40 || type > 0x44 || slot >= kSlots) return false;
    uint8_t packet[51] = {type, slot};
    size_t size = 2;
    if (type == 0x44) {
        if (!name) return false;
        size_t n = strnlen(name, 49);
        if (!n || n > 48) return false;
        memcpy(packet + 2, name, n);
        size += n + 1;
    }
    uint8_t target[6]; bool selected;
    portENTER_CRITICAL(&mux);
    selected = hasPair;
    if (selected) memcpy(target, pairedMac, 6);
    portEXIT_CRITICAL(&mux);
    if (!selected) return type == 0x40 && espnow_send_data(packet, size);
    return espnow_send_to(target, packet, size);
}
String ir_link_json() {
    Slot copy[kSlots];
    bool paired;
    portENTER_CRITICAL(&mux);
    memcpy(copy, slots, sizeof(copy));
    paired = hasPair;
    portEXIT_CRITICAL(&mux);
    String out = "[";
    const uint32_t now = millis();
    for (uint8_t i = 0; i < kSlots; ++i) {
        if (i) out += ',';
        out += "{\"state\":" + String(copy[i].state);
        out += ",\"online\":";
        out += paired && copy[i].seen && now - copy[i].seen < 12000 ? "true" : "false";
        out += ",\"name\":" + escape(copy[i].name) + "}";
    }
    return out + ']';
}
String ir_link_connection_json() {
    uint8_t paired[6], candidate[6]; bool pairedSet, candidateSet;
    uint32_t candidateAt, newest = 0;
    portENTER_CRITICAL(&mux);
    pairedSet = hasPair; candidateSet = hasCandidate; candidateAt = candidateSeen;
    memcpy(paired, pairedMac, 6); memcpy(candidate, candidateMac, 6);
    for (const auto &slot : slots) if (slot.seen > newest) newest = slot.seen;
    portEXIT_CRITICAL(&mux);
    String out = "{\"paired\":";
    out += pairedSet ? escape(macText(paired).c_str()) : "null";
    out += ",\"candidate\":";
    out += candidateSet && millis() - candidateAt < 12000 ? escape(macText(candidate).c_str()) : "null";
    out += ",\"online\":";
    out += pairedSet && newest && millis() - newest < 12000 ? "true" : "false";
    out += ",\"writable\":";
    out += pairingWritable ? "true}" : "false}";
    return out;
}
bool ir_link_pair(const char *mac) {
    uint8_t parsed[6];
    if (!pairingWritable || !parseMac(mac, parsed)) return false;
    bool discovered;
    portENTER_CRITICAL(&mux);
    discovered = hasCandidate && millis() - candidateSeen < 12000 && !memcmp(parsed, candidateMac, 6);
    portEXIT_CRITICAL(&mux);
    if (!discovered || pairingStore.putBytes("mac", parsed, 6) != 6) return false;
    portENTER_CRITICAL(&mux);
    memcpy(pairedMac, parsed, 6); hasPair = true;
    memset(slots, 0, sizeof(slots));
    portEXIT_CRITICAL(&mux);
    return true;
}
bool ir_link_unpair() {
    if (!pairingWritable || !pairingStore.remove("mac")) return false;
    portENTER_CRITICAL(&mux);
    hasPair = false; hasCandidate = false;
    memset(pairedMac, 0, sizeof(pairedMac)); memset(slots, 0, sizeof(slots));
    portEXIT_CRITICAL(&mux);
    return true;
}
