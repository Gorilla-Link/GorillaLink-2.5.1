#include "targets.h"
#include "common.h"
#include "device.h"

#include "helpers.h"
#include "logging.h"

// Even though we aren't using anything this keeps the PIO dependency analyzer happy!
#include "POWERMGNT.h"

#if defined(GPIO_PIN_BUZZER) && (GPIO_PIN_BUZZER != UNDEF_PIN)

static void initializeBuzzer()
{
    pinMode(GPIO_PIN_BUZZER, OUTPUT);
}

static const uint16_t failedTune[][2] = {{480, 200},{400, 200}};
static const uint16_t crossfireTune[][2] = {{520, 150},{676, 300},{0,1000}}; // we have a dead-time to stop spamming
static const uint16_t noCrossfireTune[][2] = {{676, 300},{520, 150}};
#if defined(MY_STARTUP_MELODY_ARR)
// It's silly but I couldn't help myself. See: BLHeli32 startup tones.
static const uint16_t melody[][2] = MY_STARTUP_MELODY_ARR;
#elif defined(JUST_BEEP_ONCE)
static const uint16_t melody[][2] = {{400,200},{480,200}};
#else
static const uint16_t melody[][2] = {{659,300},{659,300},{523,100},{659,300},{783,550},{392,575}};
#endif

static uint8_t tunepos = 0;
static bool callAfterComplete = false;
static bool startComplete = false;
static const uint16_t (*_tune)[2];
static int _numTones;

static int playTune()
{
    if (tunepos >= _numTones)
    {
        noTone(GPIO_PIN_BUZZER);
        pinMode(GPIO_PIN_BUZZER, INPUT);
        tunepos = 0;
        DBGVLN(">> end tune");
        return DURATION_NEVER;
    }
    if (_tune[tunepos][0] == 0)
    {
        // we have dead-time so play no-tone and just set the timeout
        noTone(GPIO_PIN_BUZZER);
    }
    else
    {
        tone(GPIO_PIN_BUZZER, _tune[tunepos][0], _tune[tunepos][1]);
    }
    tunepos++;
    return _tune[tunepos-1][1];
}

static int startTune(const uint16_t tune[][2], int numTones)
{
    _tune = tune;
    _numTones = numTones;
    pinMode(GPIO_PIN_BUZZER, OUTPUT);
    tunepos = 0;
    return playTune();
}

static int updateBuzzer()
{
    if (connectionState == radioFailed)
    {
        DBGVLN(">> start failed tune");
        return startTune(failedTune, ARRAY_SIZE(failedTune));
    }
    else if (connectionState == noCrossfire)
    {
        DBGVLN(">> start no-xfire tune");
        return startTune(noCrossfireTune, ARRAY_SIZE(noCrossfireTune));
    }
#if !defined(DISABLE_STARTUP_BEEP)
    else if (connectionState == connected || connectionState == disconnected)
    {
        DBGVLN(">> start conn/disconn tune");
        return startTune(crossfireTune, ARRAY_SIZE(crossfireTune));
    }
#endif
    return DURATION_NEVER;
}

static int start()
{
#if !defined(DISABLE_STARTUP_BEEP)
    DBGVLN(">> start startup tune");
    return startTune(melody, ARRAY_SIZE(melody));
#else
    return DURATION_NEVER;
#endif
}

static int event()
{
    static connectionState_e lastConnectionState = MODE_STATES;
    if (tunepos == 0)
    {
        if (connectionState != lastConnectionState)
        {
            DBGVLN(">> starting %d -> %d", lastConnectionState, connectionState);
            lastConnectionState = connectionState;
            return updateBuzzer();
        }
        lastConnectionState = connectionState;
        return DURATION_NEVER;
    }
    if(!startComplete)
    {
        // if we are currently playing the startup tune then set a flag so that we trigger an update after it completes
        DBGVLN(">> call after");
        callAfterComplete = true;
    }
    else
    {
        DBGVLN(">> ignored %d -> %d", lastConnectionState, connectionState);
    }
    return DURATION_IGNORE;
}

static int timeout()
{
    int duration = playTune();
    DBGVLN(">> timeout %d", duration);
    if (duration == DURATION_NEVER && !startComplete)
    {
        startComplete = true;
        if (callAfterComplete)
        {
            // tune completed and the flag is set to start the next one
            callAfterComplete = false;
            DBGVLN(">> call after update");
            duration = updateBuzzer();
        }
    }
    return duration;
}

device_t Buzzer_device = {
    .initialize = initializeBuzzer,
    .start = start,
    .event = event,
    .timeout = timeout
};

#endif
