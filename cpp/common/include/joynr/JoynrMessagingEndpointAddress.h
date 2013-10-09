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
#ifndef JOYNRMESSAGINGENDPOINTADDRESS_H
#define JOYNRMESSAGINGENDPOINTADDRESS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/EndpointAddressBase.h"

#include <QMetaType>
#include <QString>

namespace joynr {

class JOYNRCOMMON_EXPORT JoynrMessagingEndpointAddress : public EndpointAddressBase {
    Q_OBJECT

    Q_PROPERTY(QString channelId READ getChannelId WRITE setChannelId)

public:
    static const QString& ENDPOINT_ADDRESS_TYPE();

    JoynrMessagingEndpointAddress();
    JoynrMessagingEndpointAddress(const QString &channelId);
    JoynrMessagingEndpointAddress(const JoynrMessagingEndpointAddress& other);
    virtual ~JoynrMessagingEndpointAddress();

    virtual JoynrMessagingEndpointAddress& operator =(const JoynrMessagingEndpointAddress& other);
    virtual bool operator ==(const JoynrMessagingEndpointAddress& other) const;

    QString getChannelId() const;

    void setChannelId(const QString &channelId);
private:
    QString channelId;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::JoynrMessagingEndpointAddress)

#endif //JOYNRMESSAGINGENDPOINTADDRESS_H
