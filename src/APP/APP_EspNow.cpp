#include "APP_EspNow.h"
#include "Service/AP.h"
#include "Service/EventBus.h"

void APP_espnow_init()
{
    EventBus::publish(EVENT_SYS_ESPNOW);
}
