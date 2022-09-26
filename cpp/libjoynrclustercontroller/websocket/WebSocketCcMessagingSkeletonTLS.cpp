/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "WebSocketCcMessagingSkeletonTLS.h"

#include <mutex>
#include <utility>

#include <mococrw/distinguished_name.h>
#include <mococrw/openssl_lib.h>
#include <mococrw/openssl_wrap.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/Logger.h"
#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{
WebSocketCcMessagingSkeletonTLS::WebSocketCcMessagingSkeletonTLS(
        boost::asio::io_service& ioService,
        std::shared_ptr<IMessageRouter> messageRouter,
        std::shared_ptr<WebSocketMessagingStubFactory> messagingStubFactory,
        const system::RoutingTypes::WebSocketAddress& serverAddress,
        const std::string& caPemFile,
        const std::string& certPemFile,
        const std::string& privateKeyPemFile)
        : WebSocketCcMessagingSkeleton<websocketpp::config::asio_tls>(
                  ioService,
                  std::move(messageRouter),
                  std::move(messagingStubFactory),
                  serverAddress.getPort()),
          _caPemFile(caPemFile),
          _certPemFile(certPemFile),
          _privateKeyPemFile(privateKeyPemFile)
{
}

void WebSocketCcMessagingSkeletonTLS::init()
{
    WebSocketCcMessagingSkeleton<websocketpp::config::asio_tls>::init();

    // ensure that OpenSSL is correctly initialized
    ::SSL_library_init();
    ::SSL_load_error_strings();
    ::OpenSSL_add_all_algorithms();

    _endpoint.set_tls_init_handler(
            [thisWeakPtr = joynr::util::as_weak_ptr(
                     std::dynamic_pointer_cast<WebSocketCcMessagingSkeletonTLS>(
                             this->shared_from_this()))](
                    ConnectionHandle hdl) -> std::shared_ptr<SSLContext> {
                if (auto thisSharedPtr = thisWeakPtr.lock()) {
                    return thisSharedPtr->createSSLContext(thisSharedPtr->_caPemFile,
                                                           thisSharedPtr->_certPemFile,
                                                           thisSharedPtr->_privateKeyPemFile,
                                                           std::move(hdl));
                } else {
                    return nullptr;
                }
            });

    startAccept();
}

bool WebSocketCcMessagingSkeletonTLS::preprocessIncomingMessage(
        std::shared_ptr<ImmutableMessage> message)
{
    smrf::ByteArrayView signature;
    try {
        signature = message->getSignature();
    } catch (smrf::EncodingException& error) {
        JOYNR_LOG_ERROR(logger(),
                        "Validation of message with ID {} failed: {}",
                        message->getId(),
                        error.what());
        return false;
    }

    std::string signatureString(reinterpret_cast<const char*>(signature.data()), signature.size());
    JOYNR_LOG_TRACE(logger(), "Received message with signature: {}", signatureString);
    message->setCreator(signatureString);

    return true;
}

bool WebSocketCcMessagingSkeletonTLS::validateIncomingMessage(
        const ConnectionHandle& hdl,
        std::shared_ptr<ImmutableMessage> message)
{
    std::lock_guard<std::mutex> lock1(_clientsMutex);

    auto it = _clients.find(hdl);
    if (it == _clients.cend()) {
        // This should never be the case
        JOYNR_LOG_FATAL(logger(),
                        "Clients map contains no entry for connection/ConnectionHandle of incoming "
                        "message with ID {}.",
                        message->getId());
        return false;
    }
    const std::string& expectedOwnerId = it->second._ownerId;
    if (expectedOwnerId.empty()) {
        // This should never happen because the ownerId is already checked in sslContext verify
        // callback
        JOYNR_LOG_ERROR(logger(), "OwnerId (common name) of the TLS certificate is empty.");
        return false;
    }

    if (expectedOwnerId != message->getCreator()) {
        JOYNR_LOG_ERROR(logger(),
                        "Validation of message with ID {} failed: invalid signature. "
                        "Message will be dropped.",
                        message->getId());
        return false;
    }
    return true;
}

