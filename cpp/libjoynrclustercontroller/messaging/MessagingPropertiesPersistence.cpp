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
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/Util.h"
#include "joynr/Settings.h"

namespace joynr
{

MessagingPropertiesPersistence::MessagingPropertiesPersistence(const std::string& filename)
        : filename(filename)
{
}

std::string MessagingPropertiesPersistence::getChannelId()
{
    // Read and write to the persistence file using a Settings object
    Settings settings(filename);

    std::string channelId;

    // Get the persisted channel id if one exists
    if (settings.contains(CHANNEL_ID_KEY())) {
        channelId = settings.get<std::string>(CHANNEL_ID_KEY());
    } else {
        // Create and persist a channelId
        channelId = util::createUuid();
        settings.set(CHANNEL_ID_KEY(), channelId);
        settings.sync();
    }

    return channelId;
}

std::string MessagingPropertiesPersistence::getReceiverId()
{
    // Read and write to the persistence file using a Settings object
    Settings settings(filename);

    std::string receiverId;

    // Get the persisted receiver id if one exists
    if (settings.contains(RECEIVER_ID_KEY())) {
        receiverId = settings.get<std::string>(RECEIVER_ID_KEY());
    } else {
        // Create and persist a receiverId
        receiverId = util::createUuid();
        settings.set(RECEIVER_ID_KEY(), receiverId);
        settings.sync();
    }

    return receiverId;
}

const std::string& MessagingPropertiesPersistence::CHANNEL_ID_KEY()
{
    static const std::string value("messaging/channelId");
    return value;
}

const std::string& MessagingPropertiesPersistence::RECEIVER_ID_KEY()
{
    static const std::string value("messaging/receiverId");
    return value;
}

} // namespace joynr
