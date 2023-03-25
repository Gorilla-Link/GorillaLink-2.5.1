#pragma once

#include "targets.h"
#include "DAC.h"
#include "device.h"

#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
#include "SX127xDriver.h"
#elif Regulatory_Domain_ISM_2400
#include "SX1280Driver.h"
#endif

#if defined(PLATFORM_ESP32)
#include <nvs_flash.h>
#include <nvs.h>
#endif

#if defined(TARGET_RX)
    // These are "fake" values as the power on the RX is not user selectable
    #define MinPower PWR_10mW
    #define MaxPower PWR_10mW
#endif

#if defined(HighPower) && !defined(UNLOCK_HIGHER_POWER)
    #undef MaxPower
    #define MaxPower HighPower
#endif

#if !defined(DefaultPower)
    #define DefaultPower PWR_50mW
#endif

#if defined(Regulatory_Domain_EU_CE_2400)
    #undef MinPower
    #define MinPower PWR_10mW

    #undef MaxPower
    #define MaxPower PWR_10mW

    #if defined(HighPower)
        #undef HighPower
        #define HighPower PWR_10mW
    #endif
#endif

typedef enum
{
    PWR_10mW = 0,
    PWR_25mW = 1,
    PWR_50mW = 2,
    PWR_100mW = 3,
    PWR_250mW = 4,
    PWR_500mW = 5,
    PWR_1000mW = 6,
    PWR_2000mW = 7,
    PWR_COUNT = 8
} PowerLevels_e;

class POWERMGNT
{

private:
    static PowerLevels_e CurrentPower;
    static int8_t CurrentSX1280Power;
    static PowerLevels_e FanEnableThreshold;
    static void updateFan();
#if defined(PLATFORM_ESP32)
    static nvs_handle  handle;
#endif
    static void LoadCalibration();

public:
    static void setPower(PowerLevels_e Power);
    static PowerLevels_e incPower();
    static PowerLevels_e decPower();
    static PowerLevels_e currPower();
    static void incSX1280Ouput();
    static void decSX1280Ouput();
    static int8_t currentSX1280Ouput();
    static uint8_t powerToCrsfPower(PowerLevels_e Power);
    static PowerLevels_e getDefaultPower();
    static uint8_t getPowerIndBm();
    static void setDefaultPower();
    static void init();
    static void SetPowerCaliValues(int8_t *values, size_t size);
    static void GetPowerCaliValues(int8_t *values, size_t size);
};


#define CALIBRATION_MAGIC    0x43414C << 8   //['C', 'A', 'L']
#define CALIBRATION_VERSION   1
