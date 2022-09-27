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
#ifndef MULTICASTPUBLICATION_H
#define MULTICASTPUBLICATION_H

#include "joynr/BasePublication.h"
#include "joynr/BaseReply.h"
#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT MulticastPublication : public BasePublication
{
public:
    MulticastPublication();

    explicit MulticastPublication(BaseReply&& reply);

    MulticastPublication(MulticastPublication&&) = default;
    MulticastPublication& operator=(MulticastPublication&&) = default;

    bool operator==(const MulticastPublication& other) const;
    bool operator!=(const MulticastPublication& other) const;

    const std::string& getMulticastId() const;
    void setMulticastId(const std::string& multicastIdPararm);
    void setMulticastId(std::string&& multicastIdPararm);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<BasePublication>(this), MUESLI_NVP(multicastId));
    }

private:
    // printing MulticastPublication with google-test and google-mock
    friend void PrintTo(const MulticastPublication& MulticastPublication, ::std::ostream* os);
    std::string multicastId;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::MulticastPublication, "joynr.MulticastPublication")

#endif // MULTICASTPUBLICATION_H
