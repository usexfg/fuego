use anchor_lang::prelude::*;
use anchor_spl::token::{self, Token, TokenAccount, Mint, Burn};

pub mod state;
pub mod error;

use state::*;
use error::*;

/// Percentage of the losing vault that is permanently burned (in basis points = 8%)
const BURN_BPS: u16 = 800; // 8%

declare_id!("FuEGo11111111111111111111111111111111111111");

#[program]
pub mod fuego_forecast {
    use super::*;

    /// Initialize the FuegoForecast prediction market
    pub fn initialize_forecast(
        ctx: Context<InitializeForecast>,
        epoch_duration: i64,
        fee_bps: u16,
        price_buffer_bps: u16,
        max_position_size: Option<u64>,
        max_vault_imbalance_bps: Option<u16>,
        require_multiple_oracles: Option<bool>,
        deposit_cutoff_hours: Option<i64>,
        enable_temporal_bonuses: Option<bool>,
    ) -> Result<()> {
        require!(fee_bps <= MAX_FEE_BPS, ForecastError::InvalidFee);
        require!(price_buffer_bps <= MAX_PRICE_BUFFER_BPS, ForecastError::InvalidPriceBuffer);
        require!(epoch_duration > 0 && epoch_duration <= 7 * 24 * 60 * 60, ForecastError::InvalidEpochDuration); // Max 7 days
        
        let cutoff_hours = deposit_cutoff_hours.unwrap_or(DEFAULT_DEPOSIT_CUTOFF_HOURS);
        require!(
            cutoff_hours >= MIN_DEPOSIT_CUTOFF_HOURS && cutoff_hours <= MAX_DEPOSIT_CUTOFF_HOURS,
            ForecastError::InvalidDepositCutoff
        );
        
        let forecast_config = &mut ctx.accounts.forecast_config;
        
        forecast_config.authority = ctx.accounts.authority.key();
        forecast_config.current_epoch = 0;
        forecast_config.epoch_duration = epoch_duration;
        forecast_config.fee_bps = fee_bps;
        forecast_config.price_buffer_bps = price_buffer_bps;
        forecast_config.treasury = ctx.accounts.authority.key(); // Initially set authority as treasury
        forecast_config.bonding_vault = ctx.accounts.bonding_vault.key();
        forecast_config.token_mint = ctx.accounts.token_mint.key();
        forecast_config.is_active = true;
        
        // Anti-manipulation safeguards
        forecast_config.max_position_size = max_position_size.unwrap_or(MAX_POSITION_SIZE);
        forecast_config.max_vault_imbalance_bps = max_vault_imbalance_bps.unwrap_or(MAX_VAULT_IMBALANCE_BPS);
        forecast_config.price_resolution_delay = PRICE_RESOLUTION_DELAY;
        forecast_config.require_multiple_oracles = require_multiple_oracles.unwrap_or(true);
        forecast_config.min_oracle_sources = MIN_ORACLE_SOURCES;
        forecast_config.max_oracle_deviation_bps = MAX_ORACLE_DEVIATION_BPS;
        
        // Temporal gaming prevention
        forecast_config.deposit_cutoff_hours = cutoff_hours;
        forecast_config.early_bird_bonus_bps = EARLY_BIRD_BONUS_BPS;
        forecast_config.late_deposit_penalty_bps = LATE_DEPOSIT_PENALTY_BPS;
        forecast_config.commitment_period_hours = COMMITMENT_PERIOD_HOURS;
        forecast_config.enable_temporal_bonuses = enable_temporal_bonuses.unwrap_or(true);
        
        forecast_config.bump = *ctx.bumps.get("forecast_config").unwrap();

        msg!("FuegoForecast initialized with temporal gaming prevention:");
        msg!("- Deposit cutoff: {} hours before epoch end", cutoff_hours);
        msg!("- Early bird bonus: {}%", forecast_config.early_bird_bonus_bps / 100);
        msg!("- Late deposit penalty: {}%", forecast_config.late_deposit_penalty_bps / 100);
        msg!("- Commitment period: {} hours", forecast_config.commitment_period_hours);
        msg!("- Temporal bonuses enabled: {}", forecast_config.enable_temporal_bonuses);
        Ok(())
    }
    
