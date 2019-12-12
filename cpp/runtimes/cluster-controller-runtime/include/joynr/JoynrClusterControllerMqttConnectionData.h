/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

#include <memory>

#ifndef JOYNRCLUSTERCONTROLLERMQTTCONNECTIONDATA_H
#define JOYNRCLUSTERCONTROLLERMQTTCONNECTIONDATA_H

namespace joynr
{

class ITransportMessageReceiver;
class ITransportMessageSender;
class MosquittoConnection;

class JoynrClusterControllerMqttConnectionData
{
public:
    JoynrClusterControllerMqttConnectionData();

    virtual ~JoynrClusterControllerMqttConnectionData();

    virtual std::shared_ptr<MosquittoConnection> getMosquittoConnection() const;
    virtual void setMosquittoConnection(const std::shared_ptr<MosquittoConnection>& value);

    virtual std::shared_ptr<ITransportMessageReceiver> getMqttMessageReceiver() const;
    virtual void setMqttMessageReceiver(const std::shared_ptr<ITransportMessageReceiver>& value);

    virtual std::shared_ptr<ITransportMessageSender> getMqttMessageSender() const;
    virtual void setMqttMessageSender(const std::shared_ptr<ITransportMessageSender>& value);

private:
    std::shared_ptr<MosquittoConnection> mosquittoConnection;
    std::shared_ptr<ITransportMessageReceiver> mqttMessageReceiver;
    std::shared_ptr<ITransportMessageSender> mqttMessageSender;
};

} // namespace joynr
#endif // JOYNRCLUSTERCONTROLLERMQTTCONNECTIONDATA_H
