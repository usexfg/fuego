use anchor_lang::prelude::*;

// ============================================================================
// FUEGO FORECAST PREDICTION MARKET STATES
// ============================================================================

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub struct OraclePrice {
    pub price: u64,
    pub timestamp: i64,
    pub source: Pubkey, // Oracle source identifier
    pub confidence: u64, // Confidence interval
}

#[account]
#[derive(InitSpace)]
pub struct ForecastEpoch {
    pub epoch_id: u64,
    pub start_timestamp: i64,
    pub end_timestamp: i64,
    pub start_price: u64,
    pub close_price: u64,
    pub up_vault_total: u64,
    pub down_vault_total: u64,
    pub total_amount: u64,
    pub is_resolved: bool,
    pub winning_position: Option<EpochOutcome>,
    pub fee_collected: u64,
    
    // Anti-manipulation features
    #[max_len(10)] // Maximum 10 oracle sources
    pub oracle_prices: Vec<OraclePrice>, // Multiple oracle sources
    pub resolution_timestamp: i64, // When resolution can occur (with delay)
    pub is_circuit_breaker_triggered: bool, // Emergency pause mechanism
    pub suspicious_activity_detected: bool, // Flag for unusual patterns
    
    // Temporal gaming prevention
    pub deposit_cutoff_timestamp: i64, // When deposits stop being accepted
    pub early_deposit_count: u32, // Number of early deposits (first 25% of epoch)
    pub late_deposit_count: u32, // Number of late deposits (last 25% of epoch)
    pub total_early_bird_bonuses: u64, // Total bonuses paid to early depositors
    pub total_late_penalties: u64, // Total penalties collected from late depositors
}

#[account]
#[derive(InitSpace)]
pub struct UserPosition {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub position: Position,
    pub amount: u64,
    pub timestamp: i64, // When position was created
    pub has_claimed: bool,
    pub claimed_at: i64,
    
    // Temporal gaming prevention
    pub is_early_bird: bool, // Deposited in first 25% of epoch
    pub is_late_deposit: bool, // Deposited in last 25% of epoch (before cutoff)
    pub temporal_bonus: u64, // Bonus/penalty amount applied
    pub commitment_end_time: i64, // When position can be withdrawn (if enabled)
    
    // Progressive fee tracking
    pub applied_fee_bps: u16, // Fee rate applied to this position's winnings
    pub bypass_cooldown_fee_paid: u64, // Bypass fee paid for this position
}

#[account]
#[derive(InitSpace)]
pub struct UserWinStreak {
    pub user: Pubkey,
    pub consecutive_wins: u8, // Current win streak
    pub total_wins: u32, // Total wins across all epochs
    pub last_win_epoch: u64, // Last epoch where user won
    pub current_fee_level: u16, // Current progressive fee level (basis points)
    pub total_progressive_fees_paid: u64, // Total progressive fees paid to treasury
    pub total_bypass_fees_paid: u64, // Total bypass fees paid
    pub voluntary_cooldown_count: u8, // Number of voluntary cooldowns taken
    pub last_activity_epoch: u64, // Last epoch user participated in
    pub cooldown_suggested: bool, // Whether cooldown is currently suggested
    
    // Manual cooldown system
    pub cooldown_active: bool, // Whether user has manually started cooldown
    pub cooldown_start_timestamp: i64, // When cooldown was manually started
    pub cooldown_end_timestamp: i64, // When cooldown period ends
    pub auto_reset_eligible: bool, // Whether user is eligible for auto-reset (2 epochs inactive)
    
    pub bump: u8,
}

#[account]
#[derive(InitSpace)]
pub struct ForecastConfig {
    pub authority: Pubkey,
    pub current_epoch: u64,
    pub epoch_duration: i64,
    pub fee_bps: u16,
    pub price_buffer_bps: u16, // Buffer around baseline price
    pub treasury: Pubkey,
    pub bonding_vault: Pubkey,
    pub token_mint: Pubkey,
    pub is_active: bool,
    
    // Anti-manipulation safeguards
    pub max_position_size: u64,
    pub max_vault_imbalance_bps: u16,
    pub price_resolution_delay: i64,
    pub require_multiple_oracles: bool,
    pub min_oracle_sources: u8,
    pub max_oracle_deviation_bps: u16,
    
    // Temporal gaming prevention
    pub deposit_cutoff_hours: i64, // Hours before epoch end when deposits stop
    pub early_bird_bonus_bps: u16, // Bonus for early deposits
    pub late_deposit_penalty_bps: u16, // Penalty for late deposits
    pub commitment_period_hours: i64, // Minimum time positions must be held
    pub enable_temporal_bonuses: bool, // Enable/disable bonus system
    
    // Progressive fee system for consecutive winners
    pub base_treasury_fee_bps: u16, // Base treasury fee (basis points)
    pub max_progressive_fee_bps: u16, // Maximum total fee (basis points) - 50% at 5 wins
    pub consecutive_win_threshold: u8, // Wins before progressive fees kick in (2)
    pub cooldown_suggestion_threshold: u8, // Wins before cooldown suggestion (2)
    pub cooldown_bypass_fee_bps: u16, // Flat fee to bypass suggested cooldown
    pub enable_progressive_fees: bool, // Enable/disable progressive fee system
    
