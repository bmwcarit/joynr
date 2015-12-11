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
#ifndef SUBSCRIPTIONPUBLICATION_H
#define SUBSCRIPTIONPUBLICATION_H

#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"

#include <QObject>
#include <vector>

#include <memory>

#include "joynr/Variant.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionPublication : public QObject
{
    Q_OBJECT

    Q_PROPERTY(std::string subscriptionId READ getSubscriptionId WRITE setSubscriptionId)
    Q_PROPERTY(std::vector<Variant> response READ getResponse WRITE setResponse)
public:
    SubscriptionPublication& operator=(const SubscriptionPublication& other);
    bool operator==(const SubscriptionPublication& other) const;
    bool operator!=(const SubscriptionPublication& other) const;

    const static SubscriptionPublication NULL_RESPONSE;

    SubscriptionPublication(const SubscriptionPublication& other);
    SubscriptionPublication();

    std::string getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionId);

    std::vector<Variant> getResponse() const;
    void setResponse(const std::vector<Variant>& response);

    const Variant& getError() const;
    void setError(const Variant& error);

private:
    std::string subscriptionId;
    std::vector<Variant> response;
    Variant error;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::SubscriptionPublication)
Q_DECLARE_METATYPE(std::shared_ptr<joynr::SubscriptionPublication>)

#endif // SUBSCRIPTIONPUBLICATION_H
