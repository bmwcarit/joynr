# JEE Discovery Directory

The JEE discovery directory application is an example of how the joynr discovery
directory service can be implemented for deployment on Java Enterprise Edition containers.
This will allow the service to be clustered and to use enterprise level databases
such as Oracle or DB2 for persistence of the discovery data. It can also be better
integrated into enterprise infrastructures.

It is recommended that you use the HiveMQ specific shared subscription ability for
clustering this application.

The `persistence.xml` is configured so that you are only required to provide a datasource
resource configured under the JNDI name `joynr/DiscoveryDirectoryDS`. Below is an
example of how to configure that for Payara / Glassfish 4.

Although this version of the discovery directory can be used as-is in principal, note
that it is only intended to be an example of how this service could be implemented for
a JEE environment, and has not been fully tested / load tested for use in production.

## Configuration

When deploying the JEE Discovery Directory to your own environment, you'll most likely
want to change some of the configuration parameters to match your setup. See the JavaDoc
in `io.joynr.discovery.jee.JoynrConfigurationProvider` for details on how this can be
done. Essentially, you have a choice of either setting OS level environment variables or
specifying Java system properties.

## Payara / Glassfish

First, if not already done so, create a connection pool for the database you want the
discovery directory to use for persistence. For this example, we'll create a database
on the JavaDB (based on Derby) database which is installed as part of Payara / Glassfish:

    bin/asadmin create-jdbc-connection-pool \
        --datasourceclassname org.apache.derby.jdbc.ClientDataSource \
        --restype javax.sql.XADataSource \
        --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool

Next, create a datasource resource pointing to that database connection. Here's an
example of what that would look like when using the connection pool created above:

`bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS`

After this, you can start the database:

`bin/asadmin start-database`

Next, create the executor service:

    bin/asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor

Start a MQTT broker, e.g. [Mosquitto](http://mosquitto.org), and make sure it's accepting traffic
on port `1883`.

And finally deploy the discovery directory:

    bin/asadmin deploy \
        <joynr home>/java/backend-services/discovery-directory-jee/target/discovery-directory-jee-<version>.war

>Note: by default the configuration expects the application to be available under
>the context root `discovery-directory-jee` as it is configured in JoynrConfigurationProvider.
>Note that when specifying the servlet context root via configuration you have to include the
>`/messaging` part of the URL.

