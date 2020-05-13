#include "USBComposite.h"

//================================================================================
//================================================================================
//	Mouse

void HIDDigitizer::begin(void){
}

void HIDDigitizer::end(void){
}

void HIDDigitizer::move(int16 x, int16 y)
{
    report.x = x;
    report.y = y;

    sendReport();
}

void HIDDigitizer::buttons(uint8_t b)
{
	if (b != report.buttons)
	{
        report.buttons = b;
        sendReport();
	}
}

void HIDDigitizer::press(uint8_t b)
{
	buttons(report.buttons | b);
}

void HIDDigitizer::release(uint8_t b)
{
	buttons(report.buttons & ~b);
}

bool HIDDigitizer::isPressed(uint8_t b)
{
	if ((b & report.buttons) != 0)
		return true;
	return false;
}
