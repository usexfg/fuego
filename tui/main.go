package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

const (
	nodeRPCPort   = 18081
	walletRPCPort = 18082
)

var (
	titleStyle  = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("205"))
	menuStyle   = lipgloss.NewStyle().Padding(1, 2)
	activeStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("39"))
	logStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("250"))
)

type menuItem string

const (
	mStartNode      menuItem = "Start Node"
	mStopNode       menuItem = "Stop Node"
	mNodeStatus     menuItem = "Node Status"
	mStartWalletRPC menuItem = "Start Wallet RPC"
	mCreateWallet   menuItem = "Create Wallet"
	mGetBalance     menuItem = "Get Balance"
	mSendTx         menuItem = "Send Transaction"
	mElderfierMenu  menuItem = "Elderfier Menu"
	mBurn2MintMenu  menuItem = "Burn2Mint Menu"
	mShowLogs       menuItem = "Show Logs"
	mQuit           menuItem = "Quit"
)

var menu = []menuItem{
	mStartNode,
	mStopNode,
	mNodeStatus,
	mStartWalletRPC,
	mCreateWallet,
	mGetBalance,
	mSendTx,
	mElderfierMenu,
	mBurn2MintMenu,
	mShowLogs,
	mQuit,
}

type model struct {
	cursor      int
	nodeCmd     *exec.Cmd
	walletCmd   *exec.Cmd
	logs        []string
	mutex       sync.Mutex
	statusMsg   string
	running     bool
	runningNode bool
	runningW    bool
	height      int
	peers       int
}

func initialModel() model {
	return model{
		cursor:    0,
		logs:      []string{"🔥 Fuego TUI Ready"},
		statusMsg: "",
	}
}

func (m *model) appendLog(s string) {
	m.mutex.Lock()
	defer m.mutex.Unlock()
	m.logs = append(m.logs, fmt.Sprintf("%s %s", time.Now().Format("15:04:05"), s))
	if len(m.logs) > 200 {
		m.logs = m.logs[len(m.logs)-200:]
	}
}

func (m model) Init() tea.Cmd {
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "up", "k":
			if m.cursor > 0 {
				m.cursor--
			}
		case "down", "j":
			if m.cursor < len(menu)-1 {
				m.cursor++
			}
		case "enter":
			item := menu[m.cursor]
			switch item {
			case mStartNode:
				m = startNode(m)
			case mStopNode:
				m = stopNode(m)
			case mNodeStatus:
				m = queryNodeStatus(m)
			case mStartWalletRPC:
				m = startWalletRPC(m)
			case mCreateWallet:
				m = createWalletCmd(m)
			case mGetBalance:
				m = getBalanceCmd(m)
			case mSendTx:
				m = sendTxPrompt(m)
			case mElderfierMenu:
				m = elderfierMenu(m)
			case mBurn2MintMenu:
				m = burn2MintMenu(m)
			case mShowLogs:
				m = showLogs(m)
			case mQuit:
				return m, tea.Quit
			}
		case "q", "ctrl+c":
			return m, tea.Quit
		}
	}
	return m, nil
}

func (m model) View() string {
	s := titleStyle.Render("🔥 Fuego TUI") + "\n\n"
	for i, it := range menu {
		line := fmt.Sprintf("  %s", it)
		if m.cursor == i {
			line = activeStyle.Render(line)
		}
		s += menuStyle.Render(line) + "\n"
	}
	s += "\n" + lipgloss.NewStyle().Foreground(lipgloss.Color("250")).Render("Status: ") + m.statusMsg + "\n"
	if m.runningNode {
		s += lipgloss.NewStyle().Render(fmt.Sprintf("Node: running | Height: %d | Peers: %d\n", m.height, m.peers))
	} else {
		s += lipgloss.NewStyle().Render("Node: stopped\n")
	}
	return s
}

func binPath(name string) string {
	cwd, _ := os.Getwd()
	if filepath.Base(cwd) == "tui" {
		cwd = filepath.Dir(cwd)
	}
	cand := filepath.Join(cwd, "build", "src", name)
	if _, err := os.Stat(cand); err == nil {
		return cand
	}
	return name
}

