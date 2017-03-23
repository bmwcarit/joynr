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

namespace joynr
{
WebSocketPpClientTLS::WebSocketPpClientTLS(const WebSocketSettings& wsSettings,
                                           boost::asio::io_service& ioService,
                                           const std::string& caPemFile,
                                           const std::string& certPemFile,
                                           const std::string& privateKeyPemFile)
        : WebSocketPpClient<websocketpp::config::asio_tls_client>(wsSettings, ioService)
{
    endpoint.set_tls_init_handler([this, caPemFile, certPemFile, privateKeyPemFile](
                                          ConnectionHandle hdl) -> std::shared_ptr<SSLContext> {
        std::ignore = hdl;
        return createSSLContext(caPemFile, certPemFile, privateKeyPemFile, logger);
    });
}

std::shared_ptr<WebSocketPpClientTLS::SSLContext> WebSocketPpClientTLS::createSSLContext(
        const std::string& caPemFile,
        const std::string& certPemFile,
        const std::string& privateKeyPemFile,
        Logger& logger)
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
