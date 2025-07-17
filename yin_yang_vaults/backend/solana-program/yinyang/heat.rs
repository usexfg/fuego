use anchor_lang::prelude::*;
use anchor_spl::token::{self, Token, TokenAccount, Mint, Transfer};

// =============================
// HEAT-ONLY FUEGO FORECAST STATE
// =============================

#[account]
pub struct HeatForecastConfig {
    pub authority: Pubkey,
    pub current_epoch: u64,
    pub epoch_duration: i64,
    pub fee_bps: u16,
    pub price_buffer_bps: u16,
    pub treasury: Pubkey,
    pub token_mint: Pubkey, // HEAT SPL token
    pub is_active: bool,
    pub max_position_size: u64,
    pub max_vault_imbalance_bps: u16,
    pub price_resolution_delay: i64,
    pub bump: u8,
    pub protocol_fee_bps: u16, // New: Protocol fee in basis points
    pub protocol_treasury: Pubkey, // New: Treasury for protocol fees
}

#[account]
pub struct HeatForecastEpoch {
    pub epoch_id: u64,
    pub start_timestamp: i64,
    pub end_timestamp: i64,
    pub start_price: u64,
    pub close_price: u64,
    pub up_vault_total: u64,
    pub down_vault_total: u64,
    pub total_amount: u64,
    pub is_resolved: bool,
    pub winning_position: Option<HeatEpochOutcome>,
    pub fee_collected: u64,
    pub resolution_timestamp: i64,
    pub deposit_cutoff_timestamp: i64,
    pub bump: u8,
}

#[account]
pub struct HeatUserPosition {
    pub user: Pubkey,
    pub epoch_id: u64,
    pub position: HeatPosition,
    pub amount: u64,
    pub timestamp: i64,
    pub has_claimed: bool,
    pub claimed_at: i64,
    pub bump: u8,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, Debug)]
pub enum HeatPosition {
    Up,
    Down,
}

#[derive(AnchorSerialize, AnchorDeserialize, Clone, Copy, PartialEq, Eq, Debug)]
pub enum HeatEpochOutcome {
    Up,
    Down,
    Neutral,
}

// =============================
// HEAT-ONLY FUEGO FORECAST PROGRAM
// =============================

#[program]
pub mod fuego_forecast_heat {
    use super::*;

    pub fn initialize_heat_forecast(
        ctx: Context<InitializeHeatForecast>,
        epoch_duration: i64,
        fee_bps: u16,
        price_buffer_bps: u16,
        max_position_size: u64,
        max_vault_imbalance_bps: u16,
        price_resolution_delay: i64,
        protocol_fee_bps: u16, // New: Protocol fee in basis points
        protocol_treasury: Pubkey, // New: Treasury for protocol fees
    ) -> Result<()> {
        let config = &mut ctx.accounts.heat_forecast_config;
        config.authority = ctx.accounts.authority.key();
        config.current_epoch = 0;
        config.epoch_duration = epoch_duration;
        config.fee_bps = fee_bps;
        config.price_buffer_bps = price_buffer_bps;
        config.treasury = ctx.accounts.treasury.key();
        config.token_mint = ctx.accounts.token_mint.key();
        config.is_active = true;
        config.max_position_size = max_position_size;
        config.max_vault_imbalance_bps = max_vault_imbalance_bps;
        config.price_resolution_delay = price_resolution_delay;
        config.protocol_fee_bps = protocol_fee_bps;
        config.protocol_treasury = protocol_treasury;
        config.bump = *ctx.bumps.get("heat_forecast_config").unwrap();
        Ok(())
    }

    pub fn start_new_heat_epoch(
        ctx: Context<StartNewHeatEpoch>,
        start_price: u64,
        end_timestamp: i64,
        deposit_cutoff_timestamp: i64,
    ) -> Result<()> {
        let config = &mut ctx.accounts.heat_forecast_config;
        let epoch = &mut ctx.accounts.heat_epoch;
        require!(config.is_active, CustomError::MarketInactive);
        config.current_epoch += 1;
        epoch.epoch_id = config.current_epoch;
        epoch.start_timestamp = Clock::get()?.unix_timestamp;
        epoch.end_timestamp = end_timestamp;
        epoch.start_price = start_price;
        epoch.close_price = 0;
        epoch.up_vault_total = 0;
        epoch.down_vault_total = 0;
        epoch.total_amount = 0;
        epoch.is_resolved = false;
        epoch.winning_position = None;
        epoch.fee_collected = 0;
        epoch.resolution_timestamp = end_timestamp + config.price_resolution_delay;
        epoch.deposit_cutoff_timestamp = deposit_cutoff_timestamp;
        epoch.bump = *ctx.bumps.get("heat_epoch").unwrap();
        Ok(())
    }

