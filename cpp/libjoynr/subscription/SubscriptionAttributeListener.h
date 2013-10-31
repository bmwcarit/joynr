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
#ifndef SUBSCRIPTIONATTRIBUTELISTENER_H
#define SUBSCRIPTIONATTRIBUTELISTENER_H

#include "joynr/JoynrExport.h"
#include "joynr/IAttributeListener.h"

#include <QString>

namespace joynr {

class PublicationManager;

/**
 * An attribute listener used for onChange subscriptions
 */
class JOYNR_EXPORT SubscriptionAttributeListener : public IAttributeListener
{
public:
    /**
     * Create an attribute listener linked to a subscription
     */
    SubscriptionAttributeListener(const QString& subscriptionId, PublicationManager &publicationManager);

    // Implementation of IAttributeListener::attributeValueChanged
    void attributeValueChanged(const QVariant& value);
private:
    QString subscriptionId;
    PublicationManager& publicationManager;
};

} // namespace joynr

#endif // SUBSCRIPTIONATTRIBUTELISTENER_H
