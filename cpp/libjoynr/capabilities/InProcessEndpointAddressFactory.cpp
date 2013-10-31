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
#include "QtCore"
#include "libjoynr/capabilities/InProcessEndpointAddressFactory.h"
#include "joynr/InProcessEndpointAddress.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/CapabilityEntry.h"


namespace joynr {

QList<CapabilityEntry> InProcessEndpointAddressFactory::create(QList<CapabilityEntry> &entryList,
                                                               IRequestCallerDirectory *requestCallerDirectory){
    QList<CapabilityEntry>::iterator entryIterator;
    for(entryIterator = entryList.begin(); entryIterator != entryList.end(); ++entryIterator) {
        QString participantId = entryIterator->getParticipantId();
        if(requestCallerDirectory->containsRequestCaller(participantId)){
            QSharedPointer<RequestCaller> requestCaller = requestCallerDirectory->lookupRequestCaller(participantId);
            QSharedPointer<InProcessEndpointAddress> inProcessAddress(new InProcessEndpointAddress(requestCaller));
            entryIterator->prependEndpointAddress(inProcessAddress);
        }
    }
    return entryList;
}

} // namespace joynr
