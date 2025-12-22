"""
Simple WebSocket server to test mobile controller connection
Receives button events from the Flutter mobile app and prints them
"""
import asyncio
import json
import socket
import websockets

def get_local_ip():
    """Get the local IP address of this machine"""
    try:
        # Create a socket to determine local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "localhost"

async def handle_controller(websocket):
    """Handle incoming controller connections"""
    client_address = websocket.remote_address
    print(f"\n✅ Controller connected from {client_address[0]}:{client_address[1]}")
    
    try:
        async for message in websocket:
            try:
                # Parse the JSON message from mobile app
                data = json.loads(message)
                
                if data.get('type') == 'button':
                    button = data.get('button')
                    pressed = data.get('pressed')
                    status = "PRESSED" if pressed else "RELEASED"
                    print(f"🎮 Button: {button:15s} {status}")
                
                elif data.get('type') == 'analog':
                    stick = data.get('stick')
                    x = data.get('x', 0)
                    y = data.get('y', 0)
                    print(f"🕹️  Analog {stick:5s}: X={x:+.2f}, Y={y:+.2f}")
                
                else:
                    print(f"📦 Unknown message: {data}")
                    
            except json.JSONDecodeError:
                print(f"⚠️  Invalid JSON: {message}")
                
    except websockets.exceptions.ConnectionClosed:
        print(f"\n❌ Controller disconnected from {client_address[0]}:{client_address[1]}")

async def main():
    """Start the WebSocket server"""
    host = "0.0.0.0"  # Listen on all network interfaces
    port = 8765
    
    local_ip = get_local_ip()
    
    print("=" * 60)
    print("🎮 Virtual Controller Test Server")
    print("=" * 60)
    print(f"\n📡 Server starting on {local_ip}:{port}")
    print(f"\n📱 Enter this in your mobile app:")
    print(f"   IP Address: {local_ip}")
    print(f"   Port: {port}")
    print(f"\n⏳ Waiting for controller connection...\n")
    
    async with websockets.serve(handle_controller, host, port):
        await asyncio.Future()  # Run forever

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\n🛑 Server stopped by user")