    pub bump: u8,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub enum Position {
    Up,
    Down,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub enum EpochOutcome {
    Up,
    Down,
    Neutral, // Price stayed within buffer zone
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub enum DepositTiming {
    EarlyBird, // First 25% of epoch
    Normal,    // Middle 50% of epoch
    Late,      // Last 25% of epoch (before cutoff)
}

// Events for the prediction market
#[event]
pub struct EpochStarted {
    pub epoch_id: u64,
    pub start_price: u64,
    pub start_timestamp: i64,
    pub end_timestamp: i64,
    pub resolution_timestamp: i64,
    pub deposit_cutoff_timestamp: i64,
}

#[event]
pub struct ForecastDeposit {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub amount: u64,
    pub position: Position,
    pub timestamp: i64,
    pub vault_imbalance_bps: u64,
    pub deposit_timing: DepositTiming,
    pub temporal_adjustment: u64,
    pub is_bonus: bool,
}

#[event]
pub struct EpochResolved {
    pub epoch_id: u64,
    pub close_price: u64,
    pub winning_position: EpochOutcome,
    pub up_vault_total: u64,
    pub down_vault_total: u64,
    pub fee_collected: u64,
    pub price_buffer_bps: u16,
    pub oracle_sources_used: u8,
    pub suspicious_activity_detected: bool,
    pub timestamp: i64,
}

#[event]
pub struct RewardsClaimed {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub original_stake: u64,
    pub base_winnings: u64,
    pub temporal_adjustment: u64,
    pub progressive_fee: u64,
    pub total_claim: u64,
    pub applied_fee_bps: u16,
    pub consecutive_wins: u8,
    pub is_early_bird: bool,
    pub is_late_deposit: bool,
    pub cooldown_suggested: bool,
    pub timestamp: i64,
}

#[event]
pub struct ProgressiveFeeApplied {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub consecutive_wins: u8,
    pub base_fee_bps: u16,
    pub applied_fee_bps: u16,
    pub fee_amount: u64,
    pub winnings_before_fee: u64,
    pub winnings_after_fee: u64,
    pub cooldown_suggested: bool,
    pub timestamp: i64,
}

#[event]
pub struct VoluntaryCooldownTaken {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub consecutive_wins_before: u8,
    pub fee_level_before: u16,
    pub cooldown_count: u8,
    pub timestamp: i64,
}

#[event]
pub struct CooldownBypassFeePaid {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub consecutive_wins: u8,
    pub bypass_fee_bps: u16,
    pub bypass_fee_amount: u64,
    pub total_bypass_fees_paid: u64,
    pub timestamp: i64,
}

#[event]
pub struct ManualCooldownStarted {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub consecutive_wins_before: u8,
    pub cooldown_start_timestamp: i64,
    pub cooldown_end_timestamp: i64,
    pub cooldown_count: u8,
    pub timestamp: i64,
}

#[event]
pub struct CooldownCompleted {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub cooldown_duration_hours: i64,
    pub was_manual: bool,
    pub timestamp: i64,
}

#[event]
pub struct AutoResetTriggered {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub consecutive_wins_before: u8,
    pub epochs_inactive: u64,
    pub timestamp: i64,
}

#[event]
pub struct TreasuryFee {
    pub epoch: u64,
    pub amount: u64,
}

#[event]
pub struct BondingFee {
    pub epoch: u64,
    pub amount: u64,
}

// Constants for the prediction market
pub const DEFAULT_EPOCH_DURATION: i64 = 8 * 60 * 60; // 8 hours in seconds
pub const DEFAULT_FEE_BPS: u16 = 500; // 5%
pub const MAX_FEE_BPS: u16 = 1000; // 10% maximum fee
pub const DEFAULT_PRICE_BUFFER_BPS: u16 = 50; // 0.5% buffer (50 basis points)
pub const MAX_PRICE_BUFFER_BPS: u16 = 500; // 5% maximum buffer

// Anti-manipulation safeguards
pub const MIN_POSITION_SIZE: u64 = 1_000_000; // 0.001 tokens minimum
pub const MAX_POSITION_SIZE: u64 = 1_000_000_000_000; // 1M tokens maximum per position
pub const MAX_VAULT_IMBALANCE_BPS: u16 = 8000; // 80% max imbalance (prevents 90%+ dominance)
pub const PRICE_RESOLUTION_DELAY: i64 = 300; // 5 minutes after epoch end before resolution
pub const MAX_ORACLE_DEVIATION_BPS: u16 = 1000; // 10% max deviation between oracles
pub const MIN_ORACLE_SOURCES: u8 = 2; // Minimum oracle sources required

// Temporal gaming prevention
pub const DEFAULT_DEPOSIT_CUTOFF_HOURS: i64 = 4; // 4 hours before epoch end - no more deposits
pub const MIN_DEPOSIT_CUTOFF_HOURS: i64 = 2; // Minimum 2 hours cutoff
pub const MAX_DEPOSIT_CUTOFF_HOURS: i64 = 6; // Maximum 6 hours cutoff (3/4 of 8h epoch)
pub const EARLY_BIRD_BONUS_BPS: u16 = 150; // 1.5% bonus for early deposits (first 2 hours)
pub const LATE_DEPOSIT_PENALTY_BPS: u16 = 300; // 3% penalty for late deposits (hours 4-6)
pub const COMMITMENT_PERIOD_HOURS: i64 = 4; // 4 hours minimum commitment period

// Progressive fee system for consecutive winners
pub const DEFAULT_BASE_TREASURY_FEE_BPS: u16 = 500; // 5% base treasury fee
pub const MAX_PROGRESSIVE_FEE_BPS: u16 = 5000; // 50% maximum total fee at 5+ consecutive wins
pub const CONSECUTIVE_WIN_THRESHOLD: u8 = 2; // Start progressive fees after 2 consecutive wins
pub const COOLDOWN_SUGGESTION_THRESHOLD: u8 = 2; // Suggest cooldown after 2 consecutive wins
pub const COOLDOWN_BYPASS_FEE_BPS: u16 = 500; // 5% flat fee to bypass suggested cooldown
pub const COOLDOWN_DURATION_HOURS: i64 = 8; // 8 hours cooldown period
pub const AUTO_RESET_EPOCHS: u64 = 2; // Auto-reset after 2 epochs of inactivity

// Progressive fee schedule: 3rd=10%, 4th=20%, 5th=30%, 6th=40%, 7th+=50%
pub const WIN_3_FEE_BPS: u16 = 1000; // 10% for 3rd consecutive win
pub const WIN_4_FEE_BPS: u16 = 2000; // 20% for 4th consecutive win
pub const WIN_5_FEE_BPS: u16 = 3000; // 30% for 5th consecutive win
pub const WIN_6_FEE_BPS: u16 = 4000; // 40% for 6th consecutive win
pub const WIN_7_PLUS_FEE_BPS: u16 = 5000; // 50% for 7th+ consecutive wins

// ============================================================================
// SYBIL ATTACK DETECTION SYSTEM
// ============================================================================

#[account]
#[derive(InitSpace)]
pub struct WalletClusterAnalysis {
    pub wallet: Pubkey,
    pub cluster_id: Option<u64>, // Assigned cluster ID if part of suspected group
    pub risk_score: u16, // 0-10000 basis points (0-100%)
    pub creation_timestamp: i64,
    pub last_analysis_timestamp: i64,
    
    // Behavioral patterns
    pub total_deposits: u32,
    pub total_epochs_participated: u32,
    pub average_position_size: u64,
    pub position_size_variance: u64, // Statistical variance in position sizes
    pub timing_pattern_score: u16, // How predictable their timing is (0-10000)
    pub win_rate: u16, // Win rate in basis points (0-10000)
    
    // Funding analysis
    pub primary_funding_source: Option<Pubkey>, // Most common funding source
    pub funding_source_count: u8, // Number of different funding sources
    pub funding_source_entropy: u16, // Diversity of funding sources (0-10000)
    pub total_funded_amount: u64,
    pub average_funding_amount: u64,
    pub last_funding_timestamp: i64,
    
    // Network analysis
    pub connected_wallets_count: u8, // Number of wallets with shared patterns
    #[max_len(20)] // Maximum 20 connected wallets tracked
    pub connected_wallets: Vec<Pubkey>,
    #[max_len(10)] // Maximum 10 shared funding sources
    pub shared_funding_sources: Vec<Pubkey>,
    pub interaction_frequency: u16, // How often this wallet interacts with connected ones
    
    // Temporal patterns
    pub most_active_hour: u8, // Hour of day most active (0-23)
    pub activity_concentration: u16, // How concentrated activity is in time (0-10000)
    pub deposit_timing_predictability: u16, // How predictable deposit timing is (0-10000)
    pub cooldown_avoidance_score: u16, // Score indicating cooldown avoidance behavior (0-10000)
    
    // Flags and status
    pub is_flagged: bool, // Manually flagged for review
    pub is_verified_human: bool, // Manually verified as legitimate
    pub auto_flagged_reason: Option<SybilFlag>, // Automatic flagging reason
    pub requires_manual_review: bool,
    pub last_manual_review: i64,
    
    pub bump: u8,
}

#[account]
#[derive(InitSpace)]
pub struct SybilCluster {
    pub cluster_id: u64,
    pub creation_timestamp: i64,
    pub last_updated: i64,
    
    // Cluster composition
    #[max_len(50)] // Maximum 50 wallets per cluster
    pub member_wallets: Vec<Pubkey>,
    pub member_count: u8,
    pub confidence_score: u16, // Confidence this is a Sybil cluster (0-10000)
    
    // Shared characteristics
    #[max_len(10)] // Maximum 10 shared funding sources
    pub shared_funding_sources: Vec<Pubkey>,
    pub similar_timing_patterns: bool,
    pub similar_position_sizes: bool,
    pub coordinated_behavior_detected: bool,
    
    // Risk assessment
    pub risk_level: ClusterRiskLevel,
    pub total_cluster_stake: u64,
    pub average_member_stake: u64,
    pub cluster_win_rate: u16,
    
    // Enforcement actions
    pub is_restricted: bool, // Cluster under restrictions
    #[max_len(100)] // Maximum 100 character restriction reason
    pub restriction_reason: Option<String>,
    pub restriction_timestamp: i64,
    pub requires_kyc: bool, // Requires know-your-customer verification
    
    pub bump: u8,
}

#[account]
#[derive(InitSpace)]
pub struct FundingSourceAnalysis {
    pub funding_source: Pubkey,
    pub analysis_timestamp: i64,
    
    // Distribution patterns
    #[max_len(100)] // Maximum 100 funded wallets tracked
    pub funded_wallets: Vec<Pubkey>,
    pub funded_wallet_count: u16,
    pub total_distributed_amount: u64,
    pub average_distribution_amount: u64,
    pub distribution_variance: u64,
    
    // Timing analysis
    pub distribution_frequency: u16, // Distributions per day
    pub distribution_timing_pattern: u16, // How regular the timing is (0-10000)
    pub burst_distribution_count: u8, // Number of burst distributions detected
    pub last_burst_timestamp: i64,
    
    // Risk indicators
    pub sybil_risk_score: u16, // Risk this source is funding Sybil attacks (0-10000)
    pub exchange_withdrawal_pattern: bool, // Looks like exchange withdrawal
    pub mixer_usage_detected: bool, // Privacy mixer usage detected
    pub new_wallet_funding_ratio: u16, // Ratio of funding new vs existing wallets
    
    // Flags
    pub is_flagged: bool,
    pub is_whitelisted: bool, // Known legitimate source (exchange, etc.)
    pub requires_monitoring: bool,
    
    pub bump: u8,
}

#[account]
#[derive(InitSpace)]
pub struct SybilDetectionConfig {
    pub authority: Pubkey,
    pub is_active: bool,
    
    // Risk thresholds
    pub high_risk_threshold: u16, // Risk score threshold for high risk (basis points)
    pub auto_flag_threshold: u16, // Risk score threshold for auto-flagging
    pub cluster_confidence_threshold: u16, // Confidence threshold for cluster formation
    
    // Analysis parameters
    pub min_wallets_for_cluster: u8, // Minimum wallets to form a cluster
    pub max_cluster_size: u8, // Maximum wallets in a cluster
    pub analysis_lookback_hours: u32, // Hours to look back for pattern analysis
    pub funding_analysis_depth: u8, // Levels of funding source analysis
    
    // Behavioral thresholds
    pub timing_similarity_threshold: u16, // Threshold for similar timing patterns
    pub position_size_similarity_threshold: u16, // Threshold for similar position sizes
    pub funding_concentration_threshold: u16, // Threshold for funding concentration
    pub win_rate_similarity_threshold: u16, // Threshold for similar win rates
    
    // Enforcement settings
    pub enable_auto_restrictions: bool,
    pub require_manual_review_high_risk: bool,
    pub enable_progressive_restrictions: bool,
    pub cluster_stake_limit_multiplier: u16, // Multiplier for individual stake limits
    
    // Cooldown bypass restrictions
    pub restrict_high_risk_bypass: bool, // Prevent high-risk wallets from bypassing cooldown
    pub require_cluster_cooldown: bool, // Require entire cluster to cooldown together
    pub cluster_cooldown_multiplier: u16, // Multiplier for cluster cooldown duration
    
    pub bump: u8,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub enum SybilFlag {
    HighRiskScore,
    SuspiciousFunding,
    CoordinatedTiming,
    SimilarPositionSizes,
    SharedFundingSources,
    CooldownAvoidance,
    ClusterBehavior,
    ManualFlag,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, InitSpace, Debug)]
pub enum ClusterRiskLevel {
    Low,
    Medium,
    High,
    Critical,
}

// Sybil detection events
#[event]
pub struct SybilClusterDetected {
    pub cluster_id: u64,
    pub member_count: u8,
    pub confidence_score: u16,
    pub risk_level: ClusterRiskLevel,
    pub detection_reason: String,
    pub total_cluster_stake: u64,
    pub timestamp: i64,
}

#[event]
pub struct WalletFlagged {
    pub wallet: Pubkey,
    pub risk_score: u16,
    pub flag_reason: SybilFlag,
    pub cluster_id: Option<u64>,
    pub requires_manual_review: bool,
    pub timestamp: i64,
}

#[event]
pub struct FundingSourceFlagged {
    pub funding_source: Pubkey,
    pub funded_wallet_count: u16,
    pub sybil_risk_score: u16,
    pub total_distributed_amount: u64,
    pub timestamp: i64,
}

#[event]
pub struct ClusterRestrictionApplied {
    pub cluster_id: u64,
    pub restriction_type: String,
    pub affected_wallets: u8,
    pub timestamp: i64,
}

#[event]
pub struct CooldownBypassBlocked {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub risk_score: u16,
    pub cluster_id: Option<u64>,
    pub block_reason: String,
    pub timestamp: i64,
}

// ============================================================================
// SYBIL DETECTION CONSTANTS
// ============================================================================

// Risk scoring thresholds
pub const HIGH_RISK_THRESHOLD: u16 = 7000; // 70% risk score
pub const AUTO_FLAG_THRESHOLD: u16 = 8000; // 80% risk score
pub const CRITICAL_RISK_THRESHOLD: u16 = 9000; // 90% risk score
pub const CLUSTER_CONFIDENCE_THRESHOLD: u16 = 7500; // 75% confidence for cluster formation

// Cluster analysis parameters
pub const MIN_WALLETS_FOR_CLUSTER: u8 = 3; // Minimum 3 wallets to form cluster
pub const MAX_CLUSTER_SIZE: u8 = 50; // Maximum 50 wallets per cluster
pub const ANALYSIS_LOOKBACK_HOURS: u32 = 168; // 1 week lookback for analysis
pub const FUNDING_ANALYSIS_DEPTH: u8 = 3; // 3 levels of funding source analysis

// Behavioral similarity thresholds
pub const TIMING_SIMILARITY_THRESHOLD: u16 = 8000; // 80% timing similarity
pub const POSITION_SIZE_SIMILARITY_THRESHOLD: u16 = 9000; // 90% position size similarity
pub const FUNDING_CONCENTRATION_THRESHOLD: u16 = 8500; // 85% funding from same sources
pub const WIN_RATE_SIMILARITY_THRESHOLD: u16 = 500; // 5% win rate difference threshold

// Funding source risk indicators
pub const BURST_DISTRIBUTION_THRESHOLD: u8 = 10; // 10+ distributions in short time = burst
pub const NEW_WALLET_FUNDING_THRESHOLD: u16 = 8000; // 80% funding new wallets = suspicious
pub const DISTRIBUTION_TIMING_REGULARITY_THRESHOLD: u16 = 9000; // 90% timing regularity

// Enforcement parameters
pub const CLUSTER_STAKE_LIMIT_MULTIPLIER: u16 = 5000; // 50% of individual limit for clusters
pub const CLUSTER_COOLDOWN_MULTIPLIER: u16 = 2; // 2x cooldown duration for clusters
pub const HIGH_RISK_BYPASS_BLOCK_THRESHOLD: u16 = 8000; // Block bypass at 80% risk

// Size calculations for new accounts
pub const FORECAST_EPOCH_SIZE: usize = 8 + // discriminator
    8 +  // epoch_id
    8 +  // start_timestamp
    8 +  // end_timestamp
    8 +  // start_price
    8 +  // close_price
    8 +  // up_vault_total
    8 +  // down_vault_total
    8 +  // total_amount
    1 +  // is_resolved
    1 + 1 + // Option<Position> (1 byte for Some/None + 1 byte for enum)
    8;   // fee_collected

pub const USER_POSITION_SIZE: usize = 8 + // discriminator
    32 + // user
    8 +  // epoch_id
    1 +  // position (enum)
    8 +  // amount
    8 +  // timestamp
    1 +  // has_claimed
    8 +  // claimed_at
    1 +  // is_early_bird
    1 +  // is_late_deposit
    8 +  // temporal_bonus
    8 +  // commitment_end_time
    2 +  // applied_fee_bps
    8;   // bypass_cooldown_fee_paid

pub const USER_WIN_STREAK_SIZE: usize = 8 + // discriminator
    32 + // user
    1 +  // consecutive_wins
    4 +  // total_wins
    8 +  // last_win_epoch
    2 +  // current_fee_level
    8 +  // total_progressive_fees_paid
    8 +  // total_bypass_fees_paid
    1 +  // voluntary_cooldown_count
    8 +  // last_activity_epoch
    1 +  // cooldown_suggested
    1 +  // cooldown_active
    8 +  // cooldown_start_timestamp
    8 +  // cooldown_end_timestamp
    1 +  // auto_reset_eligible
    1;   // bump

pub const FORECAST_CONFIG_SIZE: usize = 8 + // discriminator
    32 + // authority
    8 +  // current_epoch
    8 +  // epoch_duration
    2 +  // fee_bps
    2 +  // price_buffer_bps
    32 + // treasury
    32 + // bonding_vault
    32 + // token_mint
    1 +  // is_active
    8 +  // max_position_size
    2 +  // max_vault_imbalance_bps
    8 +  // price_resolution_delay
    1 +  // require_multiple_oracles
    1 +  // min_oracle_sources
    2 +  // max_oracle_deviation_bps
    8 +  // deposit_cutoff_hours
    2 +  // early_bird_bonus_bps
    2 +  // late_deposit_penalty_bps
    8 +  // commitment_period_hours
    1 +  // enable_temporal_bonuses
    2 +  // base_treasury_fee_bps
    2 +  // max_progressive_fee_bps
    1 +  // consecutive_win_threshold
    1 +  // cooldown_suggestion_threshold
    2 +  // cooldown_bypass_fee_bps
    1 +  // enable_progressive_fees
    1;   // bump

// Sybil detection account sizes
pub const WALLET_CLUSTER_ANALYSIS_SIZE: usize = 8 + // discriminator
    32 + // wallet
    (8 + 1) + // cluster_id (Option<u64>)
    2 + // risk_score
    8 + // creation_timestamp
    8 + // last_analysis_timestamp
    4 + // total_deposits
    4 + // total_epochs_participated
    8 + // average_position_size
    8 + // position_size_variance
    2 + // timing_pattern_score
    2 + // win_rate
    (32 + 1) + // primary_funding_source (Option<Pubkey>)
    1 + // funding_source_count
    2 + // funding_source_entropy
    8 + // total_funded_amount
    8 + // average_funding_amount
    8 + // last_funding_timestamp
    1 + // connected_wallets_count
    (4 + 20 * 32) + // connected_wallets Vec<Pubkey>
    (4 + 10 * 32) + // shared_funding_sources Vec<Pubkey>
    2 + // interaction_frequency
    1 + // most_active_hour
    2 + // activity_concentration
    2 + // deposit_timing_predictability
    2 + // cooldown_avoidance_score
    1 + // is_flagged
    1 + // is_verified_human
    (1 + 1) + // auto_flagged_reason (Option<SybilFlag>)
    1 + // requires_manual_review
    8 + // last_manual_review
    1; // bump

pub const SYBIL_CLUSTER_SIZE: usize = 8 + // discriminator
    8 + // cluster_id
    8 + // creation_timestamp
    8 + // last_updated
    (4 + 50 * 32) + // member_wallets Vec<Pubkey>
    1 + // member_count
    2 + // confidence_score
    (4 + 10 * 32) + // shared_funding_sources Vec<Pubkey>
    1 + // similar_timing_patterns
    1 + // similar_position_sizes
    1 + // coordinated_behavior_detected
    1 + // risk_level
    8 + // total_cluster_stake
    8 + // average_member_stake
    2 + // cluster_win_rate
    1 + // is_restricted
    (4 + 100) + // restriction_reason (Option<String>)
    8 + // restriction_timestamp
    1 + // requires_kyc
    1; // bump

pub const FUNDING_SOURCE_ANALYSIS_SIZE: usize = 8 + // discriminator
    32 + // funding_source
    8 + // analysis_timestamp
    (4 + 100 * 32) + // funded_wallets Vec<Pubkey>
    2 + // funded_wallet_count
    8 + // total_distributed_amount
    8 + // average_distribution_amount
    8 + // distribution_variance
    2 + // distribution_frequency
    2 + // distribution_timing_pattern
    1 + // burst_distribution_count
    8 + // last_burst_timestamp
    2 + // sybil_risk_score
    1 + // exchange_withdrawal_pattern
    1 + // mixer_usage_detected
    2 + // new_wallet_funding_ratio
    1 + // is_flagged
    1 + // is_whitelisted
    1 + // requires_monitoring
    1; // bump

pub const SYBIL_DETECTION_CONFIG_SIZE: usize = 8 + // discriminator
    32 + // authority
    1 + // is_active
    2 + // high_risk_threshold
    2 + // auto_flag_threshold
    2 + // cluster_confidence_threshold
    1 + // min_wallets_for_cluster
    1 + // max_cluster_size
    4 + // analysis_lookback_hours
    1 + // funding_analysis_depth
    2 + // timing_similarity_threshold
    2 + // position_size_similarity_threshold
    2 + // funding_concentration_threshold
    2 + // win_rate_similarity_threshold
    1 + // enable_auto_restrictions
    1 + // require_manual_review_high_risk
    1 + // enable_progressive_restrictions
    2 + // cluster_stake_limit_multiplier
    1 + // restrict_high_risk_bypass
    1 + // require_cluster_cooldown
    2 + // cluster_cooldown_multiplier
    1; // bump

impl ForecastEpoch {
    pub fn is_active(&self, current_time: i64) -> bool {
        current_time >= self.start_timestamp && current_time < self.end_timestamp
    }
    
    pub fn can_accept_deposits(&self, current_time: i64) -> bool {
        current_time >= self.start_timestamp && 
        current_time < self.deposit_cutoff_timestamp &&
        !self.is_circuit_breaker_triggered
    }
    
    pub fn can_be_resolved(&self, current_time: i64) -> bool {
        !self.is_resolved && 
        current_time >= self.resolution_timestamp && 
        !self.is_circuit_breaker_triggered
    }
    
    pub fn get_deposit_timing_category(&self, deposit_time: i64) -> DepositTiming {
        let epoch_duration = self.end_timestamp - self.start_timestamp;
        let early_bird_cutoff = self.start_timestamp + (epoch_duration / 4); // First 2 hours of 8h epoch
        let late_deposit_start = self.start_timestamp + (epoch_duration / 2); // Hours 4-6 of 8h epoch (before 4h cutoff)
        
        if deposit_time <= early_bird_cutoff {
            DepositTiming::EarlyBird
        } else if deposit_time >= late_deposit_start && deposit_time < self.deposit_cutoff_timestamp {
            DepositTiming::Late
        } else {
            DepositTiming::Normal
        }
    }
    
    pub fn calculate_temporal_adjustment(&self, amount: u64, timing: DepositTiming, config: &ForecastConfig) -> (u64, bool) {
        if !config.enable_temporal_bonuses {
            return (0, false);
        }
        
        match timing {
            DepositTiming::EarlyBird => {
                let bonus = (amount * config.early_bird_bonus_bps as u64) / 10000;
                (bonus, true) // true = bonus
            },
            DepositTiming::Late => {
                let penalty = (amount * config.late_deposit_penalty_bps as u64) / 10000;
                (penalty, false) // false = penalty
            },
            DepositTiming::Normal => (0, false),
        }
    }
    
    pub fn detect_temporal_gaming(&self) -> bool {
        let total_deposits = self.early_deposit_count + self.late_deposit_count;
        if total_deposits == 0 {
            return false;
        }
        
        // Flag if >40% of deposits are in the "late" window (hours 4-6 before 4h cutoff)
        // This indicates users are trying to game the system by waiting
        let late_deposit_ratio = (self.late_deposit_count * 100) / total_deposits;
        
        late_deposit_ratio > 40
    }
    
    pub fn determine_outcome(&self, close_price: u64, buffer_bps: u16) -> EpochOutcome {
        let buffer_amount = (self.start_price * buffer_bps as u64) / 10000;
        let upper_threshold = self.start_price + buffer_amount;
        let lower_threshold = self.start_price.saturating_sub(buffer_amount);
        
        if close_price > upper_threshold {
            EpochOutcome::Up
        } else if close_price < lower_threshold {
            EpochOutcome::Down
        } else {
            EpochOutcome::Neutral
        }
    }
    
    // Anti-manipulation checks
    pub fn check_vault_imbalance(&self, max_imbalance_bps: u16) -> bool {
        if self.total_amount == 0 {
            return false; // No imbalance if no deposits
        }
        
        let larger_vault = std::cmp::max(self.up_vault_total, self.down_vault_total);
        let imbalance_bps = (larger_vault * 10000) / self.total_amount;
        
        imbalance_bps > max_imbalance_bps as u64
    }
    
    pub fn validate_oracle_prices(&self, max_deviation_bps: u16, min_sources: u8) -> Option<u64> {
        if self.oracle_prices.len() < min_sources as usize {
            return None; // Insufficient oracle sources
        }
        
        if self.oracle_prices.is_empty() {
            return None; // No oracle prices available
        }
        
        // Calculate median price to reduce manipulation
        let mut prices: Vec<u64> = self.oracle_prices.iter().map(|p| p.price).collect();
        prices.sort();
        
        let median_price = if prices.len() % 2 == 0 {
            (prices[prices.len() / 2 - 1] + prices[prices.len() / 2]) / 2
        } else {
            prices[prices.len() / 2]
        };
        
        // Check if all prices are within acceptable deviation from median
        for price in &prices {
            let deviation = if *price > median_price {
                ((*price - median_price) * 10000) / median_price
            } else {
                ((median_price - *price) * 10000) / median_price
            };
            
            if deviation > max_deviation_bps as u64 {
                return None; // Oracle price deviation too high
            }
        }
        
        Some(median_price)
    }
    
    pub fn detect_suspicious_activity(&mut self) -> bool {
        // Flag 1: Extreme vault imbalance (>95%)
        let extreme_imbalance = if self.total_amount > 0 {
            let larger_vault = std::cmp::max(self.up_vault_total, self.down_vault_total);
            (larger_vault * 100) / self.total_amount > 95
        } else {
            false
        };
        
        // Flag 2: Temporal gaming detected
        let temporal_gaming = self.detect_temporal_gaming();
        
        if extreme_imbalance || temporal_gaming {
            self.suspicious_activity_detected = true;
        }
        
        self.suspicious_activity_detected
    }
    
    pub fn get_losing_vault_total(&self) -> u64 {
        if let Some(outcome) = &self.winning_position {
            match outcome {
                EpochOutcome::Up => self.down_vault_total,
                EpochOutcome::Down => self.up_vault_total,
                EpochOutcome::Neutral => 0, // No losing vault in neutral outcome
            }
        } else {
            0
        }
    }
    
    pub fn get_winning_vault_total(&self) -> u64 {
        if let Some(outcome) = &self.winning_position {
            match outcome {
                EpochOutcome::Up => self.up_vault_total,
                EpochOutcome::Down => self.down_vault_total,
                EpochOutcome::Neutral => self.up_vault_total + self.down_vault_total, // Both vaults "win" in neutral
            }
        } else {
            0
        }
    }
}

impl UserWinStreak {
    pub fn calculate_progressive_fee(&self, config: &ForecastConfig) -> u16 {
        if !config.enable_progressive_fees {
            return config.base_treasury_fee_bps;
        }

        // Fixed progressive fee schedule based on consecutive wins
        match self.consecutive_wins {
            0 | 1 => config.base_treasury_fee_bps, // 5% for wins 1-2
            2 => config.base_treasury_fee_bps,    // 5% for win 2 (threshold but no increase yet)
            3 => WIN_3_FEE_BPS,                   // 10% for win 3
            4 => WIN_4_FEE_BPS,                   // 20% for win 4
            5 => WIN_5_FEE_BPS,                   // 30% for win 5
            6 => WIN_6_FEE_BPS,                   // 40% for win 6
            _ => WIN_7_PLUS_FEE_BPS,              // 50% for win 7+
        }
    }

    pub fn update_on_win(&mut self, epoch_id: u64, progressive_fee_paid: u64) {
        self.consecutive_wins = self.consecutive_wins.saturating_add(1);
        self.total_wins = self.total_wins.saturating_add(1);
        self.last_win_epoch = epoch_id;
        self.last_activity_epoch = epoch_id;
        self.total_progressive_fees_paid = self.total_progressive_fees_paid.saturating_add(progressive_fee_paid);
        
        // Update cooldown suggestion status
        self.cooldown_suggested = self.consecutive_wins >= COOLDOWN_SUGGESTION_THRESHOLD;
        
        // Reset auto-reset eligibility since user is active
        self.auto_reset_eligible = false;
        
        // Note: current_fee_level will be updated when config is available
    }

    pub fn update_on_loss(&mut self, epoch_id: u64) {
        self.consecutive_wins = 0; // Reset streak on loss
        self.current_fee_level = 0; // Reset fee level
        self.cooldown_suggested = false; // Reset cooldown suggestion
        self.last_activity_epoch = epoch_id;
        
        // Reset auto-reset eligibility since user is active
        self.auto_reset_eligible = false;
    }

    pub fn start_manual_cooldown(&mut self, current_timestamp: i64) {
        self.cooldown_active = true;
        self.cooldown_start_timestamp = current_timestamp;
        self.cooldown_end_timestamp = current_timestamp + (COOLDOWN_DURATION_HOURS * 60 * 60);
        self.cooldown_suggested = false; // Clear suggestion since user is taking cooldown
        self.voluntary_cooldown_count = self.voluntary_cooldown_count.saturating_add(1);
        
        // Reset streak and fees immediately when cooldown starts
        self.consecutive_wins = 0;
        self.current_fee_level = 0;
        self.auto_reset_eligible = false;
    }

    pub fn complete_cooldown(&mut self) {
        self.cooldown_active = false;
        self.cooldown_start_timestamp = 0;
        self.cooldown_end_timestamp = 0;
        // Streak and fees already reset when cooldown started
    }

    pub fn check_auto_reset(&mut self, current_epoch: u64) -> bool {
        // Check if user has been inactive for 2+ epochs
        let epochs_inactive = current_epoch.saturating_sub(self.last_activity_epoch);
        
        if epochs_inactive >= AUTO_RESET_EPOCHS {
            // Auto-reset the streak
            self.consecutive_wins = 0;
            self.current_fee_level = 0;
            self.cooldown_suggested = false;
            self.auto_reset_eligible = true;
            return true; // Reset occurred
        }
        
        false // No reset
    }

    pub fn is_cooldown_complete(&self, current_timestamp: i64) -> bool {
        self.cooldown_active && current_timestamp >= self.cooldown_end_timestamp
    }

    pub fn pay_bypass_fee(&mut self, bypass_fee: u64) {
        self.total_bypass_fees_paid = self.total_bypass_fees_paid.saturating_add(bypass_fee);
        // Note: cooldown suggestion remains true, but user can continue playing
    }

    pub fn is_cooldown_suggested(&self) -> bool {
        self.cooldown_suggested
    }

    pub fn update_fee_level(&mut self, config: &ForecastConfig) {
        self.current_fee_level = self.calculate_progressive_fee(config);
    }
}

impl UserPosition {
    pub fn calculate_winnings(&self, epoch: &ForecastEpoch, fee_bps: u16) -> u64 {
        if !epoch.is_resolved || epoch.winning_position.is_none() {
            return 0;
        }
        
        let outcome = epoch.winning_position.as_ref().unwrap();
        
        // Handle neutral outcome - everyone gets their stake back, no winnings
        if matches!(outcome, EpochOutcome::Neutral) {
            return 0; // Original stake returned, no additional winnings
        }
        
        // Check if user's position matches the winning outcome
        let user_won = match (outcome, &self.position) {
            (EpochOutcome::Up, Position::Up) => true,
            (EpochOutcome::Down, Position::Down) => true,
            _ => false,
        };
        
        if !user_won {
            return 0; // User lost
        }
        
        let losing_vault_total = epoch.get_losing_vault_total();
        let winning_vault_total = match outcome {
            EpochOutcome::Up => epoch.up_vault_total,
            EpochOutcome::Down => epoch.down_vault_total,
            EpochOutcome::Neutral => unreachable!(), // Already handled above
        };
        
        if winning_vault_total == 0 || losing_vault_total == 0 {
            return 0; // No winnings if no opposing bets or no winning vault
        }
        
        // Calculate fee and prize pool
        let fee_amount = (losing_vault_total * fee_bps as u64) / 10000;
        let prize_pool = losing_vault_total - fee_amount;
        
        // Calculate user's share of the prize pool
        let user_winnings = (prize_pool * self.amount) / winning_vault_total;
        
        user_winnings
    }
    
    pub fn calculate_winnings_with_progressive_fee(&self, epoch: &ForecastEpoch, win_streak: &UserWinStreak, config: &ForecastConfig) -> (u64, u16, u64) {
        if !epoch.is_resolved || epoch.winning_position.is_none() {
            return (0, 0, 0);
        }
        
        let outcome = epoch.winning_position.as_ref().unwrap();
        
        // Handle neutral outcome - everyone gets their stake back, no winnings
        if matches!(outcome, EpochOutcome::Neutral) {
            return (0, 0, 0); // No winnings, no fee, no fee amount
        }
        
        // Check if user's position matches the winning outcome
        let user_won = match (outcome, &self.position) {
            (EpochOutcome::Up, Position::Up) => true,
            (EpochOutcome::Down, Position::Down) => true,
            _ => false,
        };
        
        if !user_won {
            return (0, 0, 0); // User lost
        }
        
        let losing_vault_total = epoch.get_losing_vault_total();
        let winning_vault_total = match outcome {
            EpochOutcome::Up => epoch.up_vault_total,
            EpochOutcome::Down => epoch.down_vault_total,
            EpochOutcome::Neutral => unreachable!(), // Already handled above
        };
        
        if winning_vault_total == 0 || losing_vault_total == 0 {
            return (0, 0, 0); // No winnings if no opposing bets or no winning vault
        }
        
        // Calculate user's share of the losing vault (before fees)
        let user_share = (losing_vault_total * self.amount) / winning_vault_total;
        
        // Calculate progressive fee rate for this user (simplified - no large win discrimination)
        let progressive_fee_bps = win_streak.calculate_progressive_fee(config);
        
        // Apply progressive fee
        let fee_amount = (user_share * progressive_fee_bps as u64) / 10000;
        let net_winnings = user_share.saturating_sub(fee_amount);
        
        (net_winnings, progressive_fee_bps, fee_amount)
    }
    
    pub fn calculate_total_claim(&self, epoch: &ForecastEpoch, fee_bps: u16) -> u64 {
        // In neutral outcome, everyone gets their original stake back
        if let Some(EpochOutcome::Neutral) = epoch.winning_position {
            return self.amount;
        }
        
        // Otherwise, original stake + winnings (if any)
        self.amount + self.calculate_winnings(epoch, fee_bps)
    }
    
    pub fn calculate_total_claim_with_progressive_fee(&self, epoch: &ForecastEpoch, win_streak: &UserWinStreak, config: &ForecastConfig) -> u64 {
        // In neutral outcome, everyone gets their original stake back
        if let Some(EpochOutcome::Neutral) = epoch.winning_position {
            return self.amount;
        }
        
        // Otherwise, original stake + winnings (with progressive fee applied)
        let (winnings, _, _) = self.calculate_winnings_with_progressive_fee(epoch, win_streak, config);
        self.amount + winnings
    }
} 