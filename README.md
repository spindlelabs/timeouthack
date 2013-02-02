Adjusts `TCP_USER_TIMEOUT` for some destinations by intercepting calls 
to `connect(2)`.

See `Makefile` for sample usage.

This module supports neither IPv6 nor IPv4-mapped IPv6 addresses (RFC 
4291 2.5.5); to use it with the Java virtual machine, set the system 
property `java.net.preferIPv4Stack` to `true`.

https://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commitdiff;h=dca43c75e7e545694a9dd6288553f55c53e2a3a3
