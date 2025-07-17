use anchor_lang::prelude::*;

#[error_code]
pub enum ForecastError {
    #[msg("Invalid fee percentage")]
    InvalidFee,
    
    #[msg("Invalid epoch duration")]
    InvalidEpochDuration,
    
    #[msg("Invalid price buffer percentage")]
    InvalidPriceBuffer,
    
    #[msg("Market is currently inactive")]
    MarketInactive,
    
    #[msg("Invalid amount specified")]
    InvalidAmount,
    
    #[msg("Epoch is closed for deposits")]
    EpochClosed,
    
    #[msg("Rewards have already been claimed")]
    AlreadyClaimed,
    
    #[msg("Position mismatch - user already has a different position")]
    PositionMismatch,
    
    #[msg("Cannot resolve epoch yet")]
    CannotResolve,
    
    #[msg("Epoch is not resolved")]
    EpochNotResolved,
    
    #[msg("Unauthorized access")]
    Unauthorized,
    
    #[msg("Position size too small")]
    PositionTooSmall,
    
    #[msg("Position size exceeds maximum allowed")]
    PositionTooLarge,
    
    #[msg("Vault imbalance exceeds maximum threshold")]
    VaultImbalanceExceeded,
    
    #[msg("Resolution delay period not yet passed")]
    ResolutionDelayActive,
    
    #[msg("Circuit breaker triggered - trading halted")]
    CircuitBreakerTriggered,
    
    #[msg("Insufficient oracle sources")]
    InsufficientOracleSources,
    
    #[msg("Oracle price deviation too high")]
    OraclePriceDeviationHigh,
    
    #[msg("Suspicious activity detected")]
    SuspiciousActivityDetected,
    
    #[msg("Oracle price too stale")]
    OraclePriceStale,
    
    #[msg("Single position too large relative to vault")]
    SinglePositionTooLarge,
    
    #[msg("Too many large positions from same user")]
    TooManyLargePositions,
    
    #[msg("Epoch is not currently active")]
    EpochNotActive,
    
    #[msg("Epoch has already been resolved")]
    EpochAlreadyResolved,
    
    #[msg("Epoch has not ended yet")]
    EpochNotEnded,
    
    #[msg("Deposit cutoff period has passed")]
    DepositCutoffPassed,
    
    #[msg("Position still in commitment period")]
    CommitmentPeriodActive,
    
    #[msg("Temporal gaming detected")]
    TemporalGamingDetected,
    
    #[msg("Invalid deposit cutoff period")]
    InvalidDepositCutoff,
} 