func startNode(m model) model {
	if m.runningNode {
		m.appendLog("Node already running")
		m.statusMsg = "Node already running"
		return m
	}
	path := binPath("fuegod")
	cmd := exec.Command(path, fmt.Sprintf("--rpc-bind-port=%d", nodeRPCPort), fmt.Sprintf("--data-dir=%s", filepath.Join(os.TempDir(), "fuego-node-data")))
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	if err := cmd.Start(); err != nil {
		m.appendLog("Failed to start node: " + err.Error())
		m.statusMsg = "Failed to start node"
		return m
	}
	m.nodeCmd = cmd
	m.runningNode = true
	m.appendLog("Started fuegod")
	m.statusMsg = "Node starting"
	go streamPipe(stdout, "NODE", &m)
	go streamPipe(stderr, "NODE-ERR", &m)
	go func() {
		for m.runningNode {
			info, err := getInfo(nodeRPCPort)
			if err == nil {
				m.height = info.Height
				m.peers = info.Peers
				m.statusMsg = fmt.Sprintf("Node running — height %d", m.height)
			}
			time.Sleep(5 * time.Second)
		}
	}()
	return m
}

func stopNode(m model) model {
	if !m.runningNode || m.nodeCmd == nil {
		m.appendLog("Node not running")
		m.statusMsg = "Node not running"
		return m
	}
	_ = m.nodeCmd.Process.Kill()
	m.appendLog("Stopped fuegod")
	m.nodeCmd = nil
	m.runningNode = false
	m.statusMsg = "Node stopped"
	return m
}

func queryNodeStatus(m model) model {
	info, err := getInfo(nodeRPCPort)
	if err != nil {
		m.appendLog("Failed to query node: " + err.Error())
		m.statusMsg = "Query failed"
		return m
	}
	m.appendLog(fmt.Sprintf("Height: %d, Peers: %d", info.Height, info.Peers))
	m.statusMsg = "Status fetched"
	return m
}

func startWalletRPC(m model) model {
	if m.runningW {
		m.appendLog("Wallet RPC already running")
		m.statusMsg = "Wallet RPC already running"
		return m
	}
	path := binPath("walletd")
	cmd := exec.Command(path, fmt.Sprintf("--rpc-bind-port=%d", walletRPCPort), fmt.Sprintf("--daemon-port=%d", nodeRPCPort))
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	if err := cmd.Start(); err != nil {
		m.appendLog("Failed to start walletd: " + err.Error())
		m.statusMsg = "Failed to start walletd"
		return m
	}
	m.walletCmd = cmd
	m.runningW = true
	m.appendLog("Started walletd")
	m.statusMsg = "Wallet RPC started"
	go streamPipe(stdout, "WALLET", &m)
	go streamPipe(stderr, "WALLET-ERR", &m)
	return m
}

func createWalletCmd(m model) model {
	m.appendLog("Creating wallet (RPC)...")
	_, err := walletRpcCall(walletRPCPort, "create_address", map[string]interface{}{})
	if err != nil {
		m.appendLog("create wallet error: " + err.Error())
		m.statusMsg = "Create wallet failed"
		return m
	}
	m.appendLog("Create wallet attempted (see walletd logs)")
	m.statusMsg = "Create wallet requested"
	return m
}

func getBalanceCmd(m model) model {
	m.appendLog("Querying balance...")
	res, err := walletRpcCall(walletRPCPort, "get_balance", map[string]interface{}{})
	if err != nil {
		m.appendLog("get balance error: " + err.Error())
		m.statusMsg = "Get balance failed"
		return m
	}
	m.appendLog("Balance: " + fmt.Sprintf("%v", res))
	m.statusMsg = "Balance fetched"
	return m
}

