#include <USBComposite.h>

uint8 val;
unsigned long last_audio_read_ms;
double theta;
double inc;

USBCompositeSerial CompositeSerial;
USBAUDIO AUDIO;

void setup()
{
  theta = 0.0;
  inc = 0.05;

  AUDIO.begin(CompositeSerial, MIC_MONO);

  last_audio_read_ms = micros();
}

void loop()
{
  unsigned long current_micros = micros();
  unsigned long elapsed = current_micros - last_audio_read_ms;

  if (elapsed >= AUDIO.getSamplePeriod())
  {
    theta += inc;

    val = (uint8) (((sin(theta) + 1.0) / 2.0) * 255);

    last_audio_read_ms = current_micros;

    while (!AUDIO.write(&val, 1));
  }

  if (CompositeSerial.available())
  {
    char ch = CompositeSerial.read();
    if (ch == '+')
    {
      inc += 0.005;
      CompositeSerial.println(inc);
    }
    else if (ch == '-')
    {
      inc -= 0.005;
      CompositeSerial.println(inc);
    }
  }
}
