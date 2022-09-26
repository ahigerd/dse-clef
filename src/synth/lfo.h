#ifndef SYNTH_LFO_H
#define SYNTH_LFO_H

#include "../chunks/prgichunk.h"

struct LFO : public LFOInfo {
  double frequencyHz;
  double scale;

  LFO() = default;
  LFO(const LFOInfo& other);
  LFO(const LFO& other) = default;
  LFO(LFO&& other) = default;
  LFO& operator=(const LFO& other) = default;
  LFO& operator=(LFO&& other) = default;

  inline bool isEnabled() const { return isEnabled(*this); }
  //double apply(Route r, double base, double startTime, double time) const;
  //static double applyAll(const std::vector<LFO>& lfos, Route r, double base, double startTime, double time);
  static bool isEnabled(const LFOInfo& lfo);
  static bool isEnabled(const std::vector<LFO>& lfos, Route r);

private:
  //double sample(double time) const;
};

#endif
