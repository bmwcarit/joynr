/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#include <QDateTime>

namespace joynr {


template <class T>
class ContentWithTtl
{
public:
    ContentWithTtl(T& content, qint64 ttl_ms):
            initialTtl_ms(ttl_ms),
            content(content)
    {
        entryTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }

    ContentWithTtl():
            content(NULL)
    {
    }

    const T getContent() const {
        return content;
    }
    qint64 getRemainingTtl_ms() const {
        return initialTtl_ms - (QDateTime::currentDateTime().toMSecsSinceEpoch() - entryTime);
    }

    qint64 getEntryTime() const {
        return entryTime;
    }

    bool isExpired() const {
        return QDateTime::currentDateTime().toMSecsSinceEpoch() - entryTime > initialTtl_ms;
    }

private:

    qint64 entryTime;
    qint64 initialTtl_ms;
    T content;
};


class Message;
typedef  ContentWithTtl<QSharedPointer<const QByteArray> > SerializedMessageWithTtl;



} // namespace joynr
#endif // CONTENTWITHTTL_H



