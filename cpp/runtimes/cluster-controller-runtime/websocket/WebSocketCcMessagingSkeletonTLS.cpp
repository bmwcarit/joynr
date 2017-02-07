/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "runtimes/cluster-controller-runtime/websocket/WebSocketCcMessagingSkeletonTLS.h"

#include <openssl/ssl.h>

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
        : WebSocketCcMessagingSkeleton<websocketpp::config::asio_tls>(ioService,
                                                                      messageRouter,
                                                                      messagingStubFactory)
{
    // ensure that OpenSSL is correctly initialized
    ::SSL_library_init();
    ::SSL_load_error_strings();
    ::OpenSSL_add_all_algorithms();

    endpoint.set_tls_init_handler([this, caPemFile, certPemFile, privateKeyPemFile](
                                          ConnectionHandle hdl) -> std::shared_ptr<SSLContext> {
        std::ignore = hdl;
        return createSSLContext(caPemFile, certPemFile, privateKeyPemFile);
    });

    startAccept(serverAddress.getPort());
}

std::shared_ptr<WebSocketCcMessagingSkeletonTLS::SSLContext> WebSocketCcMessagingSkeletonTLS::
        createSSLContext(const std::string& caPemFile,
                         const std::string& certPemFile,
                         const std::string& privateKeyPemFile)
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
    } catch (boost::system::system_error& e) {
        JOYNR_LOG_ERROR(logger, "Failed to initialize TLS session {}", e.what());
        return nullptr;
    }

    return sslContext;
}

} // namespace joynr