    /// Start a new prediction epoch
    pub fn start_new_epoch(
        ctx: Context<StartNewEpoch>,
        start_price: u64,
    ) -> Result<()> {
        let forecast_config = &mut ctx.accounts.forecast_config;
        let epoch = &mut ctx.accounts.epoch;
        let current_time = Clock::get()?.unix_timestamp;
        
        require!(forecast_config.is_active, ForecastError::MarketInactive);
        
        forecast_config.current_epoch += 1;
        
        epoch.epoch_id = forecast_config.current_epoch;
        epoch.start_timestamp = current_time;
        epoch.end_timestamp = current_time + forecast_config.epoch_duration;
        epoch.start_price = start_price;
        epoch.close_price = 0;
        epoch.up_vault_total = 0;
        epoch.down_vault_total = 0;
        epoch.total_amount = 0;
        epoch.is_resolved = false;
        epoch.winning_position = None;
        epoch.fee_collected = 0;
        
        // Anti-manipulation initialization
        epoch.oracle_prices = Vec::new();
        epoch.resolution_timestamp = epoch.end_timestamp + forecast_config.price_resolution_delay;
        epoch.is_circuit_breaker_triggered = false;
        epoch.suspicious_activity_detected = false;
        
        // Temporal gaming prevention initialization
        let cutoff_seconds = forecast_config.deposit_cutoff_hours * 60 * 60;
        epoch.deposit_cutoff_timestamp = epoch.end_timestamp - cutoff_seconds;
        epoch.early_deposit_count = 0;
        epoch.late_deposit_count = 0;
        epoch.total_early_bird_bonuses = 0;
        epoch.total_late_penalties = 0;

        emit!(EpochStarted {
            epoch_id: epoch.epoch_id,
            start_price,
            start_timestamp: current_time,
            end_timestamp: epoch.end_timestamp,
            resolution_timestamp: epoch.resolution_timestamp,
            deposit_cutoff_timestamp: epoch.deposit_cutoff_timestamp,
        });

        let cutoff_hours = forecast_config.deposit_cutoff_hours;
        msg!("Epoch {} started: price ${}", epoch.epoch_id, start_price);
        msg!("Deposits accepted until: {} ({} hours before end)", epoch.deposit_cutoff_timestamp, cutoff_hours);
        msg!("Resolution at: {} (5min delay)", epoch.resolution_timestamp);
        Ok(())
    }

