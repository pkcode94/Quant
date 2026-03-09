# Quant Sync — Architecture

## Overview

```
┌──────────────────────┐          TLS / HTTPS          ┌──────────────────────────┐
│    Quant App         │  ◄─────────────────────────►  │    quant-server          │
│  (Android / Desktop) │                                │  (C++ / httplib)         │
│                      │   POST /api/sync/upload        │                          │
│  ┌────────────────┐  │   ───────────────────►         │  ┌────────────────────┐  │
│  │ TradeDatabase   │  │   encrypted blob              │  │ SyncStore          │  │
│  │ (JSON files)    │  │                               │  │ (opaque blobs)     │  │
│  └───────┬────────┘  │   GET /api/sync/download       │  └────────────────────┘  │
│          │ encrypt   │   ◄───────────────────          │                          │
│  ┌───────▼────────┐  │   encrypted blob              │  ┌────────────────────┐  │
│  │ VaultCrypto     │  │                               │  │ UserManager        │  │
│  │ AES-256-GCM    │  │   POST /api/login             │  │ (auth + sessions)  │  │
│  │ (client-side)   │  │   ───────────────────►         │  └────────────────────┘  │
│  └────────────────┘  │   {username, password}         │                          │
│                      │                               │  ┌────────────────────┐  │
│                      │   POST /api/ticket/create      │  │ TicketSystem       │  │
│                      │   ───────────────────►         │  │ (quota requests)   │  │
└──────────────────────┘                                │  └────────────────────┘  │
                                                        │                          │
                                                        │  ┌────────────────────┐  │
                                                        │  │ Admin Console      │  │
                                                        │  │ (web UI at /admin) │  │
                                                        │  └────────────────────┘  │
                                                        └──────────────────────────┘
```

## Zero-Knowledge Design

The server **never** sees plaintext trade data. All encryption and decryption
happens on the client before any network transfer:

1. User sets a **vault password** (separate from their login password)
2. Client derives AES-256 key via PBKDF2 (100k iterations, SHA-256)
3. Client packs DB directory → single tar-like blob
4. Client encrypts blob with AES-256-GCM (random 96-bit IV per upload)
5. Client uploads encrypted bytes to server
6. Server stores opaque bytes — cannot read, search, or index

```
Blob format:
┌──────────┬─────────┬──────────┬──────────┬───────────────┬──────────┐
│ "QVLT"   │ version │ salt     │ IV       │ ciphertext    │ GCM tag  │
│ 4 bytes  │ 4 bytes │ 32 bytes │ 12 bytes │ variable      │ 16 bytes │
└──────────┴─────────┴──────────┴──────────┴───────────────┴──────────┘
```

## Server API

### Authentication

| Method | Path              | Body                                | Response                     |
|--------|-------------------|-------------------------------------|------------------------------|
| POST   | `/api/register`   | `{username, password, email}`       | `{ok: true}` or `{error}`   |
| POST   | `/api/login`      | `{username, password}`              | `{token, username}`          |
| POST   | `/api/logout`     | Header: `Authorization: <token>`    | `{ok: true}`                 |

### Sync

| Method | Path                 | Body / Headers                   | Response                        |
|--------|----------------------|----------------------------------|---------------------------------|
| GET    | `/api/sync/status`   | Auth header                      | `{lastSync, usedBytes, quota}`  |
| POST   | `/api/sync/upload`   | Auth header + raw encrypted body | `{ok, usedBytes}`               |
| GET    | `/api/sync/download` | Auth header                      | Raw encrypted blob              |

### Quotas & Tickets

| Method | Path                  | Body                          | Response                |
|--------|-----------------------|-------------------------------|-------------------------|
| GET    | `/api/quota`          | Auth header                   | `{used, limit, unit}`   |
| POST   | `/api/ticket/create`  | Auth + `{subject, message}`   | `{ticketId}`            |
| GET    | `/api/ticket/list`    | Auth header                   | `[{ticket}, ...]`       |
| GET    | `/api/ticket/{id}`    | Auth header                   | `{ticket with messages}`|
| POST   | `/api/ticket/{id}/reply` | Auth + `{message}`         | `{ok}`                  |

### Admin Console (web, ADMIN-only)

| Method | Path                     | Description                         |
|--------|--------------------------|-------------------------------------|
| GET    | `/admin`                 | Dashboard (user count, storage)     |
| GET    | `/admin/users`           | User list with quota controls       |
| POST   | `/admin/set-quota`       | Set user's storage quota            |
| POST   | `/admin/set-premium`     | Toggle premium status               |
| GET    | `/admin/tickets`         | All open tickets                    |
| POST   | `/admin/ticket/reply`    | Admin reply to ticket               |
| POST   | `/admin/ticket/close`    | Close a ticket                      |
| POST   | `/admin/ticket/set-btc`  | Set BTC address + amount for ticket |
| GET    | `/admin/console`         | Server log viewer                   |

## Storage Quotas

- Default: **5 MB** per user (configurable in `server.conf`)
- Premium users: **50 MB** (configurable)
- Custom: admin can set per-user via console
- Exceeding quota: upload rejected with `413 Payload Too Large`
- To increase: user creates a ticket → admin sets BTC price →
  user pays → admin confirms payment → quota increased

## Bitcoin Payment Flow

```
User ──► "I need 100MB storage"
         ▼
     POST /api/ticket/create
         ▼
Admin sees ticket in /admin/tickets
         ▼
Admin sets BTC price: POST /admin/ticket/set-btc
  {ticketId, btcAddress: "bc1q...", requiredBtc: 0.001}
         ▼
User sees payment info in app: GET /api/ticket/{id}
         ▼
User pays manually (any wallet)
         ▼
Admin verifies on-chain, then:
  POST /admin/confirm-payment {username, quotaBytes}
         ▼
User's quota increased, ticket closed
```

## Server Storage Layout

```
server-data/
├── users.json              # UserManager (auth, sessions)
├── quotas.json             # {username: {usedBytes, limitBytes}}
├── tickets.json            # Support tickets
├── pending_payments.json   # BTC payment tracking
├── server.log              # Rotating log
└── vaults/
    ├── alice/
    │   ├── db.enc          # Encrypted database blob
    │   └── meta.json       # {lastSync, sizeBytes, checksum}
    └── bob/
        ├── db.enc
        └── meta.json
```

## Client-Side Encryption (Android)

```java
// VaultCrypto.java — client-side AES-256-GCM
SecretKey key = deriveKey(vaultPassword, salt);  // PBKDF2
Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
cipher.init(Cipher.ENCRYPT_MODE, key, new GCMParameterSpec(128, iv));
byte[] encrypted = cipher.doFinal(plaintext);
// Prepend header: magic + version + salt + iv
// Append GCM tag (included in encrypted output)
```

## Build & Run

```bash
# Build server
cd quant-server
make

# Run server (default port 9443)
./quant-server --port 9443 --data ./server-data

# With TLS
./quant-server --port 9443 --cert cert.pem --key key.pem
```
