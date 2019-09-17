/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2017 BMW Car IT GmbH
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

#include "WebSocketPpClientTLS.h"

#include <mococrw/x509.h>
#include <mococrw/key.h>
#include <mococrw/distinguished_name.h>
#include <mococrw/openssl_lib.h>
#include <mococrw/openssl_wrap.h>

#include "joynr/IKeychain.h"

namespace joynr
{
WebSocketPpClientTLS::WebSocketPpClientTLS(const WebSocketSettings& wsSettings,
                                           boost::asio::io_service& ioService,
                                           std::shared_ptr<joynr::IKeychain> keyChain)
        : WebSocketPpClient<websocketpp::config::asio_tls_client>(wsSettings, ioService),
          useEncryptedTls{wsSettings.getEncryptedTlsUsage()}
{
    _endpoint.set_tls_init_handler(
            [this, keyChain](ConnectionHandle hdl) -> std::shared_ptr<SSLContext> {
                std::ignore = hdl;
                return createSSLContext(keyChain);
            });
}

std::shared_ptr<WebSocketPpClientTLS::SSLContext> WebSocketPpClientTLS::createSSLContext(
        std::shared_ptr<joynr::IKeychain> keyChain)
{
    std::shared_ptr<SSLContext> sslContext;

    const std::string password = "";
    const std::string certificatePem = keyChain->getTlsCertificate()->toPEM();
    const std::string privateKeyPem = keyChain->getTlsKey()->privateKeyToPem(password);
    const std::string certificateAuthorityCertificatePem =
            keyChain->getTlsRootCertificate()->toPEM();

    const boost::asio::const_buffer certificatePemBuffer(
            certificatePem.data(), certificatePem.length());
    const boost::asio::const_buffer privateKeyPemBuffer(
            privateKeyPem.data(), privateKeyPem.length());
    const boost::asio::const_buffer certificateAuthorityCertificatePemBuffer(
            certificateAuthorityCertificatePem.data(), certificateAuthorityCertificatePem.length());

    try {
        sslContext = std::make_shared<SSLContext>(SSLContext::tlsv12);

        sslContext->set_verify_mode(websocketpp::lib::asio::ssl::verify_peer |
                                    websocketpp::lib::asio::ssl::verify_fail_if_no_peer_cert);
        sslContext->set_options(SSLContext::single_dh_use | SSLContext::no_sslv2 |
                                SSLContext::no_sslv3 | SSLContext::no_tlsv1 |
                                SSLContext::no_tlsv1_1 | SSLContext::no_compression);

        sslContext->add_certificate_authority(certificateAuthorityCertificatePemBuffer);
        sslContext->use_private_key(privateKeyPemBuffer, WebSocketPpClientTLS::SSLContext::pem);
        sslContext->use_certificate(certificatePemBuffer, WebSocketPpClientTLS::SSLContext::pem);

        using VerifyContext = websocketpp::lib::asio::ssl::verify_context;
        auto clientCertCheck = [](bool preverified, VerifyContext& ctx) {
            JOYNR_LOG_TRACE(logger(), "clientCertCheck: preverified = {}", preverified);
            std::string errStr(mococrw::openssl::lib::OpenSSLLib::SSL_X509_verify_cert_error_string(
                    mococrw::openssl::lib::OpenSSLLib::SSL_X509_STORE_CTX_get_error(
                            ctx.native_handle())));
            JOYNR_LOG_TRACE(logger(), "clientCertCheck: errStr = {}", errStr);
            return preverified;
        };
        sslContext->set_verify_callback(std::move(clientCertCheck));
    } catch (boost::system::system_error& e) {
        JOYNR_LOG_FATAL(logger(), "Failed to initialize TLS session {}", e.what());
        return nullptr;
    }

    if (!useEncryptedTls) {
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
        SSL_CTX_set_security_level(sslContext->native_handle(), 0);
        if (SSL_CTX_get_security_level(sslContext->native_handle()) != 0) {
            JOYNR_LOG_FATAL(
                    logger(), "Failed to initialize TLS session: Could not set security level 0");
            return nullptr;
        }
#endif
        int opensslResult = SSL_CTX_set_cipher_list(sslContext->native_handle(), "eNULL");
        if (opensslResult == 0) {
            JOYNR_LOG_FATAL(
                    logger(),
                    "[DEPRECATED] Failed to initialize TLS session: Could not set eNULL cipher");
            return nullptr;
        }
        JOYNR_LOG_WARN(logger(), "[DEPRECATED] TLS session: Set cipher list to eNULL");
    }

    return sslContext;
}

} // namespace joynr
