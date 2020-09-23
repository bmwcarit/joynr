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

#include <cstddef>
#include <string>
#include <memory>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include <joynr/JoynrRuntime.h>
#include <joynr/Settings.h>
#include <joynr/WebSocketSettings.h>

#include <joynr/tests/DummyKeychainImpl.h>
#include <joynr/tests/DummyKeyChainParameters.h>

#include "../common/Enum.h"
#include "PerformanceConsumer.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

JOYNR_ENUM(SyncMode, (SYNC)(ASYNC));
JOYNR_ENUM(TestCase, (SEND_STRING)(SEND_BYTEARRAY)(SEND_BYTEARRAY_WITH_SIZE_TIMES_K)(SEND_STRUCT));

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYPC", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    namespace po = boost::program_options;

    std::string domain;
    std::size_t runs;
    SyncMode syncMode;
    TestCase testCase;
    std::size_t byteArraySize;
    std::size_t stringLength;
    bool useKeyChain = false;
    const std::string ccUrlForTLS("wss://localhost:4243");
    joynr::tests::DummyKeyChainParameters keyChainInputParams;

    auto validateRuns = [](std::size_t value) {
        if (value == 0) {
            throw po::validation_error(
                    po::validation_error::invalid_option_value, "runs", std::to_string(value));
        }
    };

    po::options_description desc("Available options");
    desc.add_options()("help,h", "produce help message")(
            "domain,d", po::value(&domain)->required(), "domain")(
            "runs,r", po::value(&runs)->required()->notifier(validateRuns), "number of runs")(
            "testCase,t",
            po::value(&testCase)->required(),
            "SEND_STRING|SEND_BYTEARRAY|SEND_BYTEARRAY_WITH_SIZE_TIMES_K|SEND_STRUCT")(
            "syncMode,s", po::value(&syncMode)->required(), "SYNC|ASYNC")(
            "stringLength,l", po::value(&stringLength)->required(), "length of string")(
            "byteArraySize,b", po::value(&byteArraySize)->required(), "size of bytearray")(
            "useKeychain",
            po::value(&useKeyChain)->default_value(false),
            "Should KeyChain be used? Default: false.")(
            "root-certificate",
            po::value(&keyChainInputParams.rootCertFileName),
            "Root certificate in PEM encoded format.")(
            "pub-certificate",
            po::value(&keyChainInputParams.pubCertFileName),
            "Public certificate in PEM encoded format.")(
            "private-key",
            po::value(&keyChainInputParams.privKeyFileName),
            "Private key in PEM encoded format.")(
            "private-key-pwd",
            po::value(&keyChainInputParams.privKeyPassword)->default_value(""),
            "Passsword of private key. Default: empty string.");

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }

        po::notify(vm);

        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        auto joynrSettings = std::make_unique<joynr::Settings>(
                (appDirectory + "/resources/performancetest-consumer.settings"));

        std::shared_ptr<joynr::IKeychain> keyChain;

        if (useKeyChain) {
            keyChain = joynr::tests::DummyKeychainImpl::createFromPEMFiles(keyChainInputParams);
            // also change settings file accordingly
            joynr::WebSocketSettings wsSettings(*joynrSettings);
            wsSettings.setClusterControllerMessagingUrl(ccUrlForTLS);
        }

        // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
        // implementation.
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
                [&](const joynr::exceptions::JoynrRuntimeException& exception) {
            std::cout << "Unexpected joynr runtime error occured: " << exception.getMessage()
                      << std::endl;
        };

        std::shared_ptr<joynr::JoynrRuntime> runtime(joynr::JoynrRuntime::createRuntime(
                std::move(joynrSettings), std::move(onFatalRuntimeError), std::move(keyChain)));

        std::unique_ptr<joynr::IPerformanceConsumer> consumer;

        if (syncMode == SyncMode::SYNC) {
            consumer = std::make_unique<joynr::SyncEchoConsumer>(
                    std::move(runtime), runs, stringLength, byteArraySize, domain);
        } else {
            consumer = std::make_unique<joynr::AsyncEchoConsumer>(
                    std::move(runtime), runs, stringLength, byteArraySize, domain);
        }

        switch (testCase) {
        case TestCase::SEND_BYTEARRAY:
            consumer->runByteArray();
            break;
        case TestCase::SEND_BYTEARRAY_WITH_SIZE_TIMES_K:
            consumer->runByteArrayWithSizeTimesK();
            break;
        case TestCase::SEND_STRING:
            consumer->runString();
            break;
        case TestCase::SEND_STRUCT:
            consumer->runStruct();
            break;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
