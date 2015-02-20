# Java Artifacts
Java artifacts are deployed to Maven repositories.

## Snapshot Versions
Snapshot versions are available from the [Sonatype Nexus Snapshots]
(https://oss.sonatype.org/content/repositories/snapshots) repository.

To access snapshot versions through Maven add the following repository snippet to the
`<repositories>` section of your Maven POM:

```xml
<repository>
	<id>sonatype-nexus-snapshots</id>
	<name>Sonatype Nexus Snapshots</name>
	<url>https://oss.sonatype.org/content/repositories/snapshots</url>
	<releases>
		<enabled>false</enabled>
	</releases>
	<snapshots>
		<enabled>true</enabled>
	</snapshots>
</repository>
```

## Release Versions

Release versions are directly available from [Maven Central](http://maven.org) repository.

## PGP Signatures

All artifacts are signed and deployed with PGP signatures.

### joynr PGP Public Key

Key: 472E45DC

Key fingerprint: 239B 2DC6 4197 E7E7 A51F  16AE AC7E 77EF 472E 45DC

```text
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v1.4.11 (GNU/Linux)

mQGiBFKGO+MRBACmliR3EdLe8bAcB7yT6q5eaTtrhpMhN0dODQykiGPv0kA5MjPI
tOTIKPD0bVJX0sMP3rqTC1G+jOS0HjSF0VYeByBw8NU7wCj0tIpSDeHbGmS8WRxG
ijatvjzZDx90IaMHOwO6r7Gjr726Ohtg//6qSJpkc6XoeRW/kqflgrzaUwCgqz0W
jEz+jhB1GKefrlwTaT9gw4ED/iFk98AIeDu4oWgABE+KpYkjTjid3v21AJRih+tn
e6cBkhyHk1cLGpbspe0Ts02A/eDKKVHDSy0DXCRXSWCNVEc4P+HmaJVSvxZz5uGN
PuZtNvQDuOSHLQhGxxScMLDsKtuYcbeN9DbtDIVGFd6OlDIuAkJSek8TXKOqzL1b
Hl7FBACL78kZUJOIZlo5g8/K6RIztAv/Eh5mqg7NvABKY8y1X/AnGcJ5nnRCZpeQ
4hoMrUyeBgnxsm9ZoONV9FSdlG4pPOhYvGwZA6tBqQcLWhinw8ZzPAcKg8AEfhee
OyIhWiLdjD7TW++rYvh1q6ADEpDrc6ffz99L7IXlRJeSfT2+e7Qdam95bnIuaW8g
PGpveW5yQGJtdy1jYXJpdC5kZT6IXwQTEQIAHwUCUoY74wIbAwYLCQgHAwIEFQII
AwMWAgECHgECF4AACgkQrH5370cuRdy5cACgoHVGGAwPnyr1zWneBP1h1I1uRmwA
ninxTOSdrF9MpIcs9MNyQYjlzEAGuQINBFKGO+MQCADJj+t0aJz54sb/wQrj7I2B
XSJsHm/bop5qrx+0sBcp4KALfhdR/iFj75UmIe9Fi4B7aewCjJ1NIBGKsGX3QTag
B6FWrCH0u/wvL29plCzMnAHia6NUIamMBhw2xsQMTctWiRzv9cxPQH9LyOoSIrCE
QNj5Vyy/zC4rzRF2AP51KQZkAN/ezwCSi7NUNw+LE/pUOtZHd1027rOl7k04xPFV
OCZlIdimD3Wdgykwk1hsMY6QwcSn2Ha4gPZMFlgVE88l8nch+tsMB5ZGgZ4uDZL8
4jtTt21547Ppb4XHSCW4IUSyGCWMOEgHxhycMuAHFBJNYQFpvV/xKIZh7ZkUwcHv
AAMFCACesmV0JtWBSiLETPZ6b72j3XAQdTinD9CxfEmls4ZQXEunbY2Kxp46gQFU
+WBCD5+C3KdUaQlD+LXucbI28dezKENDFEUBvdFJ/aQA6WesxCkHoE0R392fwSGN
CPJ2kTh/rzQO9raHXQkGQbeUW+V/31fyaLODzbL6dNliMxrI4V0TI51isUFF/xLU
GH8Zd/Q+IxsWF2ZEL/e9Fjxntt9Xrte+X/grxXz5I7gR/w6KQOtVyhRQNX63JgQL
NVbgNMY1ngEqsfTvfTJXAxHgyMzUDAm8Y3th49eCnUsJrs0r0po5lqOeHQttSKho
kV6SaDyjkt0lPTMRhbP2fhY6HXTDiEkEGBECAAkFAlKGO+MCGwwACgkQrH5370cu
RdxTzACfRDOSgU3zaPEHWKLT6pfFMMc5qE4Aniqe++21Geal59e6MBXR0yABvAlO
=2G/b
-----END PGP PUBLIC KEY BLOCK-----
```

# C++ Binaries
C++ binaries are currently not distributed.