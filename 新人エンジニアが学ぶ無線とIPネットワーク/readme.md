# Mermaid 図 一括ファイル



---

## 1. 無線LANの基本構成例

```mermaid
flowchart LR
    Laptop((ノートPC)) -- "Wi-Fi電波" --> AP[アクセスポイント]
    Smartphone((スマートフォン)) -- "Wi-Fi電波" --> AP
    AP -- "有線LAN" --> Router[ルーター]
    Router -- "インターネット接続" --> Internet[インターネット]
```

---

## 2. OSI参照モデル（例）

```mermaid
flowchart TB
    subgraph OSI
        L1[物理層] 
        L2[データリンク層]
        L3[ネットワーク層]
        L4[トランスポート層]
        L5[セッション層]
        L6[プレゼンテーション層]
        L7[アプリケーション層]
    end
```

---

## 3. IPアドレスとサブネット分割イメージ

```mermaid
flowchart TB
    A[/192.168.0.0<br>/24/] --> B[/192.168.0.0<br>/25/]
    A --> C[/192.168.0.128<br>/25/]
```

---

## 4. 無線LANと有線LANの比較図

```mermaid
graph LR
    subgraph LAN1[有線LAN]
        PC1[PC1] --> SW[スイッチ]
        PC2[PC2] --> SW
        SW --> R1[ルーター]
    end
    subgraph LAN2[無線LAN]
        PC3[PC3] -. Wi-Fi .-> AP[AP]
        SP[スマホ] -. Wi-Fi .-> AP
        AP --> R2[ルーター]
    end
    R1 --- NET[インターネット]
    R2 --- NET
```

---

## 5. LAN内通信（スイッチ）の流れ

```mermaid
flowchart LR
    PC1[PC1 192.168.1.10] -- "フレーム送信 (宛先MAC:PC2)" --> SwitchA[スイッチ]
    SwitchA -- "宛先MAC=PC2 →該当ポートへ転送" --> PC2[PC2 192.168.1.20]
```

---

## 6. ルータによるネットワーク間通信

```mermaid
flowchart LR
    subgraph Network1 192.168.1.0/24
      PC1_a[PC1 192.168.1.10] --> R1_a[ルーター 192.168.1.1]
    end
    subgraph Network2 192.168.2.0/24
      R1_b[ルーター 192.168.2.1] --> PC3_a[PC3 192.168.2.10]
    end
    PC1_a -- "宛先IP:192.168.2.10 (他ネットワーク)" --> R1_a
    R1_b -- "転送(IPルーティング)" --> PC3_a
```

---

## 7. DHCPのDORAシーケンス

```mermaid
sequenceDiagram
    participant PC as クライアントPC
    participant DHCP as DHCPサーバ
    PC->>DHCP: DHCP Discover<br/>（アドレス要求のブロードキャスト）
    DHCP-->>PC: DHCP Offer<br/>（候補IPアドレスの提示）
    PC->>DHCP: DHCP Request<br/>（OfferされたIPアドレスを要求）
    DHCP-->>PC: DHCP ACK<br/>（要求の承認、設定情報の通知）
```

---

## 8. DNS問い合わせの基本シーケンス

```mermaid
sequenceDiagram
    participant Client as クライアントPC
    participant DNSServer as DNSサーバ
    Client->>DNSServer: 「www.example.com」のIPアドレスを問い合わせ
    DNSServer-->>Client: 応答: 「www.example.com」は 93.184.216.34
    Client->>Client: 応答をキャッシュし、通信開始
```

---

## 9. NATの変換イメージ

```mermaid
sequenceDiagram
    participant PC_int as PC(内部:192.168.1.100)
    participant RouterNAT as ルーター(NAT有効:203.0.113.10)
    participant ServerExt as サーバ(外部:93.184.216.34)
    Note over PC_int,RouterNAT: 内部ネットワーク(プライベートIP)
    Note over RouterNAT,ServerExt: 外部ネットワーク(インターネット)
    PC_int->>RouterNAT: 送信元IP=192.168.1.100<br/>宛先IP=93.184.216.34
    RouterNAT->>ServerExt: **送信元IP=203.0.113.10**（変換後）<br/>宛先IP=93.184.216.34
    ServerExt-->>RouterNAT: 返信 宛先IP=203.0.113.10
    RouterNAT-->>PC_int: **宛先IP=192.168.1.100**（再変換）<br/>内部PCへ転送
```