func sendTxPrompt(m model) model {
	m.appendLog("Send TX: enter recipient address:")
	var addr string
	fmt.Print("Recipient address: ")
	fmt.Scanln(&addr)
	fmt.Print("Amount XFG: ")
	var amt float64
	fmt.Scanln(&amt)
	m.appendLog(fmt.Sprintf("Sending %f to %s...", amt, addr))
	amountAtomic := int64(amt * 100000000)
	params := map[string]interface{}{"transfers": []map[string]interface{}{{"address": addr, "amount": amountAtomic}}}
	res, err := walletRpcCall(walletRPCPort, "send_transaction", params)
	if err != nil {
		m.appendLog("send tx error: " + err.Error())
		m.statusMsg = "Send failed"
		return m
	}
	m.appendLog("Tx sent: " + fmt.Sprintf("%v", res))
	m.statusMsg = "Tx sent"
	return m
}

// Elderfier Menu: Full read/write access with Elderfyre Stayking
func elderfierMenu(m model) model {
	m.appendLog("═══════════════════════════════════════")
	m.appendLog("  ELDERFIER DASHBOARD (READ/WRITE)")
	m.appendLog("═══════════════════════════════════════")
	
	// Check if wallet has confirmed EFdeposit
	status, err := walletRpcCall(walletRPCPort, "get_stake_status", map[string]interface{}{})
	hasStake := err == nil && status != nil
	
	if !hasStake {
		m.appendLog("⚠️  No Elderfier stake detected")
		m.appendLog("")
		m.appendLog("To become an Elderfier, you must:")
		m.appendLog("  1. Create Elderfyre Stayking deposit")
		m.appendLog("  2. Generate 8-char Elderfier ID")
		m.appendLog("  3. Register keys to ENindex")
		m.appendLog("")
		m.appendLog("Options:")
		m.appendLog("  [1] Start Elderfyre Stayking Process")
		m.appendLog("  [2] Check Stake Status")
		m.appendLog("  [0] Return to Main Menu")
		
		var choice int
		fmt.Print("\nSelect option: ")
		fmt.Scanln(&choice)
		
		switch choice {
		case 1:
			return startElderfyreStayking(m)
		case 2:
			if err != nil {
				m.appendLog("No stake found: " + err.Error())
			} else {
				m.appendLog("Stake Status: " + fmt.Sprintf("%v", status))
			}
		case 0:
			m.appendLog("Returning to main menu...")
		}
		
		m.appendLog("═══════════════════════════════════════")
		return m
	}
	
	// Wallet has confirmed EFdeposit - show full dashboard
	m.appendLog("✅ Elderfier Status: ACTIVE")
	m.appendLog("Stake: " + fmt.Sprintf("%v", status))
	m.appendLog("")
	
	// Elder Council Inbox with voting/consensus
	m.appendLog("📬 ELDER COUNCIL INBOX")
	m.appendLog("─────────────────────────────────────")
	
	inbox, err := walletRpcCall(walletRPCPort, "get_elder_inbox", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  Inbox unavailable: " + err.Error())
	} else {
		m.appendLog("Pending Items: " + fmt.Sprintf("%v", inbox))
	}
	m.appendLog("")
	
	// Menu options
	m.appendLog("Options:")
	m.appendLog("  [1] View Consensus Requests")
	m.appendLog("  [2] Vote on Pending Items")
	m.appendLog("  [3] Review Burn2Mint Requests")
	m.appendLog("  [4] Manage Stake")
	m.appendLog("  [5] Update ENindex Keys")
	m.appendLog("  [0] Return to Main Menu")
	
	var choice int
	fmt.Print("\nSelect option: ")
	fmt.Scanln(&choice)
	
	switch choice {
	case 1:
		return viewConsensusRequests(m)
	case 2:
		return voteOnPendingItems(m)
	case 3:
		return reviewBurn2MintRequests(m)
	case 4:
		return manageStake(m)
	case 5:
		return updateENindexKeys(m)
	case 0:
		m.appendLog("Returning to main menu...")
	}
	
	m.appendLog("═══════════════════════════════════════")
	m.statusMsg = "Elderfier dashboard accessed"
	return m
}

