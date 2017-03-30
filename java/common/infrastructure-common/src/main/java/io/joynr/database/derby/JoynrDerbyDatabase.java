package io.joynr.database.derby;

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

import io.joynr.database.JoynrEmbeddedDatabase;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class JoynrDerbyDatabase extends JoynrEmbeddedDatabase {
    private static final String dbUrl_prefix = "jdbc:derby";
    private static final String dbUrl_createpostfix = ";create=true";
    private static final String dbUrl_closepostfix = ";shutdown=true";

    private static Logger log = LoggerFactory.getLogger(JoynrDerbyDatabase.class);

    @Inject
    public JoynrDerbyDatabase(@Named(PROPERTY_DATABASE_NAME) String dbName) throws ClassNotFoundException {
        super(dbName);
        try {
            Class.forName("org.apache.derby.jdbc.EmbeddedDriver");
        } catch (ClassNotFoundException e) {
            log.error("Class \"org.apache.derby.jdbc.EmbeddedDriver\" could not be found in the classpath. Please fix the classpath!",
                      e);
            throw e;
        }
    }

    @Override
    protected String getDBCreateUrl() {
        return dbUrl_prefix + ":" + getDBName() + dbUrl_createpostfix;
    }

    @Override
    protected String getDBCloseUrl() {
        return dbUrl_prefix + ":" + getDBName() + dbUrl_closepostfix;
    }

    @Override
    public synchronized void close() throws SQLException {
        Connection connection = null;
        try {
            connection = DriverManager.getConnection(getDBCloseUrl());
        } catch (SQLException e) {
            log.debug("Expected exception \"" + e.getMessage() + "\" ocurred", e);
        } finally {
            if (connection != null) {
                connection.close();
            }
        }
        super.close();
    }
}
