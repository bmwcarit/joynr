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
#ifndef MESSAGINGPROPERTIESPERSISTENCE_H
#define MESSAGINGPROPERTIESPERSISTENCE_H

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/PrivateCopyAssign.h"
#include <string>

namespace joynr
{

/**
 * Persists messaging properties
 */
class JOYNRCLUSTERCONTROLLER_EXPORT MessagingPropertiesPersistence
{
public:
    /**
     * Persist message properties to the default persistence file
     */
    explicit MessagingPropertiesPersistence(const std::string& filename);

    /**
     * Get, and create if needed, the channel Id
     */
    std::string getChannelId();

    /**
     * Get, and create if needed, the receiver Id
     */
    std::string getReceiverId();

    /**
     * Get the key in the settings file that identifies the channel id
     */
    static const std::string& CHANNEL_ID_KEY();

    /**
     * Get the key in the settings file that identifies the receiver id
     */
    static const std::string& RECEIVER_ID_KEY();

private:
    DISALLOW_COPY_AND_ASSIGN(MessagingPropertiesPersistence);
    std::string _filename;

    std::string getIdFromPersistence(const std::string& key);
};

} // namespace joynr
#endif // MESSAGINGPROPERTIESPERSISTENCE_H
