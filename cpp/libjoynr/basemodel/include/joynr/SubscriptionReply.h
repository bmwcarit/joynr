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
#ifndef SUBSCRIPTIONREPLY_H
#define SUBSCRIPTIONREPLY_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionReply
{
public:
    SubscriptionReply& operator=(const SubscriptionReply& other);
    bool operator==(const SubscriptionReply& other) const;
    bool operator!=(const SubscriptionReply& other) const;

    SubscriptionReply(const SubscriptionReply& other);
    SubscriptionReply();

    const std::string& getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionIdLocal);
    void setSubscriptionId(std::string&& subscriptionIdLocal);

    std::shared_ptr<exceptions::SubscriptionException> getError() const;
    void setError(std::shared_ptr<exceptions::SubscriptionException> errorLocal);

    /**
     * @brief Stringifies the class
     * @return stringified class content
     */
    std::string toString() const;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(subscriptionId), MUESLI_NVP(error));
    }

private:
    /**
     * @brief printing SubscriptionReply with google-test and google-mock
     * @param subscriptionReply the object to be printed
     * @param os the destination output stream the print should go into
     */
    friend void PrintTo(const SubscriptionReply& subscriptionReply, ::std::ostream* os);

    std::string subscriptionId;
    std::shared_ptr<exceptions::SubscriptionException> error;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionReply, "joynr.SubscriptionReply")

#endif // SUBSCRIPTIONREPLY_H
