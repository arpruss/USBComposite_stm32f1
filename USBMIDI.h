/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2013 Magnus Lundin.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @brief Wirish USB MIDI port (MidiUSB).
 */

#ifndef _USBMIDI_H_
#define _USBMIDI_H_

#include <boards.h>
#include <USBComposite.h>
#include "usb_generic.h"

class USBMidi {
private:
    bool enabled = false;
    int inputChannel;
    void dispatchPacket(uint32 packet);
    
public:
	static bool init(USBMidi* me);
	bool registerComponent();
	void setChannel(unsigned channel=0);
	unsigned getChannel() {
		return inputChannel;
	}
    void begin(unsigned int channel = 0);
    void end();
    
    uint32 available(void);
    
    uint32 readPackets(void *buf, uint32 len);
    uint32 readPacket(void);
    
    void writePacket(uint32);
    void writePackets(const void*, uint32);
    
    uint8 isConnected();
    uint8 pending();

	// call each time in loop(), but only if you are handling MIDI messages
    void poll();
    
    void sendNoteOff(unsigned int channel, unsigned int note, unsigned int velocity);
    void sendNoteOn(unsigned int channel, unsigned int note, unsigned int velocity);
    void sendVelocityChange(unsigned int channel, unsigned int note, unsigned int velocity);
    void sendControlChange(unsigned int channel, unsigned int controller, unsigned int value);
    void sendProgramChange(unsigned int channel, unsigned int program);
    void sendAfterTouch(unsigned int channel, unsigned int velocity);
    void sendPitchChange(unsigned int pitch);
    void sendSongPosition(unsigned int position);
    void sendSongSelect(unsigned int song);
    void sendTuneRequest(void);
    void sendSync(void);
    void sendStart(void);
    void sendContinue(void);
    void sendStop(void);
    void sendActiveSense(void);
    void sendReset(void);
    
    // for overloading in a subclass in order to handle data from MIDI interface
    virtual void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity);
    virtual void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity);
    virtual void handleVelocityChange(unsigned int channel, unsigned int note, unsigned int velocity);
    virtual void handleControlChange(unsigned int channel, unsigned int controller, unsigned int value);
    virtual void handleProgramChange(unsigned int channel, unsigned int program);
    virtual void handleAfterTouch(unsigned int channel, unsigned int velocity);
    virtual void handlePitchChange(unsigned int pitch);
    virtual void handleSongPosition(unsigned int position);
    virtual void handleSongSelect(unsigned int song);
    virtual void handleTuneRequest(void);
    virtual void handleSync(void);
    virtual void handleStart(void);
    virtual void handleContinue(void);
    virtual void handleStop(void);
    virtual void handleActiveSense(void);
    virtual void handleReset(void);
};

extern USBMidi USBMIDI;


#endif
