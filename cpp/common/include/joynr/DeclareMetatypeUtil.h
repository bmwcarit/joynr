/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef DECLAREMETATYPEUTIL_H
#define DECLAREMETATYPEUTIL_H

#include <QMetaType>
#include <QObject>
#include <QSharedPointer>
#include <QList>

// http://doc.qt.nokia.com/4.7-snapshot/qmetatype.html#Q_DECLARE_METATYPE
// Ideally, the Q_DECLARE_METATYPE macro should be placed below the declaration
// of the class or struct. If that is not possible, it can be put in a private
// header file which has to be included every time that type is used in a QVariant.

// Use this header file to place Q_DECLARE_METATYPE that could not placed
// directly below the declaration of the class or struct.

Q_DECLARE_METATYPE(QSharedPointer<QObject>)
Q_DECLARE_METATYPE(qint8)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<qint8>)
Q_DECLARE_METATYPE(QList<qint64>)
Q_DECLARE_METATYPE(QList<bool>)
Q_DECLARE_METATYPE(QList<double>)
Q_DECLARE_METATYPE(QList<QString>)

template <class T>
uint qHash(const QList<T>& key, uint seed = 0)
{
    uint hashCode = 0;
    uint prime = 31;
    foreach (T k, key) {
        hashCode = prime * hashCode + qHash(k, seed);
    }
    return hashCode;
}

//#include "Reply.h"
// Q_DECLARE_METATYPE(QSharedPointer<Reply>) // TM I think this should be moved to Reply.h

#endif // DECLAREMETATYPEUTIL_H
