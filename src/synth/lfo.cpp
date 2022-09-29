#include "lfo.h"
#include "../chunks/prgichunk.h"
#include "../chunks/trackchunk.h"
#include <iostream>
#include <cmath>

LFO::LFO(const LFOInfo& other) : LFOInfo(other), scale(-1.0/510.0)
{
  if (rate < 0) {
    // TODO: This is 100% guesswork right now
    rate = -rate;
    scale = -scale;
  }
  if (route == Volume) {
    // It's unclear why this scaling factor is there, and it'll require a
    // disassembly to verify that it's correct.
    frequencyHz = 2048.0 / rate;
  } else if (route == Pan) {
    // Pan scale has to be cut in half because of later processing.
    // Unclear on the rate scaling too
    frequencyHz = 2048.0 / rate;
  } else {
    frequencyHz = 256.0 / rate;
    // TODO: this will be wrong for non-sampler instruments
    if (scale < 0) {
      scale = TrkEvent::frequency(69, scale * -32.0) / -440.0 + 1.0;
    } else {
      scale = TrkEvent::frequency(69, scale * 32.0) / 440.0 - 1.0;
    }
  }
}

bool LFO::isEnabled(const LFOInfo& lfo)
{
  return lfo.route != Disabled && lfo.waveform != NoWave;
}

/*
double LFO::sample(double time) const
{
  // TODO: PP wiki says "may or may not be Hertz"
  double phase = std::fmod(time / halfWave, 2.0);
  // TODO: is there an offset from center? a duty cycle?
  // square LFOs are usually used for gate effects
  switch (waveform) {
    case Square:
      return phase < 1 ? depth : -depth;
    case Sine:
      return depth * std::sin(M_PI * phase);
    case Saw:
      return depth * (phase - 1);
    case Noise:
    case Random:
      // TODO: what's the difference?
      return depth - std::rand() * double(2 * depth) / RAND_MAX;
    case Triangle:
    case Unknown:
    default:
      // TODO: this code is for triangle, being used as the fallback
      // for unknown waveforms because it's cheap; explore better meanings
      if (phase < 0.5) {
        return depth * 2 * phase;
      } else if (phase < 1.5) {
        return depth * 2 * (1 - phase);
      } else {
        return depth * 2 * (phase - 1);
      }
      return 0;
  }
}

double LFO::apply(Route r, double base, double startTime, double time) const
{
  if (route != r || time < startTime + delay) {
    return base;
  }
  double lfoTime = time - startTime - delay;
  double fadeLevel = 1.0;
  // The wiki doesn't document this behavior but it looks like negative
  // values are meant to be a fade-in while positive ones are a fade-out
  if (fade > 0) {
    fadeLevel = lfoTime < fade ? 1.0 - (lfoTime / fade) : 0;
  } else if (fade < 0) {
    fadeLevel = lfoTime < -fade ? (lfoTime / -fade) : 1.0;
  }
  return base + sample(lfoTime) * scale * fadeLevel;
}

double LFO::applyAll(const std::vector<LFO>& lfos, Route r, double base, double startTime, double time)
{
  for (const LFO& lfo : lfos) {
    base = lfo.apply(r, base, startTime, time);
  }
  return base;
}
*/

bool LFO::isEnabled(const std::vector<LFO>& lfos, Route r)
{
  for (const LFO& lfo : lfos) {
    if (lfo.isEnabled() && lfo.route == r) {
      return true;
    }
  }
  return false;
}