    /// Deposit tokens to predict price direction
    pub fn deposit_forecast(
        ctx: Context<DepositForecast>,
        amount: u64,
        position: Position,
    ) -> Result<()> {
        let forecast_config = &ctx.accounts.forecast_config;
        let epoch = &mut ctx.accounts.epoch;
        let user_position = &mut ctx.accounts.user_position;
        let current_time = Clock::get()?.unix_timestamp;
        
        require!(forecast_config.is_active, ForecastError::MarketInactive);
        require!(epoch.is_active(current_time), ForecastError::EpochNotActive);
        require!(!epoch.is_circuit_breaker_triggered, ForecastError::CircuitBreakerTriggered);
        require!(!epoch.suspicious_activity_detected, ForecastError::SuspiciousActivityDetected);
        
        // Temporal gaming prevention - check deposit cutoff
        require!(
            epoch.can_accept_deposits(current_time),
            ForecastError::DepositCutoffPassed
        );
        
        // Anti-manipulation checks
        require!(amount >= MIN_POSITION_SIZE, ForecastError::PositionTooSmall);
        require!(amount <= forecast_config.max_position_size, ForecastError::PositionTooLarge);
        
        // Check if this deposit would create excessive vault imbalance
        let new_up_total = if matches!(position, Position::Up) {
            epoch.up_vault_total + amount
        } else {
            epoch.up_vault_total
        };
        let new_down_total = if matches!(position, Position::Down) {
            epoch.down_vault_total + amount
        } else {
            epoch.down_vault_total
        };
        let new_total = new_up_total + new_down_total;
        
        if new_total > 0 {
            let larger_vault = std::cmp::max(new_up_total, new_down_total);
            let imbalance_bps = (larger_vault * 10000) / new_total;
            require!(
                imbalance_bps <= forecast_config.max_vault_imbalance_bps as u64,
                ForecastError::VaultImbalanceExceeded
            );
        }
        
        // Check for single position being too large relative to vault
        let vault_total = match position {
            Position::Up => new_up_total,
            Position::Down => new_down_total,
        };
        if vault_total > 0 && amount > 0 {
            let position_percentage = (amount * 100) / vault_total;
            require!(position_percentage <= 50, ForecastError::SinglePositionTooLarge); // Max 50% of vault
        }
        
        // Calculate temporal bonuses/penalties
        let deposit_timing = epoch.get_deposit_timing_category(current_time);
        let (temporal_adjustment, is_bonus) = epoch.calculate_temporal_adjustment(amount, deposit_timing, forecast_config);
        
        // Update temporal tracking
        match deposit_timing {
            DepositTiming::EarlyBird => {
                epoch.early_deposit_count += 1;
                if is_bonus {
                    epoch.total_early_bird_bonuses += temporal_adjustment;
                }
            },
            DepositTiming::Late => {
                epoch.late_deposit_count += 1;
                if !is_bonus {
                    epoch.total_late_penalties += temporal_adjustment;
                }
            },
            DepositTiming::Normal => {},
        }
        
        // Transfer tokens from user to program
        let cpi_accounts = token::Transfer {
            from: ctx.accounts.user_token_account.to_account_info(),
            to: ctx.accounts.program_token_account.to_account_info(),
            authority: ctx.accounts.user.to_account_info(),
        };
        let cpi_program = ctx.accounts.token_program.to_account_info();
        let cpi_ctx = CpiContext::new(cpi_program, cpi_accounts);
        token::transfer(cpi_ctx, amount)?;

        // Update epoch totals
        match position {
            Position::Up => epoch.up_vault_total += amount,
            Position::Down => epoch.down_vault_total += amount,
        }
        epoch.total_amount += amount;

        // Update user position
        user_position.user = ctx.accounts.user.key();
        user_position.epoch_id = epoch.epoch_id;
        user_position.position = position;
        user_position.amount += amount; // Allow multiple deposits
        user_position.timestamp = current_time;
        user_position.has_claimed = false;
        
        // Temporal gaming prevention fields
        user_position.is_early_bird = matches!(deposit_timing, DepositTiming::EarlyBird);
        user_position.is_late_deposit = matches!(deposit_timing, DepositTiming::Late);
        user_position.temporal_bonus = temporal_adjustment;
        user_position.commitment_end_time = current_time + (forecast_config.commitment_period_hours * 60 * 60);
        
        // Detect suspicious activity after deposit
        epoch.detect_suspicious_activity();

        emit!(ForecastDeposit {
            user: ctx.accounts.user.key(),
            epoch_id: epoch.epoch_id,
            amount,
            position,
            timestamp: current_time,
            vault_imbalance_bps: if epoch.total_amount > 0 {
                (std::cmp::max(epoch.up_vault_total, epoch.down_vault_total) * 10000) / epoch.total_amount
            } else { 0 },
            deposit_timing,
            temporal_adjustment,
            is_bonus,
        });

        let timing_msg = match deposit_timing {
            DepositTiming::EarlyBird => format!("EARLY BIRD (+{}%)", forecast_config.early_bird_bonus_bps / 100),
            DepositTiming::Late => format!("LATE (-{}%)", forecast_config.late_deposit_penalty_bps / 100),
            DepositTiming::Normal => "NORMAL".to_string(),
        };
        
        msg!("User {} deposited {} tokens for {:?} in epoch {} [{}]", 
             ctx.accounts.user.key(), amount, position, epoch.epoch_id, timing_msg);
        
        if temporal_adjustment > 0 {
            msg!("Temporal adjustment: {} tokens ({})", temporal_adjustment, if is_bonus { "bonus" } else { "penalty" });
        }
        
        Ok(())
    }

