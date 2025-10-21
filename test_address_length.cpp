#include <iostream>
#include <string>
#include "src/Common/Base58.h"
#include "src/CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "src/CryptoNoteConfig.h"

using namespace CryptoNote;

int main() {
    // Create a test account public address
    AccountPublicAddress testAddr;
    
    // Fill with some test data (32 bytes for spend key + 32 bytes for view key)
    for (int i = 0; i < 32; i++) {
        testAddr.spendPublicKey.data[i] = i;
        testAddr.viewPublicKey.data[i] = i + 32;
    }
    
    // Generate address string
    std::string address = getAccountAddressAsStr(parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, testAddr);
    
    std::cout << "Generated address: " << address << std::endl;
    std::cout << "Address length: " << address.length() << std::endl;
    
    return 0;
}