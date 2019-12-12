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
#ifndef CONTENTWITHDECAYTIME_H
#define CONTENTWITHDECAYTIME_H

#include "joynr/ObjectWithDecayTime.h"

namespace joynr
{

template <class T>
class ContentWithDecayTime : public ObjectWithDecayTime
{
public:
    ContentWithDecayTime(const T& content, const TimePoint& decayTime)
            : ObjectWithDecayTime(decayTime), _content(content)
    {
    }

    T getContent() const
    {
        return _content;
    }

private:
    T _content;
};

} // namespace joynr
#endif // CONTENTWITHDECAYTIME_H
