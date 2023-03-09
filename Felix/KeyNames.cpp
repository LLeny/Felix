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
    std::array<char const*, 256> mNames;
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
    mNames['0'] = "0";
    mNames['1'] = "1";
    mNames['2'] = "2";
    mNames['3'] = "3";
    mNames['4'] = "4";
    mNames['5'] = "5";
    mNames['6'] = "6";
    mNames['7'] = "7";
    mNames['8'] = "8";
    mNames['9'] = "9";
    mNames['A'] = "A";
    mNames['B'] = "B";
    mNames['C'] = "C";
    mNames['D'] = "D";
    mNames['E'] = "E";
    mNames['F'] = "F";
    mNames['G'] = "G";
    mNames['H'] = "H";
    mNames['I'] = "I";
    mNames['J'] = "J";
    mNames['K'] = "K";
    mNames['L'] = "L";
    mNames['M'] = "M";
    mNames['N'] = "N";
    mNames['O'] = "O";
    mNames['P'] = "P";
    mNames['Q'] = "Q";
    mNames['R'] = "R";
    mNames['S'] = "S";
    mNames['T'] = "T";
    mNames['U'] = "U";
    mNames['V'] = "V";
    mNames['W'] = "W";
    mNames['X'] = "X";
    mNames['Y'] = "Y";
    mNames['Z'] = "Z";
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
    return key < mNames.size() ? mNames[key] : nullptr;
  }

}

char const* keyName( uint32_t key )
{
  static KeyNames keyNames;

  return keyNames.name( key );
}
