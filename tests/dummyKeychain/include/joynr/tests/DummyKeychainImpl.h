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

#ifndef DUMMYKEYCHAINIMPL_H
#define DUMMYKEYCHAINIMPL_H

#include <memory>
#include <string>

#include <joynr/IKeychain.h>

#include <joynr/tests/DummyKeyChainParameters.h>

namespace joynr
{
namespace tests
{
class DummyKeychainImpl : public IKeychain
{
public:
    DummyKeychainImpl(std::shared_ptr<const mococrw::X509Certificate> tlsCertificate,
                      std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey,
                      std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate);

    std::shared_ptr<const mococrw::X509Certificate> getTlsCertificate() const final;
    std::shared_ptr<const mococrw::AsymmetricPrivateKey> getTlsKey() const final;
    std::shared_ptr<const mococrw::X509Certificate> getTlsRootCertificate() const final;
    std::string getOwnerId() const final;

    static std::shared_ptr<IKeychain> createFromPEMFiles(
            const DummyKeyChainParameters& inputParams);

private:
    std::shared_ptr<const mococrw::X509Certificate> tlsCertificate;
    std::shared_ptr<const mococrw::AsymmetricPrivateKey> tlsKey;
    std::shared_ptr<const mococrw::X509Certificate> tlsRootCertificate;
    std::string ownerId;
};
} // namespace tests
} // namespace joynr

#endif // DUMMYKEYCHAINIMPL_H
