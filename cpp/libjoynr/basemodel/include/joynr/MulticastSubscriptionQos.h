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

#ifndef MULTICASTSUBSCRIPTIONQOS_H
#define MULTICASTSUBSCRIPTIONQOS_H

#include "joynr/SubscriptionQos.h"

#include <cstdint>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/**
 * @brief Placeholder class representing a multicast subscription quality of service.
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>non selective broadcasts</b>.
 *
 * Refer to SubscriptionQos for further details.
 */
class JOYNR_EXPORT MulticastSubscriptionQos : public SubscriptionQos
{

public:
    /** @brief Default constructor */
    MulticastSubscriptionQos() = default;

    /**
     * @brief Constructor with full parameter set.
     *
     * @param validityMs Time span in milliseconds during which publications will be sent
     */
    explicit MulticastSubscriptionQos(std::int64_t validityMs);

    /**
     * @brief Copy constructor
     * @param subscriptionQos The MulticastSubscriptionQos object to be copied from.
     */
    MulticastSubscriptionQos(const MulticastSubscriptionQos& subscriptionQos) = default;

    /** @brief Assignment operator */
    MulticastSubscriptionQos& operator=(const MulticastSubscriptionQos& subscriptionQos);

    /** @brief Equality operator */
    bool operator==(const MulticastSubscriptionQos& subscriptionQos) const;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<SubscriptionQos>(this));
    }
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::MulticastSubscriptionQos,
                                 joynr::SubscriptionQos,
                                 "joynr.MulticastSubscriptionQos")

#endif // MULTICASTSUBSCRIPTIONQOS_H
