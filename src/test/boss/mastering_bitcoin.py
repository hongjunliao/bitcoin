from bitcoinlib.keys import Key

# Generate a new Bitcoin key pair
key = Key("L53fCHmQhbNp1B4JipfBtfYgv8G9vsxeUE8cCJKyNAgbieXbUz2i", network="bitcoin")
# Extract the private key in different formats
private_key_hex = key.private_hex      # 32 bytes (256-bit) in hexadecimal
private_key_wif = key.wif()           # Wallet Import Format

print(f"Private Key (HEX): {private_key_hex}")
print(f"Private Key (WIF): {private_key_wif}")
# Generate public keys in both formats
public_key_compressed = key.public_hex          # 33 bytes
public_key_uncompressed = key.public_uncompressed_hex  # 65 bytes

print(f"Compressed:   {public_key_compressed}")
print(f"Uncompressed: {public_key_uncompressed}")
# Taproot uses x-only public keys (32 bytes)
taproot_pubkey = key.public_hex[2:]  # Remove the 02/03 prefix
print(f"X-only Public Key: {taproot_pubkey}")

# Generate different address types from the same key
legacy_address = key.address()                          # P2PKH
segwit_native = key.address(encoding='bech32')          # P2WPKH
segwit_p2sh = key.address(encoding='base58', script_type='p2sh')  # P2SH-P2WPKH
taproot_address = key.address(encoding='bech32',script_type='p2tr')       # P2TR

print(f"Legacy (P2PKH):     {legacy_address}")
print(f"SegWit Native:      {segwit_native}")
print(f"SegWit P2SH:        {segwit_p2sh}")
print(f"Taproot:            {taproot_address}")


#p2sh
from bitcoinutils.setup import setup
from bitcoinutils.keys import PrivateKey
from bitcoinutils.script import Script
from bitcoinutils.keys import P2shAddress

def create_multisig_p2sh():
    setup('testnet')

    # Stakeholder public keys
    alice_pk = '02898711e6bf63f5cbe1b38c05e89d6c391c59e9f8f695da44bf3d20ca674c8519'
    bob_pk = '0284b5951609b76619a1ce7f48977b4312ebe226987166ef044bfb374ceef63af5'
    carol_pk = '0317aa89b43f46a0c0cdbd9a302f2508337ba6a06d123854481b52de9c20996011'

    # 2-of-3 multisig redeem script
    redeem_script = Script([
        'OP_2',           # Require 2 signatures
        alice_pk,         # Alice's public key
        bob_pk,           # Bob's public key
        carol_pk,         # Carol's public key
        'OP_3',           # Total of 3 keys
        'OP_CHECKMULTISIG' # Multisig verification
    ])

    # Generate P2SH address
    p2sh_addr = P2shAddress.from_script(redeem_script)
    return p2sh_addr, redeem_script