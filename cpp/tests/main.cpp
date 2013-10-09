/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include <QtTest/QTest>
#include <QtCore>
// Unittests
#include "MessagingTest.h"
#include "DispatcherTest.h"
#include "ClientCacheTest.h"
#include "CurlHandlePoolTest.h"
#include "LocalCapabilitiesDirectoryTest.h"
#include "ClientMultiCacheTest.h"
#include "TClientMultiCacheTest.h"
#include "PingTest.h"
#include "DirectoryTest.h"
// Integrationtests
#include "DispatcherIntegrationTest.h"
#include "MessagingIntegrationTest.h"
#include "CapabilitiesClientIntegrationTest.h"
// Systemtests
#include "ServiceDemoTest.h"
#include "PublisherRunnableTest.h"

/**
  * This executable runs the unit, integration and systemintegration tests.
  * To run all tests, just run the program with no arguments
  * To run a single test of your choice, run
  *     runUnitAndIntegrationAndSystemTests -single -MyFavoriteTest
  * Add your test and execution logic both in the unit | integration | system test
  * itself and here in the appropriate section.
  *
  */
// executable: runUnitAndIntegrationAndSystemTests
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    bool xmlFileOutput = false;
    int code = 0;
    bool testFailed = false;
    bool runAllTests = true;
    QString errorMessage;
    QTextStream out(stdout);

    if (args.contains("-xunitxml")) {
        xmlFileOutput = true;
    }
    if(args.contains("-single")){
        args.removeAll("-single");
        runAllTests = false;
    }
    //
    // Unittests
    //
    qRegisterMetaType<JoynrMessage>("joynr.JoynrMessage");
    qRegisterMetaType <MessageWithDecayTime> ("MessageWithDecayTime");
    bool runDirectoryTest = false;
    bool runDispatcherTest = false;
    bool runMessagingTest = false;
    bool runClientCacheTest = false;
    bool runCurlHandlePoolTest = false;
    bool runLocalCapabilitiesDirectoryTest = false;
    bool runClientMultiCacheTest = false;
    bool runtClientMultiCacheTest = false;
    bool runPingTest = false;
    bool runServiceDemoTest = false;
    bool runDispatcherIntegrationTest = false;
    bool runMessagingIntegrationTest = false;
    bool runCapabilitiesTest = false;


    DirectoryTest directoryTest;
    DispatcherTest dispatcherTest;
    MessagingTest messagingTest;
    ClientCacheTest clientCacheTest;
    CurlHandlePoolTest curlHandlePoolTest;
    LocalCapabilitiesDirectoryTest localCapabilitiesDirectoryTest;
    ClientMultiCacheTest clientMultiCacheTest;
    TClientMultiCacheTest tClientMultiCacheTest;
    PingTest pingTest;


    if(args.contains("-DirectoryTest")){
        args.removeAll("-DirectoryTest");
        runDirectoryTest = true;
    }

    if(args.contains("-PingTest")){
        args.removeAll("-PingTest");
        runPingTest = true;
    }

    if(args.contains("-tClientMultiCacheTest")){
        args.removeAll("-tClientMultiCacheTest");
        runtClientMultiCacheTest = true;
    }

    if(args.contains("-ClientMultiCacheTest")){
        args.removeAll("-ClientMultiCacheTest");
        runClientMultiCacheTest = true;
    }

    if(args.contains("-LocalCapabilitiesDirectoryTest")){
        args.removeAll("-LocalCapabilitiesDirectoryTest");
        runLocalCapabilitiesDirectoryTest = true;
    }

    if(args.contains("-CurlHandlePoolTest")){
        args.removeAll("-CurlHandlePoolTest");
        runCurlHandlePoolTest = true;
    }

    if(args.contains("-DispatcherTest")){
        args.removeAll("-DispatcherTest");
        runDispatcherTest = true;
    }

    if(args.contains("-MessagingTest")){
        args.removeAll("-MessagingTest");
        runMessagingTest = true;
    }

    if(args.contains("-ClientCacheTest")){
        args.removeAll("-ClientCacheTest");
        runClientCacheTest = true;
    }

    if (args.contains("-xunitxml")) {
        xmlFileOutput = true;
    }

    if(args.contains("-DispatcherIntegrationTest")){
        args.removeAll("-DispatcherIntegrationTest");
        runDispatcherIntegrationTest = true;
    }
    if (args.contains("-CapabilitiesTest")){
        args.removeAll("-CapabilitiesTest");
        runCapabilitiesTest = true;
    }
    if(args.contains("-MessagingIntegrationTest")){
        args.removeAll("-MessagingIntegrationTest");
        runMessagingIntegrationTest = true;
    }

    if(args.contains("-ServiceDemoTest")){
        args.removeAll("-ServiceDemoTest");
        runServiceDemoTest = true;
    }

    if(runAllTests || runDirectoryTest){
        QStringList directoryTestArgs(args);
        if (xmlFileOutput) {
            directoryTestArgs.append("-o");
            directoryTestArgs.append("DirectoryTest-results.xml");
        }
        code = QTest::qExec((QObject*)&directoryTest, directoryTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: DirectoryTest \n";
        }
    }

    if(runAllTests || runtClientMultiCacheTest){
        QStringList tClientMultiCacheTestArgs(args);
        if (xmlFileOutput) {
            tClientMultiCacheTestArgs.append("-o");
            tClientMultiCacheTestArgs.append("TClientMultiCacheTest-results.xml");
        }
        code = QTest::qExec((QObject*)&tClientMultiCacheTest,   tClientMultiCacheTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: tClientMultiCacheTest \n";
        }
    }

    if(runAllTests || runClientMultiCacheTest){
        QStringList clientMultiCacheTestArgs(args);
        if (xmlFileOutput) {
            clientMultiCacheTestArgs.append("-o");
            clientMultiCacheTestArgs.append("ClientMultiCacheTest-results.xml");
        }
        code = QTest::qExec((QObject*)&clientMultiCacheTest,  clientMultiCacheTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: clientMultiCacheTest \n";
        }
    }

    if(runAllTests || runDispatcherTest){
        QStringList dispatcherTestArgs(args);
        if (xmlFileOutput) {
            dispatcherTestArgs.append("-o");
            dispatcherTestArgs.append("DispatcherTest-results.xml");
        }
        code = QTest::qExec((QObject*)&dispatcherTest, dispatcherTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: dispatcherTest \n";
        }
    }

    if(runAllTests || runMessagingTest){
        QStringList messagingTestArgs(args);
        if (xmlFileOutput) {
            messagingTestArgs.append("-o");
            messagingTestArgs.append("MessagingTest-results.xml");
        }
        code = QTest::qExec((QObject*)&messagingTest, messagingTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: messagingTest \n";
        }
    }

    if(runAllTests || runCurlHandlePoolTest){
        QStringList curlHandlePoolTestArgs(args);
        if (xmlFileOutput) {
            curlHandlePoolTestArgs.append("-o");
            curlHandlePoolTestArgs.append("CurlHandlePoolTest-results.xml");
        }
        code = QTest::qExec((QObject*)&curlHandlePoolTest, curlHandlePoolTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: curlHandlePoolTest \n";
        }
    }

    if(runAllTests || runLocalCapabilitiesDirectoryTest){
        QStringList localCapabilitiesDirectoryTestArgs(args);
        if (xmlFileOutput) {
            localCapabilitiesDirectoryTestArgs.append("-o");
            localCapabilitiesDirectoryTestArgs.append("LocalCapabilitiesDirectoryTest-results.xml");
         }
        code = QTest::qExec((QObject*)&localCapabilitiesDirectoryTest, localCapabilitiesDirectoryTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: localCapabilitiesDirectoryTest \n";
        }
    }

    if(runAllTests || runClientCacheTest){
        QStringList clientCacheTestArgs(args);
        if (xmlFileOutput) {
            clientCacheTestArgs.append("-o");
            clientCacheTestArgs.append("ClientCacheTest-results.xml");
        }
        code = QTest::qExec((QObject*)&clientCacheTest,  clientCacheTestArgs);
        if (code != 0) {
            testFailed = true;
           errorMessage += " Failed at: clientCacheTest \n";
        }
    }

    // This is only used to manually test the ping executable
    if ( runPingTest ){
        QStringList pingTestArgs(args);
        code = QTest::qExec((QObject*)&pingTest, pingTestArgs);
    }

    if(testFailed){
        out << "*****************************************************************" << endl;
        out << "* UNIT TEST FAILED " << endl;
        out << "*****************************************************************" << endl;
        out << errorMessage << endl;
        out << "*****************************************************************" << endl;
    }else{
        out << "*****************************************************************" << endl;
        out << "* ALL UNIT TESTS CLEAR " << endl;
        out << "*****************************************************************" << endl;
    }
    //
    //Integrationtests
    //
    testFailed = false;
    errorMessage = "";
    DispatcherIntegrationTest dispatcherIntegrationTest;
    MessagingIntegrationTest messagingIntegrationTest;
    CapabilitiesClientIntegrationTest capabilitiesClientIntegrationTest;

    if(runAllTests || runDispatcherIntegrationTest){
        QStringList dispatcherIntegrationTestArgs(args);
        if (xmlFileOutput) {
            dispatcherIntegrationTestArgs.append("-o");
            dispatcherIntegrationTestArgs.append("DispatcherIntegrationTest-results.xml");
        }
        code = QTest::qExec((QObject*)&dispatcherIntegrationTest, dispatcherIntegrationTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: dispatcherIntegrationTest \n";
        }
    }


    if(runAllTests || runCapabilitiesTest){
        QStringList CapabilitiesClientIntegrationTestArgs(args);
        if (xmlFileOutput) {
            CapabilitiesClientIntegrationTestArgs.append("-o");
            CapabilitiesClientIntegrationTestArgs.append("CapabilitiesClientIntegrationTest-results.xml");
        }
        code = QTest::qExec((QObject*)&capabilitiesClientIntegrationTest, CapabilitiesClientIntegrationTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: capabilitiesClientIntegrationTest \n";
        }
    }

    if(runAllTests || runMessagingIntegrationTest){
        QStringList MessagingIntegrationTestArgs(args);
        if (xmlFileOutput) {
            MessagingIntegrationTestArgs.append("-o");
            MessagingIntegrationTestArgs.append("MessagingIntegrationTest-results.xml");
        }
        code = QTest::qExec((QObject*)&messagingIntegrationTest, MessagingIntegrationTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: messagingIntegrationTest \n";
        }
    }

    if(testFailed){
        out << "*****************************************************************" << endl;
        out << "* INTEGRATION TEST FAILED " << endl;
        out << "*****************************************************************" << endl;
        out << errorMessage << endl;
        out << "*****************************************************************" << endl;
    }else{
        out << "*****************************************************************" << endl;
        out << "* ALL INTEGRATION TESTS CLEAR " << endl;
        out << "*****************************************************************" << endl;
    }
    //
    // SystemIntegrationTest
    //
    testFailed = false;
    errorMessage = "";
    code = 0;
    ServiceDemoTest serviceDemoTest(&app);

    if(runAllTests || runServiceDemoTest){
        QStringList ServiceDemoTestArgs(args);
        if (xmlFileOutput) {
            ServiceDemoTestArgs.append("-o");
            ServiceDemoTestArgs.append("ServiceDemoTestArgs-results.xml");
        }
        code = QTest::qExec((QObject*)&serviceDemoTest, ServiceDemoTestArgs);
        if (code != 0) {
            testFailed = true;
            errorMessage += " Failed at: ServiceDemoTest \n";
        }
    }

    if(testFailed){
        out << "*****************************************************************" << endl;
        out << "* SYSTEM TEST FAILED " << endl;
        out << "*****************************************************************" << endl;
        out << errorMessage << endl;
        out << "*****************************************************************" << endl;
    }else{
        out << "*****************************************************************" << endl;
        out << "* ALL SYSTEM TESTS CLEAR " << endl;
        out << "*****************************************************************" << endl;
    }

    return code;
}



