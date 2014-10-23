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
#ifndef ICLIENTMULTIHASH_H
#define ICLIENTMULTIHASH_H

#include "joynr/MessagingQos.h"
#include "joynr/CachedValue.h"

#include <QList>
#include <QVariant>
#include <QString>

namespace joynr
{

/*
 * Interface for an attribute cache that can be used by one or many clients.
 * It stores several entries under the same key, i.e. an insert does not overwrite
 * the current entry. Rather, a list of entries is maintained for a key.
 */
class IClientMultiCache
{
public:
    virtual ~IClientMultiCache()
    {
    }
    /*
    * Returns the list of values stored for the attributeId. If none exists, it returns an empty
    * QList object that can be tested for by using the isEmpty() method.
    *
    */
    virtual QList<QVariant> lookUp(const QString& attributeId, qint64 maxAcceptedAgeInMs) = 0;
    /*
    * Inserts the key (attributeId) and value into the cache.  If the attributeId already
    * exists in the cache the value is added to a list (no overwrite).
    * Note, this insert does not perform any validation on the value.
    */
    virtual void insert(QString attributeId, QVariant value) = 0;
};

} // namespace joynr
#endif // ICLIENTMULTIHASH_H
