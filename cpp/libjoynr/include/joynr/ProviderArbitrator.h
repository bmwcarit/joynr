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

#ifndef PROVIDERARBITRATOR_H
#define PROVIDERARBITRATOR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/IArbitrationListener.h"
#include "joynr/system/Address.h"
#include "joynr/joynrlogging.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/system/DiscoveryQos.h"

#include <QSharedPointer>
#include <QString>
#include <QSemaphore>


namespace joynr {

namespace system {
    class IDiscoverySync;
}

/*
 *  Base class for different arbitration strategies.
 */
class JOYNR_EXPORT ProviderArbitrator {

public:
    virtual ~ProviderArbitrator();

    /*
     *  Arbitrate until successful or until a timeout occurs
     */
    void startArbitration();

    /*
     *  attemptArbitration() has to be implemented by the concrete arbitration strategy.
     *  This method attempts arbitration and sets arbitrationStatus to indicate the
     *  state of arbitration.
     */
    virtual void attemptArbitration() = 0;

    /*
     *  Returns the result of the arbitration.
     */
    QString getParticipantId();

    joynr::system::CommunicationMiddleware::Enum getConnection();

    /*
     *  setArbitrationCallback expects a callback to a JoynrProviderProxy object which
     *  should be notified as soon as the arbitration is completed.
     */
    void setArbitrationListener(IArbitrationListener* listener);
    void removeArbitationListener();

protected:
    /*
     *  Creates a new ProviderArbitrator object which blocks the arbitration finished
     *  notification as long as no callback onject has been specified.
     *  This blocking is need for example for the fixed channel arbitrator which
     *  sets the channelId instantly.
     */
    ProviderArbitrator(
            const QString& domain,
            const QString& interfaceName,
            joynr::system::IDiscoverySync& discoveryProxy,
            const DiscoveryQos& discoveryQos
    );
    /*
     *  setArbitrationStatus uses the callback to notify the ProviderProxy about the arbitration results.
     */
    void updateArbitrationStatusParticipantIdAndAddress(
            ArbitrationStatus::ArbitrationStatusType arbitrationStatus,
            QString participantId,
            const joynr::system::CommunicationMiddleware::Enum& connection
    );
    /**
     * @brief selectPreferredCommunicationMiddleware Selects the preferred communication middleware
     * from a list of available connections.
     *
     * @param connections List of available connections.
     * @return The preferred connection.
     */
    virtual joynr::system::CommunicationMiddleware::Enum selectPreferredCommunicationMiddleware(
            const QList<joynr::system::CommunicationMiddleware::Enum>& connections
    );
    joynr::system::IDiscoverySync& discoveryProxy;
    DiscoveryQos discoveryQos;
    joynr::system::DiscoveryQos systemDiscoveryQos;
    QString domain;
    QString interfaceName;


private:
    DISALLOW_COPY_AND_ASSIGN(ProviderArbitrator);
    void setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus);
    void setParticipantId(QString participantId);
    void setConnection(const joynr::system::CommunicationMiddleware::Enum& connection);
    QString participantId;
    joynr::system::CommunicationMiddleware::Enum connection;
    ArbitrationStatus::ArbitrationStatusType arbitrationStatus;
    IArbitrationListener* listener;
    QSemaphore listenerSemaphore;
    static joynr_logging::Logger* logger;
};


} // namespace joynr
#endif //PROVIDERARBITRATOR_H
