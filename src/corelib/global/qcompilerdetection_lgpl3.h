// Copyright (C) 2022 The XD Company Ltd.
// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcompilerdetection.h"

/*
 * SG10's SD-6 feature detection and some useful extensions from Clang and GCC
 * https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations
 * http://clang.llvm.org/docs/LanguageExtensions.html#feature-checking-macros
 */
#ifdef __has_builtin
#  define QT_HAS_BUILTIN(x)             __has_builtin(x)
#else
#  define QT_HAS_BUILTIN(x)             0
#endif
#ifdef __has_attribute
#  define QT_HAS_ATTRIBUTE(x)           __has_attribute(x)
#else
#  define QT_HAS_ATTRIBUTE(x)           0
#endif
#ifdef __has_cpp_attribute
#  define QT_HAS_CPP_ATTRIBUTE(x)       __has_cpp_attribute(x)
#else
#  define QT_HAS_CPP_ATTRIBUTE(x)       0
#endif
#ifdef __has_include
#  define QT_HAS_INCLUDE(x)             __has_include(x)
#else
#  define QT_HAS_INCLUDE(x)             0
#endif
#ifdef __has_include_next
#  define QT_HAS_INCLUDE_NEXT(x)        __has_include_next(x)
#else
#  define QT_HAS_INCLUDE_NEXT(x)        0
#endif


#if defined(__cplusplus) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(clang::fallthrough)
#    define Q_FALLTHROUGH() [[clang::fallthrough]]
#  elif __has_cpp_attribute(gnu::fallthrough)
#    define Q_FALLTHROUGH() [[gnu::fallthrough]]
#  elif __has_cpp_attribute(fallthrough)
#    define Q_FALLTHROUGH() [[fallthrough]]
#  endif
#endif /* __cplusplus */

#ifndef Q_FALLTHROUGH
#  if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
#    define Q_FALLTHROUGH() __attribute__((fallthrough))
#  else
#    define Q_FALLTHROUGH() (void)0
#  endif
#endif

/// Use to mark any intentional non-explicit cast
/// (no matter if constructor or operator).
#define Q_IMPLICIT
