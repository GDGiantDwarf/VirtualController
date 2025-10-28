# 🎮 Project Design Brief: Hybrid Multiplayer System with Virtual Phone Controllers

## 1. Project Overview
Develop a multiplayer platform where players can join using either a **physical controller** or a **smartphone app**. Each player (or group of players) installs a **desktop app** that connects to a central server running the game logic. The phone app acts as a **virtual controller**, sending only standardized inputs.  

Key point: **multiple controllers can connect to a single desktop app**. This allows both:  
- **Traditional local multiplayer** (several people around one computer).  
- **Grouped remote play** (e.g., 2 players on Computer A in City 1 + 2 players on Computer B in City 2 → all 4 play together in the same server‑hosted game).  

---

## 2. Core Components

### 🖥️ **Desktop App (Player Client)**
- **Role:** Installed application that displays the game state and bridges inputs.  
- **Responsibilities:**
  - Render the game state streamed from the server.  
  - Accept input from:
    - Physical controllers (USB/Bluetooth).  
    - Phone apps (via Wi‑Fi/internet).  
  - Convert phone input into a **virtual controller signal** before sending it to the server.  
  - Support **multiple controllers per desktop app**, whether for local multiplayer or grouped remote play.  
  - Provide local feedback (UI overlays, sound, optional rumble passthrough).  

---

### 📱 **Phone App (Controller)**
- **Role:** Native app installed on iOS/Android, acting as a virtual controller.  
- **Responsibilities:**
  - Provide a fixed set of inputs matching the **virtual controller format** (e.g., joystick, buttons, triggers).  
  - Connect to the desktop app and send input events.  
  - Allow multiple phones to connect to the same desktop app.  

---

### 🌐 **Server (Game Host)**
- **Role:** Runs the authoritative game logic.  
- **Responsibilities:**
  - Maintain game state and synchronize across all desktop apps.  
  - Receive controller inputs from desktop apps.  
  - Broadcast updated game state to all clients.  
  - Manage matchmaking, sessions, and latency smoothing.  

---

## 3. Data Flow
1. **Desktop app** connects to the server and joins a session.  
2. **Phone app(s)** connect to the desktop app → send standardized virtual controller inputs.  
3. **Desktop app → Server:** Forwards inputs (from phone or physical controllers).  
4. **Server → Desktop app(s):** Sends updated game state.  
5. **Desktop app(s):** Render visuals and play feedback.  

---

## 4. User Stories

### 🎮 Core Play Experience
- **As a player**, I want to install the desktop app and connect my controller (phone or physical), so that I can join a game session hosted on the server and see the game state on my own screen.  
- **As a group of friends in the same room**, we want to connect multiple controllers to a single desktop app, so that we can all play together on one computer without needing separate machines.  

### 🌍 Grouped Remote Play
- **As two friends in City A and two friends in City B**, we want to connect our controllers to our respective desktop apps, so that all four of us can play in the same server‑hosted game even though we’re in different locations.  
- **As a player**, I want to see the same synchronized game state on my desktop app as my friends in other cities, so that we all share a consistent experience.  

### 📱 Phone as Virtual Controller
- **As a player without a physical controller**, I want to install the phone app and use it as a virtual controller, so that I can still participate in the game without extra hardware.  
- **As a developer**, I want the phone app to send standardized virtual controller inputs, so that I don’t have to design custom layouts or handle device‑specific quirks.  

### 🖥️ Desktop App as Bridge
- **As a player**, I want the desktop app to translate phone inputs into virtual controller signals, so that the server treats them the same as physical controller inputs.  
- **As a group**, we want the desktop app to handle multiple controllers at once, so that we can easily add or remove players without restarting the game.  

### 🌐 Server‑Hosted Game Logic
- **As a player**, I want the server to run the authoritative game logic, so that no single player has an unfair advantage and the game state is always consistent.  
- **As a developer**, I want the server to manage matchmaking and sessions, so that players can easily find and join games without manual setup.