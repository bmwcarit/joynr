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

#ifndef DISCOVERYQOS_H
#define DISCOVERYQOS_H

#include <cstdint>
#include <map>
#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/types/CustomParameter.h"
#include "joynr/types/DiscoveryScope.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for the provider discovery process
 */
class JOYNR_EXPORT DiscoveryQos
{
public:
    /** @brief Default Constructor */
    DiscoveryQos();

    /**
     * @brief Constructor with definition of cache expiry
     * @param cacheMaxAgeMs The maximum age in milliseconds a cached entry will be considered for
     * discovery
     */
    explicit DiscoveryQos(std::int64_t cacheMaxAgeMs);

    /** @brief Destructor */
    virtual ~DiscoveryQos() = default;

    /**
     * The strategy specifies which type of Arbitrator will be
     * created by the ArbitratorFactory
     */
    enum class ArbitrationStrategy {
        /** the participant that was last refreshed (i.e. with the most current last seen
           date) will be selected */
        LAST_SEEN = 0,
        /** the participant which matches the provided participantId will be selected, if
         * existing
           */
        FIXED_PARTICIPANT = 1,
        /** only local participants will be considered */
        LOCAL_ONLY = 2,
        /** only participants which match a keyword will be considered */
        KEYWORD = 3,
        /** the participant with the highest priority will be selected.
            If multiple provider with the same highest priority are found,
            one of these will be arbitrarly selected. */
        HIGHEST_PRIORITY = 4
    };

    /**
     * @brief Gets the undefined value
     * @return The value signalizing that this value is yet undefined
     */
    static std::int64_t NO_VALUE();

    /**
     * @brief Gets the value signalling that no timeout has been selected
     * @return the value signalling that no timeout has been selected
     */
    static std::int64_t NO_TIMEOUT();

    /**
     * @brief Gets the default arbitration strategy value
     * @return the default arbitration strategy value
     */
    static ArbitrationStrategy& DEFAULT_ARBITRATIONSTRATEGY();

    /**
     * @brief Gets the default maximum cache age value in milliseconds
     * @return the default maximum cache age value in milliseconds
     */
    static std::int64_t DEFAULT_CACHEMAXAGE_MS();

    /**
     * @brief Gets the value signalling that no cache is to be used
     * @return the value signalling that no cache is to be used
     */
    static std::int64_t DO_NOT_USE_CACHE();

    /**
     * @brief Gets the default discovery scope value
     * @return the default discovery scope value
     */
    static joynr::types::DiscoveryScope::Enum& DEFAULT_DISCOVERYSCOPE();

