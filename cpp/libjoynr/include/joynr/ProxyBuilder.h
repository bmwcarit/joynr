/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/IDiscovery.h"
#include "Future.h"
#include <QCoreApplication>
#include "joynr/Semaphore.h"
#include <QList>
#include <string>
#include <stdint.h>
#include <joynr/TypeUtil.h>
#include <cassert>
#include <memory>

namespace joynr
{

class ICapabilities;

/**
 * @brief Class to build a proxy object for the given interface T.
 *
 * Default proxy properties can be overwritten by the set...Qos methods.
 * After calling build the proxy can be used like a local instance of the provider.
 * All invocations will be queued until either the message TTL expires or the
 * arbitration finishes successfully. Synchronous calls will block until the
 * arbitration is done.
 */
template <class T>
class ProxyBuilder : public IArbitrationListener
{
public:
    /**
     * @brief Constructor
     * @param proxyFactory Pointer to proxy factory object
     * @param discoveryProxy Reference to IDiscoverySync object
     * @param domain The provider domain
     * @param dispatcherAddress The address of the dispatcher
     * @param messageRouter A shared pointer to the message router object
     */
    ProxyBuilder(ProxyFactory* proxyFactory,
                 joynr::system::IDiscoverySync& discoveryProxy,
                 const std::string& domain,
                 std::shared_ptr<joynr::system::RoutingTypes::QtAddress> dispatcherAddress,
                 std::shared_ptr<MessageRouter> messageRouter);

    /** Destructor */
    ~ProxyBuilder();

    /**
     * @brief Build the proxy object
     *
     * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
     * @return The proxy object
     */
    T* build();

    /**
     * @brief Sets whether the object is to be cached
     * @param cached True, if the object is to be cached, false otherwise
     * @return The ProxyBuilder object
     */
    ProxyBuilder* setCached(const bool cached);

    /**
     * @brief Sets the messaging qos settings
     * @param messagingQos The message quality of service settings
     * @return The ProxyBuilder object
     */
    ProxyBuilder* setMessagingQos(const MessagingQos& messagingQos);

    /**
     * @brief Sets the discovery qos settings
     * @param discoveryQos The discovery quality of service settings
     * @return The ProxyBuilder object
     */
    ProxyBuilder* setDiscoveryQos(const DiscoveryQos& discoveryQos);

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyBuilder);

    /**
     * @brief Waits a specified time for the arbitration to complete
     *
     * Throws an JoynrArbitrationException if the arbitration is canceled
     * or waits for the time specified in timeout (in milliseconds) for the
     * arbitration to complete.
     *
     * @param timeout The timeout value in milliseconds
     */
    void waitForArbitrationAndCheckStatus(uint16_t timeout);

    /**
     * @brief Waits predefined time for the arbitration to complete until
     *
     *  Calls waitForArbitrationAndCheckStatus(uint16_t timeout) using the
     * one-way time-to-live value predefined in the MessagingQos.
     */
    void waitForArbitrationAndCheckStatus();

    /**
     * @brief Determine the arbitration status
     *
     * setArbitrationStatus is called by the arbitrator when the status changes.
     *
     * @param arbitrationStatus The arbitration status to set
     */
    void setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus);

    /**
     * @brief Sets the participantId
     *
     * If the arbitration finished successfully the arbitrator uses setParticipantId
     * to set the  arbitration result.
     *
     * @param participantId The participant's id
     */
    void setParticipantId(const std::string& participantId);

    /**
     * @brief Sets the kind of connection
     *
     * Sets the end point address.
     *
     * @param connection The kind of connection.
     */
    void setConnection(const joynr::types::CommunicationMiddleware::Enum& connection);

    /*
     * arbitrationFinished is called when the arbitrationStatus is set to successful and the
     * channelId has been set to a non empty string. The implementation differs for
     * synchronous and asynchronous communication.
     */

    /**
     * @brief Wait for arbitration to finish until specified time interval is expired
     *
     * waitForArbitration(uint16_t timeout) is used internally before a remote action is executed to
     * check whether arbitration is already completed.
     *
     * @param timeout specifies the maximal time to wait in milliseconds.
     */
    void waitForArbitration(uint16_t timeout);

    /**
     * @brief Wait for arbitration to finish until predefined time interval is expired
     *
     * waitForArbitration() has the same functionality as waitForArbitration(uint16_t timeout), but
     * uses the one-way time-to-live value predefined in the MessagingQos.
     */
    void waitForArbitration();

    std::string domain;
    bool cached;
    bool hasArbitrationStarted;
    MessagingQos messagingQos;
    ProxyFactory* proxyFactory;
    joynr::system::IDiscoverySync& discoveryProxy;
    ProviderArbitrator* arbitrator;
    joynr::Semaphore arbitrationSemaphore;
    std::string participantId;
    joynr::types::CommunicationMiddleware::Enum connection;
    ArbitrationStatus::ArbitrationStatusType arbitrationStatus;
    qint64 discoveryTimeout;

    std::shared_ptr<joynr::system::RoutingTypes::QtAddress> dispatcherAddress;
    std::shared_ptr<MessageRouter> messageRouter;
};

