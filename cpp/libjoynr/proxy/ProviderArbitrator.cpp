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
#include "joynr/ProviderArbitrator.h"
#include "joynr/exceptions.h"
#include "joynr/joynrlogging.h"
#include "joynr/ICapabilities.h"

#include <cassert>

namespace joynr {

joynr_logging::Logger* ProviderArbitrator::logger = joynr_logging::Logging::getInstance()->getLogger("Arb", "ProviderArbitrator");

ProviderArbitrator::ProviderArbitrator(const QString& domain, const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub, const DiscoveryQos& discoveryQos)
    : capabilitiesStub(capabilitiesStub),
      discoveryQos(discoveryQos),
      domain(domain),
      interfaceName(interfaceName),
      participantId(""),
      endpointAddress(NULL),
      arbitrationStatus(ArbitrationStatus::ArbitrationRunning),
      listener(NULL),
      listenerSemaphore(0)
{
}


void ProviderArbitrator::startArbitration(){
    QTime timer;
    QSemaphore semaphore;

    // Arbitrate until successful or timed out
    while (true) {

        timer.start();

        // Attempt arbitration (overloaded in subclasses)
        attemptArbitration();

        // Finish on success or failure
        if (arbitrationStatus != ArbitrationStatus::ArbitrationRunning) return;

        // Reduce the timeout by the elapsed time
        discoveryQos.setDiscoveryTimeout(discoveryQos.getDiscoveryTimeout() - timer.restart());
        if (discoveryQos.getDiscoveryTimeout() <= 0) break;

        // If there are no suitable providers, wait for a second before trying again
        semaphore.tryAcquire(1, discoveryQos.getRetryInterval());

        // Reduce the timeout again
        discoveryQos.setDiscoveryTimeout(discoveryQos.getDiscoveryTimeout() - timer.elapsed());
        if (discoveryQos.getDiscoveryTimeout() <= 0) break;
    }

    // If this point is reached the arbitration timed out
    updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationCanceledForever, "", QSharedPointer<EndpointAddressBase>());
}

QString ProviderArbitrator::getParticipantId(){
    if(participantId.isEmpty()){
        throw JoynrArbitrationException("ParticipantId is empty: Called getParticipantId() before arbitration has finished / Arbitrator did not set participantId.");
    }
    return participantId;
}

void ProviderArbitrator::setParticipantId(QString participantId){
    this->participantId = participantId;
    if(listenerSemaphore.tryAcquire(1)){
        assert(listener!=NULL);
        listener->setParticipantId(participantId);
        listenerSemaphore.release(1);
    }
}

QSharedPointer<EndpointAddressBase> ProviderArbitrator::getEndpointAddress(){
    if(endpointAddress.isNull()){
        throw JoynrArbitrationException("EndpointAddress is NULL: Called getEndpointAddress() before arbitration has finished / Arbitrator did not set endpointAddress.");
    }
    return endpointAddress;
}

void ProviderArbitrator::setEndpointAddress(QSharedPointer<EndpointAddressBase> endpointAddress){
    this->endpointAddress = endpointAddress;
    if(listenerSemaphore.tryAcquire(1)){
        assert(listener!=NULL);
        listener->setEndpointAddress(endpointAddress);
        listenerSemaphore.release(1);
    }
}

void ProviderArbitrator::updateArbitrationStatusParticipantIdAndAddress(ArbitrationStatus::ArbitrationStatusType arbitrationStatus, QString participantId, QSharedPointer<EndpointAddressBase> endpointAddress){
    setParticipantId(participantId);
    setEndpointAddress(endpointAddress);
    setArbitrationStatus(arbitrationStatus);
}


void ProviderArbitrator::setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus){
    this->arbitrationStatus = arbitrationStatus;
    if(listenerSemaphore.tryAcquire(1)){
        try {
            assert(listener!=NULL);
            listener->setArbitrationStatus(arbitrationStatus);
        } catch (std::exception& e) { //TODO replace by expected exception type
            LOG_ERROR(logger, "Exception while setting arbitration status: " + QString::fromStdString(e.what()));
            listenerSemaphore.release(1);
            throw;
        }
        listenerSemaphore.release(1);
    }
}

void ProviderArbitrator::removeArbitationListener(){
    if (listener!=NULL){
        this->listener = NULL;
        listenerSemaphore.acquire(1);
    }
}

void ProviderArbitrator::setArbitrationListener(IArbitrationListener *listener){
    this->listener = listener;
    listenerSemaphore.release(1);
    if (arbitrationStatus == ArbitrationStatus::ArbitrationSuccessful){
        listener->setParticipantId(participantId);
        listener->setEndpointAddress(endpointAddress);
        listener->setArbitrationStatus(arbitrationStatus);
    }
}




} // namespace joynr
