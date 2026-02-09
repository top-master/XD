
// Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1 OR LGPL-3.0-only

// Shared config for new atomics.

#if defined(Q_COMPILER_CONSTEXPR) && defined(Q_COMPILER_DEFAULT_MEMBERS) && defined(Q_COMPILER_DELETE_MEMBERS)
# if defined(Q_CC_CLANG) && Q_CC_CLANG < 303
   /*
      Do not define QT_BASIC_ATOMIC_HAS_CONSTRUCTORS for Clang before version 3.3.
      For details about the bug: see http://llvm.org/bugs/show_bug.cgi?id=12670
    */
# else
#  define QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
# endif
#endif
