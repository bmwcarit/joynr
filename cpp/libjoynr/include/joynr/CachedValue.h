/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <chrono>

namespace joynr
{

using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

template <class T>
class CachedValue
{

public:
    CachedValue<T>();
    CachedValue<T>(const CachedValue<T>&) = default;
    CachedValue<T>(T value, TimeStamp timestamp);

    T getValue();
    TimeStamp getTimestamp();

    CachedValue<T>& operator=(const CachedValue<T>&) = default;
    bool operator==(const CachedValue<T>& other) const;
    bool operator!=(const CachedValue<T>& other) const;

private:
    T value;
    TimeStamp timestamp;
};

template <class T>
CachedValue<T>::CachedValue()
        : value(T()), timestamp(TimeStamp::min())
{
}

template <class T>
CachedValue<T>::CachedValue(T value, TimeStamp timestamp)
        : value(value), timestamp(timestamp)
{
}

template <class T>
TimeStamp CachedValue<T>::getTimestamp()
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
    return other.value == value;
}

template <class T>
bool CachedValue<T>::operator!=(const CachedValue<T>& other) const
{
    return !(*this == other);
}

} // namespace joynr
#endif // CACHEDVALUE_H
