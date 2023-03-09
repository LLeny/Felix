#pragma once
#include "IInputSource.hpp"
#include "IUserInput.hpp"
#include "ImageProperties.hpp"
#include "GLFW/glfw3.h"

struct SysConfig;

class UserInput : public IUserInput
{
public:
  UserInput();
  ~UserInput() override;

  void keyDown( int code );
  void keyUp( int code );
  void lostFocus();
  void setRotation( ImageProperties::Rotation rotation );

  KeyInput getInput( bool leftHand ) const override;

  int getVirtualCode( KeyInput::Key k ) override;
  void updateMapping( KeyInput::Key k, int code ) override;
  int firstKeyPressed() const override;

  void updateGamepad();
  void recheckGamepad();

private:

  bool pressed( int code ) const;
  bool pressed( KeyInput::Key key ) const;


private:

private:
  mutable std::mutex mMutex;
  ImageProperties::Rotation mRotation;
  std::array<int, 9> mMapping;
  std::vector<int> mPressedCodes;
  uint32_t mGamepadPacket;
  GLFWgamepadstate mLastState;
  bool mHasGamepad;
};
