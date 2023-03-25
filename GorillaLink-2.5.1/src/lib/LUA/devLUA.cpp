#ifdef TARGET_TX

#include "common.h"
#include "device.h"

#include "CRSF.h"
#include "POWERMGNT.h"
#include "config.h"
#include "logging.h"
#include "lua.h"
#include "OTA.h"
#include "hwTimer.h"

#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
#include "SX127xDriver.h"
extern SX127xDriver Radio;
#elif defined(Regulatory_Domain_ISM_2400)
#include "SX1280Driver.h"
extern SX1280Driver Radio;
#else
#error "Radio configuration is not valid!"
#endif

static const char emptySpace[1] = {0};
static char strPowerLevels[] = "10;25;50;100;250;500;1000;2000";

static struct luaItem_selection luaAirRate = {
    {"Packet Rate", CRSF_TEXT_SELECTION},
    0, // value
#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
    "25(-123dbm);50(-120dbm);100(-117dbm);200(-112dbm)",
#elif defined(Regulatory_Domain_ISM_2400)
    "50(-117dbm);150(-112dbm);250(-108dbm);500(-105dbm)",
#endif
    "Hz"
};

static struct luaItem_selection luaTlmRate = {
    {"Telem Ratio", CRSF_TEXT_SELECTION},
    0, // value
    "Off;1:128;1:64;1:32;1:16;1:8;1:4;1:2",
    emptySpace
};

//----------------------------POWER------------------
static struct luaItem_folder luaPowerFolder = {
    {"TX Power", CRSF_FOLDER},
};

static struct luaItem_selection luaPower = {
    {"Max Power", CRSF_TEXT_SELECTION},
    0, // value
    strPowerLevels,
    "mW"
};

static struct luaItem_selection luaDynamicPower = {
    {"Dynamic", CRSF_TEXT_SELECTION},
    0, // value
    "Off;On;AUX9;AUX10;AUX11;AUX12",
    emptySpace
};

#if defined(GPIO_PIN_FAN_EN)
static struct luaItem_selection luaFanThreshold = {
    {"Fan Thresh", CRSF_TEXT_SELECTION},
    0, // value
    "10mW;25mW;50mW;100mW;250mW;500mW;1000mW;2000mW;Never",
    emptySpace // units embedded so it won't display "NevermW"
};
#endif

#if defined(Regulatory_Domain_EU_CE_2400)
static struct luaItem_string luaCELimit = {
    {"10mW CE LIMIT", CRSF_INFO},
    emptySpace
};
#endif

//----------------------------POWER------------------

static struct luaItem_selection luaSwitch = {
    {"Switch Mode", CRSF_TEXT_SELECTION},
    0, // value
    "Hybrid;Wide",
    emptySpace
};

static struct luaItem_selection luaModelMatch = {
    {"Model Match", CRSF_TEXT_SELECTION},
    0, // value
    "Off;On",
    emptySpace
};

static struct luaItem_command luaBind = {
    {"Bind", CRSF_COMMAND},
    0, // step
    emptySpace
};

static struct luaItem_string luaInfo = {
    {"Bad/Good", (crsf_value_type_e)(CRSF_INFO | CRSF_FIELD_ELRS_HIDDEN)},
    emptySpace
};

static struct luaItem_string luaELRSversion = {
    {version, CRSF_INFO},
    commit
};

//---------------------------- WiFi -----------------------------
static struct luaItem_folder luaWiFiFolder = {
    {"WiFi Connectivity", CRSF_FOLDER}
};

#if defined(PLATFORM_ESP32)
static struct luaItem_command luaWebUpdate = {
    {"Enable WiFi", CRSF_COMMAND},
    0, // step
    emptySpace
};
#endif

static struct luaItem_command luaRxWebUpdate = {
    {"Enable Rx WiFi", CRSF_COMMAND},
    0, // step
    emptySpace
};

#if defined(USE_TX_BACKPACK)
static struct luaItem_command luaTxBackpackUpdate = {
    {"Enable Backpack WiFi", CRSF_COMMAND},
    0, // step
    emptySpace
};

