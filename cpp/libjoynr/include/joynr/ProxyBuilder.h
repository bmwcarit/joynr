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
#ifndef PROXYBUILDER_H
#define PROXYBUILDER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/MessagingQos.h"
#include "joynr/ProxyFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/ArbitrationStatus.h"
#include "joynr/IArbitrationListener.h"
#include "joynr/ProviderArbitrator.h"
#include "joynr/ProviderArbitratorFactory.h"
#include "joynr/MessageRouter.h"
#include "joynr/exceptions.h"
#include "joynr/system/IDiscovery.h"
#include "Future.h"
#include <QSemaphore>
#include <QList>
#include <cassert>

namespace joynr
{

class ICapabilities;

template <class T>
class ProxyBuilder : public IArbitrationListener
{
public:
    ProxyBuilder(ProxyFactory* proxyFactory,
                 joynr::system::IDiscoverySync& discoveryProxy,
                 const QString& domain,
                 QSharedPointer<joynr::system::Address> dispatcherAddress,
                 QSharedPointer<MessageRouter> messageRouter);

    ~ProxyBuilder();
    /*
      * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
      */

    T* build();

    ProxyBuilder* setCached(const bool cached);
    ProxyBuilder* setMessagingQos(const MessagingQos& messagingQos);
    ProxyBuilder* setDiscoveryQos(const DiscoveryQos& discoveryQos);

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyBuilder);

    /*
     * Throws an JoynrArbitrationException if the arbitration is cancled
     * or waits for the time specified in timeout (in milliseconds) for the
     * arbitration to complete.
     */
    void waitForArbitrationAndCheckStatus(int timeout);

    /*
     *  Calls waitForArbitrationAndCheckStatus(int timeout) using the
     *  one-way time-to-live value predefined in the MessagingQos.
     */
    void waitForArbitrationAndCheckStatus();

    /*
     *  setArbitrationStatus is called by the arbitrator when the status changes.
     *
     */
    void setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus);

    /*
     *  If the arbitration finished successfully the arbitrator uses setChannelId to set the
     * arbitration result.
     */
    void setParticipantId(const QString& participantId);

    /*
     * Sets the end point address.
     */
    void setConnection(const joynr::system::CommunicationMiddleware::Enum& connection);

    /*
      * arbitrationFinished is called when the arbitrationStatus is set to successful and the
      * channelId has been set to a non empty string. The implementation differs for
      * synchronous and asynchronous communication.
      */

    /*
     *  waitForArbitration(int timeout) is used internally before a remote action is executed to
     * check
     *  whether arbitration is already completed.
     *  timeout specifies the maximal time to wait in milliseconds.
     */
    void waitForArbitration(int timeout);
    /*
     *  waitForArbitration() has the same functionality as waitForArbitration(int timeout), but
     *  uses the one-way time-to-live value predefined in the MessagingQos.
     */
    void waitForArbitration();

    QString domain;
    bool cached;
    bool hasArbitrationStarted;
    MessagingQos messagingQos;
    ProxyFactory* proxyFactory;
    joynr::system::IDiscoverySync& discoveryProxy;
    ProviderArbitrator* arbitrator;
    QSemaphore arbitrationSemaphore;
    QString participantId;
    joynr::system::CommunicationMiddleware::Enum connection;
    ArbitrationStatus::ArbitrationStatusType arbitrationStatus;
    qint64 discoveryTimeout;

    QSharedPointer<joynr::system::Address> dispatcherAddress;
    QSharedPointer<MessageRouter> messageRouter;
};

template <class T>
ProxyBuilder<T>::ProxyBuilder(ProxyFactory* proxyFactory,
                              joynr::system::IDiscoverySync& discoveryProxy,
                              const QString& domain,
                              QSharedPointer<joynr::system::Address> dispatcherAddress,
                              QSharedPointer<MessageRouter> messageRouter)
        : domain(domain),
          cached(false),
          hasArbitrationStarted(false),
          messagingQos(),
          proxyFactory(proxyFactory),
          discoveryProxy(discoveryProxy),
          arbitrator(NULL),
          arbitrationSemaphore(1),
          participantId(""),
          connection(joynr::system::CommunicationMiddleware::NONE),
          arbitrationStatus(ArbitrationStatus::ArbitrationRunning),
          discoveryTimeout(-1),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter)
{
}

