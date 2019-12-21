/* Copyright (c) 2019, Scott Moreau
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "USBComposite.h"

#include <stdint.h>

static bool enabledAUDIO = false;

/*
 * USB AUDIO interface
 */

bool USBAUDIO::init(USBAUDIO* me) {
    usb_audio_setEPSize(me->txPacketSize);

    return true;
}

bool USBAUDIO::registerComponent() {

    return USBComposite.add(&usbAUDIOPart, this, (USBPartInitializer)&USBAUDIO::init);
}

uint32 USBAUDIO::write(const uint8* buffer, uint32 length)
{
    return usb_audio_write_tx_data(buffer, length);
}

uint32 USBAUDIO::read(uint8* buffer, uint32 length)
{
    return usb_audio_read_rx_data(buffer, length);
}

uint8 USBAUDIO::getSamplePeriod(void)
{
    return samplePeriod;
}

static void setSamplePeriod(uint16 *samplePeriod, uint16 sampleRate)
{
    *samplePeriod = 1000000 / sampleRate - 1;
}

void USBAUDIO::setPacketCallback(void (*callback)(uint8 size)) {
    audio_set_packet_callback(callback);
}

void USBAUDIO::begin(uint16 type, uint16 sampleRate) {
    if (enabledAUDIO)
        return;

    USBComposite.clear();

    txPacketSize = usb_audio_init(type, sampleRate);
    setSamplePeriod(&samplePeriod, sampleRate);

    registerComponent();

    USBComposite.begin();

    enabledAUDIO = true;
}

void USBAUDIO::begin(USBCompositeSerial serial, uint16 type, uint16 sampleRate) {
    if (enabledAUDIO)
        return;

    USBComposite.clear();

    txPacketSize = usb_audio_init(type, sampleRate);
    setSamplePeriod(&samplePeriod, sampleRate);

    registerComponent();

    serial.registerComponent();

    USBComposite.begin();

    enabledAUDIO = true;
}

void USBAUDIO::end(void){
    if(enabledAUDIO) {
        USBComposite.end();
        enabledAUDIO = false;
    }
}
