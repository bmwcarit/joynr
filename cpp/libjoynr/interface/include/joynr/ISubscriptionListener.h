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
#ifndef ISUBSCRIPTIONLISTENER_H
#define ISUBSCRIPTIONLISTENER_H

#include <string>

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class ISubscriptionListenerBase
{
public:
    virtual ~ISubscriptionListenerBase() = default;

    /**
     * @brief onError Gets called on every error that is detected on the subscription
     *
     * Since the onError callback is called by a communication middleware thread, it should not
     * be blocked, wait for user interaction, or do larger computation.
     *
     * @param error JoynrRuntimeException describing the error
     */
    virtual void onError(const exceptions::JoynrRuntimeException& error) = 0;

    /**
     * @brief onSubscribed Gets called when the subscription is successfully registered at the
     * provider
     * @param subscriptionId the subscription id of the subscription as string
     *
     * Since the onSubscribed callback is called by a communication middleware thread, it should
     * not be blocked, wait for user interaction, or do larger computation.
     */
    virtual void onSubscribed(const std::string& subscriptionId) = 0;
};

/**
 * @brief Class interface to be extended by attribute or broadcast subscription listeners
 */
template <typename T, typename... Ts>
class ISubscriptionListener : public ISubscriptionListenerBase
{
public:
    /**
     * @brief onReceive Gets called on every received publication
     *
     * Since the onReceive callback is called by a communication middleware thread, it should not
     * be blocked, wait for user interaction, or do larger computation.
     * @param values associated with the subscription this listener is listening to
     */
    virtual void onReceive(const T& value, const Ts&... values) = 0;
};

template <>
class ISubscriptionListener<void> : public ISubscriptionListenerBase
{
public:
    virtual void onReceive() = 0;
};

} // namespace joynr
#endif // SUBSCRIPTIONLISTENER_H
