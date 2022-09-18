#include "note.h"
#include "sample.h"
#include "../dseutil.h"
#include "../chunks/trackchunk.h"
#include <cmath>

Note::Note(const TrkEvent& ev, double time, int octave, int lastLength)
: noteNumber(ev.midiNote(octave)), remaining(ev.duration(lastLength)), bendRange(2), loopSample(true),
  useEnvelope(false), sample(nullptr), startTime(time), releasedAt(HUGE_VAL), pitch(noteNumber),
  velocity(ev.velocity()), phase(0), leftGain(1.0), rightGain(1.0), attackLevel(1.0), attackTime(0),
  holdTime(0), decayTime(0), sustainLevel(0), fadeTime(HUGE_VAL), releaseTime(80)
{
  // initializers only
}

double Note::envelope(double time) const
{
  if (time > fadeTime) {
    return 0;
  } else if (!useEnvelope) {
    return 1.0;
  } else if (time > releasedAt) {
    return lerp(time, releasedAt, fadeTime, releaseLevel, 0);
  } else if (time > decayTime) {
    return lerp(time, decayTime, fadeTime, sustainLevel, 0);
  } else if (time > holdTime) {
    return lerp(time, holdTime, decayTime, 1.0, sustainLevel);
  } else if (time > attackTime) {
    return 1.0;
  } else {
    return lerp(time, startTime, attackTime, attackLevel, 1.0);
  }
}

void Note::release(double time)
{
  if (releasedAt > time) {
    releaseLevel = envelope(time);
    releasedAt = time;
    fadeTime = time + releaseTime;
  }
}

void Note::makeNoiseDrum()
{
  sample = nullptr;
  pitch = -1;
  useEnvelope = true;
  attackTime = startTime;
  attackLevel = 1.0;
  holdTime = attackTime;
  decayTime = holdTime + 20.0;
  sustainLevel = 0.3;
  fadeTime = decayTime + 500.0;
  releaseTime = 80.0;
}
