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

#ifndef KEYCHAINIMPL_H
#define KEYCHAINIMPL_H

#include <memory>
#include <string>

#include <joynr/IKeychain.h>

namespace joynr
{
class KeychainImpl : public IKeychain
{
public:
    KeychainImpl(std::shared_ptr<const mococrw::X509Certificate> tlsCertificate,
                 std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey,
                 std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate);

    std::shared_ptr<const mococrw::X509Certificate> getTlsCertificate() const final;
    std::shared_ptr<const mococrw::AsymmetricPrivateKey> getTlsKey() const final;
    std::shared_ptr<const mococrw::X509Certificate> getTlsRootCertificate() const final;
    std::string getOwnerId() const final;

    static std::shared_ptr<IKeychain> createFromPEMFiles(
            const std::string& tlsCertificatePEMFilename,
            const std::string& tlsKeyPEMFilename,
            const std::string& tlsRootCertificatePEMFilename,
            const std::string& privateKeyPassword);

private:
    std::shared_ptr<const mococrw::X509Certificate> tlsCertificate;
    std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey;
    std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate;
};
} // namespace joynr

#endif // KEYCHAINIMPL_H
