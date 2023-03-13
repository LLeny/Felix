#include "pch.hpp"
#include "AudioOut.hpp"
#include "Core.hpp"
#include "Log.hpp"
#include "IEncoder.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

#define NUM_CHANNELS      2
#define PA_SAMPLE_TYPE    paInt16
#define SAMPLE_RATE       48000.0
#define FRAMES_PER_BUFFER 10000

AudioOut::AudioOut() : mWav{}, mNormalizer{ 1.0f / 32768.0f }
{
  PA_CHECK( Pa_Initialize() );

  mInputParameters.device = Pa_GetDefaultInputDevice();
  mInputParameters.channelCount = NUM_CHANNELS;
  mInputParameters.sampleFormat = PA_SAMPLE_TYPE;
  mInputParameters.suggestedLatency = Pa_GetDeviceInfo( mInputParameters.device )->defaultHighInputLatency;
  mInputParameters.hostApiSpecificStreamInfo = NULL;

  mOutputParameters.device = Pa_GetDefaultOutputDevice();
  mOutputParameters.channelCount = NUM_CHANNELS;
  mOutputParameters.sampleFormat = PA_SAMPLE_TYPE;
  mOutputParameters.suggestedLatency = Pa_GetDeviceInfo( mOutputParameters.device )->defaultHighOutputLatency;
  mOutputParameters.hostApiSpecificStreamInfo = NULL;

  PA_CHECK( Pa_OpenStream( &mStream, &mInputParameters, &mOutputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL ) );
  PA_CHECK( Pa_StartStream( mStream ) );

  mSamplesDelta = mSamplesDeltaDelta = 0;
  mSampleRate = Pa_GetStreamInfo( mStream )->sampleRate;

  mTimeToSamples = mSampleRate / 10000000.0;

  std::fill( mWindow.begin(), mWindow.end(), 0 );

  auto sysConfig = gConfigProvider.sysConfig();
  mute( sysConfig->audio.mute );
}

AudioOut::~AudioOut()
{
  PA_CHECK( Pa_StopStream( mStream ) );
  PA_CHECK( Pa_CloseStream( mStream ) );
  PA_CHECK( Pa_Terminate() );

  if ( mWav )
  {
    wav_close( mWav );
    mWav = nullptr;
  }

  auto sysConfig = gConfigProvider.sysConfig();
  sysConfig->audio.mute = mute();
}

void AudioOut::setEncoder( std::shared_ptr<IEncoder> pEncoder )
{
  mEncoder = std::move( pEncoder );
}

void AudioOut::setWavOut( std::filesystem::path path )
{
  mWav = wav_open( path.string().c_str(), WAV_OPEN_WRITE );
  if ( wav_err()->code != WAV_OK )
  {
    L_ERROR << "Error opening wav file " << path.string() << ": " << wav_err()->message;
    if ( mWav )
    {
      wav_close( mWav );
      mWav = nullptr;
    }
    return;
  }

  wav_set_format( mWav, WAV_FORMAT_IEEE_FLOAT );
  wav_set_num_channels( mWav, NUM_CHANNELS );
  wav_set_sample_rate( mWav, mSampleRate );
  wav_set_sample_size( mWav, sizeof(float) );
}

void AudioOut::mute( bool value )
{
  mNormalizer = value ? 0.0f : 1.0f;
}

bool AudioOut::mute() const
{
  return mNormalizer == 0;
}

int32_t AudioOut::correctedSPS( int64_t samplesEmittedPerFrame, int64_t renderingTimeQPC )
{
  auto baseResult = mSampleRate;

  if ( samplesEmittedPerFrame == 0 )
    return baseResult;

  if ( renderingTimeQPC == 0 )
    return baseResult;

  auto samplesPerRenderingTimeR = renderingTimeQPC * mTimeToSamples;
  auto samplesPerRenderingTimeI = (int32_t)std::round( samplesPerRenderingTimeR );

  auto newDelta = samplesPerRenderingTimeI - (int32_t)samplesEmittedPerFrame;
  mSamplesDeltaDelta = newDelta - mSamplesDelta;
  mSamplesDelta = newDelta;

  int sum = 0;
  for ( size_t i = 0; i < mWindow.size() - 1; ++i )
  {
    sum += mWindow[i];
    mWindow[i] = mWindow[i + 1];
  }
  sum += mWindow.back();

  int avg = sum / (int)mWindow.size();

  mWindow.back() = samplesPerRenderingTimeI;

  double ratio = (double)avg / samplesEmittedPerFrame;

  //L_DEBUG << "SPF: " << samplesEmittedPerFrame << "\t\tSPR: " << samplesPerRenderingTimeI << "\t\tavg: " << avg << "\t" << ratio;

  return baseResult;
}

bool AudioOut::wait()
{
  return true;
}

CpuBreakType AudioOut::fillBuffer( std::shared_ptr<Core> instance, int64_t renderingTimeQPC, RunMode runMode )
{
  uint32_t framesAvailable = Pa_GetStreamWriteAvailable( mStream );

  if ( framesAvailable > 0 )
  {
    if ( mSamplesBuffer.size() < framesAvailable )
    {
      mSamplesBuffer.resize( framesAvailable );
    }

    if ( !instance )
      return CpuBreakType::NEXT;

    auto sps = correctedSPS( instance->globalSamplesEmittedPerFrame(), renderingTimeQPC );

    auto cpuBreakType = instance->advanceAudio( sps, std::span<AudioSample>{ mSamplesBuffer.data(), framesAvailable }, runMode );

    uint16_t volumeAdjustBuffer[sizeof(AudioSample)*framesAvailable];
    int pBuff = 0;

    for( uint16_t c = 0; c < framesAvailable; ++c )
    {
      volumeAdjustBuffer[pBuff++] = mSamplesBuffer[c].left * mNormalizer;  
      volumeAdjustBuffer[pBuff++] = mSamplesBuffer[c].right * mNormalizer;
    }

    PA_CHECK( Pa_WriteStream( mStream, volumeAdjustBuffer, framesAvailable ) );

    // TODO
  /*  if ( mEncoder )
      mEncoder->pushAudioBuffer( std::span<float const>( mSamplesBuffer, framesAvailable * NUM_CHANNELS ) );*/

    if ( mWav )
      wav_write( mWav, mSamplesBuffer.data(), framesAvailable );

    return cpuBreakType;
  }

  return CpuBreakType::NEXT;
}




