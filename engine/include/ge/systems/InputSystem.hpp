#pragma once

#include <map>
#include <set>

#include <ge/systems/Systems.hpp>

namespace GE
{
    class GfxApplication;

    namespace Keyboard
    {
        enum Key
        {
            A = 65,            //!<  A key
            B,                 //!<  B key
            C,                 //!<  C key
            D,                 //!<  D key
            E,                 //!<  E key
            F,                 //!<  F key
            G,                 //!<  G key
            H,                 //!<  H key
            I,                 //!<  I key
            J,                 //!<  J key
            K,                 //!<  K key
            L,                 //!<  L key
            M,                 //!<  M key
            N,                 //!<  N key
            O,                 //!<  O key
            P,                 //!<  P key
            Q,                 //!<  Q key
            R,                 //!<  R key
            S,                 //!<  S key
            T,                 //!<  T key
            U,                 //!<  U key
            V,                 //!<  V key
            W,                 //!<  W key
            X,                 //!<  X key
            Y,                 //!<  Y key
            Z,                 //!<  Z key
            D1 = 48,           //!<  1 key
            D2,                //!<  2 key
            D3,                //!<  3 key
            D4,                //!<  4 key
            D5,                //!<  5 key
            D6,                //!<  6 key
            D7,                //!<  7 key
            D8,                //!<  8 key
            D9,                //!<  9 key
            D0,                //!<  0 key
            Return = 257,      //!<  ENTER key
            Escape = 256,      //!<  ESC key
            Backspace = 259,   //!<  BACKSPACE key
            Tab = 258,         //!<  TAB key
            Space = 32,        //!<  SPACEBAR
            Minus = 45,        //!<  MINUS SIGN key.
            Plus = 61,         //!<  PLUS SIGN key.
            OpenBracket = 91,  //!<  LEFT BRACKET ([) key
            CloseBracket = 93, //!<  RIGHT BRACKET (]) key
            Pipe = 92,         //!<  VERTICAL BAR (|) key
            Tilde = 97,        //!<  TILDE (~) key
            Semicolon = 59,    //!<  SEMICOLON (,) key
            Quote = 39,        //!<  APOSTROPHE (') key
            Backquote = 96,    //!<  GRAVE ACCENT (`) key
            Comma = 44,        //!<  COMMA (,) key
            Period = 46,       //!<  PERIOD (.) key
            Slash = 47,        //!<  SLASH (/) key
            CapsLock = 280,    //!<  CAPS LOCK key
            F1 = 290,          //!<  F1 key
            F2,                //!<  F2 key
            F3,                //!<  F3 key
            F4,                //!<  F4 key
            F5,                //!<  F5 key
            F6,                //!<  F6 key
            F7,                //!<  F7 key
            F8,                //!<  F8 key
            F9,                //!<  F9 key
            F10,               //!<  F10 key
            F11,               //!<  F11 key
            F12,               //!<  F12 key

