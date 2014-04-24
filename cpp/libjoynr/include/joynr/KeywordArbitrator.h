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
#ifndef KEYWORDARBITRATOR
#define KEYWORDARBITRATOR
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include "joynr/ProviderArbitrator.h"
#include <QString>
#include <QSharedPointer>

namespace joynr {

namespace system {
    class IDiscoverySync;
    class DiscoveryEntry;
}

/**
  * Arbitrator which requests CapabilityEntries for a give interfaceName and domain from
  * the LocalCapabilitiesDirectory and filters them by a keyword in the qos. The first address
  * of the first entry with the correct keyword is returned.
  */


class JOYNR_EXPORT KeywordArbitrator : public ProviderArbitrator {

public:
    virtual ~KeywordArbitrator() { }

    KeywordArbitrator(
            const QString& domain,
            const QString& interfaceName,
            joynr::system::IDiscoverySync& discoveryProxy,
            const DiscoveryQos &discoveryQos
    );

    /*
     *  Attempts to the arbitrate. This function is called by the ProviderArbitrator
    */
    void attemptArbitration();

    /*
     * The key of the keyword parameter in the provider Qos parameters
     */
    static const QString KEYWORD_PARAMETER_KEY();

    /*
     * Made public for testing purposes
     */
    void receiveCapabilitiesLookupResults(
            const QList<joynr::system::DiscoveryEntry>& discoveryEntries
    );

private:
    DISALLOW_COPY_AND_ASSIGN(KeywordArbitrator);
    QString keyword;
    static int ARBITRATION_RETRY_INTERVAL;
    joynr_logging::Logger* logger;

};


} // namespace joynr
#endif //KEYWORDARBITRATOR
