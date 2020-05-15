# Multicast-Key-Agreement

To build, simply type `make all`. It will take a few minutes to compile everything.

Make sure you have internet connection, as one needs to download the `botan` library.

Note: On OSX, one might have to add the line `export DYLD_FALLBACK_LIBRARY_PATH="$LD_LIBRARY_PATH:[TREE DIRECTORY PATH]/Multicast-Key-Agreement/libbotan/lib"` in `zshrc` in the home directory.
For example, `export DYLD_FALLBACK_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib:/Users/Alex/nyu/networks/Multicast-Key-Agreement/libbotan/lib"`.

Note: On Linux, one might have to edit `LD_LIBRARY_PATH` in `bashrc` in the home directory to include `[TREE DIRECTORY PATH]/Multicast-Key-Agreement/libbotan/lib` in the same manner as above; and also add the line `export LD_LIBRARY_PATH` if it is not already there.

The group manager driver, `crypto_driver.c` is inside the `driver` directory, and the user driver, `user_driver.c` is inside the `users` directory.

To run the compiled `crypto_driver` or `user_driver`, simply run it without arguments to see the command line options that need to be specified.

The following table represents the commands that the group manager can input to `stdin`, and their corresponding operations:

| Command | Operation |
|---|---|
| -1 n | Create(ID_1,...,ID_n)|
| 0 ID | Add(ID) |
| 1 ID | Update(ID) |
| 2 ID | Remove(ID) |
| -2 0 | Broadcast an encryption of "test" over the multicast socket using the most recent group secret |

Note that user clients are assigned integer IDs, starting from 0, in the order they are launched.
The first command in the above table, `-1 n` will create a group with the first n user clients that were launched. Further note that there is no error handling, so for example, updating a user that is not in the group may result in unexpected behavior, including crashing.

Example invocation:

Group Manager: `./crypto_driver 10.0.0.71 3000 239.255.255.255 3004 0 0 0`

Note: the last 3 options should *always* be specified as 0s (for now).

User: `./user_driver 10.0.0.71 3000 239.255.255.255 3004`

See: https://www.youtube.com/watch?v=FnNwLf8xY8Q for a short example execution of the protocol.
