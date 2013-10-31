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
#include "joynr/JoynrMessagingEndpointAddress.h"

namespace joynr {

const QString& JoynrMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE() {
    static QString value("joynr::JoynrMessagingEndpointAddress");
    return value;
}

JoynrMessagingEndpointAddress::JoynrMessagingEndpointAddress() :
    EndpointAddressBase(),
    channelId()
{

}

JoynrMessagingEndpointAddress::JoynrMessagingEndpointAddress(const QString &channelId) :
    EndpointAddressBase(),
    channelId(channelId)
{

}

JoynrMessagingEndpointAddress::JoynrMessagingEndpointAddress(const JoynrMessagingEndpointAddress &other) :
    EndpointAddressBase(other),
    channelId(other.getChannelId())
{
}

JoynrMessagingEndpointAddress::~JoynrMessagingEndpointAddress(){

}

JoynrMessagingEndpointAddress &JoynrMessagingEndpointAddress::operator =(const JoynrMessagingEndpointAddress &other)
{
    EndpointAddressBase::operator =(other);
    channelId = other.getChannelId();
    return *this;
}

bool JoynrMessagingEndpointAddress::operator ==(const JoynrMessagingEndpointAddress &other) const
{
    return EndpointAddressBase::operator ==(other)
            && channelId == other.getChannelId();
}

void JoynrMessagingEndpointAddress::setChannelId(const QString &channelId){
    this->channelId = channelId;
}

QString JoynrMessagingEndpointAddress::getChannelId() const{
    return channelId;
}



} // namespace joynr
