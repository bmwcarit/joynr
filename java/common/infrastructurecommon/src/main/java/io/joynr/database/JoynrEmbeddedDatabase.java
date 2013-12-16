package io.joynr.database;

/*
 * #%L
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

import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class JoynrEmbeddedDatabase {

    public static final String PROPERTY_DATABASE_NAME = "joynr.database.embedded.database";
    private Connection connection;
    private boolean started = false;
    private String dbUrl;

    @Inject
    public JoynrEmbeddedDatabase(@Named(PROPERTY_DATABASE_NAME) String dbUrl) {
        this.dbUrl = dbUrl;
    }

    public void start() throws SQLException {
        connection = DriverManager.getConnection(dbUrl);
        started = true;
    }

    public synchronized boolean isTableAlreadyContainedInDatabase(String tableName) {
        if (!started) {
            throw new IllegalStateException("Database is not started");
        }
        try {
            DatabaseMetaData metaData = connection.getMetaData();
            ResultSet result = metaData.getTables(null, null, null, new String[]{ "TABLE" });
            while (result.next()) {
                if (result.getString("TABLE_NAME").equalsIgnoreCase(tableName)) {
                    return true;
                }
            }
        } catch (SQLException e) {
        }
        return false;
    }

    public synchronized boolean execute(String sqlStatement) throws SQLException {
        if (!started) {
            throw new IllegalStateException("No database access shall be made while it is not started!");
        }
        Statement statement = connection.createStatement();
        boolean result = statement.execute(sqlStatement);
        statement.close();
        return result;
    }

    public synchronized ResultSet executeQuery(String sqlStatement) throws SQLException {
        if (!started) {
            throw new IllegalStateException("No query shall be made to the database while it is not started!");
        }
        Statement statement = connection.createStatement();
        ResultSet result = statement.executeQuery(sqlStatement);
        return result;
    }

    public synchronized void close() throws SQLException {
        connection.close();
        started = false;
    }

}
