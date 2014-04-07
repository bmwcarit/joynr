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
#ifndef CAPABILITIESAGGREGATOR_H
#define CAPABILITIESAGGREGATOR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/ICapabilities.h"
#include "joynr/ILocalCapabilitiesCallback.h"

namespace joynr {

class CapabilityEntry;


/**
 * @brief CapabilitiesAggregator handles async calls to the
 * localCapabilitiesDirectory via sync-only middlewares and
 * adds in-process endpoint addresses to incoming CapabilityEntries
 * when ever a requestCaller is registered locally for the given participantId
 */
class JOYNR_EXPORT CapabilitiesAggregator : public ICapabilities {
public:
/**
 * @brief
 *
 * @param capabilitiesStub Pointer to the capabilitiesStub to communicate with the cluster-controller.
 * @param requestCallerDirectory Pointer to the object implementing the IRequestCallerDirectory interface.
 */
    CapabilitiesAggregator(ICapabilities* capabilitiesStub, IRequestCallerDirectory* requestCallerDirectory);
    /**
     * @brief
     *
     */
    ~CapabilitiesAggregator(){ }
    /**
     * @brief Synchronous lookup method for lookups by domain and interfaceName which will add inProcess addresses to the
     * results recieved from the cluster-controller
     *
     * @param domain
     * @param interfaceName
     * @param qos
     * @param discoveryQos
     * @return QList<CapabilityEntry>
     */
    QList<CapabilityEntry> lookup(const QString& domain,
                                  const QString& interfaceName,
                                  const DiscoveryQos& discoveryQos);

    /**
     * @brief Synchronous lookup method for lookups by participantId which will add inProcess addresses to the
     * results recieved from the cluster-controller
     *
     * @param participantId
     * @param discoveryQos
     * @return QList<CapabilityEntry>
     */
    QList<CapabilityEntry> lookup(const QString& participantId,
                                  const DiscoveryQos& discoveryQos);

    /**
     * @brief Async lookup method for lookups by domain/interfaceName.
     * A thread pool is used to handle the sync call on the capabilities interface.
     * Inprocess addresses are added to the results when available.
     *
     * @param domain
     * @param interfaceName
     * @param qos
     * @param discoveryQos
     * @param callback
     */
    void lookup(const QString& domain,
                const QString& interfaceName,
                const DiscoveryQos& discoveryQos,
                QSharedPointer<ILocalCapabilitiesCallback> callback);

    /**
     * @brief
     *
     * @param participantId
     * @param discoveryQos
     * @param callback
     */
    void lookup(const QString& participantId,
                const DiscoveryQos& discoveryQos,
                QSharedPointer<ILocalCapabilitiesCallback> callback);


    /**
     * @brief
     *
     * @param participantId
     * @param messagingStubAddress
     */
    void addEndpoint(const QString& participantId,
                     QSharedPointer<joynr::system::Address> messagingStubAddress,
                     const qint64& timeout_ms);

    /**
     * @brief
     *
     * @param domain
     * @param interfaceName
     * @param participantId
     * @param qos
     * @param endpointAddressList
     * @param messagingStubAddress
     */
    void add(const QString& domain,
             const QString& interfaceName,
             const QString& participantId,
             const types::ProviderQos& qos,
             QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
             QSharedPointer<joynr::system::Address> messagingStubAddress,
             const qint64& timeout_ms);

    /**
     * @brief
     *
     * @param participantId
     */
    void remove(const QString& participantId, const qint64& timeout_ms);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesAggregator); /**< TODO */

    /**
     * @brief
     *
     * @param entryList
     */
    QList<CapabilityEntry> checkForInprocessParticiants(QList<CapabilityEntry>& entryList);

    ICapabilities* capabilitiesStub; /**< TODO */
    IRequestCallerDirectory* requestCallerDirectory; /**< TODO */
    // NOTE: This thread pool is used to implement the async API of the capablities aggregator,
    // since currently the middleware to communicate with the local capabilities directory only
    // support synchronous method calls.
    QThreadPool threadPool; //will the Aggregator have its own threadpool or a shared threadpool? /**< TODO */
};


} // namespace joynr
#endif //CAPABILITIESAGGREGATOR_H
