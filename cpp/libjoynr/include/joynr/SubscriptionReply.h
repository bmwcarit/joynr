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
#ifndef SUBSCRIPTIONREPLY_H
#define SUBSCRIPTIONREPLY_H

#include "joynr/JoynrExport.h"

#include <QObject>
#include <string>

#include <memory>

namespace joynr
{

class JOYNR_EXPORT SubscriptionReply : public QObject
{
    Q_OBJECT

    Q_PROPERTY(std::string subscriptionId READ getSubscriptionId WRITE setSubscriptionId)
public:
    SubscriptionReply& operator=(const SubscriptionReply& other);
    bool operator==(const SubscriptionReply& other) const;
    bool operator!=(const SubscriptionReply& other) const;

    const static SubscriptionReply NULL_RESPONSE;

    SubscriptionReply(const SubscriptionReply& other);
    SubscriptionReply();

    std::string getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionId);

private:
    std::string subscriptionId;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::SubscriptionReply)
Q_DECLARE_METATYPE(std::shared_ptr<joynr::SubscriptionReply>)

#endif // SUBSCRIPTIONREPLY_H