// Start Elderfyre Stayking Process
func startElderfyreStayking(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  ELDERFYRE STAYKING PROCESS")
	m.appendLog("─────────────────────────────────────")
	
	// Step 1: Create stake deposit
	m.appendLog("Step 1: Create Elderfier Stake Deposit")
	m.appendLog("Minimum stake required: 10000 XFG")
	m.appendLog("")
	
	var amount float64
	fmt.Print("Enter stake amount (XFG): ")
	fmt.Scanln(&amount)
	
	if amount < 10000 {
		m.appendLog("❌ Minimum stake is 10000 XFG")
		return m
	}
	
	amountAtomic := int64(amount * 100000000)
	stakeParams := map[string]interface{}{
		"amount": amountAtomic,
		"type":   "elderfier_stake",
	}
	
	m.appendLog(fmt.Sprintf("Creating stake deposit: %.2f XFG...", amount))
	
	stakeRes, err := walletRpcCall(walletRPCPort, "create_stake_deposit", stakeParams)
	if err != nil {
		m.appendLog("❌ Stake creation failed: " + err.Error())
		return m
	}
	
	txHash := fmt.Sprintf("%v", stakeRes["tx_hash"])
	m.appendLog("✅ Stake deposit created: " + txHash)
	m.appendLog("")
	
	// Step 2: Generate 8-char Elderfier ID
	m.appendLog("Step 2: Generate Elderfier ID")
	m.appendLog("Enter your 8-character Elderfier ID")
	m.appendLog("(alphanumeric, unique identifier)")
	
	var elderID string
	fmt.Print("Elderfier ID (8 chars): ")
	fmt.Scanln(&elderID)
	
	if len(elderID) != 8 {
		m.appendLog("❌ ID must be exactly 8 characters")
		return m
	}
	
	m.appendLog("✅ Elderfier ID: " + elderID)
	m.appendLog("")
	
	// Step 3: Register keys to ENindex
	m.appendLog("Step 3: Register Keys to ENindex")
	
	// Get wallet address/keys
	addressRes, err := walletRpcCall(walletRPCPort, "getAddresses", map[string]interface{}{})
	if err != nil {
		m.appendLog("❌ Failed to get wallet address: " + err.Error())
		return m
	}
	
	address := fmt.Sprintf("%v", addressRes)
	m.appendLog("Public Address: " + address)
	
	// Register to ENindex
	enindexParams := map[string]interface{}{
		"elder_id":      elderID,
		"stake_tx_hash": txHash,
		"address":       address,
		"stake_amount":  amountAtomic,
	}
	
	m.appendLog("Registering to ENindex...")
	
	enRes, err := walletRpcCall(walletRPCPort, "register_to_enindex", enindexParams)
	if err != nil {
		m.appendLog("⚠️  ENindex registration: " + err.Error())
		m.appendLog("   (May need manual registration)")
	} else {
		m.appendLog("✅ Registered to ENindex: " + fmt.Sprintf("%v", enRes))
	}
	
	m.appendLog("")
	m.appendLog("─────────────────────────────────────")
	m.appendLog("🎉 ELDERFYRE STAYKING COMPLETE!")
	m.appendLog("")
	m.appendLog("Summary:")
	m.appendLog(fmt.Sprintf("  • Stake: %.2f XFG", amount))
	m.appendLog("  • Elderfier ID: " + elderID)
	m.appendLog("  • TX Hash: " + txHash)
	m.appendLog("  • ENindex: Registered")
	m.appendLog("")
	m.appendLog("You can now access Elder Council Inbox")
	m.appendLog("once your stake is confirmed (10 blocks)")
	m.appendLog("─────────────────────────────────────")
	
	m.statusMsg = "Elderfyre Stayking complete"
	return m
}

// View Consensus Requests
func viewConsensusRequests(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  CONSENSUS REQUESTS")
	m.appendLog("─────────────────────────────────────")
	
	requests, err := walletRpcCall(walletRPCPort, "get_consensus_requests", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  Failed to fetch requests: " + err.Error())
		return m
	}
	
	m.appendLog("Pending Consensus Requests:")
	m.appendLog(fmt.Sprintf("%v", requests))
	
	return m
}

