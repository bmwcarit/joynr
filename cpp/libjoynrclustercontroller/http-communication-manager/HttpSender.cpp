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
#include "libjoynrclustercontroller/http-communication-manager/HttpSender.h"

#include <algorithm>
#include <cassert>
#include <chrono>

#include <curl/curl.h>

#include "joynr/exceptions/JoynrException.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Util.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "libjoynrclustercontroller/httpnetworking/HttpNetworking.h"
#include "libjoynrclustercontroller/httpnetworking/HttpResult.h"

namespace joynr
{

std::chrono::milliseconds HttpSender::MIN_ATTEMPT_TTL()
{
    return std::chrono::seconds(2);
}

std::int64_t HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL()
{
    return 3;
}

HttpSender::HttpSender(const BrokerUrl& brokerUrl,
                       std::chrono::milliseconds maxAttemptTtl,
                       std::chrono::milliseconds messageSendRetryInterval)
        : _brokerUrl(brokerUrl),
          _maxAttemptTtl(maxAttemptTtl),
          _messageSendRetryInterval(messageSendRetryInterval)
{
}

HttpSender::~HttpSender()
{
}

void HttpSender::sendMessage(
        const system::RoutingTypes::Address& destinationAddress,
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    const auto* channelAddress =
            dynamic_cast<const system::RoutingTypes::ChannelAddress*>(&destinationAddress);
    if (channelAddress == nullptr) {
        JOYNR_LOG_DEBUG(logger(), "Invalid destination address type provided");
        onFailure(exceptions::JoynrRuntimeException("Invalid destination address type provided"));
        return;
    }

    JOYNR_LOG_TRACE(logger(), "sendMessage: ...");

    auto startTime = std::chrono::system_clock::now();

    std::chrono::milliseconds remainingTtl = message->getExpiryDate().relativeFromNow();

    // A channelId can have several Url's. Hence, we cannot use up all the time we have for
    // testing just one (in case it is not available). So we use just a fraction, yet at
    // least MIN... and at most MAX... seconds.
    std::int64_t curlTimeout = std::max(
            remainingTtl.count() / HttpSender::FRACTION_OF_MESSAGE_TTL_USED_PER_CONNECTION_TRIAL(),
            HttpSender::MIN_ATTEMPT_TTL().count());

    JOYNR_LOG_TRACE(logger(),
                    "Sending message; url: {}, time left: {}",
                    toUrl(*channelAddress),
                    message->getExpiryDate().relativeFromNow().count());

    JOYNR_LOG_TRACE(logger(), "going to buildRequest");

    // TODO transmit message->getSerializedMessage() instead
    std::string serializedMessage = "FIX ME I AM EMPTY";
    HttpResult sendMessageResult = buildRequestAndSend(
            serializedMessage, toUrl(*channelAddress), std::chrono::milliseconds(curlTimeout));

    // Delay the next request if an error occurs
    auto now = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    std::chrono::milliseconds delay;
    if (_messageSendRetryInterval < timeDiff) {
        delay = std::chrono::milliseconds(10);
    } else {
        delay = _messageSendRetryInterval - timeDiff;
    }

    JOYNR_LOG_TRACE(
            logger(), "sendMessageResult.getStatusCode() = {}", sendMessageResult.getStatusCode());
    if (sendMessageResult.getStatusCode() != 201) {
        std::string body("NULL");
        if (!sendMessageResult.getBody().empty()) {
            body = sendMessageResult.getBody();
        }
        JOYNR_LOG_ERROR(logger(),
                        "sending message - fail; error message {}; contents {}; rescheduling "
                        "for retry...",
                        sendMessageResult.getErrorMessage(),
                        body);
        if (sendMessageResult.isCurlError()) {
            // curl error
            handleCurlError(sendMessageResult, delay, onFailure);
        } else {
            // http error
            handleHttpError(sendMessageResult, delay, onFailure);
        }
    } else {
        JOYNR_LOG_DEBUG(logger(),
                        "sending message - success; url: {} status code: {}",
                        toUrl(*channelAddress),
                        sendMessageResult.getStatusCode());
    }
}

HttpResult HttpSender::buildRequestAndSend(const std::string& data,
                                           const std::string& url,
                                           std::chrono::milliseconds curlTimeout)
{
    JOYNR_LOG_TRACE(logger(), "buildRequestAndSend.createHttpPostBuilder...");
    std::shared_ptr<IHttpPostBuilder> sendMessageRequestBuilder(
            HttpNetworking::getInstance()->createHttpPostBuilder(url));

    JOYNR_LOG_TRACE(logger(), "buildRequestAndSend.sendMessageRequestBuilder...");
    std::shared_ptr<HttpRequest> sendMessageRequest(
            sendMessageRequestBuilder->withContentType("application/json")
                    ->withTimeout(std::min(_maxAttemptTtl, curlTimeout))
                    ->postContent(data)
                    ->build());
    JOYNR_LOG_TRACE(logger(), "builtRequest");
    JOYNR_LOG_TRACE(logger(), "Sending Message: {}", util::truncateSerializedMessage(data));

    return sendMessageRequest->execute();
}

std::string HttpSender::toUrl(const system::RoutingTypes::ChannelAddress& channelAddress) const
{
    std::string result;
    result.reserve(256);

    result.append(channelAddress.getMessagingEndpointUrl());

    if (result.length() > 0 && result.back() != '/') {
        result.append("/");
    }

    result.append("message/");

    return result;
}

void HttpSender::handleCurlError(
        const HttpResult& sendMessageResult,
        const std::chrono::milliseconds& delay,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) const
{
    std::int32_t curlError = sendMessageResult.getCurlError();
    if (curlError == CURLcode::CURLE_HTTP_RETURNED_ERROR && sendMessageResult.getStatusCode()) {
        handleHttpError(sendMessageResult, delay, onFailure);
        return;
    }
    switch (sendMessageResult.getCurlError()) {
    // non recoverable errors
    case CURLcode::CURLE_UNSUPPORTED_PROTOCOL:   // 1
    case CURLcode::CURLE_URL_MALFORMAT:          // 3
    case CURLcode::CURLE_NOT_BUILT_IN:           // 4
    case CURLcode::CURLE_FTP_WEIRD_SERVER_REPLY: // 8
    case CURLcode::CURLE_REMOTE_ACCESS_DENIED:   // 9
    case CURLcode::CURLE_FTP_ACCEPT_FAILED:      // 10
    case CURLcode::CURLE_FTP_WEIRD_PASS_REPLY:   // 11
    case CURLcode::CURLE_FTP_WEIRD_PASV_REPLY:   // 13
    case CURLcode::CURLE_FTP_WEIRD_227_FORMAT:   // 14
    case CURLcode::CURLE_FTP_CANT_GET_HOST:      // 15
    case CURLcode::CURLE_FTP_COULDNT_SET_TYPE:   // 17
    case CURLcode::CURLE_PARTIAL_FILE:           // 18
    case CURLcode::CURLE_FTP_COULDNT_RETR_FILE:  // 19
    case CURLcode::CURLE_HTTP_RETURNED_ERROR:    // 22
    case CURLcode::CURLE_FTP_PORT_FAILED:        // 30
    case CURLcode::CURLE_FTP_COULDNT_USE_REST:   // 31
    case CURLcode::CURLE_RANGE_ERROR:            // 33
    case CURLcode::CURLE_BAD_DOWNLOAD_RESUME:    // 36
    case CURLcode::CURLE_FILE_COULDNT_READ_FILE: // 37
    case CURLcode::CURLE_LDAP_CANNOT_BIND:       // 38
    case CURLcode::CURLE_LDAP_SEARCH_FAILED:     // 39
    case CURLcode::CURLE_FUNCTION_NOT_FOUND:     // 41
    case CURLcode::CURLE_ABORTED_BY_CALLBACK:    // 42
    case CURLcode::CURLE_BAD_FUNCTION_ARGUMENT:  // 43
    case CURLcode::CURLE_INTERFACE_FAILED:       // 45
    case CURLcode::CURLE_TOO_MANY_REDIRECTS:     // 47
    case CURLcode::CURLE_UNKNOWN_OPTION:         // 48
    case CURLcode::CURLE_TELNET_OPTION_SYNTAX:   // 49
#if LIBCURL_VERSION_NUM < 0x073e00
    case CURLcode::CURLE_PEER_FAILED_VERIFICATION: // 51
#endif
    case CURLcode::CURLE_SSL_ENGINE_NOTFOUND:  // 53
    case CURLcode::CURLE_SSL_ENGINE_SETFAILED: // 54
    case CURLcode::CURLE_SSL_CERTPROBLEM:      // 58
    case CURLcode::CURLE_SSL_CIPHER:           // 59
#if LIBCURL_VERSION_NUM < 0x073e00
    case CURLcode::CURLE_SSL_CACERT: // 60
#else
    case CURLcode::CURLE_PEER_FAILED_VERIFICATION: // 60
#endif
    case CURLcode::CURLE_BAD_CONTENT_ENCODING:    // 61
    case CURLcode::CURLE_LDAP_INVALID_URL:        // 62
    case CURLcode::CURLE_FILESIZE_EXCEEDED:       // 63
    case CURLcode::CURLE_USE_SSL_FAILED:          // 64
    case CURLcode::CURLE_SSL_ENGINE_INITFAILED:   // 66
    case CURLcode::CURLE_LOGIN_DENIED:            // 67
    case CURLcode::CURLE_TFTP_NOTFOUND:           // 68
    case CURLcode::CURLE_TFTP_PERM:               // 69
    case CURLcode::CURLE_REMOTE_DISK_FULL:        // 70
    case CURLcode::CURLE_TFTP_ILLEGAL:            // 71
    case CURLcode::CURLE_TFTP_UNKNOWNID:          // 72
    case CURLcode::CURLE_REMOTE_FILE_EXISTS:      // 73
    case CURLcode::CURLE_TFTP_NOSUCHUSER:         // 74
    case CURLcode::CURLE_CONV_FAILED:             // 75
    case CURLcode::CURLE_CONV_REQD:               // 76
    case CURLcode::CURLE_SSL_CACERT_BADFILE:      // 77
    case CURLcode::CURLE_REMOTE_FILE_NOT_FOUND:   // 78
    case CURLcode::CURLE_SSH:                     // 79
    case CURLcode::CURLE_SSL_CRL_BADFILE:         // 82
    case CURLcode::CURLE_SSL_ISSUER_ERROR:        // 83
    case CURLcode::CURLE_FTP_PRET_FAILED:         // 84
    case CURLcode::CURLE_FTP_BAD_FILE_LIST:       // 87
    case CURLcode::CURLE_CHUNK_FAILED:            // 88
    case CURLcode::CURLE_NO_CONNECTION_AVAILABLE: // 89
    // CURLcode::CURLE_SSL_PINNEDPUBKEYNOTMATCH has been added in libcurl 7_39_0
    // The joynr docker containers use fedora 21 and the latest libcurl 7_37_0
    // from the fedora 21 repositories
    //    case CURLcode::CURLE_SSL_PINNEDPUBKEYNOTMATCH: // 90
    // CURLcode::CURLE_SSL_INVALIDCERTSTATUS has been added in libcurl 7_41_0
    // The joynr docker containers use fedora 21 and the latest libcurl 7_37_0
    // from the fedora 21 repositories
    //    case CURLcode::CURLE_SSL_INVALIDCERTSTATUS:    // 91
    case CURLcode::CURLE_OBSOLETE20: // NOT USED
    case CURLcode::CURLE_OBSOLETE24: // NOT USED
    case CURLcode::CURLE_OBSOLETE29: // NOT USED
    case CURLcode::CURLE_OBSOLETE32: // NOT USED
    case CURLcode::CURLE_OBSOLETE40: // NOT USED
    case CURLcode::CURLE_OBSOLETE44: // NOT USED
    case CURLcode::CURLE_OBSOLETE46: // NOT USED
    case CURLcode::CURLE_OBSOLETE50: // NOT USED
    case CURLcode::CURLE_OBSOLETE57: // NOT IN USE
    case CURLcode::CURL_LAST:        // 92: never use!
        onFailure(exceptions::JoynrMessageNotSentException(
                "sending message - fail; error message " + sendMessageResult.getErrorMessage()));
        break;

    // recoverable errors
    //    case CURLcode::CURLE_FAILED_INIT: //2
    //    case CURLcode::CURLE_COULDNT_RESOLVE_PROXY: //5
    //    case CURLcode::CURLE_COULDNT_RESOLVE_HOST: //6
    //    case CURLcode::CURLE_COULDNT_CONNECT: //7
    //    case CURLcode::CURLE_FTP_ACCEPT_TIMEOUT: //12
    //    case CURLcode::CURLE_HTTP2: //16
    //    case CURLcode::CURLE_QUOTE_ERROR: //21
    //    case CURLcode::CURLE_WRITE_ERROR: //23
    //    case CURLcode::CURLE_UPLOAD_FAILED: //25
    //    case CURLcode::CURLE_READ_ERROR: //26
    //    case CURLcode::CURLE_OUT_OF_MEMORY: //27: A memory allocation request failed.
    //        // This is serious badness and things are severely screwed up if this ever occurs.
    //    case CURLcode::CURLE_OPERATION_TIMEDOUT: //28
    //    case CURLcode::CURLE_HTTP_POST_ERROR: //34
    //    case CURLcode::CURLE_SSL_CONNECT_ERROR: //35
    //    case CURLcode::CURLE_GOT_NOTHING: //52
    //    case CURLcode::CURLE_SEND_ERROR: //55
    //    case CURLcode::CURLE_RECV_ERROR: //56
    //    case CURLcode::CURLE_SEND_FAIL_REWIND: //65
    //    case CURLcode::CURLE_SSL_SHUTDOWN_FAILED: //80
    //    case CURLcode::CURLE_AGAIN: //81
    //    case CURLcode::CURLE_RTSP_CSEQ_ERROR: //85
    //    case CURLcode::CURLE_RTSP_SESSION_ERROR: //86
    default:
        onFailure(exceptions::JoynrDelayMessageException(
                delay,
                "sending message - fail; error message " + sendMessageResult.getErrorMessage()));
        break;
    }
}
void HttpSender::handleHttpError(
        const HttpResult& sendMessageResult,
        const std::chrono::milliseconds& delay,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) const
{
    const std::int64_t statusCode = sendMessageResult.getStatusCode();
    if (statusCode >= 400) {
        switch (statusCode) {
        // non recoverable errors
        case 400:
        case 401:
        case 402:
        case 403:
        case 404:
        case 405:
        case 406:
        case 410:
        case 411:
        case 412:
        case 414:
        case 415:
        case 416:
        case 417:
        case 418:
        case 420:
        case 421:
        case 422:
        case 425:
        case 426:
        case 428:
        case 431:
        case 451:
            onFailure(exceptions::JoynrMessageNotSentException(
                    "sending message - fail; error message " +
                    sendMessageResult.getErrorMessage()));
            break;

        // recoverable errors
        //    case 407:
        //    case 408:
        //    case 409:
        //    case 413:
        //    case 423:
        //    case 424:
        //    case 429:
        //    case 500:
        //    case 502:
        //    case 503:
        //    case 504:
        //    case 507:
        //    case 509:
        default:
            onFailure(exceptions::JoynrDelayMessageException(
                    delay,
                    "sending message - fail; error message " +
                            sendMessageResult.getErrorMessage()));
            break;
        }
    } else {
        onFailure(exceptions::JoynrMessageNotSentException(
                "sending message - fail; error message " + sendMessageResult.getErrorMessage()));
    }
}

} // namespace joynr
