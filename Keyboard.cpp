#include "USBHID.h"

//================================================================================
//================================================================================
//	Keyboard

void HIDKeyboard::begin(void){
}

void HIDKeyboard::end(void) {
}

size_t HIDKeyboard::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			//setWriteError();
			return 0;
		}
		if (k & SHIFT) {						// it's a capital letter or other character reached with shift
			keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (keyReport.keys[0] != k && keyReport.keys[1] != k &&
		keyReport.keys[2] != k && keyReport.keys[3] != k &&
		keyReport.keys[4] != k && keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (keyReport.keys[i] == 0x00) {
				keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			//setWriteError();
			return 0;
		}
	}
	
	sendReport();
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t HIDKeyboard::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && keyReport.keys[i] == k) {
			keyReport.keys[i] = 0x00;
		}
	}
	
	sendReport();
	return 1;
}

void HIDKeyboard::releaseAll(void)
{
	keyReport.keys[0] = 0;
	keyReport.keys[1] = 0;
	keyReport.keys[2] = 0;
	keyReport.keys[3] = 0;
	keyReport.keys[4] = 0;
	keyReport.keys[5] = 0;
	keyReport.modifiers = 0;
	
	sendReport();
}

size_t HIDKeyboard::write(uint8_t c)
{
	uint8_t p = 0;

	p = press(c);	// Keydown
	release(c);		// Keyup
	
	return (p);		// Just return the result of press() since release() almost always returns 1
}


HIDKeyboard Keyboard;
