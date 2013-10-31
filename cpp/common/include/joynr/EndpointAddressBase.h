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
#ifndef ENDPOINTADDRESSBASE_H
#define ENDPOINTADDRESSBASE_H

#include "joynr/JoynrCommonExport.h"

#include <QMetaType>
#include <QObject>
#include <QString>

namespace joynr {

/**
  * Base class for middleware endpoint addresses.
  * Middleware specific implementations inherit from this class.
  * EndpointAddresses are passed as shared pointers to this base class and are
  * casted to their real type before usage. The casting is done with QSharedPointer's
  * dynamic_cast method:
  * if( pointerToEndpointAddress->metaObject()->className() == JoynrMessagingEndpointAddress::ENDPOINTADDRESSTYPE) {
  * QSharedPointer<JoynrMessagingEndpointAddress> joynrAddress = pointerToEndpointAddress.dynamicCast<JoynrMessagingEndpointAddress>();
  * }
  *
  */

class JOYNRCOMMON_EXPORT EndpointAddressBase : public QObject {
    Q_OBJECT
public:
    static const QString& ENDPOINT_ADDRESS_TYPE();

    EndpointAddressBase();
    EndpointAddressBase(const EndpointAddressBase& other);
    virtual ~EndpointAddressBase();

    virtual EndpointAddressBase& operator =(const EndpointAddressBase& other);
    virtual bool operator ==(const EndpointAddressBase& other) const;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::EndpointAddressBase)

#endif //ENDPOINTADDRESSBASE_H