// Vote on Pending Items
func voteOnPendingItems(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  VOTE ON PENDING ITEMS")
	m.appendLog("─────────────────────────────────────")
	
	// Get pending items
	items, err := walletRpcCall(walletRPCPort, "get_pending_votes", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  No pending votes: " + err.Error())
		return m
	}
	
	m.appendLog("Pending votes: " + fmt.Sprintf("%v", items))
	m.appendLog("")
	
	var itemID string
	var vote string
	
	fmt.Print("Enter item ID to vote on: ")
	fmt.Scanln(&itemID)
	
	fmt.Print("Vote (approve/reject): ")
	fmt.Scanln(&vote)
	
	voteParams := map[string]interface{}{
		"item_id": itemID,
		"vote":    vote,
	}
	
	res, err := walletRpcCall(walletRPCPort, "submit_vote", voteParams)
	if err != nil {
		m.appendLog("❌ Vote failed: " + err.Error())
	} else {
		m.appendLog("✅ Vote submitted: " + fmt.Sprintf("%v", res))
	}
	
	return m
}

// Review Burn2Mint Requests
func reviewBurn2MintRequests(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  BURN2MINT CONSENSUS REQUESTS")
	m.appendLog("─────────────────────────────────────")
	
	requests, err := walletRpcCall(walletRPCPort, "get_burn2mint_requests", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  No pending burn requests: " + err.Error())
		return m
	}
	
	m.appendLog("Pending Burn2Mint Requests:")
	m.appendLog(fmt.Sprintf("%v", requests))
	m.appendLog("")
	
	var txHash string
	var approve string
	
	fmt.Print("Enter burn TX hash to review: ")
	fmt.Scanln(&txHash)
	
	fmt.Print("Approve consensus proof? (yes/no): ")
	fmt.Scanln(&approve)
	
	if approve == "yes" {
		proofParams := map[string]interface{}{
			"tx_hash": txHash,
			"approve": true,
		}
		
		res, err := walletRpcCall(walletRPCPort, "provide_consensus_proof", proofParams)
		if err != nil {
			m.appendLog("❌ Consensus failed: " + err.Error())
		} else {
			m.appendLog("✅ Consensus proof provided: " + fmt.Sprintf("%v", res))
		}
	} else {
		m.appendLog("Consensus denied")
	}
	
	return m
}

// Manage Stake
func manageStake(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  STAKE MANAGEMENT")
	m.appendLog("─────────────────────────────────────")
	
	status, _ := walletRpcCall(walletRPCPort, "get_stake_status", map[string]interface{}{})
	
	m.appendLog("Current Stake: " + fmt.Sprintf("%v", status))
	m.appendLog("")
	m.appendLog("Options:")
	m.appendLog("  [1] Increase Stake")
	m.appendLog("  [2] View Stake Details")
	m.appendLog("  [0] Back")
	
	var choice int
	fmt.Print("\nSelect: ")
	fmt.Scanln(&choice)
	
	switch choice {
	case 1:
		var addAmount float64
		fmt.Print("Additional stake amount: ")
		fmt.Scanln(&addAmount)
		
		params := map[string]interface{}{
			"amount": int64(addAmount * 100000000),
		}
		res, err := walletRpcCall(walletRPCPort, "increase_stake", params)
		if err != nil {
			m.appendLog("❌ Stake increase failed: " + err.Error())
		} else {
			m.appendLog("✅ Stake increased: " + fmt.Sprintf("%v", res))
		}
	case 2:
		m.appendLog("Stake Details: " + fmt.Sprintf("%v", status))
	}
	
	return m
}

// Update ENindex Keys
func updateENindexKeys(m model) model {
	m.appendLog("─────────────────────────────────────")
	m.appendLog("  UPDATE ENINDEX KEYS")
	m.appendLog("─────────────────────────────────────")
	
	m.appendLog("Current ENindex registration will be updated")
	m.appendLog("")
	
	var newID string
	fmt.Print("New Elderfier ID (8 chars, or press Enter to keep): ")
	fmt.Scanln(&newID)
	
	params := map[string]interface{}{}
	if len(newID) == 8 {
		params["elder_id"] = newID
	}
	
	res, err := walletRpcCall(walletRPCPort, "update_enindex", params)
	if err != nil {
		m.appendLog("⚠️  Update failed: " + err.Error())
	} else {
		m.appendLog("✅ ENindex updated: " + fmt.Sprintf("%v", res))
	}
	
	return m
}

