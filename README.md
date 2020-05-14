# Multicast-Key-Agreement

To build, simply type 'make all'. It will take a few minutes to compile everything.

Make sure you have internet connection, as one needs to download the `botan` library.

The group manager driver, `crypto_driver.c` is inside the `driver` directory, and the user driver, `user_driver.c` is inside the `users` directory.

To run the compiled `crypto_driver` or `user_driver`, simply run it without arguments to see the command line options that need to be specified.

Example invocation:

Group Manager: `./crypto_driver 10.0.0.71 3000 239.255.255.255 3004 0 0 0`

Note: the last 3 options should *always* be specified as 0s.

User: `./user_driver 10.0.0.71 3000 239.255.255.255 3004`
