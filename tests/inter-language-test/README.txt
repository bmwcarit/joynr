Shortcomings found yet:

Java:

Franca UInt8 are stored as Java Byte     -> too low range
                                            (128..255 not covered),
                                            negative range possible
Franca UInt16 are stored as Java Short   -> too low range
                                            (32768..65535 not covered),
                                            negative range possible
Franca UInt32 are stored as Java Integer -> too low range
                                            (2^31..2^32-1 not covered)
                                            negative range possible
Franca UInt64 are stored as Java Long    -> too low range
                                            (2^63..2^64-1 not covered)
                                            negative range possible

It is not possible to code tests with too low range representation.
Those tests will however fail, when coded in C++.
