/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef BASEREPLY_H
#define BASEREPLY_H

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "joynr/JoynrCommonExport.h"
#include "joynr/Variant.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/serializer/SerializationPlaceholder.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT BaseReply
{
public:
    BaseReply();
    virtual ~BaseReply() = default;

    BaseReply(const BaseReply&) = default;
    BaseReply& operator=(const BaseReply&) = default;

    BaseReply(BaseReply&&) = default;
    BaseReply& operator=(BaseReply&&) = default;

    bool operator==(const BaseReply& other) const;
    bool operator!=(const BaseReply& other) const;

    std::shared_ptr<exceptions::JoynrException> getError() const;
    void setError(std::shared_ptr<exceptions::JoynrException> error);

    template <typename... Ts>
    void setResponse(Ts&&... values)
    {
        response.setData(std::make_tuple(std::forward<Ts>(values)...));
    }

    template <typename... Ts>
    void getResponse(std::tuple<Ts...>& responseTuple)
    {
        assert(hasResponse());
        response.getData(responseTuple);
    }

    bool hasResponse() const
    {
        return response.containsInboundData();
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(error));
    }

protected:
    joynr::serializer::SerializationPlaceholder response;
    std::shared_ptr<exceptions::JoynrException> error;
};

} // namespace joynr

#endif // SUBSCRIPTIONPUBLICATION_H
