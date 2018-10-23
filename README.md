Amigo Core
==============
* [Getting Started](#getting-started)
* [Support](#support)
* [License](#license)

Amigo Core is the Amigo blockchain implementation and command-line interface.
The app wallet is [Amigo app wallet](https://amigochain.org).

Visit [amigochain.org](https://amigochain.org/) to learn about Amigo.

**NOTE:** The official Amigo git repository location, default branch, and submodule remotes were recently changed. Existing
repositories can be updated with the following steps:

    git remote set-url origin https://github.com/Amigopr/amigo-core.git
    git checkout master
    git remote set-head origin --auto
    git pull
    git submodule sync --recursive
    git submodule update --init --recursive

Getting Started
---------------
Build instructions and additional documentation are available in the
[wiki](https://github.com/Amigopr/amigo-core/wiki).

We recommend building on Ubuntu 16.04 LTS, and the build dependencies may be installed with:

    sudo apt-get update
    sudo apt-get install autoconf cmake git libboost-all-dev libssl-dev

To build after all dependencies are installed:

    git clone https://github.com/Amigopr/amigo-core.git
    cd amigo-core
    git checkout <LATEST_RELEASE_TAG>
    git submodule update --init --recursive
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
    make

**NOTE:** Amigo requires an [OpenSSL](https://www.openssl.org/) version in the 1.0.x series. OpenSSL 1.1.0 and newer are NOT supported. If your system OpenSSL version is newer, then you will need to manually provide an older version of OpenSSL and specify it to CMake using `-DOPENSSL_INCLUDE_DIR`, `-DOPENSSL_SSL_LIBRARY`, and `-DOPENSSL_CRYPTO_LIBRARY`.

**NOTE:** Amigo requires a [Boost](http://www.boost.org/) version in the range [1.57, 1.60]. Versions earlier than
1.57 or newer than 1.60 are NOT supported. If your system Boost version is newer, then you will need to manually build
an older version of Boost and specify it to CMake using `DBOOST_ROOT`.

After building, the witness node can be launched with:

    ./programs/witness_node/witness_node

The node will automatically create a data directory including a config file. It may take several hours to fully synchronize
the blockchain. After syncing, you can exit the node using Ctrl+C and setup the command-line wallet by editing
`witness_node_data_dir/config.ini` as follows:

    rpc-endpoint = 127.0.0.1:8090

After starting the witness node again, in a separate terminal you can run:

    ./programs/cli_wallet/cli_wallet

Set your inital password:

    >>> set_password <PASSWORD>
    >>> unlock <PASSWORD>

To import your initial balance:

    >>> import_balance <ACCOUNT NAME> [<WIF_KEY>] true

If you send private keys over this connection, `rpc-endpoint` should be bound to localhost for security.

To upgrade your account to lifetime member
    >>> upgrade_account <ACCOUNT NAME> true

Please note that, after running command upgrade_account, to let it take effect, you need to exit the cli_wallet(Ctrl+c) and then restart cli_wallet.
		
To create a witness for your account
	>>> create_witness <ACCOUNT NAME> <your URL> true

<your URL> is a URL to include in the witness record in the blockchain. Clients may display this when showing a list of witnesses. May be blank, or "".
Please note that, before you run command create_witness, you must run command upgrade_account to upgrade your account at first.


To update a witness object owned by the given account
	>>> update_witness <ACCOUNT NAME> <your URL> <block signing key> true

<your URL> is the same as above.
<block signing key> The new block signing public key.  The empty string makes it remain the same.

For example, if your account is test_user, and your block signing key is "AGC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", then you can run command update_witness to change block signing key
	>>> update_witness test_user "" "AGC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" 

In witness config file config.ini, you must config the option "private-key" for your witness. In the option "private-key", you can specify the signing public key and the corresponding private key for your witness. 
An example for the option "private-key" in config.ini is as below:
private-key = ["AGC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]

"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3" is the private key corresponding to the signing public key "AGC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV".

Please note that, when you running command create_witness, a random block signing public key will be assigned to your witness. So before your witness can generate block, you must run command update_witness to replace the previous random block signing public key by your block signing public key.


For convenience, we provide an example of witness config file config.ini. We provide the scripts to start and stop a witness node. Please see start_node.sh and stop_node.sh. 
We also provide a script to start a wallet. Please see start_cli_wallet.sh. Please note that wallet config file wallet_config.ini must be in the same folder in where start_cli_wallet.sh is.


Use `help` to see all available wallet commands. 
	>>> help
	
Source definition and listing of all commands is available
[here](https://github.com/Amigopr/amigo-core/blob/master/libraries/wallet/include/graphene/wallet/wallet.hpp).


Support
-------
Technical support is available in the [Amigo technical support](https://amigochain.org).


License
-------
Amigo Core is under the GNU General Public License v3. See [LICENSE](https://github.com/Amigopr/amigo-core/blob/master/LICENSE.txt)
for more information.
