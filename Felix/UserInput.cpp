#include "pch.hpp"
#include "UserInput.hpp"
#include "SysConfig.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

UserInput::UserInput() : mRotation{ ImageProperties::Rotation::NORMAL }, mMapping{}, mPressedCodes{}, mGamepadPacket{}, mHasGamepad{}
{  
  auto sysConfig = gConfigProvider.sysConfig();

  mMapping[KeyInput::OUTER] = sysConfig->keyMapping.outer;
  mMapping[KeyInput::INNER] = sysConfig->keyMapping.inner;
  mMapping[KeyInput::OPTION2] = sysConfig->keyMapping.option2;
  mMapping[KeyInput::OPTION1] = sysConfig->keyMapping.option1;
  mMapping[KeyInput::RIGHT] = sysConfig->keyMapping.right;
  mMapping[KeyInput::LEFT] = sysConfig->keyMapping.left;
  mMapping[KeyInput::DOWN] = sysConfig->keyMapping.down;
  mMapping[KeyInput::UP] = sysConfig->keyMapping.up;
  mMapping[KeyInput::PAUSE] = sysConfig->keyMapping.pause;

  recheckGamepad();
}

UserInput::~UserInput()
{
  auto sysConfig = gConfigProvider.sysConfig();

  sysConfig->keyMapping.outer = mMapping[KeyInput::OUTER];
  sysConfig->keyMapping.inner = mMapping[KeyInput::INNER];
  sysConfig->keyMapping.option2 = mMapping[KeyInput::OPTION2];
  sysConfig->keyMapping.option1 = mMapping[KeyInput::OPTION1];
  sysConfig->keyMapping.right = mMapping[KeyInput::RIGHT];
  sysConfig->keyMapping.left = mMapping[KeyInput::LEFT];
  sysConfig->keyMapping.down = mMapping[KeyInput::DOWN];
  sysConfig->keyMapping.up = mMapping[KeyInput::UP];
  sysConfig->keyMapping.pause = mMapping[KeyInput::PAUSE];
}

void UserInput::keyDown( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  if ( !pressed( code ) )
    mPressedCodes.push_back( code );
}

void UserInput::keyUp( int code )
{
  std::unique_lock<std::mutex> l{ mMutex };

  mPressedCodes.erase( std::remove( mPressedCodes.begin(), mPressedCodes.end(), code ), mPressedCodes.end() );
}

void UserInput::lostFocus()
{
  std::unique_lock<std::mutex> l{ mMutex };

  mPressedCodes.clear();
}

void UserInput::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;
}

void UserInput::updateGamepad()
{
  if ( !mHasGamepad )
    return;

  glfwGetGamepadState(GLFW_JOYSTICK_3, &mLastState);
}

void UserInput::recheckGamepad()
{
  mHasGamepad = glfwJoystickPresent(GLFW_JOYSTICK_1) && glfwJoystickIsGamepad(GLFW_JOYSTICK_1);
}

KeyInput UserInput::getInput( bool leftHand ) const
{
  std::unique_lock<std::mutex> l{ mMutex };

  bool outer = pressed( KeyInput::OUTER );
  bool inner = pressed( KeyInput::INNER );
  bool opt1 = pressed( KeyInput::OPTION1 );
  bool opt2 = pressed( KeyInput::OPTION2 );
  bool pause = pressed( KeyInput::PAUSE );
  bool left = pressed( leftHand ? KeyInput::RIGHT : KeyInput::LEFT );
  bool right = pressed( leftHand ? KeyInput::LEFT : KeyInput::RIGHT );
  bool up = pressed( leftHand ? KeyInput::DOWN : KeyInput::UP );
  bool down = pressed( leftHand ? KeyInput::UP : KeyInput::DOWN );

  if ( mHasGamepad )
  {
    right |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
    left |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
    up |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
    down |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
    outer |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_B] || mLastState.buttons[GLFW_GAMEPAD_BUTTON_Y];
    inner |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_A] || mLastState.buttons[GLFW_GAMEPAD_BUTTON_X];
    opt1 |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
    opt2 |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
    pause |= mLastState.buttons[GLFW_GAMEPAD_BUTTON_START];
  }

  KeyInput result{};

  result.set( KeyInput::OUTER, outer );
  result.set( KeyInput::INNER, inner );
  result.set( KeyInput::OPTION1, opt1 );
  result.set( KeyInput::OPTION2, opt2 );
  result.set( KeyInput::PAUSE, pause );

  switch ( mRotation )
  {
  case ImageProperties::Rotation::RIGHT:
    result.set( KeyInput::LEFT, down );
    result.set( KeyInput::RIGHT, up );
    result.set( KeyInput::UP, left );
    result.set( KeyInput::DOWN, right );
    break;
  case ImageProperties::Rotation::LEFT:
    result.set( KeyInput::LEFT, up );
    result.set( KeyInput::RIGHT, down );
    result.set( KeyInput::UP, right );
    result.set( KeyInput::DOWN, left );
    break;
  default:
    result.set( KeyInput::LEFT, left );
    result.set( KeyInput::RIGHT, right );
    result.set( KeyInput::UP, up );
    result.set( KeyInput::DOWN, down );
    break;
  }

  return result;
}

int UserInput::getVirtualCode( KeyInput::Key k )
{
  return mMapping[k];
}

void UserInput::updateMapping( KeyInput::Key k, int code )
{
  mMapping[k] = code;
}

int UserInput::firstKeyPressed() const
{
  std::unique_lock<std::mutex> l{ mMutex };

  if ( mPressedCodes.empty() )
    return 0;
  else
    return mPressedCodes[0];
  return 0;
}

bool UserInput::pressed( int code ) const
{
  return std::ranges::find( mPressedCodes, code ) != mPressedCodes.cend();
}

bool UserInput::pressed( KeyInput::Key key ) const
{
  return pressed( mMapping[key] );
}