static struct luaItem_command luaVRxBackpackUpdate = {
    {"Enable VRx WiFi", CRSF_COMMAND},
    0, // step
    emptySpace
};
#endif // USE_TX_BACKPACK
//---------------------------- WiFi -----------------------------

#if defined(PLATFORM_ESP32)
static struct luaItem_command luaBLEJoystick = {
    {"BLE Joystick", CRSF_COMMAND},
    0, // step
    emptySpace
};
#endif

//----------------------------VTX ADMINISTRATOR------------------
static struct luaItem_folder luaVtxFolder = {
    {"VTX Administrator", CRSF_FOLDER},
};

static struct luaItem_selection luaVtxBand = {
    {"Band", CRSF_TEXT_SELECTION},
    0, // value
    "Off;A;B;E;F;R;L",
    emptySpace
};

static struct luaItem_selection luaVtxChannel = {
    {"Channel", CRSF_TEXT_SELECTION},
    0, // value
    "1;2;3;4;5;6;7;8",
    emptySpace
};

static struct luaItem_selection luaVtxPwr = {
    {"Pwr Lvl", CRSF_TEXT_SELECTION},
    0, // value
    "-;1;2;3;4;5;6;7;8",
    emptySpace
};

static struct luaItem_selection luaVtxPit = {
    {"Pitmode", CRSF_TEXT_SELECTION},
    0, // value
    "Off;On",
    emptySpace
};

static struct luaItem_command luaVtxSend = {
    {"Send VTx", CRSF_COMMAND},
    0, // step
    emptySpace
};
//----------------------------VTX ADMINISTRATOR------------------

#if defined(TARGET_TX_FM30)
struct luaItem_selection luaBluetoothTelem = {
    {"BT Telemetry", CRSF_TEXT_SELECTION},
    0, // value
    "Off;On",
    emptySpace
};
#endif

#if defined(USE_TX_BACKPACK)
//---------------------------- BACKPACK ------------------
static struct luaItem_folder luaBackpackFolder = {
    {"Backpack", CRSF_FOLDER},
};

static struct luaItem_selection luaDvrAux = {
    {"DVR AUX", CRSF_TEXT_SELECTION},
    0, // value
    "Off;AUX1;!AUX1;AUX2;!AUX2;AUX3;!AUX3;AUX4;!AUX4;AUX5;!AUX5;AUX6;!AUX6;AUX7;!AUX7;AUX8;!AUX8;AUX9;!AUX9;AUX10;!AUX10",
    emptySpace
};

static struct luaItem_selection luaDvrStartDelay = {
    {"DVR Srt Dly", CRSF_TEXT_SELECTION},
    0, // value
    "0s;5s;15s;30s;45s;1min;2min",
    emptySpace
};

static struct luaItem_selection luaDvrStopDelay = {
    {"DVR Stp Dly", CRSF_TEXT_SELECTION},
    0, // value
    "0s;5s;15s;30s;45s;1min;2min",
    emptySpace
};
#endif // USE_TX_BACKPACK

//---------------------------- BACKPACK ------------------

static char luaBadGoodString[10];

extern TxConfig config;
extern void VtxTriggerSend();
extern uint8_t adjustPacketRateForBaud(uint8_t rate);
extern void SetSyncSpam();
extern void EnterBindingMode();
extern bool InBindingMode;
extern bool connectionHasModelMatch;
extern bool RxWiFiReadyToSend;
#if defined(USE_TX_BACKPACK)
extern bool TxBackpackWiFiReadyToSend;
extern bool VRxBackpackWiFiReadyToSend;
#endif
#ifdef PLATFORM_ESP32
extern unsigned long rebootTime;
extern void beginWebsever();
#endif

static void luadevGeneratePowerOpts()
{
  // This function modifies the strPowerLevels in place and must not
  // be called more than once!
  char *out = strPowerLevels;
  PowerLevels_e pwr = PWR_10mW;
  // Count the semicolons to move `out` to point to the MINth item
  while (pwr < MinPower)
  {
    while (*out++ != ';') ;
    pwr = (PowerLevels_e)((unsigned int)pwr + 1);
  }
  // There is no min field, compensate by shifting the index when sending/receiving
  // luaPower.min = (uint8_t)MinPower;
  luaPower.options = (const char *)out;

  // Continue until after than MAXth item and drop a null in the orginal
  // string on the semicolon (not after like the previous loop)
  while (pwr <= MaxPower)
  {
    // If out still points to a semicolon from the last loop move past it
    if (*out)
      ++out;
    while (*out && *out != ';')
      ++out;
    pwr = (PowerLevels_e)((unsigned int)pwr + 1);
  }
  *out = '\0';
}

