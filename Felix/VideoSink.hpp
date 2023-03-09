#pragma once
#include "IVideoSink.hpp"

class ScreenRenderingBuffer;

typedef std::array<uint8_t, 80 * 105> VideoSinkFramebuffer;

struct VideoSink : public IVideoSink
{
  VideoSink();

  std::array<uint8_t, 32> mPalette;
  std::queue<std::shared_ptr<VideoSinkFramebuffer>> mFrames;
  std::shared_ptr<VideoSinkFramebuffer> mWorkingFrame;
  uint16_t mWorkingFramePointer;
  uint64_t mBeginTick;
  uint64_t mLastTick;
  uint64_t mFrameTicks;
  mutable std::mutex mQueueMutex;

  void newFrame( uint64_t tick, uint8_t hbackup ) override;
  void newRow( uint64_t tick, int row ) override;
  void emitScreenData( std::span<uint8_t const> data ) override;
  void updateColorReg( uint8_t reg, uint8_t value ) override;
  std::shared_ptr<VideoSinkFramebuffer> pullNextFrame();
  uint8_t* getPalettePointer();
};

