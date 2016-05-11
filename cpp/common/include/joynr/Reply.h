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
#ifndef REPLY_H
#define REPLY_H

#include <string>
#include <vector>

#include "joynr/JoynrCommonExport.h"
#include "joynr/Variant.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/serializer/SerializationPlaceholder.h"

namespace joynr
{
class JOYNRCOMMON_EXPORT Reply
{
public:
    Reply();
    Reply(Reply&&) = default;
    ~Reply() = default;
    Reply& operator=(Reply&&) = default;

    bool operator==(const Reply&) const;
    bool operator!=(const Reply&) const;

    const std::string& getRequestReplyId() const;
    void setRequestReplyId(const std::string& requestReplyId);

    const std::vector<Variant>& getResponseVariant() const;
    void setResponseVariant(std::vector<Variant> response);

    const Variant& getErrorVariant() const;
    void setErrorVariant(const Variant& errorVariant);

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
        archive(MUESLI_NVP(requestReplyId), MUESLI_NVP(response), MUESLI_NVP(error));
    }

private:
    std::string requestReplyId;
    std::vector<Variant> responseVariant;
    joynr::serializer::SerializationPlaceholder response;
    Variant errorVariant;
    std::shared_ptr<exceptions::JoynrException> error;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::Reply, "joynr.Reply")

#endif // REPLY_H