std::shared_ptr<WebSocketCcMessagingSkeletonTLS::SSLContext> WebSocketCcMessagingSkeletonTLS::
        createSSLContext(const std::string& caPemFile,
                         const std::string& certPemFile,
                         const std::string& privateKeyPemFile,
                         ConnectionHandle hdl)
{
    std::shared_ptr<SSLContext> sslContext;

    try {
        sslContext = std::make_shared<SSLContext>(SSLContext::tlsv12);

        sslContext->set_verify_mode(websocketpp::lib::asio::ssl::verify_peer |
                                    websocketpp::lib::asio::ssl::verify_fail_if_no_peer_cert);
        sslContext->set_options(SSLContext::single_dh_use | SSLContext::no_sslv2 |
                                SSLContext::no_sslv3 | SSLContext::no_tlsv1 |
                                SSLContext::no_tlsv1_1 | SSLContext::no_compression);

        sslContext->load_verify_file(caPemFile);
        sslContext->use_certificate_file(certPemFile, SSLContext::pem);
        sslContext->use_private_key_file(privateKeyPemFile, SSLContext::pem);

        using VerifyContext = websocketpp::lib::asio::ssl::verify_context;

        auto getCNFromCertificate = [thisWeakPtr = joynr::util::as_weak_ptr(
                                             std::dynamic_pointer_cast<
                                                     WebSocketCcMessagingSkeletonTLS>(
                                                     this->shared_from_this())),
                                     hdl = std::move(hdl)](bool preverified, VerifyContext& ctx) {
            JOYNR_LOG_TRACE(logger(), "getCNFromCertificate: preverified = {}", preverified);
            if (auto thisSharedPtr = thisWeakPtr.lock()) {
                // getting cert out of the verification context
                X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

                std::string errStr(
                        mococrw::openssl::lib::OpenSSLLib::SSL_X509_verify_cert_error_string(
                                mococrw::openssl::lib::OpenSSLLib::SSL_X509_STORE_CTX_get_error(
                                        ctx.native_handle())));
                JOYNR_LOG_TRACE(logger(), "getCNFromCertificate: errStr = {}", errStr);

                // extracting ownerId out of cert
                X509_NAME* certSubName = mococrw::openssl::_X509_get_subject_name(cert);
                mococrw::DistinguishedName distinguishedName =
                        mococrw::DistinguishedName::fromX509Name(certSubName);
                const std::string ownerId(distinguishedName.commonName());
                if (ownerId.empty()) {
                    JOYNR_LOG_ERROR(logger(),
                                    "Rejecting secure websocket connection because the ownerId "
                                    "(common name) of the TLS client certificate is empty.");
                    return false;
                }

                // mapping the connection handler to the ownerId in clients map
                joynr::system::RoutingTypes::WebSocketClientAddress clientAddress;
                auto certEntry = CertEntry(std::move(clientAddress), std::move(ownerId));
                std::lock_guard<std::mutex> lock2(thisSharedPtr->_clientsMutex);
                thisSharedPtr->_clients[std::move(hdl)] = std::move(certEntry);
                return preverified;
            } else {
                JOYNR_LOG_ERROR(logger(),
                                "Rejecting secure websocket connection because the "
                                "WebSocketCcMessagingSkeletonTLS "
                                "is no longer available.");
                return false;
            }
        };

        // read ownerId of client's certificate and store it in clients map
        sslContext->set_verify_callback(std::move(getCNFromCertificate));

    } catch (boost::system::system_error& e) {
        JOYNR_LOG_FATAL(logger(), "Failed to initialize TLS session {}", e.what());
        return nullptr;
    }

    // ensure backward compatibility
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    const char* cipherList = "ALL:!COMPLEMENTOFDEFAULT:!eNULL";
#else
    const char* cipherList = "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2";
#endif

    if (SSL_CTX_set_cipher_list(sslContext->native_handle(), cipherList) == 0) {
        JOYNR_LOG_FATAL(logger(),
                        "Failed to initialize TLS session: Could not set cipher list to {}",
                        std::string(cipherList));
        return nullptr;
    }
    JOYNR_LOG_DEBUG(logger(), "TLS session: Set cipher list to {}", std::string(cipherList));

    return sslContext;
}

} // namespace joynr
