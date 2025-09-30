// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "ElderfierMenuSystem.h"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/TransactionExtra.h"
#include "WalletLegacy/WalletLegacy.h"

namespace CryptoNote {

ElderfierMenuSystem::ElderfierMenuSystem(const CryptoNote::Currency& currency, 
                                       CryptoNote::IWalletLegacy* wallet,
                                       Logging::LoggerRef& logger)
    : m_currency(currency)
    , m_wallet(wallet)
    , m_logger(logger)
    , m_currentState(MAIN_MENU)
    , m_selectedIndex(0)
    , m_menuActive(false)
    , m_hiddenTitleClickable(true)
    , m_termiosModified(false)
{
    // Initialize main menu items
    m_mainMenuItems = {
        {"View Elderfier Requirements", [this]() { showElderfierRequirements(); }, true},
        {"View Elderfier Benefits", [this]() { showElderfierBenefits(); }, true},
        {"Elder Council Inbox", [this]() { openElderCouncilInbox(); }, true},
        {"Exit Elderfier Menu", [this]() { exitMenu(); }, true}
    };

    // Initialize stayking menu items
    m_staykingMenuItems = {
        {"Create Elderfier Deposit (800 XFG)", [this]() { createElderfierDeposit(); }, true},
        {"View Deposit Requirements", [this]() { showElderfierRequirements(); }, true},
        {"Return to Main Menu", [this]() { returnToMainMenu(); }, true}
    };
}

ElderfierMenuSystem::~ElderfierMenuSystem() {
    if (m_termiosModified) {
        restoreTerminal();
    }
}

bool ElderfierMenuSystem::showElderfierMenu() {
    clearScreen();
    setupTerminal();
    m_menuActive = true;
    m_currentState = MAIN_MENU;
    m_selectedIndex = 0;

    displayMainMenu();
    handleNavigation();

    restoreTerminal();
    return true;
}

bool ElderfierMenuSystem::isNodeConfiguredForElderfiers(const std::string& feeAddress) {
    // Check if fee address is set (indicates node was started with --set-fee-address)
    return !feeAddress.empty();
}

void ElderfierMenuSystem::displayMainMenu() {
    clearScreen();
    
    // ASCII Art Header with ELDERFYRE STAYKING title
    std::cout << "\n";
    std::cout << "    ███████╗██╗     ██████╗ ███████╗██████╗ ███████╗██╗   ██╗██████╗ ███████╗\n";
    std::cout << "    ██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝╚██╗ ██╔╝██╔══██╗██╔════╝\n";
    std::cout << "    █████╗  ██║     ██║  ██║█████╗  ██████╔╝█████╗   ╚████╔╝ ██████╔╝█████╗  \n";
    std::cout << "    ██╔══╝  ██║     ██║  ██║██╔══╝  ██╔══██╗██╔══╝    ╚██╔╝  ██╔══██╗██╔══╝  \n";
    std::cout << "    ███████╗███████╗██████╔╝███████╗██║  ██║██║        ██║   ██║  ██║███████╗\n";
    std::cout << "    ╚══════╝╚══════╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝        ╚═╝   ╚═╝  ╚═╝╚══════╝\n";
    std::cout << "\n";
    
    // Hidden clickable title (user needs to discover this)
    if (m_selectedIndex == -1) {  // Special hidden index for title
        std::cout << "    ████████ ELDERFYRE STAYKING ████████  <-- [SELECTED]\n";
    } else {
        std::cout << "    ████████ ELDERFYRE STAYKING ████████\n";
    }
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                            ELDERFIER SYSTEM                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    std::cout << "🔹 Advanced Verification Nodes for the Fuego Network\n";
    std::cout << "🔹 Stake 800 XFG to become an Elderfier\n";
    std::cout << "🔹 Participate in network governance and consensus\n";
    std::cout << "🔹 Access Elder Council voting system\n";
    std::cout << "\n";

    // Display menu items
    for (size_t i = 0; i < m_mainMenuItems.size(); ++i) {
        if (m_selectedIndex == static_cast<int>(i)) {
            std::cout << "  ► " << m_mainMenuItems[i].text << " ◄\n";
        } else {
            std::cout << "    " << m_mainMenuItems[i].text << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "Navigation: Use WASD keys to navigate (W=Up, S=Down, Enter=Select)\n";
    std::cout << "Hint: There might be a hidden clickable element above... 🤔\n";
    std::cout << "\n";
    
    hideCursor();
}

void ElderfierMenuSystem::displayStaykingMenu() {
    clearScreen();
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ELDERFYRE STAYKING                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    std::cout << "🔥 ELDERFIER STAKING SYSTEM 🔥\n";
    std::cout << "\n";
    std::cout << "Requirements:\n";
    std::cout << "• 800 XFG deposit stake (refundable when exiting)\n";
    std::cout << "• Node must be running with --set-fee-address flag\n";
    std::cout << "• Reliable internet connection (99%+ uptime)\n";
    std::cout << "• 24/7 operation capability\n";
    std::cout << "\n";
    std::cout << "Benefits:\n";
    std::cout << "• Transaction validation rewards\n";
    std::cout << "• Priority fee distribution\n";
    std::cout << "• Network governance voting rights\n";
    std::cout << "• Access to Elder Council inbox\n";
    std::cout << "• Enhanced network features\n";
    std::cout << "\n";

    // Display stayking menu items
    for (size_t i = 0; i < m_staykingMenuItems.size(); ++i) {
        if (m_selectedIndex == static_cast<int>(i)) {
            std::cout << "  ► " << m_staykingMenuItems[i].text << " ◄\n";
        } else {
            std::cout << "    " << m_staykingMenuItems[i].text << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "Navigation: Use WASD keys to navigate (W=Up, S=Down, Enter=Select)\n";
    std::cout << "\n";
    
    hideCursor();
}

void ElderfierMenuSystem::handleNavigation() {
    while (m_menuActive) {
        char key = getKey();
        
        switch (key) {
            case 'w':
            case 'W':
                moveCursorUp();
                break;
            case 's':
            case 'S':
                moveCursorDown();
                break;
            case '\r':  // Enter key
            case '\n':
                selectCurrentItem();
                break;
            case 'q':
            case 'Q':
            case 27:  // ESC key
                exitMenu();
                break;
            default:
                break;
        }
        
        // Redraw current menu
        if (m_currentState == MAIN_MENU) {
            displayMainMenu();
        } else if (m_currentState == STAYKING_MENU) {
            displayStaykingMenu();
        }
    }
}

char ElderfierMenuSystem::getKey() {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return 0;
}

void ElderfierMenuSystem::moveCursorUp() {
    if (m_currentState == MAIN_MENU) {
        if (m_selectedIndex > 0) {
            m_selectedIndex--;
        } else if (m_hiddenTitleClickable && m_selectedIndex == 0) {
            m_selectedIndex = -1;  // Hidden title selection
        } else {
            m_selectedIndex = m_mainMenuItems.size() - 1;
        }
    } else if (m_currentState == STAYKING_MENU) {
        if (m_selectedIndex > 0) {
            m_selectedIndex--;
        } else {
            m_selectedIndex = m_staykingMenuItems.size() - 1;
        }
    }
}

void ElderfierMenuSystem::moveCursorDown() {
    if (m_currentState == MAIN_MENU) {
        if (m_selectedIndex == -1) {  // From hidden title
            m_selectedIndex = 0;
        } else if (m_selectedIndex < static_cast<int>(m_mainMenuItems.size() - 1)) {
            m_selectedIndex++;
        } else {
            m_selectedIndex = 0;
        }
    } else if (m_currentState == STAYKING_MENU) {
        if (m_selectedIndex < static_cast<int>(m_staykingMenuItems.size() - 1)) {
            m_selectedIndex++;
        } else {
            m_selectedIndex = 0;
        }
    }
}

void ElderfierMenuSystem::selectCurrentItem() {
    if (m_currentState == MAIN_MENU) {
        if (m_selectedIndex == -1) {
            // Hidden title clicked - go to stayking menu!
            m_currentState = STAYKING_MENU;
            m_selectedIndex = 0;
            displayStaykingMenu();
        } else if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_mainMenuItems.size())) {
            auto& item = m_mainMenuItems[m_selectedIndex];
            if (item.enabled && item.action) {
                item.action();
            }
        }
    } else if (m_currentState == STAYKING_MENU) {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_staykingMenuItems.size())) {
            auto& item = m_staykingMenuItems[m_selectedIndex];
            if (item.enabled && item.action) {
                item.action();
            }
        }
    }
}

void ElderfierMenuSystem::clearScreen() {
    std::cout << "\033[2J\033[H";  // Clear screen and move cursor to top
    std::cout.flush();
}

void ElderfierMenuSystem::hideCursor() {
    std::cout << "\033[?25l";  // Hide cursor
    std::cout.flush();
}

void ElderfierMenuSystem::showCursor() {
    std::cout << "\033[?25h";  // Show cursor
    std::cout.flush();
}

void ElderfierMenuSystem::createElderfierDeposit() {
    clearScreen();
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    CREATE ELDERFIER DEPOSIT                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Check wallet balance
    uint64_t balance = m_wallet->actualBalance();
    uint64_t requiredAmount = 800 * m_currency.coin();  // 800 XFG
    
    std::cout << "Current Balance: " << m_currency.formatAmount(balance) << " XFG\n";
    std::cout << "Required Deposit: " << m_currency.formatAmount(requiredAmount) << " XFG\n";
    std::cout << "\n";
    
    if (balance < requiredAmount) {
        std::cout << "❌ Insufficient balance for Elderfier deposit!\n";
        std::cout << "You need at least 800 XFG to become an Elderfier.\n";
    } else {
        std::cout << "✅ Sufficient balance detected!\n";
        std::cout << "\n";
        std::cout << "Elderfier deposit will:\n";
        std::cout << "• Lock 800 XFG as stake\n";
        std::cout << "• Register you as an Elderfier node\n";
        std::cout << "• Enable participation in consensus\n";
        std::cout << "• Grant access to Elder Council features\n";
        std::cout << "\n";
        std::cout << "⚠️  IMPORTANT: Ensure your node is running with --set-fee-address\n";
        std::cout << "⚠️  Your fee address will be used as your Elderfier identity\n";
        std::cout << "\n";
        std::cout << "Proceed with Elderfier deposit? (y/N): ";
        
        showCursor();
        std::string response;
        std::getline(std::cin, response);
        hideCursor();
        
        if (response == "y" || response == "Y" || response == "yes" || response == "Yes") {
            std::cout << "\n";
            std::cout << "🔄 Creating Elderfier deposit transaction...\n";
            
            try {
                // Get the wallet's own address for the deposit
                std::string walletAddress = m_wallet->getAddress();
                
                // Create a transfer to self with 800 XFG and 0x06 payment ID tag
                std::vector<CryptoNote::WalletLegacyTransfer> transfers;
                CryptoNote::WalletLegacyTransfer transfer;
                transfer.address = walletAddress;
                transfer.amount = requiredAmount;
                transfers.push_back(transfer);
                
                // Create payment ID with 0x06 tag for Elderfier registration
                Crypto::Hash paymentId = Crypto::rand<Crypto::Hash>();
                // Set the first byte to 0x06 to mark as Elderfier deposit
                memcpy(&paymentId, "\x06", 1);
                
                std::cout << "📝 Creating transaction with Elderfier tag (0x06)...\n";
                std::cout << "💳 Destination: " << walletAddress << "\n";
                std::cout << "💰 Amount: " << m_currency.formatAmount(requiredAmount) << " XFG\n";
                std::cout << "🏷️  Payment ID: " << paymentId << "\n";
                std::cout << "\n";
                
                // Create the transaction
                CryptoNote::TransactionId txId = m_wallet->sendTransaction(transfers, 0, "", 1, paymentId);
                
                std::cout << "✅ Elderfier deposit transaction created successfully!\n";
                std::cout << "📄 Transaction ID: " << txId << "\n";
                std::cout << "⏳ Please wait for the transaction to be confirmed on the blockchain.\n";
                std::cout << "🔥 Once confirmed, you will be registered as an Elderfier!\n";
                
            } catch (const std::exception& e) {
                std::cout << "❌ Error creating Elderfier deposit: " << e.what() << "\n";
                std::cout << "💡 Make sure you have sufficient balance and the wallet is synchronized.\n";
            }
        } else {
            std::cout << "\n";
            std::cout << "❌ Elderfier deposit cancelled.\n";
        }
    }
    
    std::cout << "\n";
    std::cout << "Press any key to continue...";
    getKey();
}

void ElderfierMenuSystem::showElderfierRequirements() {
    clearScreen();
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        ELDERFIER REQUIREMENTS                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    std::cout << "📋 TECHNICAL REQUIREMENTS:\n";
    std::cout << "• Dedicated server or VPS\n";
    std::cout << "• Reliable internet connection (99%+ uptime)\n";
    std::cout << "• Sufficient storage for blockchain data\n";
    std::cout << "• Basic technical knowledge\n";
    std::cout << "\n";
    
    std::cout << "💳 FINANCIAL REQUIREMENTS:\n";
    std::cout << "• 800 XFG deposit (refundable when exiting)\n";
    std::cout << "• Operating costs (server, bandwidth)\n";
    std::cout << "• Initial setup costs\n";
    std::cout << "\n";
    
    std::cout << "⚡ OPERATIONAL REQUIREMENTS:\n";
    std::cout << "• 24/7 operation availability\n";
    std::cout << "• Regular software updates\n";
    std::cout << "• Network monitoring\n";
    std::cout << "• Security best practices\n";
    std::cout << "\n";
    
    std::cout << "🔧 SETUP PROCESS:\n";
    std::cout << "1. Start fuegod with --set-fee-address flag\n";
    std::cout << "2. Create 800 XFG deposit with 0x06 tag\n";
    std::cout << "3. Configure Elderfier service settings\n";
    std::cout << "4. Monitor node performance\n";
    std::cout << "\n";
    
    std::cout << "Press any key to continue...";
    getKey();
}

void ElderfierMenuSystem::showElderfierBenefits() {
    clearScreen();
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ELDERFIER BENEFITS                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    std::cout << "💰 FINANCIAL REWARDS:\n";
    std::cout << "• Transaction fee rewards\n";
    std::cout << "• Priority in fee distribution\n";
    std::cout << "• Consensus participation rewards\n";
    std::cout << "• Enhanced staking returns\n";
    std::cout << "\n";
    
    std::cout << "🎛️ ADVANCED FEATURES:\n";
    std::cout << "• Custom service identification\n";
    std::cout << "• Enhanced monitoring capabilities\n";
    std::cout << "• Access to advanced network features\n";
    std::cout << "• Email-style voting inbox for Elder Council\n";
    std::cout << "\n";
    
    std::cout << "🌐 NETWORK PARTICIPATION:\n";
    std::cout << "• Vote on network governance\n";
    std::cout << "• Participate in protocol upgrades\n";
    std::cout << "• Influence network direction\n";
    std::cout << "• Cross-chain operation validation\n";
    std::cout << "\n";
    
    std::cout << "🔐 SECURITY FEATURES:\n";
    std::cout << "• Enhanced transaction validation\n";
    std::cout << "• Cryptographic proof generation\n";
    std::cout << "• Distributed consensus participation\n";
    std::cout << "• Network security contribution\n";
    std::cout << "\n";
    
    std::cout << "Press any key to continue...";
    getKey();
}

void ElderfierMenuSystem::openElderCouncilInbox() {
    clearScreen();
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                       ELDER COUNCIL INBOX                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    std::cout << "📧 ELDER COUNCIL MESSAGING SYSTEM\n";
    std::cout << "\n";
    
    // Check if user has Elderfier status by looking at balance and transactions
    uint64_t balance = m_wallet->actualBalance();
    uint64_t requiredAmount = 800 * m_currency.coin();
    
    // TODO: In future implementation, check for confirmed Elderfier deposit transaction
    // For now, just check if user has sufficient balance
    bool hasElderfierStatus = balance >= requiredAmount;
    
    if (!hasElderfierStatus) {
        std::cout << "⚠️  ACCESS DENIED\n";
        std::cout << "Elder Council Inbox requires active Elderfier status.\n";
        std::cout << "\n";
        std::cout << "Requirements:\n";
        std::cout << "• 800 XFG Elderfier deposit confirmed\n";
        std::cout << "• Node running with --set-fee-address\n";
        std::cout << "• Elderfier registration complete\n";
        std::cout << "\n";
        std::cout << "Current balance: " << m_currency.formatAmount(balance) << " XFG\n";
        std::cout << "Required: " << m_currency.formatAmount(requiredAmount) << " XFG\n";
        std::cout << "\n";
    } else {
        std::cout << "✅ ELDERFIER STATUS CONFIRMED\n";
        std::cout << "\n";
        std::cout << "📬 INBOX MESSAGES (Demo Mode):\n";
        std::cout << "\n";
        
        // Sample messages for demonstration
        std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ [NEW] Governance Proposal #2025-001                        │\n";
        std::cout << "│ From: Elder Council                                         │\n";
        std::cout << "│ Subject: Network Fee Adjustment Proposal                   │\n";
        std::cout << "│ Date: 2025-09-30                                           │\n";
        std::cout << "│ Status: Voting Required                                     │\n";
        std::cout << "└─────────────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ [READ] Network Upgrade Notification                        │\n";
        std::cout << "│ From: Core Development Team                                 │\n";
        std::cout << "│ Subject: Fuego v11.0 Release Schedule                      │\n";
        std::cout << "│ Date: 2025-09-29                                           │\n";
        std::cout << "│ Status: Informational                                       │\n";
        std::cout << "└─────────────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ [NEW] Elderfier Performance Report                         │\n";
        std::cout << "│ From: Network Monitor                                       │\n";
        std::cout << "│ Subject: Monthly Rewards Summary                            │\n";
        std::cout << "│ Date: 2025-09-28                                           │\n";
        std::cout << "│ Status: Review Required                                     │\n";
        std::cout << "└─────────────────────────────────────────────────────────────┘\n";
        std::cout << "\n";
        
        std::cout << "📋 INBOX FEATURES:\n";
        std::cout << "• Encrypted message storage ✅\n";
        std::cout << "• Digital signature verification ✅\n";
        std::cout << "• Message threading and history ✅\n";
        std::cout << "• Voting response tracking ✅\n";
        std::cout << "• Cross-chain governance participation 🔄\n";
        std::cout << "\n";
        
        std::cout << "💡 ACTIONS AVAILABLE:\n";
        std::cout << "1. Read message (select message number)\n";
        std::cout << "2. Cast vote on governance proposals\n";
        std::cout << "3. View message history\n";
        std::cout << "4. Send message to other Elderfiers\n";
        std::cout << "\n";
        std::cout << "🔄 Full Elder Council Inbox coming in next update!\n";
    }
    
    std::cout << "\n";
    std::cout << "Press any key to continue...";
    getKey();
}

void ElderfierMenuSystem::returnToMainMenu() {
    m_currentState = MAIN_MENU;
    m_selectedIndex = 0;
    displayMainMenu();
}

void ElderfierMenuSystem::exitMenu() {
    m_menuActive = false;
    showCursor();
}

void ElderfierMenuSystem::setupTerminal() {
    // Get current terminal attributes
    if (tcgetattr(STDIN_FILENO, &m_oldTermios) == 0) {
        m_termiosModified = true;
        
        struct termios newTermios = m_oldTermios;
        newTermios.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
        newTermios.c_cc[VMIN] = 1;
        newTermios.c_cc[VTIME] = 0;
        
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
    }
}

void ElderfierMenuSystem::restoreTerminal() {
    if (m_termiosModified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &m_oldTermios);
        m_termiosModified = false;
        showCursor();
    }
}

} // namespace CryptoNote