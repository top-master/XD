// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void describe()
{
    output(
        "category",
        String("A long fictional description with two placeholders: %1 and %2") .replace(firstValue()) .replace(secondValue()),
        String("Another invented description for a single placeholder: %1").replace(value()),
        String("The result has %1 items and took %2 seconds") .replace(counter->size()).replace(timer.elapsed()),
        String("The record named %1 is currently selected") .replace(recordName()),
        String("Values %1 and %2 are ready for processing") .replace(first ? 'A' : 'B') .replace(secondName())
    );
}
