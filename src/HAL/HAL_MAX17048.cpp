/*
    MAX17048电池监测芯片功能实现
*/
#include "HAL.h"
#include <Arduino.h>
#include <math.h>
#include "Config.h"

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

static bool gaugeReady=false;
SFE_MAX1704X lipo(MAX1704X_MAX17048); // Create a MAX17048

float HAL::MAX17048_getVoltage()
{
    return gaugeReady?lipo.getVoltage():NAN;
}

float HAL::MAX17048_getSOC()
{
    return gaugeReady?lipo.getSOC():NAN;
}

float HAL::MAX17048_getChangeRate()
{
    return gaugeReady?lipo.getChangeRate():NAN;
}

bool HAL::MAX17048_init()
{
    Wire1.begin(PIN_BATT_GAUGE_SDA, PIN_BATT_GAUGE_SCL, 400000);

    gaugeReady=lipo.begin(Wire1);
    if (!gaugeReady)
    {
        DualSerial.println("[ERROR] MAX17048 not detected");
        return false;
    }

    return true;
}