static void registerLuaParameters()
{
  registerLUAParameter(&luaAirRate, [](uint8_t id, uint8_t arg){
    if ((arg < RATE_MAX) && (arg >= 0))
    {
      uint8_t rate = RATE_MAX - 1 - arg;
      rate = adjustPacketRateForBaud(rate);
      config.SetRate(rate);
    }
  });
  registerLUAParameter(&luaTlmRate, [](uint8_t id, uint8_t arg){
    if ((arg <= (uint8_t)TLM_RATIO_1_2) && (arg >= (uint8_t)TLM_RATIO_NO_TLM))
    {
      config.SetTlm((expresslrs_tlm_ratio_e)arg);
    }
  });
  #if defined(TARGET_TX_FM30)
  registerLUAParameter(&luaBluetoothTelem, [](uint8_t id, uint8_t arg) {
    digitalWrite(GPIO_PIN_BLUETOOTH_EN, !arg);
    devicesTriggerEvent();
  });
  #endif
  registerLUAParameter(&luaSwitch, [](uint8_t id, uint8_t arg){
    // Only allow changing switch mode when disconnected since we need to guarantee
    // the pack and unpack functions are matched
    if (connectionState == disconnected)
    {
      // +1 to the mode because 1-bit was mode 0 and has been removed
      // The modes should be updated for 1.1RC so mode 0 can be smHybrid
      uint32_t newSwitchMode = (arg + 1) & 0b11;
      config.SetSwitchMode(newSwitchMode);
      OtaSetSwitchMode((OtaSwitchMode_e)newSwitchMode);
    }
  });
  registerLUAParameter(&luaModelMatch, [](uint8_t id, uint8_t arg){
    bool newModelMatch = arg;
    config.SetModelMatch(newModelMatch);
    if (connectionState == connected) {
      mspPacket_t msp;
      msp.reset();
      msp.makeCommand();
      msp.function = MSP_SET_RX_CONFIG;
      msp.addByte(MSP_ELRS_MODEL_ID);
      msp.addByte(newModelMatch ? CRSF::getModelID() : 0xff);
      CRSF::AddMspMessage(&msp);
    }
  });

  // POWER folder
  registerLUAParameter(&luaPowerFolder);
  luadevGeneratePowerOpts();
  registerLUAParameter(&luaPower, [](uint8_t id, uint8_t arg){
    config.SetPower((PowerLevels_e)constrain(arg + MinPower, MinPower, MaxPower));
  }, luaPowerFolder.common.id);
  registerLUAParameter(&luaDynamicPower, [](uint8_t id, uint8_t arg){
      config.SetDynamicPower(arg > 0);
      config.SetBoostChannel((arg - 1) > 0 ? arg - 1 : 0);
  }, luaPowerFolder.common.id);
#if defined(GPIO_PIN_FAN_EN)
  registerLUAParameter(&luaFanThreshold, [](uint8_t id, uint8_t arg){
      config.SetPowerFanThreshold(arg);
  }, luaPowerFolder.common.id);
#endif
#if defined(Regulatory_Domain_EU_CE_2400)
  registerLUAParameter(&luaCELimit, NULL, luaPowerFolder.common.id);
#endif
  // VTX folder
  registerLUAParameter(&luaVtxFolder);
  registerLUAParameter(&luaVtxBand, [](uint8_t id, uint8_t arg){
      config.SetVtxBand(arg);
  },luaVtxFolder.common.id);
  registerLUAParameter(&luaVtxChannel, [](uint8_t id, uint8_t arg){
      config.SetVtxChannel(arg);
  },luaVtxFolder.common.id);
  registerLUAParameter(&luaVtxPwr, [](uint8_t id, uint8_t arg){
      config.SetVtxPower(arg);
  },luaVtxFolder.common.id);
  registerLUAParameter(&luaVtxPit, [](uint8_t id, uint8_t arg){
      config.SetVtxPitmode(arg);
  },luaVtxFolder.common.id);
  registerLUAParameter(&luaVtxSend, [](uint8_t id, uint8_t arg){
    if (arg < 5) {
      VtxTriggerSend();
    }
    sendLuaCommandResponse(&luaVtxSend, arg < 5 ? 2 : 0, arg < 5 ? "Sending..." : "");
  },luaVtxFolder.common.id);

  // WIFI folder
  registerLUAParameter(&luaWiFiFolder);
  #if defined(PLATFORM_ESP32)
    registerLUAParameter(&luaWebUpdate, [](uint8_t id, uint8_t arg){
      if (arg == 4) // 4 = request confirmed, start
      {
        //confirm run on ELRSv2.lua or start command from CRSF configurator,
        //since ELRS LUA can do 2 step confirmation, it needs confirmation to start wifi to prevent stuck on
        //unintentional button press.
        sendLuaCommandResponse(&luaWebUpdate, 2, "WiFi Running...");
        connectionState = wifiUpdate;
      }
      else if (arg > 0 && arg < 4)
      {
        sendLuaCommandResponse(&luaWebUpdate, 3, "Enter WiFi Update?");
      }
      else if (arg == 5)
      {
        sendLuaCommandResponse(&luaWebUpdate, 0, "WiFi Cancelled");
        if (connectionState == wifiUpdate) {
          rebootTime = millis() + 400;
        }
      }
      else
      {
        sendLuaCommandResponse(&luaWebUpdate, luaWebUpdate.step, luaWebUpdate.info);
      }
    },luaWiFiFolder.common.id);
  #endif

  registerLUAParameter(&luaRxWebUpdate, [](uint8_t id, uint8_t arg){
    if (arg < 5) {
      RxWiFiReadyToSend = true;
    }
    sendLuaCommandResponse(&luaRxWebUpdate, arg < 5 ? 2 : 0, arg < 5 ? "Sending..." : "");
  },luaWiFiFolder.common.id);

  #if defined(USE_TX_BACKPACK)
  registerLUAParameter(&luaTxBackpackUpdate, [](uint8_t id, uint8_t arg){
    if (arg < 5) {
      TxBackpackWiFiReadyToSend = true;
    }
    sendLuaCommandResponse(&luaTxBackpackUpdate, arg < 5 ? 2 : 0, arg < 5 ? "Sending..." : "");
  },luaWiFiFolder.common.id);

  registerLUAParameter(&luaVRxBackpackUpdate, [](uint8_t id, uint8_t arg){
    if (arg < 5) {
      VRxBackpackWiFiReadyToSend = true;
    }
    sendLuaCommandResponse(&luaVRxBackpackUpdate, arg < 5 ? 2 : 0, arg < 5 ? "Sending..." : "");
  },luaWiFiFolder.common.id);
  #endif // USE_TX_BACKPACK

  #if defined(USE_TX_BACKPACK)
  // Backpack folder
  registerLUAParameter(&luaBackpackFolder);
  registerLUAParameter(&luaDvrAux, [](uint8_t id, uint8_t arg){
      config.SetDvrAux(arg);
  },luaBackpackFolder.common.id);
  registerLUAParameter(&luaDvrStartDelay, [](uint8_t id, uint8_t arg){
      config.SetDvrStartDelay(arg);
  },luaBackpackFolder.common.id);
  registerLUAParameter(&luaDvrStopDelay, [](uint8_t id, uint8_t arg){
      config.SetDvrStopDelay(arg);
  },luaBackpackFolder.common.id);
  #endif // USE_TX_BACKPACK

  #if defined(PLATFORM_ESP32)
    registerLUAParameter(&luaBLEJoystick, [](uint8_t id, uint8_t arg){
      if (arg == 4) // 4 = request confirmed, start
      {
        //confirm run on ELRSv2.lua or start command from CRSF configurator,
        //since ELRS LUA can do 2 step confirmation, it needs confirmation to start BLE to prevent stuck on
        //unintentional button press.
        sendLuaCommandResponse(&luaBLEJoystick, 2, "Joystick Running...");
        connectionState = bleJoystick;
      }
      else if (arg > 0 && arg < 4) //start command, 1 = start, 2 = running, 3 = request confirmation
      {
        sendLuaCommandResponse(&luaBLEJoystick, 3, "Start BLE Joystick?");
      }
      else if (arg == 5)
      {
        sendLuaCommandResponse(&luaBLEJoystick, 0, "Joystick Cancelled");
        if (connectionState == bleJoystick) {
          rebootTime = millis() + 400;
        }
      }
      else
      {
        sendLuaCommandResponse(&luaBLEJoystick, luaBLEJoystick.step, luaBLEJoystick.info);
      }
    });

  #endif

  registerLUAParameter(&luaBind, [](uint8_t id, uint8_t arg){
    if (arg < 5) {
      EnterBindingMode();
    }
    sendLuaCommandResponse(&luaBind, arg < 5 ? 2 : 0, arg < 5 ? "Binding..." : "");
  });

  registerLUAParameter(&luaInfo);
  registerLUAParameter(&luaELRSversion);
  registerLUAParameter(NULL);
}

