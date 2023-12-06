# Minasigo

Win32 + WebView2.

- HTML side
  - AES encryption/decryption
  - MessagePack decoding

- Win32 side
  - File I/O
  - HTTP request

Files in /deps folder are third-party libraries except for `unix_clock`.

## Build Dependency
- WebView2
- Windows Implementation Libraries (WIL)

## Sequence

The code is lengthy but the basic procedure is as follow.

1. Download and decode "story/MasterData" messagePack.
2. Find "resourcePathR18" value in the decoded MasterData.
3. Request scenario text paths.
4. Download scenario texts.
5. Find resource names in the scenario text.
6. Request scenario resource paths.
7. Download scenario resources.
