use anchor_lang::prelude::*;
use anchor_spl::{
    token::{self, Mint, Token, TokenAccount, Transfer},
    associated_token::AssociatedToken,
};
use anchor_lang::solana_program::{program::invoke_signed, program_option::COption};
use solana_program::keccak;
use plonk_verifier::verifier_core::CircuitVerifier;
use ark_bn254::Fr;

use crate::{state::*, error::*, verifier_interface};

#[derive(Accounts)]
#[instruction(proof: Vec<u8>, pub_signals: Vec<[u8; 32]>)]
pub struct VerifyAndDistribute<'info> {
    #[account(mut)]
    pub protocol_state: Account<'info, ProtocolState>,

    #[account(
        init_if_needed,
        payer = recipient,
        space = NULLIFIER_STATE_SIZE,
        seeds = [b"nullifier", pub_signals[0].as_ref()],
        bump
    )]
    pub nullifier_state: Account<'info, NullifierState>,

    #[account(mut)]
    pub recipient: Signer<'info>,

    #[account(
        mut,
        constraint = heat_token_mint.key() == protocol_state.heat_token_mint
    )]
    pub heat_token_mint: Account<'info, Mint>,

    #[account(
        mut,
        constraint = o_token_mint.key() == protocol_state.o_token_mint
    )]
    pub o_token_mint: Account<'info, Mint>,

    #[account(
        init_if_needed,
        payer = recipient,
        associated_token::mint = heat_token_mint,
        associated_token::authority = recipient
    )]
    pub recipient_heat_ata: Account<'info, TokenAccount>,

    #[account(
        init_if_needed,
        payer = recipient,
        associated_token::mint = o_token_mint,
        associated_token::authority = recipient
    )]
    pub recipient_o_ata: Account<'info, TokenAccount>,

    #[account(
        mut,
        associated_token::mint = heat_token_mint,
        associated_token::authority = protocol_state.treasury
    )]
    pub treasury_heat_ata: Account<'info, TokenAccount>,

    #[account(
        mut,
        associated_token::mint = o_token_mint,
        associated_token::authority = protocol_state.treasury
    )]
    pub treasury_o_ata: Account<'info, TokenAccount>,

    pub token_program: Program<'info, Token>,
    pub associated_token_program: Program<'info, AssociatedToken>,
    pub system_program: Program<'info, System>,
    pub rent: Sysvar<'info, Rent>,
    
    /// CHECK: This is the verifier program
    #[account(address = verifier_interface::ID)]
    pub verifier_program: AccountInfo<'info>,
}

