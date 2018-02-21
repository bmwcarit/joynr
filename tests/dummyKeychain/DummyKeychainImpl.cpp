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

#include "joynr/tests/DummyKeychainImpl.h"

#include <mococrw/x509.h>

#include <joynr/Util.h>

namespace joynr
{
namespace tests
{

DummyKeychainImpl::DummyKeychainImpl(std::shared_ptr<const mococrw::X509Certificate> tlsCertificate,
                           std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey,
                           std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate)
{
    this->tlsCertificate = tlsCertificate;
    this->tlsKey = tlsKey;
    this->tlsRootCertificate = tlsRootCertificate;
}

std::shared_ptr<const mococrw::X509Certificate> DummyKeychainImpl::getTlsCertificate() const
{
    return tlsCertificate;
}

std::shared_ptr<const mococrw::AsymmetricPrivateKey> DummyKeychainImpl::getTlsKey() const
{
    return tlsKey;
}

std::shared_ptr<const mococrw::X509Certificate> DummyKeychainImpl::getTlsRootCertificate() const
{
    return tlsRootCertificate;
}

std::string DummyKeychainImpl::getOwnerId() const
{
    return tlsCertificate->getSubjectDistinguishedName().commonName();
}

std::shared_ptr<IKeychain> DummyKeychainImpl::createFromPEMFiles(const joynr::tests::DummyKeyChainParameters& inputParams)
{
    auto tlsCertificate = std::make_shared<const mococrw::X509Certificate>(
            mococrw::X509Certificate::fromPEMFile(inputParams.pubCertFileName));
    auto tlsKey = std::make_shared<const mococrw::AsymmetricPrivateKey>(
            mococrw::AsymmetricPrivateKey::readPrivateKeyFromPEM(
                    joynr::util::loadStringFromFile(inputParams.privKeyFileName), inputParams.privKeyPassword));
    auto tlsRootCertificate = std::make_shared<const mococrw::X509Certificate>(
            mococrw::X509Certificate::fromPEMFile(inputParams.rootCertFileName));

    return std::make_shared<DummyKeychainImpl>(tlsCertificate, tlsKey, tlsRootCertificate);
}

} // namespace tests
} // namespace joynr
