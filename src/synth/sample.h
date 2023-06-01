#ifndef SYNTH_SAMPLE_H
#define SYNTH_SAMPLE_H

#include <vector>
#include <cstdint>
struct SplitInfo;
struct SampleInfo;
struct SampleData;
class DSEContext;

struct Sample
{
  Sample(const SplitInfo* split, const SampleInfo* info, SampleData* pcm, DSEContext* synth);
  Sample(const SampleInfo* info, SampleData* pcm, DSEContext* synth) : Sample(nullptr, info, pcm, synth) {}

  DSEContext* context;
  const SampleInfo* sampleInfo;
  const SampleData* sample;
};

#endif
