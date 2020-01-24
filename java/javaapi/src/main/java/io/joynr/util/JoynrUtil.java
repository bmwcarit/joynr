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
package io.joynr.util;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Base64;
import java.util.Enumeration;
import java.util.Optional;
import java.util.Properties;
import java.util.Random;
import java.util.UUID;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.JarInputStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class JoynrUtil {
    private static final Logger logger = LoggerFactory.getLogger(JoynrUtil.class);
    private static final Base64.Encoder base64Encoder = Base64.getUrlEncoder().withoutPadding();

    public enum OS {
        LINUX, WIN32, TEST, UNDEFINED
    };

    public static String getStringFromOS(OS os) {
        if (os == OS.WIN32) {
            return "win32";
        } else if (os == OS.LINUX) {
            return "linux";
        } else if (os == OS.TEST) {
            return "test";
        } else {
            return "undefined";
        }
    }

    public static Properties loadProperties(Optional<File> file) throws IOException {
        if (file.isPresent()) {
            File actualFile = file.get();
            if (!actualFile.isDirectory()) {
                InputStream inputStream = new FileInputStream(actualFile);
                return loadProperties(inputStream);
            } else {
                return loadPropertyDirectory(actualFile);
            }
        }
        return null;
    }

    public static void copyStream(InputStream in, OutputStream out) throws IOException {
        int len;
        byte[] buffer = new byte[64 * 1024];
        while ((len = in.read(buffer)) > 0) {
            out.write(buffer, 0, len);
        }

        in.close();
        out.close();
    }

    public static File createDir(String dirName) {
        File tempDir = new File(dirName);
        if (tempDir.exists() == false) {
            if (!tempDir.mkdirs()) {
                logger.debug("Creating of dir " + dirName + " failed.");
            }

        }

        tempDir.deleteOnExit();

        return tempDir;
    }

    public static File createTempDir(String dirName) {
        return createDir(getBaseTempPath() + File.separator + dirName);
    }

    public static String getBaseTempPath() {
        return System.getProperty("java.io.tmpdir");
    }

    public static File createTempDir() {

        Random rand = new Random();
        int randomInt = 1 + rand.nextInt();

        return createTempDir("tempDir" + randomInt);
    }

    // If targetLocation does not exist, it will be created.
    public static void copyDirectory(File sourceLocation, File targetLocation) throws IOException {

        if (sourceLocation.isDirectory()) {
            if (!targetLocation.exists()) {
                if (targetLocation.mkdir() == false) {
                    logger.debug("Creating target directory " + targetLocation + " failed.");
                }
            }

            String[] children = sourceLocation.list();
            if (children == null) {
                return;
            }

            for (int i = 0; i < children.length; i++) {
                copyDirectory(new File(sourceLocation, children[i]), new File(targetLocation, children[i]));
            }
        } else {
            FileInputStream in = null;
            FileOutputStream out = null;
            try {
                in = new FileInputStream(sourceLocation);
                out = new FileOutputStream(targetLocation);
                copyStream(in, out);
            } finally {
                if (in != null) {
                    in.close();
                }
                if (out != null) {
                    out.close();
                }
            }
        }
    }

    private static Properties loadPropertyDirectory(File dir) throws IOException {
        Properties returnValue = new Properties();
        File[] files = dir.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.getName().endsWith("properties")) {
                    InputStream inputStream = new FileInputStream(file);
                    loadProperties(inputStream, Optional.of(returnValue));
                }
            }
        }

        return returnValue;
    }

    public static Properties loadProperties(InputStream configInputStream) throws IOException {
        return loadProperties(configInputStream, Optional.empty());
    }

    public static Properties loadProperties(InputStream configInputStream,
                                            Optional<Properties> properties) throws IOException {
        Properties propertiesToReturn = new Properties();
        if (configInputStream != null) {
            if (!properties.isPresent()) {
                propertiesToReturn = new Properties();
            }
            propertiesToReturn.load(configInputStream);
            configInputStream.close();
            return propertiesToReturn;
        }
        return properties.get();
    }

    public static OS getOSFromString(String os) {
        if (os.equalsIgnoreCase("win32")) {
            return OS.WIN32;
        } else if (os.equalsIgnoreCase("linux")) {
            return OS.LINUX;
        } else if (os.equalsIgnoreCase("test")) {
            return OS.TEST;
        } else {
            return OS.UNDEFINED;
        }
    }

    public static void copyDirectoryFromJar(String jarName, String srcDir, File tmpDir) throws IOException {

        JarFile jf = null;
        JarInputStream jarInputStream = null;

        try {
            jf = new JarFile(jarName);
            JarEntry je = jf.getJarEntry(srcDir);
            if (je.isDirectory()) {
                FileInputStream fis = new FileInputStream(jarName);
                BufferedInputStream bis = new BufferedInputStream(fis);
                jarInputStream = new JarInputStream(bis);
                JarEntry ze = null;
                while ((ze = jarInputStream.getNextJarEntry()) != null) {
                    if (ze.isDirectory()) {
                        continue;
                    }
                    if (ze.getName().contains(je.getName())) {
                        InputStream is = jf.getInputStream(ze);
                        String name = ze.getName().substring(ze.getName().lastIndexOf("/") + 1);
                        File tmpFile = new File(tmpDir + "/" + name); //File.createTempFile(file.getName(), "tmp");
                        tmpFile.deleteOnExit();
                        OutputStream outputStreamRuntime = new FileOutputStream(tmpFile);
                        copyStream(is, outputStreamRuntime);
                    }
                }
            }
        } finally {
            if (jf != null) {
                jf.close();
            }
            if (jarInputStream != null) {
                jarInputStream.close();
            }
        }
    }

    public static void checkConnectivity(String host) throws IOException {
        InetAddress headUnitAddr = InetAddress.getByName(host);
        Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
        NetworkInterface iface;

        while (interfaces.hasMoreElements()) {
            iface = interfaces.nextElement();
            if (headUnitAddr.isReachable(iface, 0, 5000)) {
                return;
            }
        }
        throw new UnknownHostException("Connectivity check to the host \"" + host + "\" has failed");
    }

    public static String createUuidString() {
        UUID uuid = UUID.randomUUID();
        ByteBuffer uuidBytes = ByteBuffer.wrap(new byte[16]);
        uuidBytes.putLong(uuid.getMostSignificantBits());
        uuidBytes.putLong(uuid.getLeastSignificantBits());
        return base64Encoder.encodeToString(uuidBytes.array());
    }
}
