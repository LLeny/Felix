#include "Mikey.hpp"
#include <cassert>
#include "TimerCore.hpp"

Mikey::Mikey() : mAccessTick{}, mTimers{}, mDisplayGenerator{ std::make_unique<DisplayGenerator>() }, mDisplayRegs{}
{
  mTimers[0x0] = std::make_unique<TimerCore>( 0x0, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x2]->borrowIn( tick );
    mDisplayGenerator->hblank( tick, mTimers[0x02]->value() );
  } );  //timer 0 -> timer 2
  mTimers[0x1] = std::make_unique<TimerCore>( 0x1, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x3]->borrowIn( tick );
  } );  //timer 1 -> timer 3
  mTimers[0x2] = std::make_unique<TimerCore>( 0x2, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x4]->borrowIn( tick );
    mDisplayGenerator->vblank( mDisplayRegs.dispAdr );
  } );  //timer 2 -> timer 4
  mTimers[0x3] = std::make_unique<TimerCore>( 0x4, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x5]->borrowIn( tick );
  } );  //timer 3 -> timer 5
  mTimers[0x4] = std::make_unique<TimerCore>( 0x5, [this]( uint64_t tick, bool interrupt )
  {
  } );  //timer 4
  mTimers[0x5] = std::make_unique<TimerCore>( 0x6, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x7]->borrowIn( tick );
  } );  //timer 5 -> timer 7
  mTimers[0x6] = std::make_unique<TimerCore>( 0x8, [this]( uint64_t tick, bool interrupt )
  {
  } );  //timer 6
  mTimers[0x7] = std::make_unique<TimerCore>( 0x9, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x8]->borrowIn( tick );
  } );  //timer 7 -> audio 0
  mTimers[0x8] = std::make_unique<TimerCore>( 0xa, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x9]->borrowIn( tick );
  } );  //audio 0 -> audio 1
  mTimers[0x9] = std::make_unique<TimerCore>( 0xc, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0xa]->borrowIn( tick );
  } );  //audio 1 -> audio 2
  mTimers[0xa] = std::make_unique<TimerCore>( 0xd, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0xb]->borrowIn( tick );
  } );  //audio 2 -> audio 3
  mTimers[0xb] = std::make_unique<TimerCore>( 0xe, [this]( uint64_t tick, bool interrupt )
  {
    mTimers[0x0]->borrowIn( tick );
  } );  //audio 3 -> timer 1
}

Mikey::~Mikey()
{
}

uint64_t Mikey::requestAccess( uint64_t tick, uint16_t address )
{
  mAccessTick = tick + 5;

  address &= 0xff;
  int timer = -1;

  if ( address < 0x20 )
  {
    timer = ( address & 0x1f ) >> 2;
  }
  else if ( address < 0x40 )
  {
    timer = 8 + ( ( ( address - 0x20 ) & 0x1f ) >> 3 );
  }

  if ( timer >= 0 )
  {
    auto nextTick = mAccessTick & ~0xfull + ( timer * 16 / 12 );
    mAccessTick = nextTick < mAccessTick ? nextTick + 16 : nextTick;
  }
  
  return mAccessTick;
}

uint8_t Mikey::read( uint16_t address )
{
  switch ( address )
  {
  default:
    assert( false );
  }

  return uint8_t();
}

