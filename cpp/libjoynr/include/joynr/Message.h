/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

namespace joynr
{

class Message
{
public:
    Message() = delete;

    static const std::string& CUSTOM_HEADER_PREFIX()
    {
        static const std::string value("custom-");
        return value;
    }

    static const std::string& HEADER_ID()
    {
        static const std::string value("id");
        return value;
    }

    static const std::string& HEADER_TYPE()
    {
        static const std::string value("type");
        return value;
    }

    static const std::string& HEADER_CREATOR()
    {
        static const std::string value("creator");
        return value;
    }

    static const std::string& HEADER_REPLY_TO()
    {
        static const std::string value("replyTo");
        return value;
    }

    static const std::string& HEADER_EFFORT()
    {
        static const std::string value("effort");
        return value;
    }

    static const std::string& CUSTOM_HEADER_REQUEST_REPLY_ID()
    {
        static const std::string value("z4");
        return value;
    }
    static const std::string& VALUE_MESSAGE_TYPE_ONE_WAY()
    {
        static const std::string value("oneWay");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_REPLY()
    {
        static const std::string value("reply");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_REQUEST()
    {
        static const std::string value("request");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_PUBLICATION()
    {
        static const std::string value("subscriptionPublication");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_MULTICAST()
    {
        static const std::string value("multicast");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY()
    {
        static const std::string value("subscriptionReply");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST()
    {
        static const std::string value("subscriptionRequest");
        return value;
    }
    static const std::string& VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST()
    {
        static const std::string value("multicastSubscriptionRequest");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST()
    {
        static const std::string value("broadcastSubscriptionRequest");
        return value;
    }

    static const std::string& VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP()
    {
        static const std::string value("subscriptionStop");
        return value;
    }
};

} // namespace joynr

#endif // MESSAGE_H
