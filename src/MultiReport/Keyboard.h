/*
Copyright (c) 2014-2015 NicoHood
See the readme for credit to other people.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Include guard
#pragma once

#include <Arduino.h>
#include "PluggableUSB.h"
#include "HID.h"
#include "HID-Settings.h"

#include "HIDTables.h"
#include "HIDAliases.h"

#include "DescriptorPrimitives.h"

#include "BootKeyboard/BootKeyboard.h"

#define KEY_BYTES 28

typedef union {
  // Modifiers + keymap
  struct {
    uint8_t modifiers;
    uint8_t keys[KEY_BYTES ];
  };
  uint8_t allkeys[1 + KEY_BYTES];
} HID_KeyboardReport_Data_t;

typedef enum {
  BOOT_FALLBACK,
  NKRO_ONLY,
} keyboard_report_type_t;

template <keyboard_report_type_t report_type = BOOT_FALLBACK>
class Keyboard_ {
 public:
  Keyboard_(void);
  void begin(void);
  void end(void);

  size_t press(uint8_t k);
  size_t release(uint8_t k);
  void  releaseAll(void);
  int sendReport(void);

  boolean isModifierActive(uint8_t k);
  boolean wasModifierActive(uint8_t k);

  uint8_t getLEDs() { return HID().getLEDs(); };

  HID_KeyboardReport_Data_t keyReport;
  HID_KeyboardReport_Data_t lastKeyReport;
 private:
  int sendReportUnchecked(void);
};

extern Keyboard_<NKRO_ONLY> Keyboard;

///

static const uint8_t _hidMultiReportDescriptorKeyboard[] PROGMEM = {
  //  NKRO Keyboard
  D_USAGE_PAGE, D_PAGE_GENERIC_DESKTOP,
  D_USAGE, D_USAGE_KEYBOARD,
  D_COLLECTION, D_APPLICATION,
  D_REPORT_ID, HID_REPORTID_NKRO_KEYBOARD,
  D_USAGE_PAGE, D_PAGE_KEYBOARD,


  /* Key modifier byte */
  D_USAGE_MINIMUM, HID_KEYBOARD_FIRST_MODIFIER,
  D_USAGE_MAXIMUM, HID_KEYBOARD_LAST_MODIFIER,
  D_LOGICAL_MINIMUM, 0x00,
  D_LOGICAL_MAXIMUM, 0x01,
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, 0x08,
  D_INPUT, (D_DATA|D_VARIABLE|D_ABSOLUTE),


	/* 5 LEDs for num lock etc, 3 left for advanced, custom usage */
  D_USAGE_PAGE, D_PAGE_LEDS,
  D_USAGE_MINIMUM, 0x01,
  D_USAGE_MAXIMUM, 0x08,
  D_REPORT_COUNT, 0x08,
  D_REPORT_SIZE, 0x01,
  D_OUTPUT, (D_DATA | D_VARIABLE | D_ABSOLUTE),

// USB Code not within 4-49 (0x4-0x31), 51-155 (0x33-0x9B), 157-164 (0x9D-0xA4),
// 176-221 (0xB0-0xDD) or 224-231 (0xE0-0xE7) NKRO Mode
  /* NKRO Keyboard */
  D_USAGE_PAGE, D_PAGE_KEYBOARD,

  // Padding 3 bits
  // To skip HID_KEYBOARD_NON_US_POUND_AND_TILDE, which causes
  // Linux to choke on our driver.
  D_REPORT_SIZE, 0x04,
  D_REPORT_COUNT, 0x01,
  D_INPUT, (D_CONSTANT),


  D_USAGE_MINIMUM, HID_KEYBOARD_A_AND_A,
  D_USAGE_MAXIMUM, HID_KEYBOARD_BACKSLASH_AND_PIPE,
  D_LOGICAL_MINIMUM, 0x00,
  D_LOGICAL_MAXIMUM, 0x01,
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, (HID_KEYBOARD_BACKSLASH_AND_PIPE - HID_KEYBOARD_A_AND_A)+1,
  D_INPUT, (D_DATA|D_VARIABLE|D_ABSOLUTE),

  // Padding 1 bit.
  // To skip HID_KEYBOARD_NON_US_POUND_AND_TILDE, which causes
  // Linux to choke on our driver.
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, 0x01,
  D_INPUT, (D_CONSTANT),


  D_USAGE_MINIMUM, HID_KEYBOARD_SEMICOLON_AND_COLON,
  D_USAGE_MAXIMUM, HID_KEYBOARD_CANCEL,
  D_LOGICAL_MINIMUM, 0x00,
  D_LOGICAL_MAXIMUM, 0x01,
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, (HID_KEYBOARD_CANCEL-HID_KEYBOARD_SEMICOLON_AND_COLON) +1,
  D_INPUT, (D_DATA|D_VARIABLE|D_ABSOLUTE),


  // Padding 1 bit.
  // To skip HID_KEYBOARD_CLEAR, which causes
  // Linux to choke on our driver.
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, 0x01,
  D_INPUT, (D_CONSTANT),

  D_USAGE_MINIMUM, HID_KEYBOARD_PRIOR,
  D_USAGE_MAXIMUM, HID_KEYPAD_HEXADECIMAL,
  D_LOGICAL_MINIMUM, 0x00,
  D_LOGICAL_MAXIMUM, 0x01,
  D_REPORT_SIZE, 0x01,
  D_REPORT_COUNT, (HID_KEYPAD_HEXADECIMAL - HID_KEYBOARD_PRIOR)  +1,
  D_INPUT, (D_DATA|D_VARIABLE|D_ABSOLUTE),


  // Padding (w bits)
  D_REPORT_SIZE, 0x02,
  D_REPORT_COUNT, 0x01,
  D_INPUT, (D_CONSTANT),

  D_END_COLLECTION,

};

