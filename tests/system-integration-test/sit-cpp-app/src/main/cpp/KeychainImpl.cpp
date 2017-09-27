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

#include "KeychainImpl.h"

#include <mococrw/x509.h>
#include <joynr/Util.h>

namespace joynr
{

KeychainImpl::KeychainImpl(std::shared_ptr<const mococrw::X509Certificate> tlsCertificate,
                           std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey,
                           std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate,
                           const std::string& ownerId)
{
    this->tlsCertificate = tlsCertificate;
    this->tlsKey = tlsKey;
    this->tlsRootCertificate = tlsRootCertificate;
    this->ownerId = ownerId;
}

std::shared_ptr<const mococrw::X509Certificate> KeychainImpl::getTlsCertificate() const
{
    return tlsCertificate;
}

std::shared_ptr<const mococrw::AsymmetricPrivateKey> KeychainImpl::getTlsKey() const
{
    return tlsKey;
}

std::shared_ptr<const mococrw::X509Certificate> KeychainImpl::getTlsRootCertificate() const
{
    return tlsRootCertificate;
}

std::string KeychainImpl::getOwnerId() const
{
    return ownerId;
}

std::shared_ptr<IKeychain> KeychainImpl::createFromPEMFiles(
        const std::string& tlsCertificatePEMFilename,
        const std::string& tlsKeyPEMFilename,
        const std::string& tlsRootCertificatePEMFilename,
        const std::string& ownerId,
        const std::string& privateKeyPassword)
{
    auto tlsCertificate = std::make_shared<const mococrw::X509Certificate>(
            mococrw::X509Certificate::fromPEMFile(tlsCertificatePEMFilename));
    auto tlsKey = std::make_shared<const mococrw::AsymmetricPrivateKey>(
            mococrw::AsymmetricPrivateKey::readPrivateKeyFromPEM(
                    joynr::util::loadStringFromFile(tlsKeyPEMFilename), privateKeyPassword));
    auto tlsRootCertificate = std::make_shared<const mococrw::X509Certificate>(
            mococrw::X509Certificate::fromPEMFile(tlsRootCertificatePEMFilename));

    auto result =
            std::make_shared<KeychainImpl>(tlsCertificate, tlsKey, tlsRootCertificate, ownerId);

    return result;
}

} // namespace joynr