static int event()
{
  setLuaWarningFlag(LUA_FLAG_MODEL_MATCH, connectionState == connected && connectionHasModelMatch == false);
  setLuaWarningFlag(LUA_FLAG_CONNECTED, connectionState == connected);
  uint8_t rate = adjustPacketRateForBaud(config.GetRate());
  setLuaTextSelectionValue(&luaAirRate, RATE_MAX - 1 - rate);
  setLuaTextSelectionValue(&luaTlmRate, config.GetTlm());
  setLuaTextSelectionValue(&luaSwitch,(uint8_t)(config.GetSwitchMode() - 1)); // -1 for missing sm1Bit
  setLuaTextSelectionValue(&luaModelMatch,(uint8_t)config.GetModelMatch());
  setLuaTextSelectionValue(&luaPower, config.GetPower() - MinPower);
#if defined(GPIO_PIN_FAN_EN)
  setLuaTextSelectionValue(&luaFanThreshold, config.GetPowerFanThreshold());
#endif

  uint8_t dynamic = config.GetDynamicPower() ? config.GetBoostChannel() + 1 : 0;
  setLuaTextSelectionValue(&luaDynamicPower,dynamic);

  setLuaTextSelectionValue(&luaVtxBand,config.GetVtxBand());
  setLuaTextSelectionValue(&luaVtxChannel,config.GetVtxChannel());
  setLuaTextSelectionValue(&luaVtxPwr,config.GetVtxPower());
  setLuaTextSelectionValue(&luaVtxPit,config.GetVtxPitmode());
  #if defined(TARGET_TX_FM30)
    setLuaTextSelectionValue(&luaBluetoothTelem, !digitalRead(GPIO_PIN_BLUETOOTH_EN));
  #endif

  #if defined(USE_TX_BACKPACK)
  setLuaTextSelectionValue(&luaDvrAux,config.GetDvrAux());
  setLuaTextSelectionValue(&luaDvrStartDelay,config.GetDvrStartDelay());
  setLuaTextSelectionValue(&luaDvrStopDelay,config.GetDvrStopDelay());
  #endif // USE_TX_BACKPACK
  return DURATION_IMMEDIATELY;
}

static int timeout()
{
  if(luaHandleUpdateParameter())
  {
    SetSyncSpam();
  }
  return DURATION_IMMEDIATELY;
}

static int start()
{
  CRSF::RecvParameterUpdate = &luaParamUpdateReq;
  registerLuaParameters();
  registerLUAPopulateParams([](){
    itoa(CRSF::BadPktsCountResult, luaBadGoodString, 10);
    strcat(luaBadGoodString, "/");
    itoa(CRSF::GoodPktsCountResult, luaBadGoodString + strlen(luaBadGoodString), 10);
    setLuaStringValue(&luaInfo, luaBadGoodString);
  });
  event();
  return DURATION_IMMEDIATELY;
}

device_t LUA_device = {
  .initialize = NULL,
  .start = start,
  .event = event,
  .timeout = timeout
};

#endif
