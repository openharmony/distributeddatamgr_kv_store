/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_RES_COMMON_H
#define OHOS_RES_COMMON_H

#include <stdint.h>

#ifdef __EXPORT_MGR__
#define EXPORT_FUNC __declspec(dllexport)
#else
#define EXPORT_FUNC
#endif

namespace OHOS {
namespace Global {
namespace Resource {
// DIRECTION
static const char *VERTICAL = "vertical";
static const char *HORIZONTAL = "horizontal";

// DEVICETYPE
static const char *DEFAULT_STR = "default";
static const char *PHONE_STR = "phone";
static const char *TABLET_STR = "tablet";
static const char *CAR_STR = "car";
static const char *PAD_STR = "pad";
static const char *TV_STR = "tv";
static const char *WEARABLE_STR = "wearable";
static const char *TWOINONE_STR = "2in1";

// ColorMode
static const char *DARK_STR = "dark";
static const char *LIGHT_STR = "light";

// InputDevice
static const char *POINTING_DEVICE_STR = "pointingdevice";

// ScreenDensity
static const char *RE_120_STR = "sdpi";
static const char *RE_160_STR = "mdpi";
static const char *RE_240_STR = "ldpi";
static const char *RE_320_STR = "xldpi";
static const char *RE_480_STR = "xxldpi";
static const char *RE_640_STR = "xxxldpi";

static const uint32_t MCC_UNDEFINED = 0;
static const uint32_t MNC_UNDEFINED = 0;

// the type of qualifiers
typedef enum KeyType {
    LANGUAGES       = 0,
    REGION          = 1,
    SCREEN_DENSITY  = 2,
    DIRECTION       = 3,
    DEVICETYPE      = 4,
    SCRIPT          = 5,
    COLORMODE       = 6, // DARK = 0, LIGHT = 1
    MCC             = 7,
    MNC             = 8,
    // RESERVER 9
    INPUTDEVICE     = 10,
    KEY_TYPE_MAX,
} KeyType;

// the type of resources
typedef enum ResType {
    VALUES    = 0,
    ANIMATION = 1,
    DRAWABLE  = 2,
    LAYOUT    = 3,
    MENU      = 4,
    MIPMAP    = 5,
    RAW       = 6,
    XML       = 7,

    INTEGER          = 8,
    STRING           = 9,
    STRINGARRAY      = 10,
    INTARRAY         = 11,
    BOOLEAN          = 12,
    DIMEN            = 13,
    COLOR            = 14,
    ID               = 15,
    THEME            = 16,
    PLURALS          = 17,
    FLOAT            = 18,
    MEDIA            = 19,
    PROF             = 20,
    SVG              = 21,
    PATTERN          = 22,
    SYMBOL           = 23,
    MAX_RES_TYPE     = 24,
} ResType;

static const uint32_t SELECT_INTEGER = 0x0001;
static const uint32_t SELECT_STRING = 0x0002;
static const uint32_t SELECT_STRINGARRAY = 0x0004;
static const uint32_t SELECT_INTARRAY = 0x0008;
static const uint32_t SELECT_BOOLEAN = 0x0010;
static const uint32_t SELECT_COLOR = 0x0020;
static const uint32_t SELECT_THEME = 0x0040;
static const uint32_t SELECT_PLURALS = 0x0080;
static const uint32_t SELECT_FLOAT = 0x0100;
static const uint32_t SELECT_MEDIA = 0x0200;
static const uint32_t SELECT_PROF = 0x0400;
static const uint32_t SELECT_PATTERN = 0x0800;
static const uint32_t SELECT_SYMBOL = 0x1000;
static const uint32_t SELECT_ALL = 0xFFFFFFFF;

enum DeviceType : int32_t {
    DEVICE_NOT_SET  = -1,
    DEVICE_PHONE    = 0,
    DEVICE_TABLET   = 1,
    DEVICE_CAR      = 2,
    DEVICE_PAD      = 3,
    DEVICE_TV       = 4,
    DEVICE_WEARABLE = 6,
    DEVICE_TWOINONE = 7,
};

enum TimeFormat {
    HOUR_NOT_SET = -1,
    HOUR_12      = 0,
    HOUR_24      = 1,
};

enum ColorMode {
    COLOR_MODE_NOT_SET = -1,
    DARK               = 0,
    LIGHT              = 1,
};

enum InputDevice {
    INPUTDEVICE_NOT_SET = -1,
    INPUTDEVICE_POINTINGDEVICE = 0,
};

enum ScreenDensity {
    SCREEN_DENSITY_NOT_SET  = 0,
    SCREEN_DENSITY_SDPI     = 120,
    SCREEN_DENSITY_MDPI     = 160,
    SCREEN_DENSITY_LDPI     = 240,
    SCREEN_DENSITY_XLDPI    = 320,
    SCREEN_DENSITY_XXLDPI   = 480,
    SCREEN_DENSITY_XXXLDPI  = 640,
};

enum Direction {
    DIRECTION_NOT_SET = -1,
    DIRECTION_VERTICAL = 0,
    DIRECTION_HORIZONTAL = 1
};
} // namespace Resource
} // namespace Global
} // namespace OHOS
#endif // OHOS_RES_COMMON_H