    /// Emergency circuit breaker - halt trading if manipulation detected
    pub fn trigger_circuit_breaker(
        ctx: Context<TriggerCircuitBreaker>,
        reason: String,
    ) -> Result<()> {
        let forecast_config = &ctx.accounts.forecast_config;
        let epoch = &mut ctx.accounts.epoch;
        
        // Only authority can trigger circuit breaker
        require!(
            ctx.accounts.authority.key() == forecast_config.authority,
            ForecastError::Unauthorized
        );
        
        epoch.is_circuit_breaker_triggered = true;
        epoch.suspicious_activity_detected = true;
        
        msg!("🚨 CIRCUIT BREAKER TRIGGERED: {}", reason);
        msg!("Trading halted for epoch {}", epoch.epoch_id);
        
        Ok(())
    }

    /// Add oracle price for multi-source validation
    pub fn add_oracle_price(
        ctx: Context<AddOraclePrice>,
        price: u64,
        source: Pubkey,
        confidence: u64,
    ) -> Result<()> {
        let forecast_config = &ctx.accounts.forecast_config;
        let epoch = &mut ctx.accounts.epoch;
        let current_time = Clock::get()?.unix_timestamp;
        
        // Only authority can add oracle prices (in production, this would be automated)
        require!(
            ctx.accounts.authority.key() == forecast_config.authority,
            ForecastError::Unauthorized
        );
        
        require!(!epoch.is_resolved, ForecastError::EpochAlreadyResolved);
        require!(current_time >= epoch.end_timestamp, ForecastError::EpochNotEnded);
        
        let oracle_price = OraclePrice {
            price,
            timestamp: current_time,
            source,
            confidence,
        };
        
        epoch.oracle_prices.push(oracle_price);
        
        msg!("Oracle price added: {} from source {}", price, source);
        Ok(())
    }

