#include <cstdint>
#include "stubborn_sender.h"

StubbornSender::StubbornSender(uint8_t maxPackageIndex)
{
    this->maxPackageIndex = maxPackageIndex;
    this->ResetState();
}

void StubbornSender::ResetState()
{
    data = nullptr;
    bytesPerCall = 1;
    currentOffset = 0;
    currentPackage = 0;
    length = 0;
    waitUntilTelemetryConfirm = true;
    waitCount = 0;
    // 80 corresponds to UpdateTelemetryRate(ANY, 2, 1), which is what the TX uses in boost mode
    maxWaitCount = 80;
    senderState = SENDER_IDLE;
}

/***
 * Queues a message to send, will abort the current message if one is currently being transmitted
 ***/
void StubbornSender::SetDataToTransmit(uint8_t lengthToTransmit, uint8_t* dataToTransmit, uint8_t bytesPerCall)
{
    if (lengthToTransmit / bytesPerCall >= maxPackageIndex)
    {
        return;
    }

    length = lengthToTransmit;
    data = dataToTransmit;
    currentOffset = 0;
    currentPackage = 1;
    waitCount = 0;
    this->bytesPerCall = bytesPerCall;
    senderState = (senderState == SENDER_IDLE) ? SENDING : RESYNC_THEN_SEND;
}

bool StubbornSender::IsActive()
{
    return senderState != SENDER_IDLE;
}

void StubbornSender::GetCurrentPayload(uint8_t *packageIndex, uint8_t *count, uint8_t **currentData)
{
    switch (senderState)
    {
    case RESYNC:
    case RESYNC_THEN_SEND:
        *packageIndex = maxPackageIndex;
        *count = 0;
        *currentData = 0;
        break;
    case SENDING:
        *currentData = data + currentOffset;
        *packageIndex = currentPackage;
        if (bytesPerCall > 1)
        {
            if (currentOffset + bytesPerCall <= length)
            {
                *count = bytesPerCall;
            }
            else
            {
                *count = length-currentOffset;
            }
        }
        else
        {
            *count = 1;
        }
        break;
    default:
        *count = 0;
        *currentData = 0;
        *packageIndex = 0;
    }
}

void StubbornSender::ConfirmCurrentPayload(bool telemetryConfirmValue)
{
    stubborn_sender_state_s nextSenderState = senderState;

    switch (senderState)
    {
    case SENDING:
        if (telemetryConfirmValue != waitUntilTelemetryConfirm)
        {
            waitCount++;
            if (waitCount > maxWaitCount)
            {
                waitUntilTelemetryConfirm = !telemetryConfirmValue;
                nextSenderState = RESYNC;
            }
            break;
        }

        currentOffset += bytesPerCall;
        currentPackage++;
        waitUntilTelemetryConfirm = !waitUntilTelemetryConfirm;
        waitCount = 0;

        if (currentOffset >= length)
        {
            nextSenderState = WAIT_UNTIL_NEXT_CONFIRM;
        }
        break;

    case RESYNC:
    case RESYNC_THEN_SEND:
    case WAIT_UNTIL_NEXT_CONFIRM:
        if (telemetryConfirmValue == waitUntilTelemetryConfirm)
        {
            nextSenderState = (senderState == RESYNC_THEN_SEND) ? SENDING : SENDER_IDLE;
            waitUntilTelemetryConfirm = !telemetryConfirmValue;
        }
        // switch to resync if tx does not confirm value fast enough
        else if (senderState == WAIT_UNTIL_NEXT_CONFIRM)
        {
            waitCount++;
            if (waitCount > maxWaitCount)
            {
                waitUntilTelemetryConfirm = !telemetryConfirmValue;
                nextSenderState = RESYNC;
            }
        }
        break;

    case SENDER_IDLE:
        break;
    }

    senderState = nextSenderState;
}

/*
 * Called when the telemetry ratio or air rate changes, calculate
 * the new threshold for how many times the telemetryConfirmValue
 * can be wrong in a row before giving up and going to RESYNC
 */
void StubbornSender::UpdateTelemetryRate(uint16_t airRate, uint8_t tlmRatio, uint8_t tlmBurst)
{
    // consipicuously unused airRate parameter, the wait count is strictly based on number
    // of packets, not time between the telemetry packets, or a wall clock timeout
    (void)airRate;
    // The expected number of packet periods between telemetry packets
    uint32_t packsBetween = tlmRatio * (1 + tlmBurst) / tlmBurst;
    maxWaitCount = packsBetween * SSENDER_MAX_MISSED_PACKETS;
}
