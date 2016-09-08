****************************
How to run the test manually
****************************

Please replace all occurrences of $JOYNR_BINARY_DIR, $JOYNR_SRC_DIR,
$PATH_TO_JAR, $VERSION, $DOMAIN etc. by values suitable for your system.

===============
Expected output
===============

If the test runs ok, the console output of the consumer application should contain
the following lines (text is shown indented here for better reading):

    SIT RESULT success: ... -> domain (1 + 1 = 2)

Note that other output from libjoynr may still follow these lines.

The consumer application will terminate with exit status 0 in case the
tests have been successful, and with an exit status != 0 otherwise.

==========
Standalone
==========

--------------------------------------------
With Websocket over a C++ cluster-controller
--------------------------------------------

# start cluster-controller (should keep on running until getting killed from outside)
cd $JOYNR_BINARY_DIR
./cluster-controller

# in another shell start provider (should keep on running until getting killed from outside)
java -cp $PATH_TO_JAR/sit-java-app-$VERSION-jar-with-dependencies.jar io.joynr.systemintegrationtest.ProviderApplication $DOMAIN [runForever]

# in another shell start consumer (should exit)
java -cp $PATH_TO_JAR/sit-java-app-$VERSION-jar-with-dependencies.jar io.joynr.systemintegrationtest.ConsumerApplication $DOMAIN

==========================
With source tree and Maven
==========================

This assumes that joynr has been fully built for C++ and Java.

--------------------------------------------
With Websocket over a C++ cluster-controller
--------------------------------------------

# start cluster-controller (should keep on running until getting killed from outside)
cd $JOYNR_BINARY_DIR
./cluster-controller

# in another shell start provider (should keep on running until getting killed from outside)
cd $JOYNR_SRC_DIR/tests/system-integration-test/sit-java-app
mvn exec:java -Dexec.mainClass="io.joynr.systemintegrationtest.ProviderApplication" -Dexec.args="$DOMAIN [runForever]"

# in another shell start consumer (should exit)
cd $$JOYNR_SRC_DIR/tests/system-integration-test/sit-java-app
mvn exec:java -Dexec.mainClass="io.joynr.systemintegrationtest.ConsumerApplication" -Dexec.args="$DOMAIN"

# this might cause some debug output and should exit after a while with status 0 if successful.