SequencedAction Mikey::write( uint16_t address, uint8_t value )
{
  address &= 0xff;

  if ( address < 0x20 )
  {
    switch ( address & 0x3 )
    {
    case Reg::Offset::TIMER::BACKUP:
      return mTimers[(address >> 2) & 7]->setBackup( mAccessTick, value );
      break;
    case Reg::Offset::TIMER::CONTROLA:
      return mTimers[( address >> 2 ) & 7]->setControlA( mAccessTick, value );
      break;
    case Reg::Offset::TIMER::COUNT:
      return mTimers[( address >> 2 ) & 7]->setCount( mAccessTick, value );
      break;
    case Reg::Offset::TIMER::CONTROLB:
      return mTimers[( address >> 2 ) & 7]->setControlB( mAccessTick, value );
      break;
    }
  }
  else if ( address < 0x40 )
  {
    switch ( address & 0x7 )
    {
    case Reg::Offset::AUDIO::VOLCNTRL:
      break;
    case Reg::Offset::AUDIO::FEEDBACK:
      break;
    case Reg::Offset::AUDIO::OUTPUT:
      break;
    case Reg::Offset::AUDIO::SHIFT:
      break;
    case Reg::Offset::AUDIO::BACKUP:
      break;
    case Reg::Offset::AUDIO::CONTROL:
      break;
    case Reg::Offset::AUDIO::COUNTER:
      break;
    case Reg::Offset::AUDIO::OTHER:
      break;
    }
  }
  else switch ( address )
  {
  case Reg::Offset::DISPCTL:
    mDisplayRegs.dispColor = ( value & Reg::DISPCTL::DISP_COLOR ) != 0;
    mDisplayRegs.dispFourBit = ( value & Reg::DISPCTL::DISP_FOURBIT ) != 0;
    mDisplayRegs.dispFlip = ( value & Reg::DISPCTL::DISP_FLIP ) != 0;
    mDisplayRegs.DMAEnable = ( value & Reg::DISPCTL::DMA_ENABLE ) != 0;
    break;
  case Reg::Offset::PBKUP:
    mDisplayRegs.pbkup = value;
    break;
  case Reg::Offset::GREEN + 0x00:
    break;
  case Reg::Offset::GREEN + 0x01:
    break;
  case Reg::Offset::GREEN + 0x02:
    break;
  case Reg::Offset::GREEN + 0x03:
    break;
  case Reg::Offset::GREEN + 0x04:
    break;
  case Reg::Offset::GREEN + 0x05:
    break;
  case Reg::Offset::GREEN + 0x06:
    break;
  case Reg::Offset::GREEN + 0x07:
    break;
  case Reg::Offset::GREEN + 0x08:
    break;
  case Reg::Offset::GREEN + 0x09:
    break;
  case Reg::Offset::GREEN + 0x0a:
    break;
  case Reg::Offset::GREEN + 0x0b:
    break;
  case Reg::Offset::GREEN + 0x0c:
    break;
  case Reg::Offset::GREEN + 0x0d:
    break;
  case Reg::Offset::GREEN + 0x0e:
    break;
  case Reg::Offset::GREEN + 0x0f:
    break;
  case Reg::Offset::BLUERED + 0x00:
    break;
  case Reg::Offset::BLUERED + 0x01:
    break;
  case Reg::Offset::BLUERED + 0x02:
    break;
  case Reg::Offset::BLUERED + 0x03:
    break;
  case Reg::Offset::BLUERED + 0x04:
    break;
  case Reg::Offset::BLUERED + 0x05:
    break;
  case Reg::Offset::BLUERED + 0x06:
    break;
  case Reg::Offset::BLUERED + 0x07:
    break;
  case Reg::Offset::BLUERED + 0x08:
    break;
  case Reg::Offset::BLUERED + 0x09:
    break;
  case Reg::Offset::BLUERED + 0x0a:
    break;
  case Reg::Offset::BLUERED + 0x0b:
    break;
  case Reg::Offset::BLUERED + 0x0c:
    break;
  case Reg::Offset::BLUERED + 0x0d:
    break;
  case Reg::Offset::BLUERED + 0x0e:
    break;
  case Reg::Offset::BLUERED + 0x0f:
    break;
  default:
    assert( false );
    break;
  }

  return {};
}

SequencedAction Mikey::fireTimer( uint64_t tick, uint32_t timer )
{
  assert( timer < 12 );
  return mTimers[timer]->fireAction( tick );
}

uint16_t Mikey::getDMAAddress()
{
  return uint16_t();
}

void Mikey::setDMAData( uint8_t const * data )
{
}

DisplayGenerator::Pixel const * Mikey::getSrface() const
{
  return mDisplayGenerator->getSrface();
}
