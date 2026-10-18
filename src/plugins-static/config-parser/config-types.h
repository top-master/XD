// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include "./config-object.h"

#include <QtCore/QVariant>

namespace Config {

namespace Types {

Object *object(const QVariant & value);
Object *liveObject(const void *address);
void track(Object *object);
void untrack(Object *object);

} // Namespace `Types`.

} // Namespace `Config`.

#endif // CONFIG_TYPES_H
