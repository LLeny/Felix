#pragma once

#include "Utility.hpp"
#include "wav.h"
#include "Log.hpp"
#include "portaudio.h"

#define PA_CHECK(x)                                \
	do                                               \
	{                                                \
		PaError err = x;                              \
		if (err)                                       \
		{                                              \
			L_ERROR << "Detected PortAudio error: " << err; \
			abort();                                     \
		}                                              \
	} while (0)


class Core;
class IEncoder;

class AudioOut
{
public:

  AudioOut();
  ~AudioOut();

  void setEncoder( std::shared_ptr<IEncoder> pEncoder );
  bool wait();
  CpuBreakType fillBuffer( std::shared_ptr<Core> instance, int64_t renderingTime, RunMode runMode );
  void setWavOut( std::filesystem::path path );
  void mute( bool value );
  bool mute() const;

private:

  int correctedSPS( int64_t samplesEmittedPerFrame, int64_t renderingTimeQPC );

  std::shared_ptr<IEncoder> mEncoder;

  PaStream* mStream;
  PaStreamParameters mOutputParameters;
  PaStreamParameters mInputParameters;

  WavFile* mWav;

  double mTimeToSamples;
  double mSampleRate;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  int32_t mSamplesDelta;
  int32_t mSamplesDeltaDelta;

  std::array<int, 32> mWindow;

  float mNormalizer;
};
