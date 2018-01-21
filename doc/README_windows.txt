
Copyright (c) 2017-2018 Scash Developers


Scash 1.2.0

Copyright (c) 2017-2018 Scash Developers
Copyright (c) 2011-2012 Bitcoin Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
Scash is a peer-to-peer decentralized Proof of Stake Velocity digital currency with an initial Proof of Work mining distribution period lasting up to 476,918 coins mined. After the initial mining distribution period is over, the network will transition into an energy efficient Proof of Stake Velocity algorithm which will reward balance holders on the network a 8% staking annual interest.

Scash is based on the revolutionary Blockchain concept but has added some very simple and clever layers of communication and a sophisticated off-blockchain coin mixing system making it impossible for 3rd parties to trace transactions between Scash wallets.

Setup
-----
After completing windows setup then run windows command line (cmd)
  cd daemon
  scashd
You would need to create a configuration file scash.conf in the default
wallet directory. Grant access to scashd.exe in anti-virus and firewall
applications if necessary.

The software automatically finds other nodes to connect to.  You can
enable Universal Plug and Play (UPnP) with your router/firewall
or forward port 12788 (TCP) to your computer so you can receive
incoming connections.  Scash works without incoming connections,
but allowing incoming connections helps the Scash network.

Upgrade
-------
All you existing coins/transactions should be intact with the upgrade.
To upgrade first backup wallet
scashd backupwallet <destination_backup_file>
Then shutdown scashd by
scashd stop
Start up the new scashd.


See the documentation/wiki at the Scash site:
  https://www.scash.ml/
for help and more information.

