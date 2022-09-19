#ifndef SYNTH_NOTE_H
#define SYNTH_NOTE_H

#include <cstdint>
struct TrkEvent;
struct Sample;

struct Note
{
  Note(const TrkEvent& ev, int octave, int lastLength);
  Note(const Note& other) = default;
  Note(Note&& other) = default;
  Note& operator=(const Note& other) = default;
  Note& operator=(Note&& other) = default;

  int noteNumber;
  int duration;
  int bendRange;
  bool loopSample;
  bool useEnvelope;
  const Sample* sample;

  double pitch;
  double velocity;
  double gain;
  double pan;
  double attackLevel;
  double attackTime;
  double holdTime;
  double decayTime;
  double sustainLevel;
  double fadeTime;
  double releaseTime;

  void makeNoiseDrum();
};

#endif
