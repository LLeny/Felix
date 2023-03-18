#include "pch.hpp"
#include "KeyNames.hpp"
#include <GLFW/glfw3.h>

namespace
{

  class KeyNames
  {
  public:
    KeyNames();
    char const* name( uint32_t key ) const;
  private:
    std::array<char const*, GLFW_KEY_LAST+1> mNames;
  };

  KeyNames::KeyNames() : mNames{}
  {
    mNames[GLFW_KEY_BACKSPACE] = "Backspace";
    mNames[GLFW_KEY_TAB] = "Tab";
    mNames[GLFW_KEY_ENTER] = "Enter";
    mNames[GLFW_KEY_LEFT_SHIFT] = "Shift";
    mNames[GLFW_KEY_LEFT_CONTROL] = "Ctrl";
    mNames[GLFW_KEY_MENU] = "Alt";
    mNames[GLFW_KEY_PAUSE] = "Pause";
    mNames[GLFW_KEY_CAPS_LOCK] = "Caps Lock";
    mNames[GLFW_KEY_ESCAPE] = "Esc";
    mNames[GLFW_KEY_SPACE] = "Space";
    mNames[GLFW_KEY_PAGE_UP] = "Page Up";
    mNames[GLFW_KEY_PAGE_DOWN] = "Page Down";
    mNames[GLFW_KEY_END] = "End";
    mNames[GLFW_KEY_HOME] = "Home";
    mNames[GLFW_KEY_LEFT] = "Left";
    mNames[GLFW_KEY_UP] = "Up";
    mNames[GLFW_KEY_RIGHT] = "Right";
    mNames[GLFW_KEY_DOWN] = "Down";
    mNames[GLFW_KEY_PRINT_SCREEN] = "Print Screen";
    mNames[GLFW_KEY_INSERT] = "Insert";
    mNames[GLFW_KEY_DELETE] = "Delete";
    mNames[GLFW_KEY_0] = "0";
    mNames[GLFW_KEY_1] = "1";
    mNames[GLFW_KEY_2] = "2";
    mNames[GLFW_KEY_3] = "3";
    mNames[GLFW_KEY_4] = "4";
    mNames[GLFW_KEY_5] = "5";
    mNames[GLFW_KEY_6] = "6";
    mNames[GLFW_KEY_7] = "7";
    mNames[GLFW_KEY_8] = "8";
    mNames[GLFW_KEY_9] = "9";
    mNames[GLFW_KEY_A] = "A";
    mNames[GLFW_KEY_B] = "B";
    mNames[GLFW_KEY_C] = "C";
    mNames[GLFW_KEY_D] = "D";
    mNames[GLFW_KEY_E] = "E";
    mNames[GLFW_KEY_F] = "F";
    mNames[GLFW_KEY_G] = "G";
    mNames[GLFW_KEY_H] = "H";
    mNames[GLFW_KEY_I] = "I";
    mNames[GLFW_KEY_J] = "J";
    mNames[GLFW_KEY_K] = "K";
    mNames[GLFW_KEY_L] = "L";
    mNames[GLFW_KEY_M] = "M";
    mNames[GLFW_KEY_N] = "N";
    mNames[GLFW_KEY_O] = "O";
    mNames[GLFW_KEY_P] = "P";
    mNames[GLFW_KEY_Q] = "Q";
    mNames[GLFW_KEY_R] = "R";
    mNames[GLFW_KEY_S] = "S";
    mNames[GLFW_KEY_T] = "T";
    mNames[GLFW_KEY_U] = "U";
    mNames[GLFW_KEY_V] = "V";
    mNames[GLFW_KEY_W] = "W";
    mNames[GLFW_KEY_X] = "X";
    mNames[GLFW_KEY_Y] = "Y";
    mNames[GLFW_KEY_Z] = "Z";
    mNames[GLFW_KEY_LEFT_SUPER] = "Left Win";
    mNames[GLFW_KEY_RIGHT_SUPER] = "Right Win";
    mNames[GLFW_KEY_KP_0] = "Numpad 0";
    mNames[GLFW_KEY_KP_1] = "Numpad 1";
    mNames[GLFW_KEY_KP_2] = "Numpad 2";
    mNames[GLFW_KEY_KP_3] = "Numpad 3";
    mNames[GLFW_KEY_KP_4] = "Numpad 4";
    mNames[GLFW_KEY_KP_5] = "Numpad 5";
    mNames[GLFW_KEY_KP_6] = "Numpad 6";
    mNames[GLFW_KEY_KP_7] = "Numpad 7";
    mNames[GLFW_KEY_KP_8] = "Numpad 8";
    mNames[GLFW_KEY_KP_9] = "Numpad 9";
    mNames[GLFW_KEY_KP_MULTIPLY] = "Numpad *";
    mNames[GLFW_KEY_KP_ADD] = "Numpad +";
    mNames[GLFW_KEY_KP_SUBTRACT] = "Num -";
    mNames[GLFW_KEY_KP_DECIMAL] = "Numpad .";
    mNames[GLFW_KEY_KP_DIVIDE] = "Numpad /";
    mNames[GLFW_KEY_F1] = "F1";
    mNames[GLFW_KEY_F2] = "F2";
    mNames[GLFW_KEY_F3] = "F3";
    mNames[GLFW_KEY_F4] = "F4";
    mNames[GLFW_KEY_F5] = "F5";
    mNames[GLFW_KEY_F6] = "F6";
    mNames[GLFW_KEY_F7] = "F7";
    mNames[GLFW_KEY_F8] = "F8";
    mNames[GLFW_KEY_F9] = "F9";
    mNames[GLFW_KEY_F10] = "F10";
    mNames[GLFW_KEY_F11] = "F11";
    mNames[GLFW_KEY_F12] = "F12";
    mNames[GLFW_KEY_F13] = "F13";
    mNames[GLFW_KEY_F14] = "F14";
    mNames[GLFW_KEY_F15] = "F15";
    mNames[GLFW_KEY_F16] = "F16";
    mNames[GLFW_KEY_F17] = "F17";
    mNames[GLFW_KEY_F18] = "F18";
    mNames[GLFW_KEY_F19] = "F19";
    mNames[GLFW_KEY_F20] = "F20";
    mNames[GLFW_KEY_F21] = "F21";
    mNames[GLFW_KEY_F22] = "F22";
    mNames[GLFW_KEY_F23] = "F23";
    mNames[GLFW_KEY_F24] = "F24";
    mNames[GLFW_KEY_NUM_LOCK] = "Num Lock";
    mNames[GLFW_KEY_SCROLL_LOCK] = "Scrol Lock";
    mNames[GLFW_KEY_LEFT_SHIFT] = "Left Shift";
    mNames[GLFW_KEY_RIGHT_SHIFT] = "Right Shift";
    mNames[GLFW_KEY_LEFT_CONTROL] = "Left Ctrl";
    mNames[GLFW_KEY_RIGHT_CONTROL] = "Right Ctrl";
    mNames[GLFW_KEY_LEFT_ALT] = "Left Alt";
    mNames[GLFW_KEY_RIGHT_ALT] = "Right Alt";
  }

  char const* KeyNames::name( uint32_t key ) const
  {
    if ( key < mNames.size() && mNames[key] )
    {
      return mNames[key];
    }
    else
    {
      L_ERROR << "Unknown KeyName: " << key;
      return "Error";
    }
  }
}

char const* keyName( uint32_t key )
{
  static KeyNames keyNames;

  return keyNames.name( key );
}
