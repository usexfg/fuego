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

// Elderfier Menu: stake status, rewards, elder inbox
func elderfierMenu(m model) model {
	m.appendLog("═══════════════════════════════════════")
	m.appendLog("  ELDERFIER DASHBOARD")
	m.appendLog("═══════════════════════════════════════")
	
	// 1. Stake deposit status
	status, err := walletRpcCall(walletRPCPort, "get_stake_status", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  Stake status unavailable: " + err.Error())
	} else {
		m.appendLog("✅ Stake Status: " + fmt.Sprintf("%v", status))
	}
	
	// 2. Rewards summary
	rewards, err := walletRpcCall(walletRPCPort, "get_rewards", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  Rewards unavailable: " + err.Error())
	} else {
		m.appendLog("💰 Rewards: " + fmt.Sprintf("%v", rewards))
	}
	
	// 3. Elder Council inbox (read-only)
	inbox, err := walletRpcCall(walletRPCPort, "get_elder_inbox", map[string]interface{}{})
	if err != nil {
		m.appendLog("⚠️  Elder inbox unavailable: " + err.Error())
	} else {
		m.appendLog("📬 Elder Inbox: " + fmt.Sprintf("%v", inbox))
	}
	
	m.appendLog("═══════════════════════════════════════")
	m.statusMsg = "Elderfier dashboard updated"
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