            // TODO: Finish mapping below this point
            PrintScreen,       //!<  PRINT SCREEN key
            ScrollLock,        //!<  SCROLL LOCK key
            Pause,             //!<  PAUSE key
            Insert,            //!<  INSERT key
            Home,              //!<  HOME key
            PageUp,            //!<  PAGE UP key
            Delete,            //!<  DELETE key
            End,               //!<  END key
            PageDown,          //!<  PAGE DOWN key
            RightArrow,        //!<  RIGHT ARROW key
            LeftArrow,         //!<  LEFT ARROW key
            DownArrow,         //!<  DOWN ARROW key
            UpArrow,           //!<  UP ARROW key
            NumLock,           //!<  NUM LOCK key
            NumPadDivide,      //!<  SLASH MARK (/) key on the numeric keypad
            NumPadMultiply,    //!<  ASTERISK (*) key on the numeric keypad
            NumPadSubtract,    //!<  MINUS SIGN (-) key on the numeric keypad
            NumPadAdd,         //!<  PLUS SIGN (+) key on the numeric keypad
            NumPadEnter,       //!<  ENTER key on the numeric keypad
            NumPad1,           //!<  1 key on the numeric keypad
            NumPad2,           //!<  2 key on the numeric keypad
            NumPad3,           //!<  3 key on the numeric keypad
            NumPad4,           //!<  4 key on the numeric keypad
            NumPad5,           //!<  5 key on the numeric keypad
            NumPad6,           //!<  6 key on the numeric keypad
            NumPad7,           //!<  7 key on the numeric keypad
            NumPad8,           //!<  8 key on the numeric keypad
            NumPad9,           //!<  9 key on the numeric keypad
            NumPad0,           //!<  0 key on the numeric keypad
            NumPadDot,         //!<  PERIOD (.) key on the numeric keypad
            Backslash,         //!<  BACKSLASH (\) key
            Application,       //!<  Application key
            Power,             //!<  POWER key
            NumPadEquals,      //!<  EQUAL SIGN (=) key on the numeric keypad
            F13,               //!<  F13 key
            F14,               //!<  F14 key
            F15,               //!<  F15 key
            F16,               //!<  F16 key
            F17,               //!<  F17 key
            F18,               //!<  F18 key
            F19,               //!<  F19 key
            F20,               //!<  F20 key
            F21,               //!<  F21 key
            F22,               //!<  F22 key
            F23,               //!<  F23 key
            F24,               //!<  F24 key
            NumPadComma,       //!<  COMMA (,) key on the numeric keypad
            Ro,                //!<  "RO" (ろ) key
            KatakanaHiragana,  //!<  Katakana/Hiragana key
            Yen,               //!<  Symbols key
            Henkan,            //!<  Conversion key
            Muhenkan,          //!<  Non-conversion key
            NumPadCommaPc98,   //!<  COMMA (,) key on the numeric keypad (for the PC98)
            HangulEnglish,     //!<  Hangul mode key
            Hanja,             //!<  Hanja mode key
            Katakana,          //!<  Katakana key
            Hiragana,          //!<  Hiragana key
            ZenkakuHankaku,    //!<  Full width / Half width key
            LeftControl,       //!<  Left CTRL key
            LeftShift,         //!<  Left SHIFT key
            LeftAlt,           //!<  Left ALT key
            LeftGui,           //!<  Left GUI key
            RightControl,      //!<  Right CTRL key
            RightShift,        //!<  Right SHIFT key
            RightAlt,          //!<  Right ALT key
            RightGui,          //!<  Right GUI key
        };
    }
    namespace Controller
    {
		enum Button
		{
			A = 0,        //!<  Npad A Button.
			B,            //!<  Npad B Button.
			X,            //!<  Npad X Button.
			Y,            //!<  Npad Y Button.
			StickL,       //!<  Npad L Stick Button.
			StickR,       //!<  Npad R Stick Button.
			L,            //!<  Npad L Button.
			R,            //!<  Npad R Button.
			ZL,           //!<  Npad ZL Button.
			ZR,           //!<  Npad ZR Button.
			Plus,         //!<  Npad + Button.
			Minus,        //!<  Npad - Button.
			Left,         //!<  Npad +Control Pad Left.
			Up,           //!<  Npad +Control Pad Up.
			Right,        //!<  Npad +Control Pad Right.
			Down,         //!<  Npad +Control Pad Down.
			StickLLeft,   //!<  Npad left stick +Control Pad emulation - left.
			StickLUp,     //!<  Npad left stick +Control Pad emulation - up.
			StickLRight,  //!<  Npad left stick +Control Pad emulation - right.
			StickLDown,   //!<  Npad left stick +Control Pad emulation - down.
			StickRLeft,   //!<  Npad right stick +Control Pad emulation - left.
			StickRUp,     //!<  Npad right stick +Control Pad emulation - up.
			StickRRight,  //!<  Npad right stick +Control Pad emulation - right.
			StickRDown,   //!<  Npad right stick +Control Pad emulation - down.
		};
	}

	namespace Sys
	{
		struct ControllerState
		{
            std::set<int> buttonsPressed;
		};

        bool IsButtonPressed(ControllerState& state, int button);

		class InputSystem : public Sys::System
		{
		public:
			InputSystem(GfxApplication* gfxApplication);

			void Attach();
			void Detach();

			virtual void Update(int64_t tsMicroseconds) override;

            InputSystem& UpdateKeyBinding(Keyboard::Key key, Controller::Button button);
		};
	}
}