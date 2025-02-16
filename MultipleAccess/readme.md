# ロボットやIoTデバイスのための多元接続技術

このリポジトリでは、ロボットやIoTデバイスで使用される多元接続技術について、理論と実践の両面から学ぶことができます。

## 目次

1. [はじめに](#はじめに)
2. [多元接続技術の基礎](#多元接続技術の基礎)
3. [IEEE 802.11規格における多元接続](#IEEE-80211規格における多元接続)
4. [実装例とテストコード](#実装例とテストコード)
5. [応用例と実践](#応用例と実践)
6. [セットアップガイド](#セットアップガイド)

## はじめに

多元接続技術は、複数のデバイスが同時に通信を行うための重要な技術です。本ドキュメントでは、特にIoTデバイスやロボットに焦点を当てて、実践的な理解を深めていきます。

### システム構成図

```mermaid
graph TD
    A[802.11ax AP] -->|OFDMA| B[ESP32C6 #1]
    A -->|OFDMA| C[ESP32C6 #2]
    A -->|OFDMA| D[ESP32C6 #3]
    A -->|802.11b| E[従来デバイス]
    
    style A fill:#f9f,stroke:#333
    style B fill:#bbf,stroke:#333
    style C fill:#bbf,stroke:#333
    style D fill:#bbf,stroke:#333
    style E fill:#ddd,stroke:#333
```

### ハードウェア構成

- **アクセスポイント**: 市販の802.11ax対応AP
- **クライアント**: ESP32C6開発ボード（802.11ax STAモード対応）
- **従来機器**: 802.11b/g/n対応デバイス

## 多元接続技術の基礎

### DSSS (Direct Sequence Spread Spectrum)
IEEE 802.11bで採用されている拡散変調方式です。

#### DSSの動作原理

```mermaid
sequenceDiagram
    participant T as 送信機
    participant C as チップ系列
    participant R as 受信機
    T->>C: データビット
    C->>R: 拡散変調
    Note over T,R: 広帯域化による耐干渉性向上
```

#### スペクトラム拡散の仕組み

```mermaid
graph TD
    A[元のデータ信号] --> B[拡散符号]
    B --> C[拡散処理]
    C --> D[広帯域信号]
    
    style A fill:#f9f,stroke:#333
    style B fill:#bbf,stroke:#333
    style C fill:#bfb,stroke:#333
    style D fill:#fbf,stroke:#333
```

#### 特徴と利点
- 耐干渉性の向上
- セキュリティの向上
- 周波数の有効利用

### OFDMA (Orthogonal Frequency Division Multiple Access)
IEEE 802.11axで導入された次世代の多元接続技術です。ESP32C6ではクライアントモードでOFDMAを利用できます。

#### 周波数リソースの割り当て

```mermaid
graph LR
    A[802.11ax AP] --> B[RUの動的割り当て]
    B --> C[ESP32C6 #1]
    B --> D[ESP32C6 #2]
    B --> E[ESP32C6 #3]
    style A fill:#f9f,stroke:#333
    style B fill:#bbf,stroke:#333
```

#### 時間-周波数リソース割り当て

```mermaid
graph TD
    subgraph 時間スロット1
    A1[RU1] --> U1[ユーザー1]
    A2[RU2] --> U2[ユーザー2]
    end
    subgraph 時間スロット2
    B1[RU1] --> U3[ユーザー3]
    B2[RU2] --> U4[ユーザー4]
    end
```

#### OFDMAの特徴
- 高効率な周波数利用
- 低遅延性
- 柔軟なリソース割り当て

### 802.11規格の進化

```mermaid
timeline
    title WiFi規格の進化と多元接続技術
    section 初期の規格
        802.11b : DSSS : 11 Mbps
        802.11a : OFDM : 54 Mbps
    section 高速化
        802.11g : OFDM/DSSS : 54 Mbps
        802.11n : MIMO-OFDM : 600 Mbps
    section 現代
        802.11ac : MU-MIMO : 6.9 Gbps
        802.11ax : OFDMA : 9.6 Gbps
```

## 応用例と実践

### ロボット制御での活用

```mermaid
graph TD
    subgraph ロボット制御システム
    A[制御センター] -->|OFDMA| B[ロボット1]
    A -->|OFDMA| C[ロボット2]
    A -->|OFDMA| D[ロボット3]
    end
    
    subgraph データフロー
    B -->|センサーデータ| E[データ収集]
    C -->|状態情報| E
    D -->|位置情報| E
    end
```

### スマートファクトリーでの実装

```mermaid
graph LR
    subgraph 工場フロア
    A[制御システム] -->|OFDMA| B[製造ライン1]
    A -->|OFDMA| C[製造ライン2]
    A -->|DSSS| D[レガシー設備]
    end
    
    subgraph モニタリング
    B --> E[品質管理]
    C --> E
    D --> E
    end
```

### IoTセンサーネットワーク

```mermaid
graph TD
    subgraph センサーネットワーク
    A[ゲートウェイ] -->|OFDMA| B[温度センサー群]
    A -->|OFDMA| C[湿度センサー群]
    A -->|OFDMA| D[圧力センサー群]
    end
    
    subgraph データ処理
    B --> E[データ分析]
    C --> E
    D --> E
    E --> F[アラート生成]
    end
```

## 実装例とテストコード

本リポジトリには、以下の実装例が含まれています：

### 1. 基本的なWiFiテスト
- 接続状態のモニタリング
- 信号強度の測定
- PHYモードの検出

### 2. DSSS実験
- Barkerコードによる拡散
- SNR測定
- 干渉耐性の評価

### 3. OFDMA実験（クライアントモード）
- 802.11ax APへの接続
- リソースユニットの利用状況モニタリング
- スペクトル効率の測定

### プロジェクト構造
```
/
├── basic_dsss/
├── ofdma_test/
└── esp32_wifi/
```

## セットアップガイド

### 必要な環境
- PlatformIO IDE
- ESP32C6開発ボード
- 802.11ax対応AP（市販品）
- Arduino Framework

### インストール手順
1. VSCodeにPlatformIOをインストール
2. 本リポジトリをクローン
3. プロジェクトをPlatformIOで開く
4. 802.11ax APを設定

### APの要件
- IEEE 802.11ax（Wi-Fi 6）対応
- OFDMAの有効化
- 2.4GHz帯のサポート

## 実験結果の解析

### 1. DSSS性能評価

```mermaid
graph LR
    A[入力信号] -->|拡散| B[処理済信号]
    B -->|SNR測定| C[性能評価]
    C -->|結果分析| D[最適化]
```

### 2. OFDMA効率分析

```mermaid
graph TD
    A[トラフィック] -->|RU割り当て| B[リソース使用]
    B -->|効率計算| C[性能評価]
    C -->|最適化| D[パラメータ調整]
```

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。
