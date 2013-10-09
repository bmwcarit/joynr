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
#ifndef JOYNRMESSAGINGVIACCENDPOINTADDRESS_H
#define JOYNRMESSAGINGVIACCENDPOINTADDRESS_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/EndpointAddressBase.h"

#include <QString>

namespace joynr {

/*
  * This endpoint Address is needed to be handed through on libjoynr side, when a Joynr-Address is
  * needed. On Libjoynr-side no channelId is needed, and the participant-Id is also not stored
  * in the EndPointaddresses. Therefore this Class is empty, nontheless the ConnectorFactory should
  * be able to create a connector.
  * The mapping between participant-id and channel-id is done on clustercontroller side.
  *
  * TODO: Maybe we can use a static JoynrMessagingViaCCEndpointAddress Object?
  */

class JOYNRCOMMON_EXPORT JoynrMessagingViaCCEndpointAddress : public EndpointAddressBase {
    Q_OBJECT
public:
    JoynrMessagingViaCCEndpointAddress();
    virtual ~JoynrMessagingViaCCEndpointAddress();
    static const QString& ENDPOINT_ADDRESS_TYPE();
};



} // namespace joynr
#endif // JOYNRMESSAGINGVIACCENDPOINTADDRESS_H
