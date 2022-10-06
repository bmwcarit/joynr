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
#ifndef MESSAGINGSTUBFACTORY_H
#define MESSAGINGSTUBFACTORY_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash/extensions.hpp>

#include "joynr/IMessagingStubFactory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ThreadSafeMap.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

class IMessagingStub;
class IMiddlewareMessagingStubFactory;

/**
 * Creates/Stores <Middleware>MessagingStubs. MessagingStubs are used to contact remote
 *ClusterControllers (HttpCommunicationManager)
 * and libjoynrs (dummy<Libjoynr>Skeleton) on the machine.
 * A libjoynr does not need a MessagingStubFactory, as each libJoynr has one MessagingStub that
 *connects it to its cc,
 * and will nevere use any other MessagingStubs.
 *
 */

class MessagingStubFactory : public IMessagingStubFactory
{

public:
    // MessagingStubFactory is created without the necessary skeletons.
    // Those Skeletons must be registered before the MessagingStubFactory is used.
    MessagingStubFactory();

    std::shared_ptr<IMessagingStub> create(
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress)
            override;
    void remove(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&
                        destinationAddress) override;
    bool contains(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&
                          destinationAddress) override;

    void registerStubFactory(std::shared_ptr<IMiddlewareMessagingStubFactory> factory);
    void shutdown() override;

private:
    DISALLOW_COPY_AND_ASSIGN(MessagingStubFactory);

    using Address = joynr::system::RoutingTypes::Address;
    using AddressPtr = std::shared_ptr<const Address>;
    struct AddressPtrHash {
        std::size_t operator()(const AddressPtr& addressPtr) const
        {
            return addressPtr->hashCode();
        }
    };
    struct AddressPtrCompare {
        bool operator()(const AddressPtr& p1, const AddressPtr& p2) const
        {
            return *p1 == *p2;
        }
    };
    template <typename K, typename V>
    using Map = std::unordered_map<K, V, AddressPtrHash, AddressPtrCompare>;
    ThreadSafeMap<AddressPtr, std::shared_ptr<IMessagingStub>, Map> _address2MessagingStubMap;
    std::vector<std::shared_ptr<IMiddlewareMessagingStubFactory>> _factoryList;
    std::mutex mutex;
};

} // namespace joynr
#endif // MESSAGINGSTUBFACTORY
