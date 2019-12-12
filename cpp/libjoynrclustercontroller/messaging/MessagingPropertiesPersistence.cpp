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
#include "joynr/Settings.h"
#include "joynr/Util.h"

#include "MessagingPropertiesPersistence.h"

namespace joynr
{

MessagingPropertiesPersistence::MessagingPropertiesPersistence(const std::string& filename)
        : _filename(filename)
{
}

std::string MessagingPropertiesPersistence::getIdFromPersistence(const std::string& key)
{
    // Read and write to the persistence file using a Settings object
    Settings settings(_filename);

    std::string id;

    // Get the persisted specified id if one exists
    if (settings.contains(key)) {
        id = settings.get<std::string>(key);
    } else {
        // Create a new persisted id for key
        id = util::createUuid();
        settings.set(key, id);
        settings.sync();
    }

    return id;
}

std::string MessagingPropertiesPersistence::getChannelId()
{
    return getIdFromPersistence(CHANNEL_ID_KEY());
}

std::string MessagingPropertiesPersistence::getReceiverId()
{
    return getIdFromPersistence(RECEIVER_ID_KEY());
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
