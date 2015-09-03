https://gist.github.com/jed/982883
UUID
====

Returns a random v4 [UUID](http://en.wikipedia.org/wiki/Universally_unique_identifier) of the form `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`, where each `x` is replaced with a random hexadecimal digit from 0 to f, and `y` is replaced with a random hexadecimal digit from 8 to b.

There's also @LeverOne's [approach using iteration](https://gist.github.com/1308368), which is one byte shorter.