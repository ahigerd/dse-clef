#include "note.h"
#include "sample.h"
#include "../dseutil.h"
#include "../chunks/trackchunk.h"
#include <cmath>

Note::Note(const TrkEvent& ev, int octave, int lastLength)
: noteNumber(ev.midiNote(octave)), duration(ev.duration(lastLength)), bendRange(2), loopSample(true),
  useEnvelope(false), sample(nullptr),
  pitch(noteNumber), velocity(ev.velocity()), gain(1.0), pan(1.0), attackLevel(1.0),
  attackTime(0), holdTime(0), decayTime(0), sustainLevel(0), fadeTime(HUGE_VAL), releaseTime(80)
{
  // initializers only
}

void Note::makeNoiseDrum()
{
  sample = nullptr;
  pitch = -1;
  useEnvelope = true;
  attackTime = 0;
  attackLevel = 1.0;
  holdTime = 0;
  decayTime = holdTime + 20.0;
  sustainLevel = 0.3;
  fadeTime = decayTime + 500.0;
  releaseTime = 80.0;
}
