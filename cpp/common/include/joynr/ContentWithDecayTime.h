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
#ifndef CONTENTWITHTTL_H
#define CONTENTWITHTTL_H

#include "joynr/ObjectWithDecayTime.h"
#include <QDateTime>

namespace joynr
{

template <class T>
class ContentWithDecayTime : public ObjectWithDecayTime
{
public:
    ContentWithDecayTime(const T& content, QDateTime decayTime)
            : ObjectWithDecayTime(decayTime), content(content)
    {
    }

    T getContent() const
    {
        return content;
    }

private:
    T content;
};

class SendMsgRequest;
typedef ContentWithDecayTime<SendMsgRequest> SendMsgRequestWithDecayTime;

} // namespace joynr
#endif // CONTENTWITHTTL_H