    /// Resolve an epoch after its end time with anti-manipulation checks
    pub fn resolve_epoch(
        ctx: Context<ResolveEpoch>,
        manual_close_price: Option<u64>,
    ) -> Result<()> {
        let forecast_config = &ctx.accounts.forecast_config;
        let epoch = &mut ctx.accounts.epoch;
        let current_time = Clock::get()?.unix_timestamp;
        
        require!(forecast_config.is_active, ForecastError::MarketInactive);
        require!(epoch.can_be_resolved(current_time), ForecastError::CannotResolve);
        require!(
            current_time >= epoch.resolution_timestamp,
            ForecastError::ResolutionDelayActive
        );
        
        // Determine close price based on oracle configuration
        let close_price = if forecast_config.require_multiple_oracles {
            // Use oracle consensus
            match epoch.validate_oracle_prices(
                forecast_config.max_oracle_deviation_bps,
                forecast_config.min_oracle_sources,
            ) {
                Some(consensus_price) => consensus_price,
                None => {
                    msg!("Oracle validation failed: insufficient sources or high deviation");
                    return Err(ForecastError::InsufficientOracleSources.into());
                }
            }
        } else {
            // Use manual price (for testing/emergency)
            manual_close_price.ok_or(ForecastError::InvalidAmount)?
        };
        
        epoch.close_price = close_price;
        epoch.is_resolved = true;
        
        // Determine outcome using buffer logic
        let outcome = epoch.determine_outcome(close_price, forecast_config.price_buffer_bps);
        epoch.winning_position = Some(outcome);
        
        // Handle fee collection based on outcome
        match outcome {
            EpochOutcome::Neutral => {
                // No fee collection in neutral outcome - everyone gets their stake back
                epoch.fee_collected = 0;
                msg!("Neutral outcome: price stayed within buffer zone, all stakes returned");
            },
            EpochOutcome::Up | EpochOutcome::Down => {
                // Normal fee collection from losing vault
                let losing_vault_total = epoch.get_losing_vault_total();
                if losing_vault_total > 0 {
                    // 1. Burn 8% of the losing vault to create long-term scarcity
                    let burn_amount: u64 = ((losing_vault_total as u128 * BURN_BPS as u128) / 10_000u128) as u64;
                    if burn_amount > 0 {
                        let cpi_accounts = Burn {
                            from: ctx.accounts.program_token_account.to_account_info(),
                            mint: ctx.accounts.token_mint.to_account_info(),
                            authority: ctx.accounts.forecast_config.to_account_info(),
                        };
                        let signer_seeds: &[&[u8]] = &[b"forecast_config", &[ctx.accounts.forecast_config.bump]];
                        token::burn(CpiContext::new_with_signer(ctx.accounts.token_program.to_account_info(), cpi_accounts, &[signer_seeds]), burn_amount)?;
                    }

                    // 2. Apply protocol fee (5%) on the *remaining* losing amount
                    let remaining_losing: u64 = losing_vault_total - burn_amount;
                    let fee_amount: u64 = ((remaining_losing as u128 * (forecast_config.fee_bps as u128)) / 10_000u128) as u64;

                    if fee_amount > 0 {
                        // 80% → Treasury, 20% → Bonding vault PDA
                        let treasury_share: u64 = (fee_amount * 80) / 100;
                        let bonding_share: u64 = fee_amount - treasury_share;

                        // transfer to treasury
                        let cpi_ctx1 = token::transfer(
                            ctx.accounts.token_program.to_account_info(),
                            ctx.accounts.program_token_account.to_account_info(),
                            ctx.accounts.treasury_token_account.to_account_info(),
                            ctx.accounts.forecast_config.to_account_info(),
                            &[signer_seeds],
                            treasury_share,
                        )?;
                        // transfer to bonding vault
                        let cpi_ctx2 = token::transfer(
                            ctx.accounts.token_program.to_account_info(),
                            ctx.accounts.program_token_account.to_account_info(),
                            ctx.accounts.bonding_vault.to_account_info(),
                            ctx.accounts.forecast_config.to_account_info(),
                            &[signer_seeds],
                            bonding_share,
                        )?;

                        emit!(TreasuryFee {
                            epoch: epoch.epoch_id,
                            amount: treasury_share,
                        });
                        emit!(BondingFee { epoch: epoch.epoch_id, amount: bonding_share });
                    }
                }
            }
        }

        emit!(EpochResolved {
            epoch_id: epoch.epoch_id,
            close_price,
            winning_position: outcome,
            up_vault_total: epoch.up_vault_total,
            down_vault_total: epoch.down_vault_total,
            fee_collected: epoch.fee_collected,
            price_buffer_bps: forecast_config.price_buffer_bps,
            oracle_sources_used: epoch.oracle_prices.len() as u8,
            suspicious_activity_detected: epoch.suspicious_activity_detected,
            timestamp: current_time,
        });

        let buffer_amount = (epoch.start_price * forecast_config.price_buffer_bps as u64) / 10000;
        msg!("Epoch {} resolved: price {} -> {} (buffer: ±{}), outcome: {:?}", 
             epoch.epoch_id, epoch.start_price, close_price, buffer_amount, outcome);
        
        if epoch.suspicious_activity_detected {
            msg!("⚠️  Suspicious activity was detected during this epoch");
        }
        
        Ok(())
    }

