#include "mergetrack.h"
#include <cmath>
#include <iostream>

MergeTrack::MergeTrack()
{
  // initializers only
}

void MergeTrack::addTrack(ITrack* track)
{
  subTracks.push_back(track);
  nextEvents.emplace_back(nullptr);
}

bool MergeTrack::isFinished() const
{
  for (ITrack* t : subTracks) {
    if (!t->isFinished()) {
      return false;
    }
  }
  return true;
}

double MergeTrack::length() const
{
  double l = 0;
  for (ITrack* t : subTracks) {
    double tl = t->length();
    if (tl > l) {
      l = tl;
    }
  }
  return l;
}

std::shared_ptr<SequenceEvent> MergeTrack::readNextEvent()
{
  double ts = HUGE_VAL;
  std::shared_ptr<SequenceEvent> event;
  int eventTrack = -1;
  int ct = subTracks.size();
  for (int i = 0; i < ct; i++) {
    auto& tEvent = nextEvents[i];
    if (!tEvent) {
      tEvent = subTracks[i]->nextEvent();
      if (!tEvent) {
        continue;
      }
      nextEvents[i] = tEvent;
    }
    if (tEvent->timestamp < ts) {
      eventTrack = i;
      event = tEvent;
      ts = tEvent->timestamp;
    }
  }
  if (eventTrack >= 0) {
    nextEvents[eventTrack].reset();
  }
  return event;
}

void MergeTrack::internalReset()
{
  for (ITrack* t : subTracks) {
    t->reset();
  }
}
