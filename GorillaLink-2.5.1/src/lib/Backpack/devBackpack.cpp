#include "targets.h"
#include "common.h"
#include "device.h"
#include "msp.h"
#include "msptypes.h"
#include "CRSF.h"
#include "config.h"

#define BACKPACK_TIMEOUT 20    // How often to check for backpack commands

extern bool InBindingMode;
extern Stream *TxBackpack;

bool TxBackpackWiFiReadyToSend = false;
bool VRxBackpackWiFiReadyToSend = false;

bool lastRecordingState = false;

#if defined(GPIO_PIN_BACKPACK_EN) && GPIO_PIN_BACKPACK_EN != UNDEF_PIN

#if BACKPACK_LOGGING_BAUD != 460800
#error "Backpack passthrough flashing requires BACKPACK_LOGGING_BAUD==460800"
#endif

#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
#include "SX127xDriver.h"
extern SX127xDriver Radio;
#elif defined(Regulatory_Domain_ISM_2400)
#include "SX1280Driver.h"
extern SX1280Driver Radio;
#endif

#include "CRSF.h"
#include "hwTimer.h"

void startPassthrough()
{
    // stop everyhting
    devicesStop();
    Radio.End();
    hwTimer::stop();
    CRSF::End();

    // get ready for passthrough
    CRSF::Port.begin(BACKPACK_LOGGING_BAUD, SERIAL_8N1, GPIO_PIN_RCSIGNAL_RX, GPIO_PIN_RCSIGNAL_TX);
    disableLoopWDT();

    // reset ESP8285 into bootloader mode
    digitalWrite(GPIO_PIN_BACKPACK_BOOT, HIGH);
    delay(100);
    digitalWrite(GPIO_PIN_BACKPACK_EN, LOW);
    delay(100);
    digitalWrite(GPIO_PIN_BACKPACK_EN, HIGH);
    delay(50);
    digitalWrite(GPIO_PIN_BACKPACK_BOOT, LOW);

    CRSF::Port.flush();
    TxBackpack->flush();

    uint8_t buf[64];
    while (TxBackpack->available())
        TxBackpack->readBytes(buf, sizeof(buf));

    // go hard!
    for (;;)
    {
        int r = CRSF::Port.available();
        if (r > sizeof(buf))
            r = sizeof(buf);
        r = CRSF::Port.readBytes(buf, r);
        TxBackpack->write(buf, r);

        r = TxBackpack->available();
        if (r > sizeof(buf))
            r = sizeof(buf);
        r = TxBackpack->readBytes(buf, r);
        CRSF::Port.write(buf, r);
    }
}
#endif

static void BackpackWiFiToMSPOut(uint16_t command)
{
    mspPacket_t packet;
    packet.reset();
    packet.makeCommand();
    packet.function = command;
    packet.addByte(0);

    MSP::sendPacket(&packet, TxBackpack); // send to tx-backpack as MSP
}

void BackpackBinding()
{
    mspPacket_t packet;
    packet.reset();
    packet.makeCommand();
    packet.function = MSP_ELRS_BIND;
    packet.addByte(MasterUID[0]);
    packet.addByte(MasterUID[1]);
    packet.addByte(MasterUID[2]);
    packet.addByte(MasterUID[3]);
    packet.addByte(MasterUID[4]);
    packet.addByte(MasterUID[5]);

    MSP::sendPacket(&packet, TxBackpack); // send to tx-backpack as MSP
}

uint8_t GetDvrDelaySeconds(uint8_t index)
{
    constexpr uint8_t delays[] = {0, 5, 15, 30, 45, 60, 120};
    return delays[index >= sizeof(delays) ? 0 : index];
}

static void AuxStateToMSPOut()
{
#if defined(USE_TX_BACKPACK)
    if (config.GetDvrAux() == 0)
    {
        // DVR AUX control is off
        return;
    }

    uint8_t auxNumber = (config.GetDvrAux() - 1) / 2 + 4;
    uint8_t auxInverted = (config.GetDvrAux() + 1) % 2;

    bool recordingState = CRSF_to_BIT(CRSF::ChannelDataIn[auxNumber]) ^ auxInverted;

    if (recordingState == lastRecordingState)
    {
        // Channel state has not changed since we last checked
        return;
    }
    lastRecordingState = recordingState;

    uint16_t delay = 0;

    if (recordingState)
    {
        delay = GetDvrDelaySeconds(config.GetDvrStartDelay());
    }

    if (!recordingState)
    {
        delay = GetDvrDelaySeconds(config.GetDvrStopDelay());
    }

    mspPacket_t packet;
    packet.reset();
    packet.makeCommand();
    packet.function = MSP_ELRS_BACKPACK_SET_RECORDING_STATE;
    packet.addByte(recordingState);
    packet.addByte(delay & 0xFF); // delay byte 1
    packet.addByte(delay >> 8); // delay byte 2
    
    MSP::sendPacket(&packet, TxBackpack); // send to tx-backpack as MSP
#endif // USE_TX_BACKPACK
}

static void initialize()
{
#if defined(GPIO_PIN_BACKPACK_EN) && GPIO_PIN_BACKPACK_EN != UNDEF_PIN
    pinMode(0, INPUT); // setup so we can detect pinchange for passthrough mode
    // reset the ESP8285 so we know it's running
    pinMode(GPIO_PIN_BACKPACK_BOOT, OUTPUT);
    pinMode(GPIO_PIN_BACKPACK_EN, OUTPUT);
    digitalWrite(GPIO_PIN_BACKPACK_EN, LOW);   // enable low
    digitalWrite(GPIO_PIN_BACKPACK_BOOT, LOW); // bootloader pin high
    delay(50);
    digitalWrite(GPIO_PIN_BACKPACK_EN, HIGH); // enable high
#endif

    CRSF::RCdataCallback = AuxStateToMSPOut;
}

static int start()
{
    return DURATION_IMMEDIATELY;
}

static int timeout()
{
    if (InBindingMode)
    {
        BackpackBinding();
        return 1000;        // don't check for another second so we don't spam too hard :-)
    }

    if (TxBackpackWiFiReadyToSend && connectionState < MODE_STATES)
    {
        TxBackpackWiFiReadyToSend = false;
        BackpackWiFiToMSPOut(MSP_ELRS_SET_TX_BACKPACK_WIFI_MODE);
    }

    if (VRxBackpackWiFiReadyToSend && connectionState < MODE_STATES)
    {
        VRxBackpackWiFiReadyToSend = false;
        BackpackWiFiToMSPOut(MSP_ELRS_SET_VRX_BACKPACK_WIFI_MODE);
    }

#if defined(GPIO_PIN_BACKPACK_EN) && GPIO_PIN_BACKPACK_EN != UNDEF_PIN
    if (!digitalRead(0))
    {
        startPassthrough();
        return DURATION_NEVER;
    }
#endif
    return BACKPACK_TIMEOUT;
}

device_t Backpack_device = {
    .initialize = initialize,
    .start = start,
    .event = NULL,
    .timeout = timeout
};
