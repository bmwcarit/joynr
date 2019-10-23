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
#ifndef BASE_PUBLICATION_H
#define BASE_PUBLICATION_H

#include <memory>
#include <vector>

#include "joynr/BaseReply.h"
#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT BasePublication : public BaseReply
{
public:
    BasePublication();

    explicit BasePublication(BaseReply&& reply);

    BasePublication(BasePublication&&) = default;
    BasePublication& operator=(BasePublication&&) = default;

    bool operator==(const BasePublication& other) const;
    bool operator!=(const BasePublication& other) const;

    std::shared_ptr<exceptions::JoynrRuntimeException> getError() const;
    void setError(std::shared_ptr<exceptions::JoynrRuntimeException> errorLocal);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<BaseReply>(this), MUESLI_NVP(error));
    }

private:
    // printing BasePublication with google-test and google-mock
    friend void PrintTo(const BasePublication& basePublication, ::std::ostream* os);
    std::shared_ptr<exceptions::JoynrRuntimeException> error;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::BasePublication, "joynr.BasePublication")

#endif // BASE_PUBLICATION_H