impl<'info> VerifyAndDistribute<'info> {
    pub fn process(
        &mut self,
        proof: Vec<u8>,
        pub_signals: Vec<[u8; 32]>,
        bump: u8,
    ) -> Result<()> {
        // Verify the nullifier hasn't been used
        require!(
            self.nullifier_state.nullifier == [0u8; 32],
            ProtocolError::NullifierAlreadyUsed
        );

        // Verify recipient matches proof
        let recipient_hash = keccak::hash(&self.recipient.key().to_bytes());
        require!(
            recipient_hash.0 == pub_signals[2],
            ProtocolError::RecipientMismatch
        );

        // Verify the proof using the external verifier program via CPI
        let public_signals_bytes = pub_signals.iter()
            .flat_map(|signal| signal.iter())
            .cloned()
            .collect::<Vec<u8>>();
            
        let cpi_ctx = CpiContext::new(
            self.verifier_program.clone(),
            verifier_interface::VerifyProof {},
        );
        
        verifier_interface::verify(cpi_ctx, proof.clone(), public_signals_bytes)
            .map_err(|_| ProtocolError::InvalidProof)?;

        // Calculate amounts including treasury fee
        let fee_bps = self.protocol_state.treasury_fee_bps as u64;
        let total_heat = (HEAT_REWARD_AMOUNT * 10000) / (10000 - fee_bps);
        let total_o = (O_REWARD_AMOUNT * 10000) / (10000 - fee_bps);

        let heat_fee = total_heat - HEAT_REWARD_AMOUNT;
        let o_fee = total_o - O_REWARD_AMOUNT;

        // Mint and distribute HEAT tokens
        token::mint_to(
            CpiContext::new(
                self.token_program.to_account_info(),
                token::MintTo {
                    mint: self.heat_token_mint.to_account_info(),
                    to: self.recipient_heat_ata.to_account_info(),
                    authority: self.protocol_state.to_account_info(),
                },
            ),
            HEAT_REWARD_AMOUNT,
        )?;

        token::mint_to(
            CpiContext::new(
                self.token_program.to_account_info(),
                token::MintTo {
                    mint: self.heat_token_mint.to_account_info(),
                    to: self.treasury_heat_ata.to_account_info(),
                    authority: self.protocol_state.to_account_info(),
                },
            ),
            heat_fee,
        )?;

        // Mint and distribute O tokens
        token::mint_to(
            CpiContext::new(
                self.token_program.to_account_info(),
                token::MintTo {
                    mint: self.o_token_mint.to_account_info(),
                    to: self.recipient_o_ata.to_account_info(),
                    authority: self.protocol_state.to_account_info(),
                },
            ),
            O_REWARD_AMOUNT,
        )?;

        token::mint_to(
            CpiContext::new(
                self.token_program.to_account_info(),
                token::MintTo {
                    mint: self.o_token_mint.to_account_info(),
                    to: self.treasury_o_ata.to_account_info(),
                    authority: self.protocol_state.to_account_info(),
                },
            ),
            o_fee,
        )?;

        // Mark nullifier as used
        self.nullifier_state.nullifier = pub_signals[0];
        self.nullifier_state.timestamp = Clock::get()?.unix_timestamp;

        Ok(())
    }
}

#[derive(Accounts)]
pub struct UpdateTreasuryFee<'info> {
    #[account(mut)]
    pub protocol_state: Account<'info, ProtocolState>,

    #[account(
        constraint = authority.key() == protocol_state.authority
    )]
    pub authority: Signer<'info>,
}

impl<'info> UpdateTreasuryFee<'info> {
    pub fn process(&mut self, new_fee_bps: u16) -> Result<()> {
        require!(new_fee_bps <= 1000, ProtocolError::TreasuryFeeTooHigh);
        self.protocol_state.treasury_fee_bps = new_fee_bps;
        Ok(())
    }
}

#[derive(Accounts)]
pub struct UpdateTreasury<'info> {
    #[account(mut)]
    pub protocol_state: Account<'info, ProtocolState>,

    #[account(
        constraint = authority.key() == protocol_state.authority
    )]
    pub authority: Signer<'info>,

    /// CHECK: Just storing the address
    pub new_treasury: AccountInfo<'info>,
}

impl<'info> UpdateTreasury<'info> {
    pub fn process(&mut self) -> Result<()> {
        self.protocol_state.treasury = self.new_treasury.key();
        Ok(())
    }
}

