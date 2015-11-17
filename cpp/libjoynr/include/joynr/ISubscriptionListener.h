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
#ifndef ISUBSCRIPTIONLISTENER_H
#define ISUBSCRIPTIONLISTENER_H

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

/**
 * @brief Class interface to be extended by attribute or broadcast subscription listeners
 */
template <typename... Ts>
class ISubscriptionListener
{
public:
    /** @brief Constructor */
    ISubscriptionListener()
    {
    }

    /** @brief Destructor */
    virtual ~ISubscriptionListener()
    {
    }

    /**
     * @brief onReceive Gets called on every received publication
     *
     * Since the onReceive callback is called by a communication middleware thread, it should not
     * be blocked, wait for user interaction, or do larger computation.
     * @param values associated with the subscription this listener is listen to
     */
    virtual void onReceive(const Ts&... values) = 0;

    /**
     * @brief onError Gets called on every error that is detected on the subscription
     *
     * Since the onError callback is called by a communication middleware thread, it should not
     * be blocked, wait for user interaction, or do larger computation.
     */
    virtual void onError(const exceptions::JoynrRuntimeException& error) = 0;
};

} // namespace joynr
#endif // SUBSCRIPTIONLISTENER_H
