/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "SitUtil.h"

#include <stdexcept>

#include "KeychainImpl.h"

namespace joynr
{
std::shared_ptr<IKeychain> tryLoadKeychainFromCmdLineArgs(const std::string& certPemFilename,
                                                          const std::string& privateKeyPemFilename,
                                                          const std::string& caCertPemFilename)
{
    const bool oneSSLCmdLineArgProvided = !certPemFilename.empty() ||
                                          !privateKeyPemFilename.empty() ||
                                          !caCertPemFilename.empty();
    const bool allSSLCmdLineArgsProvided = !certPemFilename.empty() &&
                                           !privateKeyPemFilename.empty() &&
                                           !caCertPemFilename.empty();

    if (oneSSLCmdLineArgProvided && !allSSLCmdLineArgsProvided) {
        throw std::invalid_argument("All three ssl options must be provided (cert, key, ca-cert)");
    }

    if (allSSLCmdLineArgsProvided) {
        const std::string privateKeyPassword("");

        return KeychainImpl::createFromPEMFiles(
                certPemFilename, privateKeyPemFilename, caCertPemFilename, privateKeyPassword);
    }

    return nullptr;
}
} // namespace joynr