pub fn process_submit_deposit_proof(
    ctx: Context<SubmitDepositProof>,
    proof: Vec<u8>,
    public_signals: Vec<u8>,
    nullifier: [u8; 32],
    recipient: Pubkey,
) -> Result<()> {
    let protocol_state = &mut ctx.accounts.protocol_state;
    let nullifier_state = &mut ctx.accounts.nullifier_state;
    let depositor = &ctx.accounts.depositor;
    let heat_token_account = &ctx.accounts.heat_token_account;
    let o_token_account = &ctx.accounts.o_token_account;
    let o_mint = &ctx.accounts.o_mint;
    let heat_mint = &ctx.accounts.heat_mint;

    // Check if nullifier already exists
    require!(!nullifier_state.is_used, COLDError::NullifierAlreadyUsed);

    // Parse public signals
    let parsed_signals = CircuitVerifier::parse_public_signals(&public_signals)?;
    
    // Verify the proof
    CircuitVerifier::verify_proof(&proof, &parsed_signals)
        .map_err(|_| COLDError::InvalidProof)?;

    // Validate public signals match the provided values
    require!(parsed_signals.len() >= 2, COLDError::InvalidPublicSignals);
    
    // Convert nullifier to field element for comparison
    let nullifier_fr = Fr::from_le_bytes_mod_order(&nullifier)
        .ok_or(COLDError::InvalidPublicSignals)?;
    require!(parsed_signals[0] == nullifier_fr, COLDError::InvalidPublicSignals);
    
    // Convert recipient to field element for comparison
    let recipient_bytes = recipient.to_bytes();
    let recipient_fr = Fr::from_le_bytes_mod_order(&recipient_bytes)
        .ok_or(COLDError::InvalidPublicSignals)?;
    require!(parsed_signals[1] == recipient_fr, COLDError::InvalidPublicSignals);

    // Mark nullifier as used
    nullifier_state.nullifier = nullifier;
    nullifier_state.is_used = true;
    nullifier_state.used_at = Clock::get()?.unix_timestamp;

    // Update protocol statistics
    protocol_state.total_deposits += 1;
    protocol_state.last_deposit_time = Clock::get()?.unix_timestamp;

    // Calculate rewards based on deposit proof
    let (heat_reward, o_reward) = calculate_deposit_rewards(
        protocol_state.total_deposits,
        protocol_state.total_o_minted,
    )?;

    // Mint HEAT tokens (burn-to-earn gas token)
    if heat_reward > 0 {
        let mint_heat_ix = token::MintTo {
            mint: heat_mint.to_account_info(),
            to: heat_token_account.to_account_info(),
            authority: protocol_state.to_account_info(),
        };
        
        let seeds = &[
            b"protocol_state",
            &[ctx.bumps.protocol_state],
        ];
        let signer = &[&seeds[..]];
        
        token::mint_to(
            CpiContext::new_with_signer(
                ctx.accounts.token_program.to_account_info(),
                mint_heat_ix,
                signer,
            ),
            heat_reward,
        )?;
        
        protocol_state.total_heat_minted += heat_reward;
        msg!("Minted {} HEAT tokens to depositor", heat_reward);
    }

    // Mint O tokens (governance token with 80 max supply)
    if o_reward > 0 && protocol_state.total_o_minted + o_reward <= MAX_O_SUPPLY {
        let mint_o_ix = token::MintTo {
            mint: o_mint.to_account_info(),
            to: o_token_account.to_account_info(),
            authority: protocol_state.to_account_info(),
        };
        
        let seeds = &[
            b"protocol_state",
            &[ctx.bumps.protocol_state],
        ];
        let signer = &[&seeds[..]];
        
        token::mint_to(
            CpiContext::new_with_signer(
                ctx.accounts.token_program.to_account_info(),
                mint_o_ix,
                signer,
            ),
            o_reward,
        )?;
        
        protocol_state.total_o_minted += o_reward;
        msg!("Minted {} O tokens to depositor", o_reward);
    }

    // Emit deposit event
    emit!(DepositProcessed {
        depositor: depositor.key(),
        nullifier,
        recipient,
        heat_reward,
        o_reward,
        timestamp: Clock::get()?.unix_timestamp,
    });

    msg!(
        "Deposit proof processed successfully. HEAT: {}, O: {}, Total deposits: {}",
        heat_reward,
        o_reward,
        protocol_state.total_deposits
    );

    Ok(())
}

pub fn process_governance_proposal(
    ctx: Context<GovernanceProposal>,
    proposal_id: u64,
    proposal_type: ProposalType,
    description: String,
    voting_period: i64,
) -> Result<()> {
    let protocol_state = &mut ctx.accounts.protocol_state;
    let proposal_state = &mut ctx.accounts.proposal_state;
    let proposer = &ctx.accounts.proposer;
    let proposer_o_account = &ctx.accounts.proposer_o_account;

    // Check if proposer has sufficient O tokens to create proposal
    require!(
        proposer_o_account.amount >= MIN_O_FOR_PROPOSAL,
        COLDError::InsufficientOTokens
    );

    // Initialize proposal
    let current_time = Clock::get()?.unix_timestamp;
    proposal_state.proposal_id = proposal_id;
    proposal_state.proposer = proposer.key();
    proposal_state.proposal_type = proposal_type.clone();
    proposal_state.description = description;
    proposal_state.created_at = current_time;
    proposal_state.voting_deadline = current_time + voting_period;
    proposal_state.executed_at = 0;
    proposal_state.votes_for = 0;
    proposal_state.votes_against = 0;
    proposal_state.status = ProposalStatus::Active;

    // Update protocol state
    protocol_state.total_proposals += 1;

    emit!(ProposalCreated {
        proposal_id,
        proposer: ctx.accounts.proposer.key(),
        proposal_type,
        voting_deadline: proposal_state.voting_deadline,
    });

    msg!("Governance proposal {} created by {}", proposal_id, proposer.key());
    Ok(())
}

