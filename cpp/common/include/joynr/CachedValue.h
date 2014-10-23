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
#ifndef CACHEDVALUE_H
#define CACHEDVALUE_H

#include <QtGlobal>

namespace joynr
{

template <class T>
class CachedValue
{

public:
    CachedValue<T>();
    CachedValue<T>(const CachedValue<T>& other);
    CachedValue<T>(T value, qint64 timestamp);

    T getValue();
    qint64 getTimestamp();

    CachedValue<T>& operator=(const CachedValue<T>& other);
    bool operator==(const CachedValue<T>& other) const;
    bool operator!=(const CachedValue<T>& other) const;

private:
    T value;
    qint64 timestamp;
};

template <class T>
CachedValue<T>::CachedValue()
        : value(T()), timestamp(0)
{
}

template <class T>
CachedValue<T>::CachedValue(const CachedValue<T>& other)
        : value(other.value), timestamp(other.timestamp)
{
}

template <class T>
CachedValue<T>::CachedValue(T value, qint64 timestamp)
        : value(value), timestamp(timestamp)
{
}

template <class T>
qint64 CachedValue<T>::getTimestamp()
{
    return timestamp;
}

template <class T>
T CachedValue<T>::getValue()
{
    return value;
}

template <class T>
bool CachedValue<T>::operator==(const CachedValue<T>& other) const
{
    if (other.value == value) {
        return true;
    }
    return false;
}

template <class T>
bool CachedValue<T>::operator!=(const CachedValue<T>& other) const
{
    return !(*this == other);
}

template <class T>
CachedValue<T>& CachedValue<T>::operator=(const CachedValue<T>& other)
{
    this->value = other.value;
    this->timestamp = other.timestamp;
    return *this;
}

} // namespace joynr
#endif // CACHEDVALUE_H
