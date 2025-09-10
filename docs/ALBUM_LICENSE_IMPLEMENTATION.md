# Album License Implementation Guide (Transaction Extra Type 0x0B)

## Overview

This document provides a complete technical implementation guide for the Album License system using Fuego blockchain transaction extra type 0x0B. This system enables verifiable album purchases without requiring smart contracts, providing an interim solution until C0DL3 is available.

## Transaction Extra Type 0x0B: Album License

### Structure Definition

```cpp
struct TransactionExtraAlbumLicense {
  std::string albumId;             // Album identifier (e.g., "album_001")
  Crypto::PublicKey buyerKey;      // Public key of the buyer
  uint64_t purchaseAmount;         // XFG amount paid (in atomic units)
  uint64_t timestamp;              // Unix timestamp of purchase
  Crypto::PublicKey artistKey;     // Artist's public key
  Crypto::Signature artistSig;    // Artist's signature over license data
  uint32_t version;                // Version of the license protocol (start with 1)
};
```

### Core API Functions

```cpp
// Add license to transaction extra
bool appendAlbumLicenseToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraAlbumLicense& license);

// Extract license from transaction extra
bool getAlbumLicenseFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraAlbumLicense& license);
```

## Implementation Flow

### 1. Album Purchase Process

```typescript
// frontend-arch/src/utils/albumPurchase.ts
interface AlbumPurchaseRequest {
  albumId: string;
  artistPaymentCode: string;  // BIP47-style payment code
  priceXFG: number;           // Price in XFG
  buyerWallet: WalletInterface;
}

async function purchaseAlbum(request: AlbumPurchaseRequest): Promise<string> {
  const { albumId, artistPaymentCode, priceXFG, buyerWallet } = request;
  
  // 1. Derive unique payment address using BIP47
  const paymentAddress = await derivePaymentAddress(artistPaymentCode, buyerWallet);
  
  // 2. Create license data
  const licenseData = {
    albumId,
    buyerKey: await buyerWallet.getPublicKey(),
    purchaseAmount: priceXFG * 1000000, // Convert to atomic units
    timestamp: Math.floor(Date.now() / 1000),
    artistKey: extractArtistKey(artistPaymentCode),
    version: 1
  };
  
  // 3. Get artist signature (requires artist cooperation)
  const artistSignature = await requestArtistSignature(licenseData);
  licenseData.artistSig = artistSignature;
  
  // 4. Create transaction with 0x0B extra
  const transaction = await buyerWallet.createTransaction({
    to: paymentAddress,
    amount: priceXFG,
    extraData: {
      type: 0x0B,
      data: licenseData
    }
  });
  
  // 5. Sign and broadcast
  const txHash = await buyerWallet.sendTransaction(transaction);
  
  return txHash;
}
```

### 2. License Verification System

```typescript
// frontend-arch/src/utils/licenseCheck.ts
interface LicenseOwnership {
  albumId: string;
  ownerKey: string;
  purchaseAmount: number;
  timestamp: number;
  txHash: string;
  verified: boolean;
}

class AlbumLicenseChecker {
  private cache: Map<string, LicenseOwnership[]> = new Map();
  private fuegoRPC: FuegoRPCClient;
  
  constructor(rpcClient: FuegoRPCClient) {
    this.fuegoRPC = rpcClient;
  }
  
  async hasLicense(userPublicKey: string, albumId: string): Promise<boolean> {
    const licenses = await this.getUserLicenses(userPublicKey);
    return licenses.some(license => 
      license.albumId === albumId && license.verified
    );
  }
  
  async getUserLicenses(userPublicKey: string): Promise<LicenseOwnership[]> {
    // Check cache first
    if (this.cache.has(userPublicKey)) {
      return this.cache.get(userPublicKey)!;
    }
    
    // Scan blockchain for 0x0B transactions
    const licenses = await this.scanBlockchainForLicenses(userPublicKey);
    
    // Cache results
    this.cache.set(userPublicKey, licenses);
    
    return licenses;
  }
  
  private async scanBlockchainForLicenses(userPublicKey: string): Promise<LicenseOwnership[]> {
    const licenses: LicenseOwnership[] = [];
    
    // Query Fuego RPC for transactions with extra data type 0x0B
    const transactions = await this.fuegoRPC.getTransactionsByExtraType(0x0B);
    
    for (const tx of transactions) {
      try {
        const licenseData = this.parseAlbumLicense(tx.extra);
        
        if (licenseData.buyerKey === userPublicKey) {
          // Verify artist signature
          const isValid = await this.verifyArtistSignature(licenseData);
          
          licenses.push({
            albumId: licenseData.albumId,
            ownerKey: licenseData.buyerKey,
            purchaseAmount: licenseData.purchaseAmount,
            timestamp: licenseData.timestamp,
            txHash: tx.hash,
            verified: isValid
          });
        }
      } catch (error) {
        console.warn('Failed to parse license from transaction:', tx.hash, error);
      }
    }
    
    return licenses;
  }
  
  private parseAlbumLicense(extraData: string): TransactionExtraAlbumLicense {
    // Parse hex-encoded extra data to extract 0x0B license
    // This would use the C++ parsing logic via WASM or bridge
    return parseAlbumLicenseFromHex(extraData);
  }
  
  private async verifyArtistSignature(license: TransactionExtraAlbumLicense): Promise<boolean> {
    // Verify that artistSig is valid signature of license data by artistKey
    const dataToVerify = this.serializeLicenseForSigning(license);
    return await cryptoVerifySignature(license.artistKey, license.artistSig, dataToVerify);
  }
  
  private serializeLicenseForSigning(license: TransactionExtraAlbumLicense): string {
    // Create canonical representation for signature verification
    return `${license.albumId}:${license.buyerKey}:${license.purchaseAmount}:${license.timestamp}`;
  }
}
```

