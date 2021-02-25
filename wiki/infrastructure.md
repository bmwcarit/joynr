# joynr infrastructure

In order for joynr participants to be able to communicate with
each other, a few infrastructure components are necessary for
them to be able to discover each other and correctly route
messages between the participants.

This documents describes some examples of setting up these
infrastructure components.

## Components

The following component is necessary in order to set up a
joynr environment:

* Global Discovery service (GlobalCapabilitiesDirectory)
   * A joynr based application with which the participants
     register their providers and query to discover other
     participants

Additionally, you have to ensure that any required transport
layer components, such as MQTT brokers, are set up.

## Java

This section describes configuring and starting the JAVA version of the Discovery service.

GlobalCapabilitiesDirectory: see [GCD Readme](../java/backend-services/capabilities-directory/README.md).

## JEE

This section describes configuring and installing the JEE
versions of the Discovery service in a Glassfish / Payara container.
This document assumes you are familiar with the basics of installing and setting up the container
itself.

Please note that as two applications use the joynr JEE integration
layer, all the documentation in the [JEE Developer Guide](jee.md)
is relevant here, also. Specifically, setting up the managed
scheduler executor service is mandatory.

The documentation assumes you have already installed Glassfish /
Payara and will reference the root of the installation as
`${GF_HOME}`.

### Creating the domain and an executor service

A recommended first step is to copy the the `domain1` directory
to create a fresh domain for the joynr infrastructure:

    cd ${GF_HOME}/glassfish/domains
    cp -R domain1 joynr-infrastructure
    cd joynr-infrastucture/config
    sed -i 's/domain1/joynr-infrastructure/' domain.xml

Next, we start the new domain and create the executor service:

    cd ${GF_HOME}
    bin/asadmin start-domain joynr-infrastructure
    bin/asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor

>Note the `--corepoolsize=100` option. The default will only create one thread, which can lead to
>blocking.

### Database

The Discovery service requires a database to store its data.
Which database you use is principally up to you, but you may
have to customise the schema creation if the standard JPA
mechanisms don't work with your database of choice.

This document will describe setting up the Derby database service
which is shipped with Glassfish / Payara.

    cd ${GF_HOME}
    bin/asadmin create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool
    bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS
    bin/asadmin start-database

The last command starts the local Derby database instance. This required
before deploying the applications, and also necesary before starting the
applications. Remember to start the database each time before starting
the application server subsequently.

#### Other databases

In a production environment, you most likely won't want to use the
Derby database, but instead an enterprise level database such as
PostgreSQL, Oracle, DB2, etc. In that case, note that you might
have to alter the `persistence.xml` files of the project
[Discovery Directory JEE](../java/backend-services/discovery-directory-jee/src/main/resources/META-INF/persistence.xml)
in order to match your database.

You also might not want to use the automatic JPA schema creation in that
case, and instead manually maintain the schema or use something like
[Flyway](https://flywaydb.org). Both of these issues are outside the
scope of this guide and are not further documented here.

Of course, the `asadmin` command documented above for creating the
JDBC connection pool will also not be relevant when using a different
database system. See the Glassfish / Payara documentation on how to set
up the connection pool for your database.
The commands for creating the data sources for the connection pool should
be re-usable as is, assuming you named your connection pool `JoynrPool`
as above.

### Deploying the applications

This guide assumes you have checked out the joynr source code and have
built the entire project, so that the build artifacts for the Discovery
service is present on your local drive.

`${JOYNR_HOME}` is used to denote the root directory to which you checked
out the source code.
`${JOYNR_VERSION}` is used to denote the version of joynr you have checked
out and which appears as part of the filename for the WAR files created
by the build.

#### Single backend (default)

    cd ${GF_HOME}
    bin/asadmin deploy ${JOYNR_HOME}/java/backend-services/discovery-directory-jee/target/discovery-directory-jee-${JOYNR_VERSION}.war

#### Multiple backends

See [multiple backends guide](multiple-backends.md) for details about the multiple backends feature.

You can find an example for setting up a JEE based joynr infrastructure for two backends as Docker
images as part of the system integration tests:
* [shared database](../tests/system-integration-test/docker/joynr-backend-jee-db)
* [GCD in first backend](../tests/system-integration-test/docker/joynr-backend-jee-1)
* [GCD in second backend](../tests/system-integration-test/docker/joynr-backend-jee-2)

### Logging

If you need to see more detail about communication, or if you are
experiencing problems starting up the applications and want more details
about what is going wrong, then you can set the log levels as follows:

    cd ${GF_HOME}
    bin/asadmin set-log-levels io.joynr=FINE

Of course, you can be more specific about which joynr packages to log
at which level. Also, you can switch logging to `FINEST` for even more
log details.