    pub fn deposit_heat(
        ctx: Context<DepositHeat>,
        position: HeatPosition,
        amount: u64,
    ) -> Result<()> {
        let config = &ctx.accounts.heat_forecast_config;
        let epoch = &mut ctx.accounts.heat_epoch;
        let user_position = &mut ctx.accounts.user_position;
        require!(config.is_active, CustomError::MarketInactive);
        require!(!epoch.is_resolved, CustomError::EpochAlreadyResolved);
        require!(amount > 0, CustomError::InvalidAmount);
        require!(amount <= config.max_position_size, CustomError::PositionTooLarge);
        let now = Clock::get()?.unix_timestamp;
        require!(now < epoch.deposit_cutoff_timestamp, CustomError::DepositClosed);
        // Transfer HEAT from user to program vault
        let cpi_accounts = Transfer {
            from: ctx.accounts.user_token_account.to_account_info(),
            to: ctx.accounts.program_token_account.to_account_info(),
            authority: ctx.accounts.user.to_account_info(),
        };
        let cpi_program = ctx.accounts.token_program.to_account_info();
        let cpi_ctx = CpiContext::new(cpi_program, cpi_accounts);
        token::transfer(cpi_ctx, amount)?;
        // Update vault totals
        match position {
            HeatPosition::Up => epoch.up_vault_total += amount,
            HeatPosition::Down => epoch.down_vault_total += amount,
        }
        epoch.total_amount += amount;
        // Update user position
        user_position.user = ctx.accounts.user.key();
        user_position.epoch_id = epoch.epoch_id;
        user_position.position = position;
        user_position.amount += amount;
        user_position.timestamp = now;
        user_position.has_claimed = false;
        user_position.claimed_at = 0;
        user_position.bump = *ctx.bumps.get("user_position").unwrap();
        Ok(())
    }

    pub fn resolve_heat_epoch(
        ctx: Context<ResolveHeatEpoch>,
        close_price: u64,
        outcome: HeatEpochOutcome,
    ) -> Result<()> {
        let config = &ctx.accounts.heat_forecast_config;
        let epoch = &mut ctx.accounts.heat_epoch;
        require!(!epoch.is_resolved, CustomError::EpochAlreadyResolved);
        let now = Clock::get()?.unix_timestamp;
        require!(now >= epoch.resolution_timestamp, CustomError::ResolutionDelayActive);
        epoch.close_price = close_price;
        epoch.is_resolved = true;
        epoch.winning_position = Some(outcome);
        // Calculate and record fee (from losing vault)
        let losing_vault_total = match outcome {
            HeatEpochOutcome::Up => epoch.down_vault_total,
            HeatEpochOutcome::Down => epoch.up_vault_total,
            HeatEpochOutcome::Neutral => 0,
        };
        let fee_amount = (losing_vault_total * config.fee_bps as u64) / 10000;
        epoch.fee_collected = fee_amount;
        Ok(())
    }

    pub fn claim_heat_rewards(ctx: Context<ClaimHeatRewards>) -> Result<()> {
        let config = &ctx.accounts.heat_forecast_config;
        let epoch = &ctx.accounts.heat_epoch;
        let user_position = &mut ctx.accounts.user_position;
        require!(epoch.is_resolved, CustomError::EpochNotResolved);
        require!(!user_position.has_claimed, CustomError::AlreadyClaimed);
        // Calculate winnings
        let (user_winnings, total_claim) = calculate_heat_winnings(user_position, epoch, config);
        // Transfer winnings from program vault to user
        let cpi_accounts = Transfer {
            from: ctx.accounts.program_token_account.to_account_info(),
            to: ctx.accounts.user_token_account.to_account_info(),
            authority: ctx.accounts.heat_forecast_config.to_account_info(),
        };
        let seeds = &[b"heat_forecast_config", &[config.bump]];
        let signer = &[&seeds[..]];
        let cpi_program = ctx.accounts.token_program.to_account_info();
        let cpi_ctx = CpiContext::new_with_signer(cpi_program, cpi_accounts, signer);
        token::transfer(cpi_ctx, total_claim)?;
        user_position.has_claimed = true;
        user_position.claimed_at = Clock::get()?.unix_timestamp;
        Ok(())
    }
}

// =============================
// CONTEXTS
// =============================