template <class T>
ProxyBuilder<T>::ProxyBuilder(
        ProxyFactory* proxyFactory,
        joynr::system::IDiscoverySync& discoveryProxy,
        const std::string& domain,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> dispatcherAddress,
        std::shared_ptr<MessageRouter> messageRouter)
        : domain(domain),
          cached(false),
          hasArbitrationStarted(false),
          messagingQos(),
          proxyFactory(proxyFactory),
          discoveryProxy(discoveryProxy),
          arbitrator(NULL),
          arbitrationSemaphore(1),
          participantId(""),
          connection(joynr::types::CommunicationMiddleware::NONE),
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
        // 2. (if std::shared_ptr) delete arbitrator
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
    std::shared_ptr<Future<void>> future(new Future<void>());
    auto onSuccess = [future]() { future->onSuccess(); };
    messageRouter->addNextHop(proxy->getProxyParticipantId(), dispatcherAddress, onSuccess);

    // Wait in the Qt event loop until the result becomes available
    // processEvents() processes all events delivered to this thread
    do {
        try {
            future->wait(100);
        } catch (exceptions::JoynrException& e) {
        }
        QCoreApplication::processEvents();
    } while (future->getStatus().getCode() == RequestStatusCode::IN_PROGRESS);

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
            domain, T::INTERFACE_NAME(), discoveryProxy, discoveryQos);
    arbitrationSemaphore.wait();
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
        if (!participantId.empty() && connection != joynr::types::CommunicationMiddleware::NONE) {
            arbitrationSemaphore.notify();
        } else {
            throw exceptions::DiscoveryException("Arbitration was set to successfull by "
                                                 "arbitrator, but either ParticipantId or "
                                                 "MessagingEndpointAddress were empty");
        }
    } else {
        throw exceptions::DiscoveryException("Arbitration finished without success.");
    }
}

template <class T>
void ProxyBuilder<T>::setConnection(const joynr::types::CommunicationMiddleware::Enum& connection)
{
    this->connection = connection;
}

template <class T>
void ProxyBuilder<T>::setParticipantId(const std::string& participantId)
{
    this->participantId = participantId;
}

template <class T>
void ProxyBuilder<T>::waitForArbitrationAndCheckStatus()
{
    waitForArbitrationAndCheckStatus(discoveryTimeout);
}

template <class T>
void ProxyBuilder<T>::waitForArbitrationAndCheckStatus(uint16_t timeout)
{
    switch (arbitrationStatus) {
    case ArbitrationStatus::ArbitrationSuccessful:
        break;
    case ArbitrationStatus::ArbitrationRunning:
        waitForArbitration(timeout);
        waitForArbitrationAndCheckStatus(0);
        break;
    case ArbitrationStatus::ArbitrationCanceledForever:
        throw exceptions::DiscoveryException(
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
void ProxyBuilder<T>::waitForArbitration(uint16_t timeout)
{
    if (!arbitrationSemaphore.waitFor(std::chrono::milliseconds(timeout))) {
        throw exceptions::DiscoveryException("Arbitration could not be finished in time.");
    }
    arbitrationSemaphore.notify();
}

} // namespace joynr
#endif // PROXYBUILDER_H
