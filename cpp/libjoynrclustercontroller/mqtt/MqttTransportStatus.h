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
#ifndef MQTTTRANSPORTSTATUS_H
#define MQTTTRANSPORTSTATUS_H

#include <functional>
#include <memory>
#include <string>

#include <joynr/ITransportStatus.h>
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{
class MosquittoConnection;

class MqttTransportStatus : public ITransportStatus
{
public:
    explicit MqttTransportStatus(std::shared_ptr<MosquittoConnection> mosquittoConnection,
                                 const std::string& gbid);
    ~MqttTransportStatus() override;

    bool isReponsibleFor(std::shared_ptr<const joynr::system::RoutingTypes::Address>) override;
    bool isAvailable() override;

    void setAvailabilityChangedCallback(
            std::function<void(bool)> availabilityChangedCallback) override;

private:
    DISALLOW_COPY_AND_ASSIGN(MqttTransportStatus);

    std::shared_ptr<MosquittoConnection> _mosquittoConnection;
    std::string _gbid;
    std::function<void(bool)> _availabilityChangedCallback;
};

} // namespace joynr

#endif // MQTTSENDER_H