---

## 10. 802.1X認証（RADIUS）のイメージ

```mermaid
sequenceDiagram
    participant Client
    participant AP
    participant Radius
    Client->>AP: 認証開始
    AP->>Radius: ユーザ認証要求
    Radius-->>AP: 認証応答
    AP-->>Client: 接続許可
```

---

## 11. VLANによるネットワーク分割イメージ

```mermaid
flowchart TB
    subgraph "スイッチ"
      PC_A["PC A\n(VLAN10)"]:::vlan10 --> SwitchX
      PC_B["PC B\n(VLAN10)"]:::vlan10 --> SwitchX
      PC_C["PC C\n(VLAN20)"]:::vlan20 --> SwitchX
      PC_D["PC D\n(VLAN20)"]:::vlan20 --> SwitchX
    end
    SwitchX --> RouterX["ルーター"]
    classDef vlan10 fill:#ffefba,stroke:#f77,stroke-width:1px;
    classDef vlan20 fill:#bae1ff,stroke:#77f,stroke-width:1px;
```

---

## 12. ルーティングプロトコルのイメージ

```mermaid
flowchart LR
    R1((Router1)) -- Routing Info --> R2((Router2))
    R2 -- Routing Info --> R3((Router3))
    R3 -- Routing Info --> R1
```

---

## 13. 小規模オフィスネットワークの例

```mermaid
graph TB
    subgraph "Office LAN"
      direction LR
      SW_main["コアスイッチ"] -- "VLAN10(社内LAN)" --> PC_Group["社員PC\n(多数)"]
      SW_main -- "VLAN20(ゲストLAN)" --> GuestAP["ゲスト用AP"]
      SW_main -- "サーバLAN" --> FileSrv["ファイルサーバ"]
      SW_main -- "社内Wi-Fi" --> CorpAP["社内用AP"]
    end
    RouterGW["ルーター\nゲートウェイ"] --> SW_main
    RouterGW --> FW["ファイアウォール"]
    FW --> InternetISP["インターネット"]
```

---

## 14. 障害切り分けフローチャート例

```mermaid
graph TD
    A["トラブル: インターネット不通"] --> B{"物理接続は正常か?"}
    B -- "いいえ" --> B_fix["ケーブル再接続/無線設定確認"]
    B -- "はい" --> C{"IPアドレス取得済みか?"}
    C -- "いいえ" --> C_fix["DHCPサーバ確認\nor 静的IP設定誤り修正"]
    C -- "はい" --> D{"ゲートウェイにPING応答あり?"}
    D -- "いいえ" --> D_fix["ルータ/スイッチの障害確認\n（電源・設定・故障）"]
    D -- "はい" --> E{"外部IPにPING応答あり?"}
    E -- "いいえ" --> E_fix["回線障害 or プロバイダ障害\nルータのWAN設定確認"]
    E -- "はい" --> F{"名前解決(DNS)可能?"}
    F -- "いいえ" --> F_fix["DNSサーバ設定確認\n（代替DNSでテスト）"]
    F -- "はい" --> G["問題切り分け完了\n対応策の実施"]
```
---

## 15. SNMPによる監視シーケンス例

```mermaid
sequenceDiagram
    participant Monitor as 監視サーバ
    participant Router as ルーター
    participant Switch as スイッチ
    Monitor->>Router: SNMP GET 要求 (CPU使用率のOID)
    Router-->>Monitor: SNMP GET応答 (CPU=45%)
    Monitor->>Switch: SNMP GET 要求 (Port1ステータスのOID)
    Switch-->>Monitor: SNMP 応答 (Port1=Down)
    Note right of Monitor: Port1 Downを検知<br/>アラート送信
```

---