template <class T>
ProxyBuilder<T>::~ProxyBuilder()
{
    if (arbitrator != NULL) {
        arbitrator->removeArbitationListener();
        // question: it is only safe to delete the arbitrator here, if the proxybuilder will not be
        // deleted
        // before all arbitrations are finished.
        delete arbitrator;
        arbitrator = NULL;
        // TODO delete arbitrator
        // 1. delete arbitrator or
        // 2. (if qsharedpointer) delete arbitrator
    }
}

template <class T>
T* ProxyBuilder<T>::build()
{
    T* proxy = proxyFactory->createProxy<T>(domain, messagingQos, cached);
    waitForArbitration(discoveryTimeout);
    proxy->handleArbitrationFinished(participantId, connection);
    // add next hop to dispatcher

    /*
     * synchronously wait until the proxy participantId is registered in the
     * routing table(s)
    */
    QSharedPointer<Future<void>> future(new Future<void>());
    auto callbackFct = [future](const joynr::RequestStatus& status) { future->onSuccess(status); };
    messageRouter->addNextHop(proxy->getProxyParticipantId(), dispatcherAddress, callbackFct);

    future->waitForFinished();
    return proxy;
}

template <class T>
ProxyBuilder<T>* ProxyBuilder<T>::setCached(const bool cached)
{
    this->cached = cached;
    return this;
}

template <class T>
ProxyBuilder<T>* ProxyBuilder<T>::setMessagingQos(const MessagingQos& messagingQos)
{
    this->messagingQos = messagingQos;
    return this;
}

template <class T>
/* Sets the arbitration Qos and starts arbitration. This way arbitration will be started, before the
   ->build() is called on the proxy Builder.
   All parameter that are needed for arbitration should be set, before setDiscoveryQos is called.
*/
ProxyBuilder<T>* ProxyBuilder<T>::setDiscoveryQos(const DiscoveryQos& discoveryQos)
{
    // if DiscoveryQos is set, arbitration will be started. It shall be avoided that the
    // setDiscoveryQos method can be called twice
    assert(!hasArbitrationStarted);
    discoveryTimeout = discoveryQos.getDiscoveryTimeout();
    arbitrator = ProviderArbitratorFactory::createArbitrator(
            domain, T::getInterfaceName(), discoveryProxy, discoveryQos);
    arbitrationSemaphore.acquire();
    arbitrator->setArbitrationListener(this);
    arbitrator->startArbitration();
    hasArbitrationStarted = true;
    return this;
}

template <class T>
void ProxyBuilder<T>::setArbitrationStatus(
        ArbitrationStatus::ArbitrationStatusType arbitrationStatus)
{
    this->arbitrationStatus = arbitrationStatus;
    if (arbitrationStatus == ArbitrationStatus::ArbitrationSuccessful) {
        if (!participantId.isEmpty() &&
            connection != joynr::system::CommunicationMiddleware::NONE) {
            arbitrationSemaphore.release();
        } else {
            throw JoynrArbitrationFailedException("Arbitration was set to successfull by "
                                                  "arbitrator, but either ParticipantId or "
                                                  "MessagingEndpointAddress where empty");
        }
    } else {
        throw JoynrArbitrationFailedException("Arbitration finished unsucessfully.");
    }
}

template <class T>
void ProxyBuilder<T>::setConnection(const joynr::system::CommunicationMiddleware::Enum& connection)
{
    this->connection = connection;
}

template <class T>
void ProxyBuilder<T>::setParticipantId(const QString& participantId)
{
    this->participantId = participantId;
}

template <class T>
void ProxyBuilder<T>::waitForArbitrationAndCheckStatus()
{
    waitForArbitrationAndCheckStatus(discoveryTimeout);
}

template <class T>
void ProxyBuilder<T>::waitForArbitrationAndCheckStatus(int timeout)
{
    switch (arbitrationStatus) {
    case ArbitrationStatus::ArbitrationSuccessful:
        break;
    case ArbitrationStatus::ArbitrationRunning:
        waitForArbitration(timeout);
        waitForArbitrationAndCheckStatus(0);
        break;
    case ArbitrationStatus::ArbitrationCanceledForever:
        throw JoynrArbitrationFailedException(
                "Arbitration for this interface has not been successful.");
        break;
    }
}

template <class T>
void ProxyBuilder<T>::waitForArbitration()
{
    waitForArbitration(discoveryTimeout);
}

template <class T>
void ProxyBuilder<T>::waitForArbitration(int timeout)
{
    if (!arbitrationSemaphore.tryAcquire(1, timeout)) {
        throw JoynrArbitrationTimeOutException("Arbitration could not be finished in time.");
    }
    arbitrationSemaphore.release();
}

} // namespace joynr
#endif // PROXYBUILDER_H