pub fn process_vote_on_proposal(
    ctx: Context<VoteOnProposal>,
    proposal_id: u64,
    vote: bool, // true for yes, false for no
    voting_power: u64,
) -> Result<()> {
    let proposal_state = &mut ctx.accounts.proposal_state;
    let voter_state = &mut ctx.accounts.voter_state;
    let voter = &ctx.accounts.voter;
    let voter_o_account = &ctx.accounts.voter_o_account;

    // Check if proposal is still active
    require!(
        proposal_state.status == ProposalStatus::Active,
        COLDError::ProposalNotActive
    );

    // Check if voting period is still open
    let current_time = Clock::get()?.unix_timestamp;
    require!(
        current_time <= proposal_state.voting_deadline,
        COLDError::VotingPeriodEnded
    );

    // Check if voter has already voted
    require!(!voter_state.has_voted, COLDError::AlreadyVoted);

    // Validate voting power matches O token balance
    require!(
        voter_o_account.amount >= voting_power,
        COLDError::InsufficientVotingPower
    );

    // Record the vote
    voter_state.has_voted = true;
    voter_state.vote = vote;
    voter_state.voting_power = voting_power;
    voter_state.voted_at = current_time;

    // Update proposal vote counts
    if vote {
        proposal_state.votes_for += voting_power;
    } else {
        proposal_state.votes_against += voting_power;
    }

    emit!(VoteCast {
        proposal_id,
        voter: voter.key(),
        vote,
        voting_power,
        timestamp: current_time,
    });

    msg!(
        "Vote cast on proposal {}: {} with power {}",
        proposal_id,
        if vote { "YES" } else { "NO" },
        voting_power
    );

    Ok(())
}

pub fn process_execute_proposal(
    ctx: Context<ExecuteProposal>,
    proposal_id: u64,
) -> Result<()> {
    let protocol_state = &mut ctx.accounts.protocol_state;
    let proposal_state = &mut ctx.accounts.proposal_state;

    // Check if proposal is still active
    require!(
        proposal_state.status == ProposalStatus::Active,
        COLDError::ProposalNotActive
    );

    // Check if voting period has ended
    let current_time = Clock::get()?.unix_timestamp;
    require!(
        current_time > proposal_state.voting_deadline,
        COLDError::VotingPeriodNotEnded
    );

    // Calculate if proposal passed (simple majority)
    let total_votes = proposal_state.votes_for + proposal_state.votes_against;
    let passed = proposal_state.votes_for > proposal_state.votes_against && 
                 total_votes >= MIN_VOTES_FOR_QUORUM;

    if passed {
        proposal_state.status = ProposalStatus::Passed;
        
        // Execute proposal based on type
        match proposal_state.proposal_type {
            ProposalType::ParameterChange => {
                // Execute parameter changes
                execute_parameter_change(protocol_state, proposal_state)?;
            },
            ProposalType::ProtocolUpgrade => {
                // Mark for protocol upgrade
                protocol_state.pending_upgrade = true;
                protocol_state.upgrade_proposal_id = proposal_id;
            },
            ProposalType::EmergencyAction => {
                // Execute emergency action
                execute_emergency_action(protocol_state, proposal_state)?;
            },
        }
        
        msg!("Proposal {} executed successfully", proposal_id);
    } else {
        proposal_state.status = ProposalStatus::Rejected;
        msg!("Proposal {} rejected", proposal_id);
    }

    proposal_state.executed_at = current_time;

    emit!(ProposalExecuted {
        proposal_id,
        passed,
        votes_for: proposal_state.votes_for,
        votes_against: proposal_state.votes_against,
        executed_at: current_time,
    });

    Ok(())
}