template<keyboard_report_type_t type>
Keyboard_<type>::Keyboard_(void) {
  static HIDSubDescriptor node(_hidMultiReportDescriptorKeyboard, sizeof(_hidMultiReportDescriptorKeyboard));
  HID().AppendDescriptor(&node);
}

template<keyboard_report_type_t type>
void Keyboard_<type>::begin(void) {
  // Force API to send a clean report.
  // This is important for and HID bridge where the receiver stays on,
  // while the sender is resetted.
  releaseAll();
  sendReportUnchecked();

  if (type == BOOT_FALLBACK)
    BootKeyboard.begin();
}

template<keyboard_report_type_t type>
void Keyboard_<type>::end(void) {
  releaseAll();
  sendReportUnchecked();

  if (type == BOOT_FALLBACK)
    BootKeyboard.end();
}

template<keyboard_report_type_t type>
int Keyboard_<type>::sendReportUnchecked(void) {
    return HID().SendReport(HID_REPORTID_NKRO_KEYBOARD, &keyReport, sizeof(keyReport));
}

template<keyboard_report_type_t type>
int Keyboard_<type>::sendReport(void) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL )
      return BootKeyboard.sendReport();
  }

  // If the last report is different than the current report, then we need to send a report.
  // We guard sendReport like this so that calling code doesn't end up spamming the host with empty reports
  // if sendReport is called in a tight loop.

  if (memcmp(lastKeyReport.allkeys, keyReport.allkeys, sizeof(keyReport))) {
    // if the two reports are different, send a report
    int returnCode = sendReportUnchecked();
    memcpy(lastKeyReport.allkeys, keyReport.allkeys, sizeof(keyReport));
    return returnCode;
  }
  return -1;
}

/* Returns true if the modifer key passed in will be sent during this key report
 * Returns false in all other cases
 * */
template<keyboard_report_type_t type>
boolean Keyboard_<type>::isModifierActive(uint8_t k) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL)
      return BootKeyboard.isModifierActive(k);
  }

  if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    return !!(keyReport.modifiers & (1 << k));
  }
  return false;
}

/* Returns true if the modifer key passed in was being sent during the previous key report
 * Returns false in all other cases
 * */
template<keyboard_report_type_t type>
boolean Keyboard_<type>::wasModifierActive(uint8_t k) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL)
      return BootKeyboard.wasModifierActive(k);
  }

  if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    return !!(lastKeyReport.modifiers & (1 << k));
  }
  return false;
}

template<keyboard_report_type_t type>
size_t Keyboard_<type>::press(uint8_t k) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL)
      return BootKeyboard.press(k);
  }

  // If the key is in the range of 'printable' keys
  if (k <= HID_LAST_KEY) {
    uint8_t bit = 1 << (uint8_t(k) % 8);
    keyReport.keys[k / 8] |= bit;
    return 1;
  }

  // It's a modifier key
  else if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    // Convert key into bitfield (0 - 7)
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    keyReport.modifiers |= (1 << k);
    return 1;
  }

  // No empty/pressed key was found
  return 0;
}

template<keyboard_report_type_t type>
size_t Keyboard_<type>::release(uint8_t k) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL)
      return BootKeyboard.release(k);
  }

  // If we're releasing a printable key
  if (k <= HID_LAST_KEY) {
    uint8_t bit = 1 << (k % 8);
    keyReport.keys[k / 8] &= ~bit;
    return 1;
  }

  // It's a modifier key
  else if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    // Convert key into bitfield (0 - 7)
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    keyReport.modifiers &= ~(1 << k);
    return 1;
  }

  // No empty/pressed key was found
  return 0;
}

template<keyboard_report_type_t type>
void Keyboard_<type>::releaseAll(void) {
  if (type == BOOT_FALLBACK) {
    if (BootKeyboard.getProtocol() == HID_BOOT_PROTOCOL)
      return BootKeyboard.releaseAll();
  }

  // Release all keys
  memset(&keyReport.allkeys, 0x00, sizeof(keyReport.allkeys));
}
