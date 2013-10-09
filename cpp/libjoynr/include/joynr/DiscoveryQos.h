/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#ifndef DISCOVERYQOS_H
#define DISCOVERYQOS_H

#include "joynr/JoynrExport.h"

#include <QtGlobal>
#include <QVariant>
#include <QVariantMap>

#include "joynr/types/CustomParameter.h"

namespace joynr {

class JOYNR_EXPORT DiscoveryQos {
public:

    /**
     * Instantiates an DiscoveryQos object with default values.
     */
    DiscoveryQos();
    explicit DiscoveryQos(const qint64& cacheMaxAge);
    virtual ~DiscoveryQos(){}

    /*
     *  List of available arbitration strategies.
     *  The strategy specifies which type of Arbitrator will be
     *  created by the ProviderArbitratorFactory
     */
    enum class ArbitrationStrategy {
        NOT_SET = 0,
        FIXED_CHANNEL = 1,
        LOCAL_ONLY = 2,
        KEYWORD = 3,
        HIGHEST_PRIORITY = 4
    };

    enum class DiscoveryScope {
        LOCAL_ONLY = 0,
        LOCAL_THEN_GLOBAL = 1,
        LOCAL_AND_GLOBAL = 2,
        GLOBAL_ONLY = 3
    };

    static qint64& DEFAULT_DISCOVERYTIMEOUT();

    static qint64& NO_TIMEOUT();

    static ArbitrationStrategy& DEFAULT_ARBITRATIONSTRATEGY();

    static qint64& DEFAULT_CACHEMAXAGE();

    static qint64& DO_NOT_USE_CACHE();

    static DiscoveryScope& DEFAULT_DISCOVERYSCOPE();

    static qint64& DEFAULT_RETRYINTERVAL();

    /**
     * The discovery process outputs a list of matching providers. The arbitration strategy then
     * chooses one or more of them to be used by the proxy.
     *
     * @param arbitrationStrategy
     *            Defines the strategy used to choose the "best" provider.
     */
    void setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy);

    /**
     * The discovery process outputs a list of matching providers. The arbitration strategy then
     * chooses one or more of them to be used by the proxy.
     *
     * @return the arbitration strategy used to pick the "best" provider of the list of matching providers
     */
    ArbitrationStrategy getArbitrationStrategy() const ;

    /**
     * As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
     * is triggered. If the discovery process does not find matching providers within the
     * discovery timeout duration it will be terminated and you will get an discovery exception.
     *
     * @param discoveryTimeout
     *            Sets the amount of time the arbitrator keeps trying to find a suitable provider. The arbitration
     *            lookup might happen multiple times during this time span.
     */
    void setDiscoveryTimeout(qint64 discoveryTimeout);
    /**
     * As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
     * is triggered. If the discovery process does not find matching providers within the
     * discovery timeout duration it will be terminated and you will get an discovery exception.
     *
     * @return the duration used to discover matching providers
     */
    qint64 getDiscoveryTimeout() const;

    /**
     * addCustomParameter allows to add special parameters to the DiscoveryQos which will be used only by some strategies.
     *
     * @param name
     *            String to identify the arbitration parameter
     * @param value
     *            Any String used by the arbitrator to choose a provider.
     */
    void addCustomParameter(QString name, QString value);

    /**
     * getCustomParameter returns the parameters previously specified by addParameter
     *
     * @return Returns the value to which the specified key is mapped, or null if the map of additional parameters
     *         contains no mapping for the key
     */
    types::CustomParameter getCustomParameter(QString name) const;

    /**
     * get the map of custom parameters
     * @return
     *          The map of the current set custom parameters
     */
    QMap<QString, types::CustomParameter> getCustomParameters() const;

    /**
     * Provider entries in the global capabilities directory are cached locally. Discovery will
     * consider entries in this cache valid if they are younger as the max age of cached
     * providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
     * for and arbitrating the "best" matching provider.
     * <p>NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
     *       directory. Therefore, not all providers registered with the global capabilities
     *       directory might be taken into account during arbitration.
     *
     * @return the maximum age of locally cached provider entries to be used during discovery and arbitration
     */
    qint64 getCacheMaxAge() const;

    /**
     * Provider entries in the global capabilities directory are cached locally. Discovery will
     * consider entries in this cache valid if they are younger as the max age of cached
     * providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
     * for and arbitrating the "best" matching provider.
     * <p>NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
     *       directory. Therefore, not all providers registered with the global capabilities
     *       directory might be taken into account during arbitration.
     *
     * @param maxAgeOfCachedProviders
     *            Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the
     *            local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     */
    void setCacheMaxAge(const qint64& cacheMaxAge);

    /**
     * Indicates if the arbitration will only consider providers that support onChange subscriptions
     *
     * @return true if only providers that support onChange subscriptions are considered
     */
    bool getProviderMustSupportOnChange () const;
    /**
     * Indicate if the arbitration should only consider providers that support onChange subscriptions
     *
     * @param providerMustSupportOnChange  true if only providers that support onChange subscriptions should be considered
     */
    void setProviderMustSupportOnChange(bool providerMustSupportOnChange );

    /**
     * The scope determines where the discovery process will look for matching providers, if LOCAL_ONLY,
     * only local providers will be considered. LOCAL_THEN_GLOBAL considers both the local providers
     * and the global providers in its search results. GLOBAL_ONLY only considers providers that are
     * flagged as global.
     *
     * @return the current set discovery scope
     */
    DiscoveryScope getDiscoveryScope() const;

    /**
     * The scope determines where the discovery process will look for matching providers, if LOCAL_ONLY,
     * only local providers will be considered. LOCAL_THEN_GLOBAL considers both the local providers
     * and the global providers in its search results. GLOBAL_ONLY only considers providers that are
     * flagged as global.
     *
     * @param discoveryScope
     *                  discovery scope to be set
     */
    void setDiscoveryScope(DiscoveryScope discoveryScope);

    /**
     * The time interval (in milliseconds) between two arbitration retries. It is NOT ensured that
     * the arbitration will be restarted after the given delay.
     *
     * @return the retry interval
     */
    qint64 getRetryInterval() const;

    /**
     * The time interval (in milliseconds) between two arbitration retries. It is NOT ensured that
     * the arbitration will be restarted after the given delay.
     *
     * @param retryInterval the minimum interval between to arbitration retries
     */
    void setRetryInterval(qint64 retryInterval);

    /*
     * Constants
     */
    static const QString KEYWORD_PARAMETER();

private:
    QMap<QString, types::CustomParameter> customParameters;
    ArbitrationStrategy arbitrationStrategy;
    qint64 discoveryTimeout;
    qint64 cacheMaxAge;
    DiscoveryScope discoveryScope;
    bool providerMustSupportOnChange;
    qint64 retryInterval;
};

} // namespace joynr
#endif //DISCOVERYQOS_H