pub fn process_burn_heat_for_gas(
    ctx: Context<BurnHeatForGas>,
    amount: u64,
) -> Result<()> {
    let protocol_state = &mut ctx.accounts.protocol_state;
    let user_heat_account = &ctx.accounts.user_heat_account;
    let heat_mint = &ctx.accounts.heat_mint;

    // Validate burn amount
    require!(amount > 0, COLDError::InvalidAmount);
    require!(
        user_heat_account.amount >= amount,
        COLDError::InsufficientBalance
    );

    // Burn HEAT tokens
    let burn_ix = token::Burn {
        mint: heat_mint.to_account_info(),
        from: user_heat_account.to_account_info(),
        authority: ctx.accounts.user.to_account_info(),
    };

    token::burn(
        CpiContext::new(
            ctx.accounts.token_program.to_account_info(),
            burn_ix,
        ),
        amount,
    )?;

    // Update protocol statistics
    protocol_state.total_heat_burned += amount;

    // Calculate gas credit (1 HEAT = 1000 lamports of gas credit)
    let gas_credit = amount * 1000;

    emit!(HeatBurnedForGas {
        user: ctx.accounts.user.key(),
        amount_burned: amount,
        gas_credit,
        timestamp: Clock::get()?.unix_timestamp,
    });

    msg!("Burned {} HEAT tokens for {} lamports gas credit", amount, gas_credit);
    Ok(())
}

// Helper functions

fn calculate_deposit_rewards(
    total_deposits: u64,
    total_o_minted: u64,
) -> Result<(u64, u64)> {
    // HEAT reward: Base reward that increases with early adoption
    let heat_base = 1000; // 1000 HEAT base reward
    let heat_bonus = if total_deposits < 100 {
        500 // Early adopter bonus
    } else if total_deposits < 1000 {
        250 // Mid-stage bonus
    } else {
        0 // No bonus for late adopters
    };
    let heat_reward = heat_base + heat_bonus;

    // O token reward: Decreasing probability as supply approaches max
    let o_reward = if total_o_minted >= MAX_O_SUPPLY {
        0 // No more O tokens available
    } else {
        // Probability decreases as we approach max supply
        let remaining = MAX_O_SUPPLY - total_o_minted;
        let probability = (remaining * 100) / MAX_O_SUPPLY; // Percentage chance
        
        // Simple deterministic reward based on deposit count
        // In production, this could use VRF for true randomness
        if (total_deposits % 100) < probability {
            1 // Award 1 O token
        } else {
            0
        }
    };

    Ok((heat_reward, o_reward))
}

fn execute_parameter_change(
    protocol_state: &mut ProtocolState,
    proposal_state: &ProposalState,
) -> Result<()> {
    // Parse proposal description for parameter changes
    // This is simplified - in production, you'd have structured proposal data
    
    if proposal_state.description.contains("increase_deposit_reward") {
        // Example: increase base deposit reward
        msg!("Executing parameter change: increase deposit reward");
    } else if proposal_state.description.contains("adjust_voting_period") {
        // Example: adjust default voting period
        msg!("Executing parameter change: adjust voting period");
    }
    
    Ok(())
}

fn execute_emergency_action(
    protocol_state: &mut ProtocolState,
    proposal_state: &ProposalState,
) -> Result<()> {
    if proposal_state.description.contains("pause_protocol") {
        protocol_state.is_paused = true;
        msg!("Emergency action: Protocol paused");
    } else if proposal_state.description.contains("unpause_protocol") {
        protocol_state.is_paused = false;
        msg!("Emergency action: Protocol unpaused");
    }
    
    Ok(())
}

// Context structs moved to lib.rs

// SubmitDepositProof context moved to lib.rs 