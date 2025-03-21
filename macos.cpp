
#include "ch9329.h"
#include <Carbon/Carbon.h>
#include <array>

static std::array<uint8_t, 0x100> inner;

uint8_t key_to_scancode(uint32_t key) {

  if (key < inner.size()) {
    return inner[key];
  }
  return 0;
}

void key_to_scancode_init() {
  inner[kVK_ANSI_A] = CH9_A;
  inner[kVK_ANSI_B] = CH9_B;
  inner[kVK_ANSI_C] = CH9_C;
  inner[kVK_ANSI_D] = CH9_D;
  inner[kVK_ANSI_E] = CH9_E;
  inner[kVK_ANSI_F] = CH9_F;
  inner[kVK_ANSI_G] = CH9_G;
  inner[kVK_ANSI_H] = CH9_H;
  inner[kVK_ANSI_I] = CH9_I;
  inner[kVK_ANSI_J] = CH9_J;
  inner[kVK_ANSI_K] = CH9_K;
  inner[kVK_ANSI_L] = CH9_L;
  inner[kVK_ANSI_M] = CH9_M;
  inner[kVK_ANSI_N] = CH9_N;
  inner[kVK_ANSI_O] = CH9_O;
  inner[kVK_ANSI_P] = CH9_P;
  inner[kVK_ANSI_Q] = CH9_Q;
  inner[kVK_ANSI_R] = CH9_R;
  inner[kVK_ANSI_S] = CH9_S;
  inner[kVK_ANSI_T] = CH9_T;
  inner[kVK_ANSI_U] = CH9_U;
  inner[kVK_ANSI_X] = CH9_X;
  inner[kVK_ANSI_V] = CH9_V;
  inner[kVK_ANSI_W] = CH9_W;
  inner[kVK_ANSI_Y] = CH9_Y;
  inner[kVK_ANSI_Z] = CH9_Z;
  inner[kVK_ANSI_1] = CH9_1;
  inner[kVK_ANSI_2] = CH9_2;
  inner[kVK_ANSI_3] = CH9_3;
  inner[kVK_ANSI_4] = CH9_4;
  inner[kVK_ANSI_5] = CH9_5;
  inner[kVK_ANSI_6] = CH9_6;
  inner[kVK_ANSI_7] = CH9_7;
  inner[kVK_ANSI_8] = CH9_8;
  inner[kVK_ANSI_9] = CH9_9;
  inner[kVK_ANSI_0] = CH9_0;
  inner[kVK_Return] = CH9_Enter;
  inner[kVK_Escape] = CH9_Escape;
  inner[kVK_Delete] = CH9_Backspace;
  inner[kVK_Tab] = CH9_Tab;
  inner[kVK_Space] = CH9_Space;
  inner[kVK_ANSI_Minus] = CH9_Minus;
  inner[kVK_ANSI_LeftBracket] = CH9_LeftBracket;
  inner[kVK_ANSI_RightBracket] = CH9_RightBracket;
  inner[kVK_ANSI_Backslash] = CH9_Backslash;
  inner[kVK_ANSI_Semicolon] = CH9_Semicolon;
  inner[kVK_ANSI_Quote] = CH9_Quote;
  inner[kVK_ANSI_Grave] = CH9_BackQuote;
  inner[kVK_ANSI_Comma] = CH9_Comma;
  inner[kVK_ANSI_Period] = CH9_Period;
  inner[kVK_ANSI_Slash] = CH9_Slash;
  inner[kVK_CapsLock] = CH9_CapsLock;
  inner[kVK_F1] = CH9_F1;
  inner[kVK_F2] = CH9_F2;
  inner[kVK_F3] = CH9_F3;
  inner[kVK_F4] = CH9_F4;
  inner[kVK_F5] = CH9_F5;
  inner[kVK_F6] = CH9_F6;
  inner[kVK_F7] = CH9_F7;
  inner[kVK_F8] = CH9_F8;
  inner[kVK_F9] = CH9_F9;
  inner[kVK_F10] = CH9_F10;
  inner[kVK_F11] = CH9_F11;
  inner[kVK_F12] = CH9_F12;
  inner[kVK_Help] = CH9_Insert;
  inner[kVK_Home] = CH9_Home;
  inner[kVK_PageUp] = CH9_PageUp;
  inner[kVK_ForwardDelete] = CH9_ForwardDelete;
  inner[kVK_End] = CH9_End;
  inner[kVK_PageDown] = CH9_PageDown;
  inner[kVK_RightArrow] = CH9_RightArrow;
  inner[kVK_LeftArrow] = CH9_LeftArrow;
  inner[kVK_DownArrow] = CH9_DownArrow;
  inner[kVK_UpArrow] = CH9_UpArrow;
  inner[kVK_ANSI_KeypadClear] = CH9_KeypadNumLock;
  inner[kVK_ANSI_KeypadDivide] = CH9_KeypadDivide;
  inner[kVK_ANSI_KeypadMultiply] = CH9_KeypadMultiply;
  inner[kVK_ANSI_KeypadMinus] = CH9_KeypadMinus;
  inner[kVK_ANSI_KeypadPlus] = CH9_KeypadPlus;
  inner[kVK_ANSI_KeypadEnter] = CH9_KeypadEnter;
  inner[kVK_ANSI_Keypad1] = CH9_Keypad1;
  inner[kVK_ANSI_Keypad2] = CH9_Keypad2;
  inner[kVK_ANSI_Keypad3] = CH9_Keypad3;
  inner[kVK_ANSI_Keypad4] = CH9_Keypad4;
  inner[kVK_ANSI_Keypad5] = CH9_Keypad5;
  inner[kVK_ANSI_Keypad6] = CH9_Keypad6;
  inner[kVK_ANSI_Keypad7] = CH9_Keypad7;
  inner[kVK_ANSI_Keypad8] = CH9_Keypad8;
  inner[kVK_ANSI_Keypad9] = CH9_Keypad9;
  inner[kVK_ANSI_Keypad0] = CH9_Keypad0;
  inner[kVK_ANSI_KeypadDecimal] = CH9_KeypadPeriod;
  inner[kVK_Control] = CH9_Control;
  inner[kVK_Shift] = CH9_Shift;
  inner[kVK_Option] = CH9_Alt;
  inner[kVK_Command] = CH9_Win;
  inner[kVK_RightControl] = CH9_RightControl;
  inner[kVK_RightShift] = CH9_RightShift;
  inner[kVK_RightOption] = CH9_RightAlt;
  inner[kVK_RightCommand] = CH9_RightWin;
}
