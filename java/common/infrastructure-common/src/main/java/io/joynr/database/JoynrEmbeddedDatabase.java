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
package io.joynr.database;

import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class JoynrEmbeddedDatabase {
    private static final Logger logger = LoggerFactory.getLogger(JoynrEmbeddedDatabase.class);

    public static final String PROPERTY_DATABASE_NAME = "joynr.database.embedded.database";
    private boolean started = false;
    private Connection connection = null;
    private String dbName;

    public JoynrEmbeddedDatabase(String dbName) {
        this.dbName = dbName;
    }

    public String getDBName() {
        return dbName;
    }

    protected abstract String getDBCreateUrl();

    protected abstract String getDBCloseUrl();

    public synchronized void start() throws SQLException {
        connection = DriverManager.getConnection(getDBCreateUrl());
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
            logger.debug("Error checking if table exists:", e);
        }
        return false;
    }

    public synchronized boolean execute(String sqlStatement) throws SQLException {
        if (!started) {
            throw new IllegalStateException("No database access shall be made while it is not started!");
        }
        Statement statement = connection.createStatement();
        boolean result = false;
        try {
            result = statement.execute(sqlStatement);
        } finally {
            statement.close();
        }
        return result;
    }

    public synchronized <T> T executeQuery(String sqlStatement, QueryProcessor<T> processor) throws SQLException {
        if (!started) {
            throw new IllegalStateException("No query shall be made to the database while it is not started!");
        }
        Statement statement = connection.createStatement();
        try {
            ResultSet resultSet = statement.executeQuery(sqlStatement);
            T result = processor.processQueryResult(resultSet);
            return result;
        } finally {
            statement.close();
        }
    }

    public synchronized <T> T executeQuery(PreparedStatement preparedStatement,
                                           QueryProcessor<T> processor) throws SQLException {

        if (!started) {
            throw new IllegalStateException("No query shall be made to the database while it is not started!");
        }

        ResultSet resultSet = null;
        T result = null;
        try {
            resultSet = preparedStatement.executeQuery();
            result = processor.processQueryResult(resultSet);
        } finally {
            if (resultSet != null) {
                resultSet.close();
            }
        }
        return result;
    }

    public synchronized void close() throws SQLException {
        if (connection != null) {
            connection.close();
            connection = null;
        }
        started = false;
    }

    public synchronized PreparedStatement prepareStatement(String sql) throws SQLException {
        if (!started) {
            throw new IllegalStateException("No query shall be made to the database while it is not started!");
        }

        return connection.prepareStatement(sql);

    }
}
