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

#include "joynr/Variant.h"
#include "joynr/BaseReply.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/JoynrCommonExport.h"

namespace joynr
{
class JOYNRCOMMON_EXPORT Reply : public BaseReply
{
public:
    Reply();
    virtual ~Reply() = default;

    Reply(Reply&) = default;
    Reply& operator=(Reply&) = default;

    Reply(Reply&&) = default;
    Reply& operator=(Reply&&) = default;

    Reply(BaseReply&& baseReply);

    bool operator==(const Reply&) const;
    bool operator!=(const Reply&) const;

    const std::string& getRequestReplyId() const;
    void setRequestReplyId(const std::string& requestReplyId);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(requestReplyId), MUESLI_NVP(response), MUESLI_NVP(error));
    }

    std::shared_ptr<exceptions::JoynrException> getError() const;
    void setError(std::shared_ptr<exceptions::JoynrException> error);

    // ====== START /// TO BE DELETED
    const std::vector<Variant>& getResponseVariant() const;
    void setResponseVariant(std::vector<Variant> response);
    // ====== END /// TO BE DELETED

private:
    std::string requestReplyId;
    std::shared_ptr<exceptions::JoynrException> error;
    // ====== START /// TO BE DELETED
    std::vector<Variant> responseVariant;
    // ====== END /// TO BE DELETED
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::Reply, "joynr.Reply")

#endif // REPLY_H
