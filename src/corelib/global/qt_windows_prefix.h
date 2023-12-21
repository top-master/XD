/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/


/**
 * Under other, may enable latest-features of Windows SDK and/or DDK.
 *
 * Note to include this file first, else some compilers, like MingW,
 * may define _WIN32_WINNT with a fake and/or old value.
 */
#ifndef QT_WINDOWS_PREFIX_H
#define QT_WINDOWS_PREFIX_H

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#if defined(WIN32) || defined(_WIN32)

#ifndef WINVER
#  define QT_SET_WINVER
#endif
#ifndef _WIN32_WINNT
#  define QT_SET__WIN32_WINNT
#endif
#if defined(QT_FORCE_PREFIX) && (defined(_INC_SDKDDKVER) || defined(_INC__MINGW_H))
//
// MSVC: to debug below, pass `/showIncludes` flag in compiler's command-line,
// like `QMAKE_CXXFLAGS += /showIncludes`, also, disable multi-core build
// (maybe by passing -j1 in Make-executable's command-line).
//
#  error "qt_windows_prefix.h file should be included before any Windows-SDK header."
#endif
#include <sdkddkver.h>

// Older MingW's windows.h does not set _WIN32_WINNT,
// Borland's results to unusable WinSDK standard dialogs,
// and clang's sets fake/old number (causing errors for our redefinitions).
#ifdef QT_SET_WINVER
#  undef QT_SET_WINVER
#  ifdef WINVER
#    undef WINVER
#  endif
#  if defined(_WIN32_WINNT_WIN10)
#    define WINVER 0x0A00
#  elif defined(_WIN32_WINNT_WINBLUE)
#    define WINVER 0x603
#  elif defined(_WIN32_WINNT_WIN8)
#    define WINVER 0x602
#  elif defined(_WIN32_WINNT_WIN7)
#    define WINVER 0x601
#  elif defined(_WIN32_WINNT_VISTA)
#    define WINVER 0x600
#  elif defined(_WIN32_WINNT_WS03)
#    define WINVER 0x502
#  else
#    define WINVER 0x501
#  endif
#endif

#ifdef QT_SET__WIN32_WINNT
#  undef QT_SET__WIN32_WINNT
#  ifdef _WIN32_WINNT
#    undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT WINVER
#endif

#if _WIN32_WINNT < WINVER
#  undef _WIN32_WINNT
#  define _WIN32_WINNT WINVER
#elif WINVER < _WIN32_WINNT
#  undef WINVER
#  define WINVER _WIN32_WINNT
#endif

#ifndef NTDDI_VERSION
// Defaults to NTDDI_WIN10_CO.
#  define NTDDI_VERSION ((WINVER << 16) | 0x000B)
#endif

#if defined(_WIN32_IE) && _WIN32_IE < 0x0501
#  undef _WIN32_IE
#endif
#if !defined(_WIN32_IE)
#  define _WIN32_IE 0x0501
#endif

#ifndef NOMINMAX
#  define NOMINMAX
#endif

#endif // WIN32

#endif // QT_WINDOWS_PREFIX_H