    /// Claim rewards after epoch resolution
    pub fn claim_rewards(ctx: Context<ClaimRewards>) -> Result<()> {
        let forecast_config = &ctx.accounts.forecast_config;
        let epoch = &ctx.accounts.epoch;
        let user_position = &mut ctx.accounts.user_position;
        let current_time = Clock::get()?.unix_timestamp;
        
        require!(epoch.is_resolved, ForecastError::EpochNotResolved);
        require!(!user_position.has_claimed, ForecastError::AlreadyClaimed);
        
        // Check commitment period (if enabled)
        if forecast_config.commitment_period_hours > 0 {
            require!(
                current_time >= user_position.commitment_end_time,
                ForecastError::CommitmentPeriodActive
            );
        }
        
        // Calculate base winnings
        let base_winnings = user_position.calculate_winnings(epoch, forecast_config.fee_bps);
        let original_stake = user_position.amount;
        
        // Apply temporal bonuses/penalties to total claim
        let temporal_bonus = if forecast_config.enable_temporal_bonuses {
            user_position.temporal_bonus
        } else {
            0
        };
        
        let total_claim = if user_position.is_early_bird && temporal_bonus > 0 {
            // Early bird gets bonus on top of winnings
            original_stake + base_winnings + temporal_bonus
        } else if user_position.is_late_deposit && temporal_bonus > 0 {
            // Late deposit pays penalty (reduced claim)
            let claim_after_penalty = (original_stake + base_winnings).saturating_sub(temporal_bonus);
            claim_after_penalty
        } else {
            // Normal timing - no adjustment
            original_stake + base_winnings
        };
        
        if total_claim > 0 {
            // Transfer tokens from program to user
            let seeds = &[
                b"forecast_config".as_ref(),
                &[forecast_config.bump],
            ];
            let signer = &[&seeds[..]];
            
            let cpi_accounts = token::Transfer {
                from: ctx.accounts.program_token_account.to_account_info(),
                to: ctx.accounts.user_token_account.to_account_info(),
                authority: ctx.accounts.forecast_config.to_account_info(),
            };
            let cpi_program = ctx.accounts.token_program.to_account_info();
            let cpi_ctx = CpiContext::new_with_signer(cpi_program, cpi_accounts, signer);
            token::transfer(cpi_ctx, total_claim)?;
        }

        // Mark as claimed
        user_position.has_claimed = true;
        user_position.claimed_at = current_time;

        emit!(RewardsClaimed {
            user: ctx.accounts.user.key(),
            epoch_id: epoch.epoch_id,
            original_stake,
            base_winnings,
            temporal_adjustment: temporal_bonus,
            progressive_fee: 0, // Not implemented in basic claim function
            total_claim,
            applied_fee_bps: ctx.accounts.forecast_config.fee_bps, // Use base fee for now
            consecutive_wins: 0, // Not tracked in basic claim function
            is_early_bird: user_position.is_early_bird,
            is_late_deposit: user_position.is_late_deposit,
            cooldown_suggested: false, // Not tracked in basic claim function
            timestamp: current_time,
        });

        let timing_info = if user_position.is_early_bird {
            format!("Early Bird (+{} bonus)", temporal_bonus)
        } else if user_position.is_late_deposit {
            format!("Late Deposit (-{} penalty)", temporal_bonus)
        } else {
            "Normal Timing".to_string()
        };

        msg!("User {} claimed {} tokens for epoch {} [{}]", 
             ctx.accounts.user.key(), total_claim, epoch.epoch_id, timing_info);
        Ok(())
    }
}

// Context structs

#[derive(Accounts)]
pub struct InitializeForecast<'info> {
    #[account(
        init,
        payer = authority,
        space = FORECAST_CONFIG_SIZE,
        seeds = [b"forecast_config"],
        bump
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    pub token_mint: Account<'info, Mint>,
    
    #[account(mut)]
    pub authority: Signer<'info>,
    
    #[account(
        mut,
        constraint = program_token_account.mint == token_mint.key(),
        constraint = program_token_account.owner == forecast_config.key()
    )]
    pub program_token_account: Account<'info, TokenAccount>,

    #[account(
        init,
        payer = authority,
        token::mint = token_mint,
        token::authority = forecast_config,
        seeds = [b"bonding", forecast_config.key().as_ref()],
        bump
    )]
    pub bonding_vault: Account<'info, TokenAccount>,
    
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
#[instruction(start_price: u64)]
pub struct StartNewEpoch<'info> {
    #[account(
        mut,
        seeds = [b"forecast_config"],
        bump = forecast_config.bump,
        constraint = forecast_config.authority == authority.key()
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        init,
        payer = authority,
        space = FORECAST_EPOCH_SIZE,
        seeds = [b"epoch", (forecast_config.current_epoch + 1).to_le_bytes().as_ref()],
        bump
    )]
    pub epoch: Account<'info, ForecastEpoch>,
    
    #[account(mut)]
    pub authority: Signer<'info>,
    
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
#[instruction(position: Position, amount: u64)]
pub struct DepositForecast<'info> {
    #[account(
        seeds = [b"forecast_config"],
        bump = forecast_config.bump
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        mut,
        seeds = [b"epoch", forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump
    )]
    pub epoch: Account<'info, ForecastEpoch>,
    
    #[account(
        init_if_needed,
        payer = user,
        space = USER_POSITION_SIZE,
        seeds = [b"position", user.key().as_ref(), epoch.epoch_id.to_le_bytes().as_ref()],
        bump
    )]
    pub user_position: Account<'info, UserPosition>,
    
    #[account(
        mut,
        constraint = user_token_account.mint == forecast_config.token_mint,
        constraint = user_token_account.owner == user.key()
    )]
    pub user_token_account: Account<'info, TokenAccount>,
    
    #[account(
        mut,
        constraint = program_token_account.mint == forecast_config.token_mint,
        constraint = program_token_account.owner == forecast_config.key()
    )]
    pub program_token_account: Account<'info, TokenAccount>,
    
    #[account(mut)]
    pub user: Signer<'info>,
    
    pub token_program: Program<'info, Token>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