#[derive(Accounts)]
pub struct InitializeHeatForecast<'info> {
    #[account(
        init,
        payer = authority,
        space = 8 + 128,
        seeds = [b"heat_forecast_config"],
        bump
    )]
    pub heat_forecast_config: Account<'info, HeatForecastConfig>,
    pub token_mint: Account<'info, Mint>,
    #[account(mut)]
    pub authority: Signer<'info>,
    #[account(mut)]
    pub treasury: Signer<'info>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct StartNewHeatEpoch<'info> {
    #[account(
        mut,
        seeds = [b"heat_forecast_config"],
        bump = heat_forecast_config.bump,
        constraint = heat_forecast_config.authority == authority.key()
    )]
    pub heat_forecast_config: Account<'info, HeatForecastConfig>,
    #[account(
        init,
        payer = authority,
        space = 8 + 128,
        seeds = [b"heat_epoch", (heat_forecast_config.current_epoch + 1).to_le_bytes().as_ref()],
        bump
    )]
    pub heat_epoch: Account<'info, HeatForecastEpoch>,
    #[account(mut)]
    pub authority: Signer<'info>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct DepositHeat<'info> {
    #[account(
        mut,
        seeds = [b"heat_forecast_config"],
        bump = heat_forecast_config.bump
    )]
    pub heat_forecast_config: Account<'info, HeatForecastConfig>,
    #[account(
        mut,
        seeds = [b"heat_epoch", heat_forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump = heat_epoch.bump
    )]
    pub heat_epoch: Account<'info, HeatForecastEpoch>,
    #[account(
        init_if_needed,
        payer = user,
        space = 8 + 128,
        seeds = [b"user_position", user.key().as_ref(), heat_epoch.epoch_id.to_le_bytes().as_ref()],
        bump
    )]
    pub user_position: Account<'info, HeatUserPosition>,
    #[account(mut)]
    pub user: Signer<'info>,
    #[account(mut)]
    pub user_token_account: Account<'info, TokenAccount>,
    #[account(mut)]
    pub program_token_account: Account<'info, TokenAccount>,
    pub token_program: Program<'info, Token>,
    pub system_program: Program<'info, System>,
}

#[derive(Accounts)]
pub struct ResolveHeatEpoch<'info> {
    #[account(
        mut,
        seeds = [b"heat_forecast_config"],
        bump = heat_forecast_config.bump,
        constraint = heat_forecast_config.authority == authority.key()
    )]
    pub heat_forecast_config: Account<'info, HeatForecastConfig>,
    #[account(
        mut,
        seeds = [b"heat_epoch", heat_forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump = heat_epoch.bump
    )]
    pub heat_epoch: Account<'info, HeatForecastEpoch>,
    #[account(mut)]
    pub authority: Signer<'info>,
}

#[derive(Accounts)]
pub struct ClaimHeatRewards<'info> {
    #[account(
        mut,
        seeds = [b"heat_forecast_config"],
        bump = heat_forecast_config.bump
    )]
    pub heat_forecast_config: Account<'info, HeatForecastConfig>,
    #[account(
        mut,
        seeds = [b"heat_epoch", heat_forecast_config.current_epoch.to_le_bytes().as_ref()],
        bump = heat_epoch.bump
    )]
    pub heat_epoch: Account<'info, HeatForecastEpoch>,
    #[account(
        mut,
        seeds = [b"user_position", user.key().as_ref(), heat_epoch.epoch_id.to_le_bytes().as_ref()],
        bump = user_position.bump
    )]
    pub user_position: Account<'info, HeatUserPosition>,
    #[account(mut)]
    pub user: Signer<'info>,
    #[account(mut)]
    pub user_token_account: Account<'info, TokenAccount>,
    #[account(mut)]
    pub program_token_account: Account<'info, TokenAccount>,
    pub token_program: Program<'info, Token>,
}

// =============================
// HELPERS & ERRORS
// =============================

fn calculate_heat_winnings(
    user_position: &HeatUserPosition,
    epoch: &HeatForecastEpoch,
    config: &HeatForecastConfig,
) -> (u64, u64) {
    if !epoch.is_resolved || epoch.winning_position.is_none() {
        return (0, 0);
    }
    let outcome = epoch.winning_position.unwrap();
    if outcome == HeatEpochOutcome::Neutral {
        // Stake returned, no winnings
        return (0, user_position.amount);
    }
    let user_won = match (outcome, user_position.position) {
        (HeatEpochOutcome::Up, HeatPosition::Up) => true,
        (HeatEpochOutcome::Down, HeatPosition::Down) => true,
        _ => false,
    };
    if !user_won {
        return (0, 0);
    }
    let losing_vault_total = match outcome {
        HeatEpochOutcome::Up => epoch.down_vault_total,
        HeatEpochOutcome::Down => epoch.up_vault_total,
        HeatEpochOutcome::Neutral => 0,
    };
    let winning_vault_total = match outcome {
        HeatEpochOutcome::Up => epoch.up_vault_total,
        HeatEpochOutcome::Down => epoch.down_vault_total,
        HeatEpochOutcome::Neutral => 0,
    };
    if winning_vault_total == 0 || losing_vault_total == 0 {
        return (0, 0);
    }
    let fee_amount = (losing_vault_total * config.fee_bps as u64) / 10000;
    let prize_pool = losing_vault_total - fee_amount;
    let user_share = (prize_pool * user_position.amount) / winning_vault_total;
    let total_claim = user_position.amount + user_share;
    (user_share, total_claim)
}

#[error_code]
pub enum CustomError {
    #[msg("Market is not active")] MarketInactive,
    #[msg("Epoch already resolved")] EpochAlreadyResolved,
    #[msg("Invalid amount")] InvalidAmount,
    #[msg("Position too large")] PositionTooLarge,
    #[msg("Deposit window closed")] DepositClosed,
    #[msg("Resolution delay active")] ResolutionDelayActive,
    #[msg("Epoch not resolved")] EpochNotResolved,
    #[msg("Already claimed")] AlreadyClaimed,
} 