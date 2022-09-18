#ifndef SYNTH_SAMPLE_H
#define SYNTH_SAMPLE_H

#include <vector>
#include <cstdint>
class SplitInfo;
class SampleInfo;
class SampleData;
class DSEContext;

struct Sample
{
  Sample(const SplitInfo* split, const SampleInfo* info, const SampleData* pcm, DSEContext* synth);
  Sample(const SampleInfo* info, const SampleData* pcm, DSEContext* synth) : Sample(nullptr, info, pcm, synth) {}

  DSEContext* context;
  const SampleInfo* const sampleInfo;
  const SampleData* const sample;
};

#endif