### 3. Content Gating Integration

```typescript
// frontend-arch/src/components/AlbumPlayer.tsx
import { AlbumLicenseChecker } from '../utils/licenseCheck';

interface AlbumPlayerProps {
  album: Album;
  currentTrack: Track;
  userPublicKey: string;
}

const AlbumPlayer: React.FC<AlbumPlayerProps> = ({ album, currentTrack, userPublicKey }) => {
  const [hasLicense, setHasLicense] = useState(false);
  const [isPremium, setIsPremium] = useState(false);
  const [loading, setLoading] = useState(true);
  const licenseChecker = new AlbumLicenseChecker(fuegoRPC);
  
  useEffect(() => {
    async function checkAccess() {
      setLoading(true);
      
      // Check album license
      const albumLicense = await licenseChecker.hasLicense(userPublicKey, album.albumId);
      setHasLicense(albumLicense);
      
      // Check premium access (0.1 XFG or 1M HEAT balance)
      const premiumAccess = await checkPremiumAccess(userPublicKey);
      setIsPremium(premiumAccess);
      
      setLoading(false);
    }
    
    checkAccess();
  }, [userPublicKey, album.albumId]);
  
  const canPlayTrack = (track: Track): boolean => {
    // Always allow preview tracks
    if (track.isPreview) return true;
    
    // License holders can play everything
    if (hasLicense) return true;
    
    // Premium users can play everything
    if (isPremium) return true;
    
    // Otherwise, blocked
    return false;
  };
  
  const handlePlayTrack = (track: Track) => {
    if (!canPlayTrack(track)) {
      // Show purchase modal
      showPurchaseModal(album);
      return;
    }
    
    // Play the track
    playTrack(track);
  };
  
  if (loading) {
    return <LoadingSpinner />;
  }
  
  return (
    <div className="album-player">
      <TrackList 
        tracks={album.tracks}
        onPlayTrack={handlePlayTrack}
        canPlay={canPlayTrack}
        hasLicense={hasLicense}
        isPremium={isPremium}
      />
      
      {!hasLicense && !isPremium && (
        <PurchasePrompt 
          album={album}
          onPurchase={() => purchaseAlbum({
            albumId: album.albumId,
            artistPaymentCode: album.payment.paymentCode,
            priceXFG: album.priceXFG,
            buyerWallet: connectedWallet
          })}
        />
      )}
    </div>
  );
};
```

### 4. Artist Signature Process

For the purchase to be valid, the artist must sign the license. This can be handled in several ways:

#### Option A: Real-time Signing (Recommended)
```typescript
// Artist runs a signing service
async function requestArtistSignature(licenseData: Partial<TransactionExtraAlbumLicense>): Promise<string> {
  const artistSigningService = getArtistSigningService(licenseData.albumId);
  
  const response = await fetch(`${artistSigningService}/sign-license`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(licenseData)
  });
  
  const { signature } = await response.json();
  return signature;
}
```