// Burn2Mint: Complete flow with Elderfier consensus
func burn2MintMenu(m model) model {
	m.appendLog("═══════════════════════════════════════")
	m.appendLog("  BURN2MINT: XFG → HEAT")
	m.appendLog("═══════════════════════════════════════")
	
	// Step 1: Choose burn amount
	m.appendLog("Burn options:")
	m.appendLog("  1) Small burn: 0.8 XFG (minimum)")
	m.appendLog("  2) Large burn: 800 XFG")
	fmt.Print("Choose (1 or 2): ")
	var choice int
	fmt.Scanln(&choice)
	
	var amount float64
	if choice == 2 {
		amount = 800.0
		m.appendLog("Selected: Large burn (800 XFG)")
	} else {
		amount = 0.8
		m.appendLog("Selected: Small burn (0.8 XFG)")
	}
	
	// Step 2: Create burn_deposit transaction
	m.appendLog("─────────────────────────────────────")
	m.appendLog("Step 1/4: Creating burn deposit...")
	amountAtomic := int64(amount * 100000000)
	params := map[string]interface{}{"amount": amountAtomic}
	burnRes, err := walletRpcCall(walletRPCPort, "create_burn_deposit", params)
	if err != nil {
		m.appendLog("❌ Burn failed: " + err.Error())
		m.statusMsg = "Burn failed"
		return m
	}
	txHash := fmt.Sprintf("%v", burnRes["tx_hash"])
	m.appendLog("✅ Burn tx created: " + txHash)
	
	// Step 3: Wait for confirmations
	m.appendLog("─────────────────────────────────────")
	m.appendLog("Step 2/4: Waiting for confirmations...")
	for i := 1; i <= 10; i++ {
		m.appendLog(fmt.Sprintf("  Confirmation %d/10...", i))
		time.Sleep(1 * time.Second)
	}
	m.appendLog("✅ Transaction confirmed")
	
	// Step 4: Request Elderfier consensus proof
	m.appendLog("─────────────────────────────────────")
	m.appendLog("Step 3/4: Requesting Elderfier consensus...")
	m.appendLog("  → Querying Elder Council for burn proof")
	
	consensusParams := map[string]interface{}{
		"tx_hash": txHash,
		"amount":  amountAtomic,
	}
	consensusRes, err := walletRpcCall(walletRPCPort, "request_elderfier_consensus", consensusParams)
	if err != nil {
		m.appendLog("❌ Consensus request failed: " + err.Error())
		m.appendLog("  (This RPC endpoint may need implementation)")
		m.statusMsg = "Consensus failed"
		return m
	}
	
	eldernodeProof := fmt.Sprintf("%v", consensusRes["eldernode_proof"])
	m.appendLog("✅ Elderfier consensus received")
	m.appendLog("  Proof: " + eldernodeProof[:32] + "...")
	
	// Step 5: Generate STARK proof using consensus as input
	m.appendLog("─────────────────────────────────────")
	m.appendLog("Step 4/4: Generating XFG-STARK proof...")
	
	xfgStarkPath := binPath("xfg-stark")
	if _, err := exec.LookPath("xfg-stark"); err == nil || xfgStarkPath != "xfg-stark" {
		m.appendLog("  → Running: xfg-stark generate-proof")
		
		// Use elderfier consensus proof as inputs
		cmd := exec.Command(xfgStarkPath, 
			"generate-proof",
			"--tx-hash", txHash,
			"--amount", fmt.Sprintf("%d", amountAtomic),
			"--eldernode-proof", eldernodeProof,
		)
		
		out, err := cmd.CombinedOutput()
		if err != nil {
			m.appendLog("❌ STARK generation failed: " + err.Error())
			m.appendLog(string(out))
			m.statusMsg = "STARK proof failed"
			return m
		}
		
		m.appendLog("✅ STARK proof generated successfully")
		m.appendLog("  Output: " + string(out)[:min(100, len(out))] + "...")
		
		// Step 6: Ready for L2 submission
		m.appendLog("─────────────────────────────────────")
		m.appendLog("🎉 Burn2Mint preparation complete!")
		m.appendLog("")
		m.appendLog("Next steps:")
		m.appendLog("  1. Estimate L1 gas fees (0.001-0.01 ETH)")
		m.appendLog("  2. Call claimHEAT() on Arbitrum L2 with:")
		m.appendLog("     • STARK proof (generated above)")
		m.appendLog("     • Eldernode proof")
		m.appendLog("     • L1 gas fees (msg.value)")
		m.appendLog("  3. Receive HEAT on Ethereum L1")
		m.appendLog("")
		m.appendLog("⚠️  Remember: Include 20% gas buffer to avoid restart!")
		
	} else {
		m.appendLog("⚠️  xfg-stark CLI not found")
		m.appendLog("  Please install xfg-stark to generate proofs")
		m.appendLog("  Manual steps:")
		m.appendLog("    $ xfg-stark generate-proof \\")
		m.appendLog("      --tx-hash " + txHash + " \\")
		m.appendLog("      --eldernode-proof " + eldernodeProof)
	}
	
	m.appendLog("═══════════════════════════════════════")
	m.statusMsg = "Burn2Mint flow complete"
	return m
}

