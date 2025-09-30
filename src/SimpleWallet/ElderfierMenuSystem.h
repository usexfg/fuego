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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <termios.h>
#include <unistd.h>

#include "IWalletLegacy.h"
#include "CryptoNoteCore/Currency.h"
#include <Logging/LoggerRef.h>

namespace CryptoNote {

    class ElderfierMenuSystem {
    public:
        ElderfierMenuSystem(const CryptoNote::Currency& currency, 
                           CryptoNote::IWalletLegacy* wallet,
                           Logging::LoggerRef& logger);
        ~ElderfierMenuSystem();

        // Main menu entry point
        bool showElderfierMenu();
        
        // Check if node is running with --set-fee-address
        bool isNodeConfiguredForElderfiers(const std::string& feeAddress);

    private:
        // Menu states
        enum MenuState {
            MAIN_MENU,
            STAYKING_MENU,
            INBOX_MENU
        };

        // Menu item structure
        struct MenuItem {
            std::string text;
            std::function<void()> action;
            bool enabled;
        };

        // Core functionality
        void displayMainMenu();
        void displayStaykingMenu();
        void displayInboxMenu();
        void handleNavigation();
        
        // Navigation helpers
        char getKey();
        void moveCursorUp();
        void moveCursorDown();
        void selectCurrentItem();
        void clearScreen();
        void hideCursor();
        void showCursor();
        
        // Menu actions
        void createElderfierDeposit();
        void showElderfierRequirements();
        void showElderfierBenefits();
        void openElderCouncilInbox();
        void returnToMainMenu();
        void exitMenu();

        // Terminal handling
        void setupTerminal();
        void restoreTerminal();

        // Member variables
        const CryptoNote::Currency& m_currency;
        CryptoNote::IWalletLegacy* m_wallet;
        Logging::LoggerRef& m_logger;
        
        MenuState m_currentState;
        std::vector<MenuItem> m_mainMenuItems;
        std::vector<MenuItem> m_staykingMenuItems;
        int m_selectedIndex;
        bool m_menuActive;
        bool m_hiddenTitleClickable;
        
        struct termios m_oldTermios;
        bool m_termiosModified;
    };

} // namespace CryptoNote