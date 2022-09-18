#ifndef SYNTH_NOTE_H
#define SYNTH_NOTE_H

#include <cstdint>
struct TrkEvent;
struct Sample;

struct Note
{
  Note(const TrkEvent& ev, double time, int octave, int lastLength);
  Note(const Note& other) = default;
  Note(Note&& other) = default;
  Note& operator=(const Note& other) = default;
  Note& operator=(Note&& other) = default;

  int noteNumber;
  int remaining;
  int bendRange;
  bool loopSample;
  bool useEnvelope;
  const Sample* sample;

  double startTime;
  double releasedAt;
  double releaseLevel;

  double pitch;
  double velocity;
  double phase;
  double gain;
  double pan;
  double attackLevel;
  double attackTime;
  double holdTime;
  double decayTime;
  double sustainLevel;
  double fadeTime;
  double releaseTime;

  double envelope(double time) const;
  void release(double time);
  void makeNoiseDrum();
};

#endif
