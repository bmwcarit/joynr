/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef UNICASTSUBSCRIPTIONQOS_H
#define UNICASTSUBSCRIPTIONQOS_H

#include "joynr/SubscriptionQos.h"

#include <cstdint>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/**
 * @brief Quality of service of unicast subscription.
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>selective broadcasts</b>.
 */
class JOYNR_EXPORT UnicastSubscriptionQos : public SubscriptionQos
{

public:
    /** @brief Default constructor */
    UnicastSubscriptionQos();

    /**
     * @brief Constructor with full parameter set
     *
     * @param publicationTtlMs The publication TTL in ms.
     * @param validityMs Time span in milliseconds during which publications will be sent
     */
    UnicastSubscriptionQos(std::int64_t validityMs, std::int64_t publicationTtlMs);

    /**
     * @brief Copy constructor
     * @param subscriptionQos The SubscriptionQos object to be copied from.
     */
    UnicastSubscriptionQos(const UnicastSubscriptionQos& subscriptionQos);

    /**
     * @brief Gets the time to live value for publication messages.
     *
     * Notification messages will be sent with this time-to-live.
     * If a notification message can not be delivered within its TTL,
     * it will be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @return Returns the TTL of the publication Messages in milliseconds.
     *
     * @see SubscriptionQos#setExpiryDateMs
     */
    virtual std::int64_t getPublicationTtlMs() const;

    /**
     * @brief Sets the time to live for publication messages in milliseconds
     *
     * Notification messages will be sent with this time-to-live. If a
     * notification message can not be delivered within its TTL, it will
     * be deleted from the system.
     * <p>NOTE: If a notification message is not delivered due to an
     * expired TTL, it might raise a missed publication notification
     * (depending on the value of the alert interval QoS).
     *
     * @param publicationTtl_ms TTL of the publication Messages in milliseconds.
     * <br/><br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li>minimum publicationTtl_ms = 100. Smaller values will be rounded up.
     * <li>maximum publicationTtl_ms = 2 592 000 000 (30 days). Larger values will be rounded down.
     * </ul>
     *
     * @see SubscriptionQos#setExpiryDateMs
     */
    virtual void setPublicationTtlMs(std::int64_t publicationTtlMs);

    /** @brief Assignment operator */
    UnicastSubscriptionQos& operator=(const UnicastSubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    bool operator==(const UnicastSubscriptionQos& subscriptionQos) const;

    /**
     * @brief Returns the default publication time to live value in milliseconds:
     * 10 000 (10 secs)
     */
    static std::int64_t DEFAULT_PUBLICATION_TTL_MS();

    /**
     * @brief Returns the minimum publication time to live value in milliseconds:
     * 100
     */
    static std::int64_t MIN_PUBLICATION_TTL_MS();

    /**
     * @brief Returns the maximum publication time to live value in milliseconds:
     * 2 592 000 000 (30 days)
     */
    static std::int64_t MAX_PUBLICATION_TTL_MS();

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<SubscriptionQos>(this), MUESLI_NVP(_publicationTtlMs));
    }

protected:
    /** @brief The publication time to live in milliseconds */
    std::int64_t _publicationTtlMs;
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::UnicastSubscriptionQos,
                                 joynr::SubscriptionQos,
                                 "joynr.UnicastSubscriptionQos")

#endif // SUBSCRIPTIONQOS_H
