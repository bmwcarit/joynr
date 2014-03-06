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
#ifndef SOMEIPENDPOINTADDRESS_H
#define SOMEIPENDPOINTADDRESS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/system/Address.h"

#include <QMetaType>
#include <QString>

namespace joynr {

/*
  * This is a dummy class just to have a second type of EndpointAddress
  * (probably should be moved to test/util folder)
  *
  *
  * SomeIp has identifiers for ip-address, port, fields and methods
  * (needs probably four or more QStrings) (info from a talk, RDZ)
  *
  *
  */

class JOYNRCOMMON_EXPORT SomeIpEndpointAddress : public joynr::system::Address {
    Q_OBJECT

    Q_PROPERTY(QString ipAddress READ getIpAddress WRITE setIpAddress)
    Q_PROPERTY(int port READ getPort WRITE setPort)

public:
    static const QString& ENDPOINT_ADDRESS_TYPE();

    SomeIpEndpointAddress();
    SomeIpEndpointAddress(const QString &ipAddress, int port);
    SomeIpEndpointAddress(const SomeIpEndpointAddress& other);
    virtual ~SomeIpEndpointAddress();

    virtual SomeIpEndpointAddress& operator =(const SomeIpEndpointAddress& other);
    virtual bool operator ==(const SomeIpEndpointAddress& other) const;

    // getters
    QString getIpAddress() const;
    int getPort() const;

    // setters
    void setIpAddress(const QString& ipAddress);
    void setPort(int port);

private:
    QString ipAddress;
    int port;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::SomeIpEndpointAddress)

#endif //SOMEIPENDPOINTADDRESS_H
