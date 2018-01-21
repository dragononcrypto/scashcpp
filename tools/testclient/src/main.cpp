#include "bitcoinapi.h"

int main()
{
    std::string username = "user";
    std::string password = "pass";
    std::string address = "127.0.0.1";
    int port = 8332;

    try
    {
        /* Constructor to connect to the bitcoin daemon */
        BitcoinAPI btc(username, password, address, port);

        /* Example method - getbalance */
        std::cout << "Wallet balance: " << btc.getbalance() << std::endl;
    }
    catch(BitcoinException e)
    {
        std::cerr << e.getMessage() << std::endl;
    }
}
