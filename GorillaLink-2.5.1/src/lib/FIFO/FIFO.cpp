/*
 * FIFO Buffer
 * Implementation uses arrays to conserve memory
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Daniel Eisterhold
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * Modified/Ammended by Alessandro Carcione 2020
 */
#include "FIFO.h"
#include "logging.h"

FIFO::FIFO()
{
    head = 0;
    tail = 0;
    numElements = 0;
}

FIFO::~FIFO()
{
}

void ICACHE_RAM_ATTR FIFO::push(const uint8_t data)
{
    if (numElements == FIFO_SIZE)
    {
        ERRLN("Buffer full, will flush");
        flush();
        return;
    }
    else
    {
        numElements++;
        buffer[tail] = data;
        tail = (tail + 1) % FIFO_SIZE;
    }
}

void ICACHE_RAM_ATTR FIFO::pushBytes(const uint8_t *data, uint8_t len)
{
    if (numElements + len > FIFO_SIZE)
    {
        ERRLN("Buffer full, will flush");
        flush();
        return;
    }
    for (int i = 0; i < len; i++)
    {
        buffer[tail] = data[i];
        tail = (tail + 1) % FIFO_SIZE;
    }
    numElements += len;
}

uint8_t ICACHE_RAM_ATTR FIFO::pop()
{
    if (numElements == 0)
    {
        // DBGLN(F("Buffer empty"));
        return 0;
    }
    else
    {
        numElements--;
        uint8_t data = buffer[head];
        head = (head + 1) % FIFO_SIZE;
        return data;
    }
}

void ICACHE_RAM_ATTR FIFO::popBytes(uint8_t *data, uint8_t len)
{
    if (numElements < len)
    {
        // DBGLN(F("Buffer underrun"));
        flush();
    }
    else
    {
        numElements -= len;
        for (int i = 0; i < len; i++)
        {
            data[i] = buffer[head];
            head = (head + 1) % FIFO_SIZE;
        }
    }
}

uint8_t ICACHE_RAM_ATTR FIFO::peek()
{
    if (numElements == 0)
    {
        // DBGLN(F("Buffer empty"));
        return 0;
    }
    else
    {
        uint8_t data = buffer[head];
        return data;
    }
}

uint16_t ICACHE_RAM_ATTR FIFO::size()
{
    return numElements;
}

void ICACHE_RAM_ATTR FIFO::flush()
{
    head = 0;
    tail = 0;
    numElements = 0;
}

bool ICACHE_RAM_ATTR FIFO::ensure(uint8_t requiredSize)
{
    if(requiredSize > FIFO_SIZE)
        return false;
    while(!available(requiredSize)) {
        uint8_t len = pop();
        head = (head + len) % FIFO_SIZE;
        numElements -= len;
    }
    return true;
}
