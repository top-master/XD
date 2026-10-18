// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_ARRAY_OBJ_H
#define CONFIG_ARRAY_OBJ_H

#include "./config-object.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Config {

class ArrayObj : public Object
{
    Q_OBJECT
    typedef Object super;
    Q_PROPERTY(int size READ size)
public:
    explicit ArrayObj(const QStringList & items = QStringList(),
                      QObject *parent = Q_NULLPTR);

    inline int size() const { return m_items.size(); }
    inline const QStringList &items() const { return m_items; }

public slots:
    QString join(const QString & separator) const;
    QVariant at(int index) const;

private:
    QStringList m_items;
};

} // Namespace `Config`.

#endif // CONFIG_ARRAY_OBJ_H