#### Option B: Pre-signed Templates
```typescript
// Artist pre-signs license templates with placeholder buyer keys
// Platform fills in buyer key and validates against template
async function getPreSignedLicense(albumId: string, buyerKey: string): Promise<string> {
  const template = await getArtistLicenseTemplate(albumId);
  return fillLicenseTemplate(template, buyerKey);
}
```

## Security Considerations

### 1. Signature Verification
- Always verify artist signature before accepting license as valid
- Use canonical data serialization for signature verification
- Validate that artist public key matches the album's verified artist

### 2. Replay Protection
- Include timestamp in signature to prevent replay attacks
- Reject licenses with timestamps too far in past/future
- Consider adding nonce for additional replay protection

### 3. Key Management
- Artist keys should be stored securely
- Consider using hardware wallets for artist signing
- Implement key rotation capability for long-term security

## Performance Optimizations

### 1. Caching Strategy
```typescript
class LicenseCacheManager {
  private cache = new Map<string, CacheEntry>();
  private cacheTTL = 300000; // 5 minutes
  
  async getCachedLicenses(userKey: string): Promise<LicenseOwnership[] | null> {
    const entry = this.cache.get(userKey);
    if (!entry || Date.now() - entry.timestamp > this.cacheTTL) {
      return null;
    }
    return entry.licenses;
  }
  
  setCachedLicenses(userKey: string, licenses: LicenseOwnership[]): void {
    this.cache.set(userKey, {
      licenses,
      timestamp: Date.now()
    });
  }
}
```

### 2. Incremental Scanning
```typescript
class IncrementalLicenseScanner {
  private lastScanBlock = 0;
  
  async scanNewLicenses(userKey: string): Promise<LicenseOwnership[]> {
    const currentBlock = await this.fuegoRPC.getCurrentBlockHeight();
    const newLicenses = await this.fuegoRPC.getTransactionsByExtraType(
      0x0B, 
      this.lastScanBlock, 
      currentBlock
    );
    
    this.lastScanBlock = currentBlock;
    return this.filterUserLicenses(newLicenses, userKey);
  }
}
```

## Migration to C0DL3

When C0DL3 smart contracts become available, the migration path is straightforward:

1. **Dual System**: Run both 0x0B transactions and C0DL3 contracts in parallel
2. **License Bridge**: Create bridge contract that recognizes existing 0x0B licenses
3. **Gradual Migration**: New purchases use C0DL3, existing licenses remain valid
4. **Sunset Phase**: Eventually deprecate 0x0B scanning in favor of contract queries

## Testing Strategy

### 1. Unit Tests
```typescript
describe('AlbumLicenseChecker', () => {
  it('should detect valid licenses', async () => {
    const checker = new AlbumLicenseChecker(mockRPC);
    const hasLicense = await checker.hasLicense(userKey, albumId);
    expect(hasLicense).toBe(true);
  });
  
  it('should reject invalid signatures', async () => {
    const checker = new AlbumLicenseChecker(mockRPC);
    const invalidLicense = createInvalidLicense();
    const isValid = await checker.verifyArtistSignature(invalidLicense);
    expect(isValid).toBe(false);
  });
});
```

### 2. Integration Tests
- Test complete purchase flow from wallet to license verification
- Test blockchain scanning with various transaction types
- Test caching and performance under load

### 3. Security Tests
- Test signature verification with invalid keys
- Test replay attack prevention
- Test license validation edge cases

## Deployment Checklist

- [ ] Fuego core updated with 0x0B transaction extra support
- [ ] Client license checking implementation complete
- [ ] Artist signing service or pre-signature system implemented
- [ ] Caching and performance optimizations in place
- [ ] Security validation and testing complete
- [ ] Migration plan to C0DL3 documented
- [ ] Monitoring and analytics for license usage implemented

## Conclusion

The 0x0B Album License system provides a robust, decentralized solution for album licensing without requiring smart contracts. It offers:

- **Verifiable Ownership**: On-chain proof of album purchases
- **Artist Authentication**: Cryptographic signatures ensure legitimacy
- **Privacy Compatibility**: Works with BIP47-style payment codes
- **Performance**: Caching and incremental scanning for efficiency
- **Migration Ready**: Clear upgrade path to C0DL3 smart contracts

This system enables immediate deployment of the DIGM album marketplace while maintaining compatibility with future blockchain infrastructure improvements.
