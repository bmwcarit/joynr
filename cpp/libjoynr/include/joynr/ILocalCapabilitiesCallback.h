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
#ifndef ILOCALCAPABILITIESDIRECTORYCALLBACK_H
#define ILOCALCAPABILITIESDIRECTORYCALLBACK_H

#include "joynr/JoynrExport.h"
#include "joynr/CapabilityEntry.h"

namespace joynr {

class JOYNR_EXPORT ILocalCapabilitiesCallback {
public:
    // This class is accessed through a QSharedPointer.
    // The empty destructor is not defined in the header file:
    // https://bugreports.qt-project.org/browse/QTBUG-7302
    // http://doc.qt.digia.com/qt/qscopedpointer.html#forward-declared-pointers
    virtual ~ILocalCapabilitiesCallback();  

    virtual void capabilitiesReceived(QList<CapabilityEntry> capabilities)=0;
};


} // namespace joynr
#endif //ILOCALCAPABILITIESDIRECTORYCALLBACK_H
