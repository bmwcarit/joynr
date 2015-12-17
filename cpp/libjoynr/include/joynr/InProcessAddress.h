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
#ifndef INPROCESSADDRESS_H
#define INPROCESSADDRESS_H

#include "joynr/JoynrExport.h"
#include "joynr/system/RoutingTypes/Address.h"

#include <memory>

namespace joynr
{

class RequestCaller;

class JOYNR_EXPORT InProcessAddress : public joynr::system::RoutingTypes::Address
{
public:
    virtual ~InProcessAddress() = default;
    InProcessAddress(std::shared_ptr<RequestCaller> requestCaller);
    std::shared_ptr<RequestCaller> getRequestCaller() const;

private:
    std::shared_ptr<RequestCaller> requestCaller;
};

} // namespace joynr
#endif // INPROCESSADDRESS_H
