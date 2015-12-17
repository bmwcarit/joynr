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
#ifndef IARBITRATIONLISTENER_H
#define IARBITRATIONLISTENER_H

#include "joynr/ArbitrationStatus.h"
#include "joynr/types/CommunicationMiddleware.h"

#include <string>

namespace joynr
{

/**
 * @brief Class interface used by ProviderArbitrator
 *
 * IArbitrationListener is an interface used by ProviderArbitrator
 * to notify the AbstractProxy about a successful completion
 * of the arbitration process.
 */
class IArbitrationListener
{
public:
    /** @brief Destructor */
    virtual ~IArbitrationListener() = default;

    /**
     * @brief Set the status of the arbitration
     * @param arbitrationStatus The status of the arbitration
     */
    virtual void setArbitrationStatus(
            ArbitrationStatus::ArbitrationStatusType arbitrationStatus) = 0;

    /**
     * @brief Set the participant id
     * @param participantId The id of the participant
     */
    virtual void setParticipantId(const std::string& participantId) = 0;

    /**
     * @brief Set the connection middleware used
     * @param connection The connection middleware used
     */
    virtual void setConnection(const joynr::types::CommunicationMiddleware::Enum& connection) = 0;
};

} // namespace joynr
#endif // IARBITRATIONLISTENER_H
