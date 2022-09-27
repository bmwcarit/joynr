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
#ifndef IKEYCHAIN_H
#define IKEYCHAIN_H

#include <memory>
#include <string>

namespace mococrw
{
class AsymmetricKeypair;
class X509Certificate;
using AsymmetricPrivateKey = AsymmetricKeypair;
} // namespace mococrw

namespace joynr
{

class IKeychain
{
public:
    virtual ~IKeychain() = default;

    /**
     * @brief Get the certificate that should be used as client certitificate for the
     *        local TLS connection.
     * @return A shared pointer to a certificate object for the certificate that should
     *         be used for local TLS connections.
     */
    virtual std::shared_ptr<const mococrw::X509Certificate> getTlsCertificate() const = 0;

    /**
     * @brief Get the private key that should be used for local TLS client authentication.
     * @return A shared pointer to the private key object that should be used
     *         for local TLS authenticiation.
     */
    virtual std::shared_ptr<const mococrw::AsymmetricPrivateKey> getTlsKey() const = 0;

    /**
     * @return The TLS root certificate.
     */
    virtual std::shared_ptr<const mococrw::X509Certificate> getTlsRootCertificate() const = 0;

    /**
     * @brief Get the id of the certificate's owner.
     * @return A string containing the id of the certificate's owner.
     */
    virtual std::string getOwnerId() const = 0;
};

} // namespace joynr
#endif // IKEYCHAIN_H
