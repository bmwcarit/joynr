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
#ifndef PRETTYPRINT_H_
#define PRETTYPRINT_H_

#include "utils/Gtest.h"
#include <iostream>

#include "joynr/StatusCode.h"
#include "joynr/types/DiscoveryEntry.h"

//void initPretty(void);

namespace joynr {

// NOTE: Choosing the right PrintTo method is done by template magic by
//       the compiler. Therefore, the point in time when the PrintTo method
//       is defined is crucial. So consider defining the method in the same
//       file where your type is defined.
//
// The following PrintTo's are defined directly in the file where the type is
// defined:
//    class MessagingQos;
//    void PrintTo(const MessagingQos& value, ::std::ostream* os);

namespace system {
    class DiscoveryEntry;
    void PrintTo(const joynr::types::DiscoveryEntry& value, ::std::ostream* os);
    class WebSocketAddress;
    void PrintTo(const joynr::system::WebSocketAddress& value, ::std::ostream* os);
} // namespace system

namespace exceptions {

class JoynrException;
void PrintTo(const joynr::exceptions::JoynrException& value, ::std::ostream* os);

class JoynrRuntimeException;
void PrintTo(const joynr::exceptions::JoynrRuntimeException& value, ::std::ostream* os);

class JoynrConfigurationException;
void PrintTo(const joynr::exceptions::JoynrConfigurationException& value, ::std::ostream* os);

class JoynrTimeOutException;
void PrintTo(const joynr::exceptions::JoynrTimeOutException& value, ::std::ostream* os);

class JoynrMessageNotSentException;
void PrintTo(const joynr::exceptions::JoynrMessageNotSentException& value, ::std::ostream* os);

class JoynrDelayMessageException;
void PrintTo(const joynr::exceptions::JoynrDelayMessageException& value, ::std::ostream* os);

class JoynrParseError;
void PrintTo(const joynr::exceptions::JoynrParseError& value, ::std::ostream* os);

class DiscoveryException;
void PrintTo(const joynr::exceptions::DiscoveryException& value, ::std::ostream* os);

class NoCompatibleProviderFoundException;
void PrintTo(const joynr::exceptions::NoCompatibleProviderFoundException& value, ::std::ostream* os);

class ProviderRuntimeException;
void PrintTo(const joynr::exceptions::ProviderRuntimeException& value, ::std::ostream* os);

class MethodInvocationException;
void PrintTo(const joynr::exceptions::MethodInvocationException& value, ::std::ostream* os);

class SubscriptionException;
void PrintTo(const joynr::exceptions::SubscriptionException& value, ::std::ostream* os);

class PublicationMissedException;
void PrintTo(const joynr::exceptions::PublicationMissedException& value, ::std::ostream* os);

class ApplicationException;
void PrintTo(const joynr::exceptions::ApplicationException& value, ::std::ostream* os);

} // namespace exceptions

} // namespace joynr
void PrintTo(const joynr::StatusCodeEnum& value, ::std::ostream* os);
#endif // PRETTYPRINT_H_
