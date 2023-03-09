#include "pch.hpp"
#include "VideoSink.hpp"

void VideoSink::newFrame( uint64_t tick, uint8_t hbackup )
{
  mFrameTicks = tick - mBeginTick;
  mBeginTick = tick;

  std::scoped_lock<std::mutex> lock( mQueueMutex );

  if (mWorkingFrame)
  {
    while (mFrames.size() > 1)
    {
      mFrames.pop();
    }
    mFrames.push( std::move( mWorkingFrame ) );
  }

  mWorkingFrame = std::make_shared<VideoSinkFramebuffer>();
  mWorkingFramePointer = 0;
}

void VideoSink::newRow( uint64_t tick, int row )
{
}

void VideoSink::emitScreenData( std::span<uint8_t const> data )
{
  for (auto d : data)
  {
    mWorkingFrame->at(mWorkingFramePointer++) = d;
  }  
}

void VideoSink::updateColorReg( uint8_t reg, uint8_t value )
{
  mPalette[reg] = value;
}

std::shared_ptr<VideoSinkFramebuffer> VideoSink::pullNextFrame()
{
  std::shared_ptr<VideoSinkFramebuffer> result{};
  std::scoped_lock<std::mutex> lock( mQueueMutex );

  if (!mFrames.empty())
  {
    result = mFrames.front();
    mFrames.pop();
  }

  return result;
}

uint8_t* VideoSink::getPalettePointer()
{
  return mPalette.data();
}

VideoSink::VideoSink() : mBeginTick{}, mLastTick{}, mFrameTicks{ ~0ull }, mFrames{}, mPalette{}, mWorkingFramePointer{}
{
  newFrame( 0, 0 );
}

