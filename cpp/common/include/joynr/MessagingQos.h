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

#ifndef MESSAGINGQOS_H
#define MESSAGINGQOS_H

#include "joynr/JoynrCommonExport.h"

#include <QtGlobal>
#include <QObject>
#include <QVariantMap>
#include <QDateTime>


namespace joynr {

/**
  * Data Class that stores QoS Settings like Ttl
  */
class JOYNRCOMMON_EXPORT MessagingQos : public QObject
{
Q_OBJECT

    Q_PROPERTY(qint64 ttl READ getTtl WRITE setTtl)

public:
    MessagingQos(const MessagingQos& other);
    MessagingQos(qint64 ttl = 60000);

    qint64 getTtl() const;
    void setTtl(const qint64& ttl);

    MessagingQos& operator=(const MessagingQos& other);
    bool operator==(const MessagingQos& other) const;

private:
    qint64 ttl;
};

// printing MessagingQos with google-test and google-mock
void PrintTo(const joynr::MessagingQos& value, ::std::ostream* os);

} // namespace joynr

Q_DECLARE_METATYPE(joynr::MessagingQos)

#endif // MESSAGINGQOS_H