#[instruction(close_price: u64)]
pub struct ResolveEpoch<'info> {
    #[account(
        seeds = [b"forecast_config"],
        bump = forecast_config.bump,
        constraint = forecast_config.authority == authority.key()
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        mut,
        seeds = [b"epoch", epoch.epoch_id.to_le_bytes().as_ref()],
        bump
    )]
    pub epoch: Account<'info, ForecastEpoch>,
    
    #[account(
        mut,
        constraint = program_token_account.mint == forecast_config.token_mint,
        constraint = program_token_account.owner == forecast_config.key()
    )]
    pub program_token_account: Account<'info, TokenAccount>,
    
    #[account(
        mut,
        constraint = treasury_token_account.mint == forecast_config.token_mint,
        constraint = treasury_token_account.owner == forecast_config.treasury
    )]
    pub treasury_token_account: Account<'info, TokenAccount>,

    #[account(
        mut,
        constraint = bonding_vault.mint == forecast_config.token_mint,
        constraint = bonding_vault.key() == forecast_config.bonding_vault
    )]
    pub bonding_vault: Account<'info, TokenAccount>,
    
    pub authority: Signer<'info>,
    pub token_program: Program<'info, Token>,
}

#[derive(Accounts)]
pub struct ClaimRewards<'info> {
    #[account(
        seeds = [b"forecast_config"],
        bump = forecast_config.bump
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        seeds = [b"epoch", user_position.epoch_id.to_le_bytes().as_ref()],
        bump
    )]
    pub epoch: Account<'info, ForecastEpoch>,
    
    #[account(
        mut,
        seeds = [b"position", user.key().as_ref(), user_position.epoch_id.to_le_bytes().as_ref()],
        bump,
        constraint = user_position.user == user.key()
    )]
    pub user_position: Account<'info, UserPosition>,
    
    #[account(
        mut,
        constraint = user_token_account.mint == forecast_config.token_mint,
        constraint = user_token_account.owner == user.key()
    )]
    pub user_token_account: Account<'info, TokenAccount>,
    
    #[account(
        mut,
        constraint = program_token_account.mint == forecast_config.token_mint,
        constraint = program_token_account.owner == forecast_config.key()
    )]
    pub program_token_account: Account<'info, TokenAccount>,
    
    pub user: Signer<'info>,
    pub token_program: Program<'info, Token>,
}

#[derive(Accounts)]
pub struct TriggerCircuitBreaker<'info> {
    #[account(mut)]
    pub authority: Signer<'info>,
    
    #[account(
        seeds = [b"forecast_config"],
        bump = forecast_config.bump,
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        mut,
        seeds = [b"epoch", forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump,
    )]
    pub epoch: Account<'info, ForecastEpoch>,
}

#[derive(Accounts)]
pub struct AddOraclePrice<'info> {
    #[account(mut)]
    pub authority: Signer<'info>,
    
    #[account(
        seeds = [b"forecast_config"],
        bump = forecast_config.bump,
    )]
    pub forecast_config: Account<'info, ForecastConfig>,
    
    #[account(
        mut,
        seeds = [b"epoch", forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump,
    )]
    pub epoch: Account<'info, ForecastEpoch>,
} 