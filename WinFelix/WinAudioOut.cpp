#include "pch.hpp"
#include "WinAudioOut.hpp"
#include "Core.hpp"
#include "Log.hpp"
#include "IEncoder.hpp"

WinAudioOut::WinAudioOut()
{
  CoInitializeEx( NULL, COINIT_MULTITHREADED );

  HRESULT hr;

  ComPtr<IMMDeviceEnumerator> deviceEnumerator;
  hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( deviceEnumerator.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = deviceEnumerator->GetDefaultAudioEndpoint( eRender, eMultimedia, mDevice.ReleaseAndGetAddressOf() );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mDevice->Activate( __uuidof( IAudioClient ), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>( mAudioClient.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetMixFormat( &mMixFormat );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, mMixFormat, nullptr );
  if ( FAILED( hr ) )
    throw std::exception{};

  mEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
  if ( mEvent == NULL )
    throw std::exception{};

  mAudioClient->SetEventHandle( mEvent );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetBufferSize( &mBufferSize );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetService( __uuidof( IAudioRenderClient ), reinterpret_cast<void **>( mRenderClient.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetService( __uuidof( IAudioClock ), reinterpret_cast<void**>( mAudioClock.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  //bytes per second
  uint64_t frequency;
  hr = mAudioClock->GetFrequency( &frequency );
  if ( FAILED( hr ) )
    throw std::exception{};

  LARGE_INTEGER l;
  QueryPerformanceFrequency( &l );

  mTimeToSamples = (double)frequency / (double)(l.QuadPart * mMixFormat->nBlockAlign);

  mSamplesDelta = mSamplesDeltaDelta = 0;

  std::fill( mWindow.begin(), mWindow.end(), 0 );

  mAudioClient->Start();
}

WinAudioOut::~WinAudioOut()
{
  mAudioClient->Stop();

  if ( mEvent )
  {
    CloseHandle( mEvent );
    mEvent = NULL;
  }

  CoUninitialize();

  if ( mMixFormat )
  {
    CoTaskMemFree( mMixFormat );
    mMixFormat = nullptr;
  }
}

void WinAudioOut::setEncoder( std::shared_ptr<IEncoder> pEncoder )
{
  mEncoder = std::move( pEncoder );
}


int32_t WinAudioOut::correctedSPS( int64_t samplesEmittedPerFrame, int64_t renderingTimeQPC )
{
  int baseResult = mMixFormat->nSamplesPerSec;

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

  L_DEBUG << "SPF: " << samplesEmittedPerFrame << "\t\tSPR: " << samplesPerRenderingTimeI << "\t\tavg: " << avg << "\t" << ratio;

  return baseResult;
}

void WinAudioOut::fillBuffer( std::span<std::shared_ptr<Core> const> instances, int64_t renderingTimeQPC )
{
  DWORD retval = WaitForSingleObject( mEvent, 100 );
  if ( retval != WAIT_OBJECT_0 )
    return;

  HRESULT hr;
  uint32_t padding{};
  hr = mAudioClient->GetCurrentPadding( &padding );
  uint32_t framesAvailable = mBufferSize - padding;
  if ( framesAvailable > 0 )
  {
    if ( mSamplesBuffer.size() < framesAvailable * instances.size() )
    {
      mSamplesBuffer.resize( framesAvailable * instances.size() );
    }

    auto sps = correctedSPS( instances[0]->globalSamplesEmittedPerFrame(), renderingTimeQPC );

    for ( size_t i = 0; i < instances.size(); ++i )
    {
      instances[i]->setAudioOut( sps, std::span<AudioSample>{mSamplesBuffer.data() + framesAvailable * i, framesAvailable} );
    }

    for ( int finished = 0; finished < instances.size(); )
    {
      for ( size_t i = 0; i < instances.size(); ++i )
      {
        finished += instances[i]->advanceAudio();
      }
    }

    BYTE *pData;
    hr = mRenderClient->GetBuffer( framesAvailable, &pData );
    if ( FAILED( hr ) )
      return;
    float* pfData = reinterpret_cast<float*>( pData );
    for ( uint32_t i = 0; i < framesAvailable; ++i )
    {
      pfData[i * mMixFormat->nChannels + 0] = mSamplesBuffer[i].left / 32768.0f;
      pfData[i * mMixFormat->nChannels + 1] = mSamplesBuffer[i].right / 32768.0f;
    }

    if ( mEncoder )
      mEncoder->pushAudioBuffer( std::span<float const>( pfData, framesAvailable * mMixFormat->nChannels ) );

    hr = mRenderClient->ReleaseBuffer( framesAvailable, 0 );
  }
}




