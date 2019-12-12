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
#ifndef BASEREPLY_H
#define BASEREPLY_H

#include <cassert>
#include <utility>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/SerializationPlaceholder.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT BaseReply
{
public:
    BaseReply();
    virtual ~BaseReply() = default;

    BaseReply(BaseReply&&) = default;
    BaseReply& operator=(BaseReply&&) = default;

    bool operator==(const BaseReply& other) const;
    bool operator!=(const BaseReply& other) const;

    template <typename... Ts>
    void setResponse(Ts&&... values)
    {
        response.setData(std::forward<Ts>(values)...);
    }

    template <typename... Ts>
    void getResponse(Ts&... values)
    {
        assert(hasResponse());
        response.getData(values...);
    }

    bool hasResponse() const
    {
        return response.containsInboundData() || response.containsOutboundData();
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(response));
    }

protected:
    joynr::serializer::SerializationPlaceholder response;
};

} // namespace joynr

#endif // BASEREPLY_H