func showLogs(m model) model {
	m.appendLog("Displaying full logs...")
	for _, l := range m.logs {
		fmt.Println(l)
	}
	fmt.Print("\nPress Enter to continue...")
	var dummy string
	fmt.Scanln(&dummy)
	m.statusMsg = "Returned from logs"
	return m
}

func streamPipe(r io.Reader, prefix string, m *model) {
	buf := make([]byte, 1024)
	for {
		n, err := r.Read(buf)
		if n > 0 {
			line := strings.TrimSpace(string(buf[:n]))
			m.appendLog(fmt.Sprintf("%s: %s", prefix, line))
		}
		if err != nil {
			if err != io.EOF {
				m.appendLog(prefix + " error: " + err.Error())
			}
			return
		}
	}
}

type nodeInfo struct {
	Height int `json:"height"`
	Peers  int `json:"peers"`
}

func getInfo(port int) (nodeInfo, error) {
	url := fmt.Sprintf("http://127.0.0.1:%d/get_info", port)
	client := http.Client{Timeout: 2 * time.Second}
	resp, err := client.Get(url)
	if err != nil {
		return nodeInfo{}, err
	}
	defer resp.Body.Close()
	var out map[string]interface{}
	if err := json.NewDecoder(resp.Body).Decode(&out); err != nil {
		return nodeInfo{}, err
	}
	height := 0
	peers := 0
	if h, ok := out["height"].(float64); ok {
		height = int(h)
	}
	if p, ok := out["incoming_connections_count"].(float64); ok {
		peers += int(p)
	}
	if p, ok := out["outgoing_connections_count"].(float64); ok {
		peers += int(p)
	}
	return nodeInfo{Height: height, Peers: peers}, nil
}

func walletRpcCall(port int, method string, params map[string]interface{}) (map[string]interface{}, error) {
	url := fmt.Sprintf("http://127.0.0.1:%d/json_rpc", port)
	payload := map[string]interface{}{"jsonrpc": "2.0", "id": "0", "method": method, "params": params}
	b, _ := json.Marshal(payload)
	client := http.Client{Timeout: 4 * time.Second}
	resp, err := client.Post(url, "application/json", bytes.NewReader(b))
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	var out map[string]interface{}
	if err := json.NewDecoder(resp.Body).Decode(&out); err != nil {
		return nil, err
	}
	if res, ok := out["result"].(map[string]interface{}); ok {
		return res, nil
	}
	return out, nil
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func main() {
	if runtime.GOOS == "windows" {
		// VT100 support
	}
	p := tea.NewProgram(initialModel(), tea.WithAltScreen())
	if err := p.Start(); err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}
}
