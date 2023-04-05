/*
  The Goertzel algorithm is long standing so see
  http://en.wikipedia.org/wiki/Goertzel_algorithm for a full description.
  It is often used in DTMF tone detection as an alternative to the Fast
  Fourier Transform because it is quick with low overheard because it
  is only searching for a single frequency rather than showing the
  occurrence of all frequencies.

  This work is entirely based on the Kevin Banks code found at
  http://www.embedded.com/design/configurable-systems/4024443/The-Goertzel-Algorithm
  so full credit to him for his generic implementation and breakdown. I've
  simply massaged it into an Arduino library. I recommend reading his article
  for a full description of whats going on behind the scenes.

  Created by Jacob Rosenthal, June 20, 2012.
  Released into the public domain.
*/
// include core Wiring API
#include "Arduino.h"

// include this library's description file
#include "Goertzel.h"

Goertzel::Goertzel(float in_TARGET_FREQUENCY, float in_SAMPLING_FREQUENCY, int n)
{
  initialize(in_TARGET_FREQUENCY, in_SAMPLING_FREQUENCY, n);
}

Goertzel::Goertzel()
{
  initialize(24000, 100000, 100);
}

void Goertzel::initialize(float in_TARGET_FREQUENCY, float in_SAMPLING_FREQUENCY, int n)
{
  SAMPLING_FREQUENCY = in_SAMPLING_FREQUENCY; // on 16mhz, ~8928.57142857143, on 8mhz ~44444
  TARGET_FREQUENCY = in_TARGET_FREQUENCY;     // should be integer of SAMPLING_RATE/N
  N = n;

  // pre-calc the coeifficents for the goertzel
  float omega = (2.0 * PI * TARGET_FREQUENCY) / SAMPLING_FREQUENCY;
  cosine = cos(omega);
  coeff = 2.0 * cos(omega);
  sine = sin(omega);

  ResetGoertzel();
}

void Goertzel::reinit(float in_TARGET_FREQUENCY, float in_SAMPLING_FREQUENCY, int n)
{
  initialize(in_TARGET_FREQUENCY, in_SAMPLING_FREQUENCY, n);
}

float Goertzel::getSampleFreq()
{
  return SAMPLING_FREQUENCY;
}

float Goertzel::getTargetFreq()
{
  return TARGET_FREQUENCY;
}

/* Call this routine before every "block" (size=N) of samples. */
void Goertzel::ResetGoertzel()
{
  Q2 = 0;
  Q1 = 0;
}

float Goertzel::applyHammingWindow(float sample)
{
  return 0.54 - 0.46 * cos(2 * PI * sample / N);
}

float Goertzel::applyExactBlackman(float sample)
{
  return 0.426591 - .496561 * cos(2 * PI * sample / N) + .076848 * cos(4 * PI * sample / N);
}

/* Call this routine for every sample. */
void Goertzel::addSample(float zero_centered_scaled_sample)
{
  float Q0;
  Q0 = coeff * Q1 - Q2 + zero_centered_scaled_sample;
  Q2 = Q1;
  Q1 = Q0;
}

void Goertzel::setN(int newN)
{
  N = newN;
}

void Goertzel::setHamming(bool hamm)
{
  hamming = hamm;
}

void Goertzel::setExactBlackman(bool eb)
{
  exactBlackman = eb;
}

float Goertzel::calcMagnitudeSquared()
{
  // Serial.println(Q1 * Q1);
  // Serial.println(Q2 * Q2);
  // Serial.println(coeff * Q1 * Q2);
  return Q1 * Q1 + Q2 * Q2 - coeff * Q1 * Q2;
}

float Goertzel::calcPurity()
{
  return (2 * calcMagnitudeSquared() / N);
}

float Goertzel::calcPurityWithDifferentN(int numberOfSamples)
{
  return (2 * calcMagnitudeSquared() / numberOfSamples);
}

float Goertzel::calcRealPart()
{
  return Q1 - Q2 * cosine;
}

float Goertzel::calcImagPart()
{
  return Q2 * sine;
}

float Goertzel::detectWithDifferentN(int numberOfSamples)
{
  float purity = calcPurityWithDifferentN(numberOfSamples);
  ResetGoertzel();
  return purity;
}

float Goertzel::detect()
{
  float purity = calcPurity();
  ResetGoertzel();
  return purity;
}

float Goertzel::detectBatch(float samples[], int sizeOfSamples, int startIndex)
{
  ResetGoertzel();
  // save previous N for resetting after batch detect
  /* Process the samples. */

  for (int index = 0; index < sizeOfSamples; index++)
  {
    addSample(samples[(index + startIndex) % sizeOfSamples]);
  }

  float purity = detectWithDifferentN(sizeOfSamples);
  // Serial.println(microsfour - micros());
  return purity;
}
