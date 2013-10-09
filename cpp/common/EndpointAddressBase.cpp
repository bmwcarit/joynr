/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#include "joynr/EndpointAddressBase.h"

namespace joynr {

const QString& EndpointAddressBase::ENDPOINT_ADDRESS_TYPE() {
    static QString value("EndpointAddressBase");
    return value;
}

EndpointAddressBase::EndpointAddressBase() : QObject()
{

}

EndpointAddressBase::EndpointAddressBase(const EndpointAddressBase &other) :
    // Here we should normally call the super copy constructor.
    QObject()
{
    Q_UNUSED(other)
    // WARNING: QObjects and derived objects should be thought as identities and not as
    // values (http://doc.qt.nokia.com/4.7-snapshot/object.html#identity-vs-value). Therefore
    // they should not implement copy constructor and assignment operator
    // (http://doc.qt.nokia.com/4.7-snapshot/qobject.html#no-copy-constructor-or-assignment-operator).
}



EndpointAddressBase::~EndpointAddressBase(){

}

EndpointAddressBase &EndpointAddressBase::operator =(const EndpointAddressBase &other)
{
    // Here we should normally call the super assignment operator.
    Q_UNUSED(other);
    // WARNING: QObjects and derived objects should be thought as identities and not as
    // values (http://doc.qt.nokia.com/4.7-snapshot/object.html#identity-vs-value). Therefore
    // they should not implement copy constructor and assignment operator
    // (http://doc.qt.nokia.com/4.7-snapshot/qobject.html#no-copy-constructor-or-assignment-operator).
    return *this;
}

bool EndpointAddressBase::operator ==(const EndpointAddressBase &other) const
{
    Q_UNUSED(other);
    return true;
}


} // namespace joynr