    /**
     * @brief Sets the arbitration strategy for the discovery process
     *
     * The discovery process outputs a list of matching providers. The arbitration strategy then
     * chooses one or more of them to be used by the proxy.
     *
     * @param arbitrationStrategy
     *            Defines the strategy used to choose the "best" provider.
     */
    void setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy);

    /**
     * @brief Gets the currently used arbitration strategy
     *
     * The discovery process outputs a list of matching providers. The arbitration strategy then
     * chooses one or more of them to be used by the proxy.
     *
     * @return the arbitration strategy used to pick the "best" provider of the list of matching
     * providers
     */
    ArbitrationStrategy getArbitrationStrategy() const;

    /**
     * @brief Sets the discovery timeout value to be used
     *
     * As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
     * is triggered. If the discovery process does not find matching providers within the
     * discovery timeout duration it will be terminated and you will get an discovery exception.
     *
     * @param discoveryTimeoutMs
     *            the amount of time the arbitrator keeps trying to find a suitable provider.
     *The arbitration
     *            lookup might happen multiple times during this time span.
     */
    void setDiscoveryTimeoutMs(std::int64_t discoveryTimeoutMs);

    /**
     * @brief Gets the currently used discovery timeout value
     *
     * As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
     * is triggered. If the discovery process does not find matching providers within the
     * discovery timeout duration it will be terminated and you will get an discovery exception.
     *
     * @return the duration used to discover matching providers
     */
    std::int64_t getDiscoveryTimeoutMs() const;

    /**
     * @brief Allows to add special parameters to the DiscoveryQos which will be used
     * only by selected strategies.
     *
     * @param name
     *            String to identify the arbitration parameter
     * @param value
     *            Any String used by the arbitrator to choose a provider.
     */
    void addCustomParameter(std::string name, std::string value);

    /**
     * @brief Gets the parameters previously specified by addParameter
     *
     * @param name the parameter name for which the value should be returned
     * @return Returns the value to which the specified key is mapped, or null if the map of
     * additional parameters contains no mapping for the key
     */
    types::CustomParameter getCustomParameter(std::string name) const;

    /**
     * @brief Gets the map of custom parameters
     * @return
     *          The map of the current set custom parameters
     */
    std::map<std::string, types::CustomParameter> getCustomParameters() const;

    /**
     * @brief Gets the current set maximum age value
     *
     * Provider entries in the global capabilities directory are cached locally. Discovery will
     * consider entries in this cache valid if they are younger as the max age of cached
     * providers as defined in the QoS. All valid entries will be processed by the arbitrator when
     * searching
     * for and arbitrating the "best" matching provider.
     * <p>NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
     *       directory. Therefore, not all providers registered with the global capabilities
     *       directory might be taken into account during arbitration.
     *
     * @return the maximum age of locally cached provider entries to be used during discovery and
     * arbitration
     */
    std::int64_t getCacheMaxAgeMs() const;

    /**
     * @brief Sets the maximum age value for cached values
     *
     * Provider entries in the global capabilities directory are cached locally. Discovery will
     * consider entries in this cache valid if they are younger as the max age of cached
     * providers as defined in the QoS. All valid entries will be processed by the arbitrator when
     * searching
     * for and arbitrating the "best" matching provider.
     * <p>NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
     *       directory. Therefore, not all providers registered with the global capabilities
     *       directory might be taken into account during arbitration.
     *
     * @param cacheMaxAgeMs
     *       Maximum age of entries in the localCapabilitiesDirectory. If this value filters
     *       out all entries of the
     *       local capabilities directory a lookup in the global capabilitiesDirectory will
     *       take place.
     */
    void setCacheMaxAgeMs(std::int64_t cacheMaxAgeMs);

    /**
     * @brief Find out whether arbitration will only consider providers that support onChange
     *subscriptions
     *
     * @return true if only providers that support onChange subscriptions are considered, false
     *otherwise
     */
    bool getProviderMustSupportOnChange() const;

    /**
     * @brief Sets whether arbitration should only consider providers that support onChange
     * subscriptions
     *
     * @param providerMustSupportOnChange  true if only providers that support onChange
     *subscriptions should be considered, false otherwise
     */
    void setProviderMustSupportOnChange(bool providerMustSupportOnChange);

    /**
     * @brief Return the currently used discovery scope
     *
     * The scope determines where the discovery process will look for matching providers, if
     *LOCAL_ONLY,
     * only local providers will be considered. LOCAL_THEN_GLOBAL considers both the local providers
     * and the global providers in its search results. GLOBAL_ONLY only considers providers that are
     * flagged as global.
     *
     * @return the currently used discovery scope
     */
    joynr::types::DiscoveryScope::Enum getDiscoveryScope() const;

    /**
     * @brief Sets the currently used discovery scope.
     *
     * The scope determines where the discovery process will look for matching providers, if
     * LOCAL_ONLY,
     * only local providers will be considered. LOCAL_THEN_GLOBAL considers both the local providers
     * and the global providers in its search results. GLOBAL_ONLY only considers providers that are
     * flagged as global.
     *
     * @param discoveryScope
     *                  discovery scope to be set
     */
    void setDiscoveryScope(joynr::types::DiscoveryScope::Enum discoveryScope);

    /**
     * @brief Gets the time interval value (in milliseconds) between two arbitration retries
     *
     * It is NOT ensured that
     * the arbitration will be restarted after the given delay.
     *
     * @return the retry interval in milliseconds
     */
    std::int64_t getRetryIntervalMs() const;

    /**
     * @brief Determine the retry interval value (in milliseconds)
     *
     * The time interval (in milliseconds) between two arbitration retries. It is NOT ensured that
     * the arbitration will be restarted after the given delay.
     *
     * @param retryIntervalMs the minimum interval between to arbitration retries
     */
    void setRetryIntervalMs(std::int64_t retryIntervalMs);

    /*
     * Constants
     */

    /**
     * @brief Gets the string to be used as name for custom parameters when a keyword based
     * arbitration strategy is to be used
     * @return string value to be used as name
     */
    static const std::string KEYWORD_PARAMETER();

private:
    std::map<std::string, types::CustomParameter> _customParameters;
    ArbitrationStrategy _arbitrationStrategy;
    std::int64_t _discoveryTimeoutMs;
    std::int64_t _cacheMaxAgeMs;
    joynr::types::DiscoveryScope::Enum _discoveryScope;
    bool _providerMustSupportOnChange;
    std::int64_t _retryIntervalMs;
};

} // namespace joynr
#endif // DISCOVERYQOS_H
